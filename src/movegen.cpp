/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>

#include "movegen.h"
#include "position.h"

namespace {

  template<CastlingRight Cr, bool Checks, bool Chess960>
  ExtMove* generate_castling(const Position& pos, ExtMove* moveList, Color us) {

    constexpr bool KingSide = (Cr == WHITE_OO || Cr == BLACK_OO);

    if (pos.castling_impeded(Cr) || !pos.can_castle(Cr))
        return moveList;

    // After castling, the rook and king final positions are the same in Chess960
    // as they would be in standard chess.
    Square kfrom = pos.count<KING>(us) ? pos.square<KING>(us) : make_square(FILE_E, us == WHITE ? RANK_1 : RANK_8);
    Square rfrom = pos.castling_rook_square(Cr);
    Square kto = relative_square(us, KingSide ? SQ_G1 : SQ_C1);
    Bitboard enemies = pos.pieces(~us);

    assert(!pos.checkers());

    const Direction step = Chess960 ? kto > kfrom ? WEST : EAST
                                    : KingSide    ? WEST : EAST;

    if (type_of(pos.piece_on(kfrom)) == KING)
    {
        for (Square s = kto; s != kfrom; s += step)
            if (pos.attackers_to(s) & enemies)
                return moveList;

        // Because we generate only legal castling moves we need to verify that
        // when moving the castling rook we do not discover some hidden checker.
        // For instance an enemy queen in SQ_A1 when castling rook is in SQ_B1.
        if (Chess960 && (pos.attackers_to(kto, pos.pieces() ^ rfrom) & pos.pieces(~us)))
            return moveList;
    }

    Move m = make<CASTLING>(kfrom, rfrom);

    if (Checks && !pos.gives_check(m))
        return moveList;

    *moveList++ = m;
    return moveList;
  }


  template<Color c, GenType Type, Direction D>
  ExtMove* make_promotions(const Position& pos, ExtMove* moveList, Square to) {

    if (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
        for (PieceType pt : pos.promotion_piece_types())
            *moveList++ = make<PROMOTION>(to - D, to, pt);

    return moveList;
  }

  template<Color Us, bool Checks>
  ExtMove* generate_drops(const Position& pos, ExtMove* moveList, PieceType pt, Bitboard b) {
    if (pos.count_in_hand(Us, pt))
    {
        // Consider only drops on top of already placed pieces
        if (pos.drop_on_top())
            b &= shift<NORTH>(pos.pieces()) | Rank1BB;
        if (pt == PAWN)
        {
            b &= ~promotion_zone_bb(Us, pos.promotion_rank(), pos.max_rank());
            if (!pos.first_rank_drops())
                b &= ~rank_bb(relative_rank(Us, RANK_1, pos.max_rank()));
        }
        if (pt == SHOGI_PAWN)
            for (File f = FILE_A; f <= pos.max_file(); ++f)
                if (file_bb(f) & pos.pieces(Us, pt))
                    b &= ~file_bb(f);
        if (Checks)
            b &= pos.check_squares(pt);
        while (b)
            *moveList++ = make_drop(pop_lsb(&b), pt);
    }

    return moveList;
  }

  template<Color Us, GenType Type>
  ExtMove* generate_pawn_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

    // Compute our parametrized parameters at compile time, named according to
    // the point of view of white side.
    constexpr Color     Them     = (Us == WHITE ? BLACK      : WHITE);
    constexpr Direction Up       = (Us == WHITE ? NORTH      : SOUTH);
    constexpr Direction Down     = (Us == WHITE ? SOUTH      : NORTH);
    constexpr Direction UpRight  = (Us == WHITE ? NORTH_EAST : SOUTH_WEST);
    constexpr Direction UpLeft   = (Us == WHITE ? NORTH_WEST : SOUTH_EAST);

    // Define squares a pawn can pass during a double step
    Bitboard  TRank3BB = rank_bb(relative_rank(Us, RANK_3, pos.max_rank()));
    if (pos.first_rank_double_steps())
        TRank3BB |= rank_bb(relative_rank(Us, RANK_2, pos.max_rank()));

    Bitboard emptySquares;

    Bitboard TRank8BB = rank_bb(Us == WHITE ? pos.promotion_rank() : Rank(pos.max_rank() - pos.promotion_rank()));
    Bitboard TRank7BB = shift<Down>(TRank8BB);
    Bitboard pawnsOn7    = pos.pieces(Us, PAWN) &  TRank7BB;
    Bitboard pawnsNotOn7 = pos.pieces(Us, PAWN) & ~TRank7BB;

    Bitboard enemies = (Type == EVASIONS ? pos.pieces(Them) & target:
                        Type == CAPTURES ? target : pos.pieces(Them));

    // Single and double pawn pushes, no promotions
    if (Type != CAPTURES)
    {
        emptySquares = (Type == QUIETS || Type == QUIET_CHECKS ? target : ~pos.pieces());

        Bitboard b1 = shift<Up>(pawnsNotOn7)   & emptySquares;
        Bitboard b2 = pos.double_step_enabled() ? shift<Up>(b1 & TRank3BB) & emptySquares : 0;

        if (Type == EVASIONS) // Consider only blocking squares
        {
            b1 &= target;
            b2 &= target;
        }

        if (Type == QUIET_CHECKS && pos.count<KING>(Them))
        {
            Square ksq = pos.square<KING>(Them);

            b1 &= pos.attacks_from<PAWN>(Them, ksq);
            b2 &= pos.attacks_from<PAWN>(Them, ksq);

            // Add pawn pushes which give discovered check. This is possible only
            // if the pawn is not on the same file as the enemy king, because we
            // don't generate captures. Note that a possible discovery check
            // promotion has been already generated amongst the captures.
            Bitboard dcCandidates = pos.blockers_for_king(Them);
            if (pawnsNotOn7 & dcCandidates)
            {
                Bitboard dc1 = shift<Up>(pawnsNotOn7 & dcCandidates) & emptySquares & ~file_bb(ksq);
                Bitboard dc2 = shift<Up>(dc1 & TRank3BB) & emptySquares;

                b1 |= dc1;
                b2 |= dc2;
            }
        }

        while (b1)
        {
            Square to = pop_lsb(&b1);
            *moveList++ = make_move(to - Up, to);
        }

        while (b2)
        {
            Square to = pop_lsb(&b2);
            *moveList++ = make_move(to - Up - Up, to);
        }
    }

    // Promotions and underpromotions
    if (pawnsOn7 && (Type != EVASIONS || (target & TRank8BB)))
    {
        if (Type == CAPTURES)
            emptySquares = ~pos.pieces();

        if (Type == EVASIONS)
            emptySquares &= target;

        Bitboard b1 = shift<UpRight>(pawnsOn7) & enemies;
        Bitboard b2 = shift<UpLeft >(pawnsOn7) & enemies;
        Bitboard b3 = shift<Up     >(pawnsOn7) & emptySquares;

        while (b1)
            moveList = make_promotions<Us, Type, UpRight>(pos, moveList, pop_lsb(&b1));

        while (b2)
            moveList = make_promotions<Us, Type, UpLeft >(pos, moveList, pop_lsb(&b2));

        while (b3)
            moveList = make_promotions<Us, Type, Up     >(pos, moveList, pop_lsb(&b3));
    }

    // Standard and en-passant captures
    if (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
    {
        Bitboard b1 = shift<UpRight>(pawnsNotOn7) & enemies;
        Bitboard b2 = shift<UpLeft >(pawnsNotOn7) & enemies;

        while (b1)
        {
            Square to = pop_lsb(&b1);
            *moveList++ = make_move(to - UpRight, to);
        }

        while (b2)
        {
            Square to = pop_lsb(&b2);
            *moveList++ = make_move(to - UpLeft, to);
        }

        if (pos.ep_square() != SQ_NONE)
        {
            assert(rank_of(pos.ep_square()) == relative_rank(Them, RANK_3));

            // An en passant capture can be an evasion only if the checking piece
            // is the double pushed pawn and so is in the target. Otherwise this
            // is a discovery check and we are forced to do otherwise.
            if (Type == EVASIONS && !(target & (pos.ep_square() - Up)))
                return moveList;

            b1 = pawnsNotOn7 & pos.attacks_from<PAWN>(Them, pos.ep_square());

            assert(b1);

            while (b1)
                *moveList++ = make<ENPASSANT>(pop_lsb(&b1), pos.ep_square());
        }
    }

    return moveList;
  }


  template<bool Checks>
  ExtMove* generate_moves(const Position& pos, ExtMove* moveList, Color us, PieceType pt,
                          Bitboard target) {

    assert(pt != KING && pt != PAWN);

    const Square* pl = pos.squares(us, pt);

    for (Square from = *pl; from != SQ_NONE; from = *++pl)
    {
        // Avoid generating discovered checks twice
        if (Checks && (pos.blockers_for_king(~us) & from))
            continue;

        Bitboard b = (  (pos.attacks_from(us, pt, from) & pos.pieces())
                      | (pos.moves_from(us, pt, from) & ~pos.pieces())) & target;

        if (Checks)
            b &= pos.check_squares(pt);

        while (b)
        {
            Square s = pop_lsb(&b);
            bool piece_promotion =   pos.promoted_piece_type(pt) != NO_PIECE_TYPE
                                  && (promotion_zone_bb(us, pos.promotion_rank(), pos.max_rank()) & (SquareBB[from] | s));
            if (!piece_promotion || !pos.mandatory_piece_promotion())
                *moveList++ = make_move(from, s);
            // Shogi-style piece promotions
            if (piece_promotion)
                *moveList++ = make<PIECE_PROMOTION>(from, s);
        }
    }

    return moveList;
  }


  template<Color Us, GenType Type>
  ExtMove* generate_all(const Position& pos, ExtMove* moveList, Bitboard target) {

    constexpr bool Checks = Type == QUIET_CHECKS;

    moveList = generate_pawn_moves<Us, Type>(pos, moveList, target);
    for (PieceType pt = PieceType(PAWN + 1); pt < KING; ++pt)
        moveList = generate_moves<Checks>(pos, moveList, Us, pt, target);
    // generate drops
    if (pos.piece_drops() && Type != CAPTURES && pos.count_in_hand(Us, ALL_PIECES))
        for (PieceType pt = PAWN; pt < KING; ++pt)
            moveList = generate_drops<Us, Checks>(pos, moveList, pt, target & ~pos.pieces(~Us));

    if (Type != QUIET_CHECKS && Type != EVASIONS && pos.count<KING>(Us))
    {
        Square ksq = pos.square<KING>(Us);
        Bitboard b = pos.attacks_from<KING>(Us, ksq) & target;
        while (b)
            *moveList++ = make_move(ksq, pop_lsb(&b));
    }

    if (pos.castling_enabled() && Type != CAPTURES && Type != EVASIONS && pos.can_castle(Us))
    {
        if (pos.is_chess960())
        {
            moveList = generate_castling<MakeCastling<Us,  KING_SIDE>::right, Checks, true>(pos, moveList, Us);
            moveList = generate_castling<MakeCastling<Us, QUEEN_SIDE>::right, Checks, true>(pos, moveList, Us);
        }
        else
        {
            moveList = generate_castling<MakeCastling<Us,  KING_SIDE>::right, Checks, false>(pos, moveList, Us);
            moveList = generate_castling<MakeCastling<Us, QUEEN_SIDE>::right, Checks, false>(pos, moveList, Us);
        }
    }

    return moveList;
  }

} // namespace


/// generate<CAPTURES> generates all pseudo-legal captures and queen
/// promotions. Returns a pointer to the end of the move list.
///
/// generate<QUIETS> generates all pseudo-legal non-captures and
/// underpromotions. Returns a pointer to the end of the move list.
///
/// generate<NON_EVASIONS> generates all pseudo-legal captures and
/// non-captures. Returns a pointer to the end of the move list.

template<GenType Type>
ExtMove* generate(const Position& pos, ExtMove* moveList) {

  assert(Type == CAPTURES || Type == QUIETS || Type == NON_EVASIONS);
  assert(!pos.checkers());

  Color us = pos.side_to_move();

  Bitboard target =  Type == CAPTURES     ?  pos.pieces(~us)
                   : Type == QUIETS       ? ~pos.pieces()
                   : Type == NON_EVASIONS ? ~pos.pieces(us) : 0;
  target &= pos.board_bb();

  return us == WHITE ? generate_all<WHITE, Type>(pos, moveList, target)
                     : generate_all<BLACK, Type>(pos, moveList, target);
}

// Explicit template instantiations
template ExtMove* generate<CAPTURES>(const Position&, ExtMove*);
template ExtMove* generate<QUIETS>(const Position&, ExtMove*);
template ExtMove* generate<NON_EVASIONS>(const Position&, ExtMove*);


/// generate<QUIET_CHECKS> generates all pseudo-legal non-captures and knight
/// underpromotions that give check. Returns a pointer to the end of the move list.
template<>
ExtMove* generate<QUIET_CHECKS>(const Position& pos, ExtMove* moveList) {

  assert(!pos.checkers());

  Color us = pos.side_to_move();
  Bitboard dc = pos.blockers_for_king(~us) & pos.pieces(us);

  while (dc)
  {
     Square from = pop_lsb(&dc);
     PieceType pt = type_of(pos.piece_on(from));

     if (pt == PAWN)
         continue; // Will be generated together with direct checks

     Bitboard b = pos.moves_from(us, pt, from) & ~pos.pieces();

     if (pt == KING)
         b &= ~PseudoAttacks[~us][QUEEN][pos.square<KING>(~us)];

     while (b)
         *moveList++ = make_move(from, pop_lsb(&b));
  }

  return us == WHITE ? generate_all<WHITE, QUIET_CHECKS>(pos, moveList, ~pos.pieces())
                     : generate_all<BLACK, QUIET_CHECKS>(pos, moveList, ~pos.pieces());
}


/// generate<EVASIONS> generates all pseudo-legal check evasions when the side
/// to move is in check. Returns a pointer to the end of the move list.
template<>
ExtMove* generate<EVASIONS>(const Position& pos, ExtMove* moveList) {

  assert(pos.checkers());

  Color us = pos.side_to_move();
  Square ksq = pos.square<KING>(us);
  Bitboard sliderAttacks = 0;
  Bitboard sliders = pos.checkers();

  // Find all the squares attacked by slider checkers. We will remove them from
  // the king evasions in order to skip known illegal moves, which avoids any
  // useless legality checks later on.
  while (sliders)
  {
      Square checksq = pop_lsb(&sliders);
      sliderAttacks |=  attacks_bb(~us, type_of(pos.piece_on(checksq)), checksq, pos.pieces() ^ ksq);
  }

  // Generate evasions for king, capture and non capture moves
  Bitboard b = pos.attacks_from<KING>(us, ksq) & ~pos.pieces(us) & ~sliderAttacks;
  while (b)
      *moveList++ = make_move(ksq, pop_lsb(&b));

  if (more_than_one(pos.checkers()))
      return moveList; // Double check, only a king move can save the day

  // Generate blocking evasions or captures of the checking piece
  Square checksq = lsb(pos.checkers());
  Bitboard target = between_bb(checksq, ksq) | checksq;
  // Leaper attacks can not be blocked
  if (LeaperAttacks[~us][type_of(pos.piece_on(checksq))][checksq] & ksq)
      target = SquareBB[checksq];

  return us == WHITE ? generate_all<WHITE, EVASIONS>(pos, moveList, target)
                     : generate_all<BLACK, EVASIONS>(pos, moveList, target);
}


/// generate<LEGAL> generates all the legal moves in the given position

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList) {

  if (pos.is_variant_end())
      return moveList;

  ExtMove* cur = moveList;

  moveList = pos.checkers() ? generate<EVASIONS    >(pos, moveList)
                            : generate<NON_EVASIONS>(pos, moveList);
  while (cur != moveList)
      if (!pos.legal(*cur))
          *cur = (--moveList)->move;
      else
          ++cur;

  return moveList;
}
