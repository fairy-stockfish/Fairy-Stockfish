/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2020 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

  template<MoveType T>
  ExtMove* make_move_and_gating(const Position& pos, ExtMove* moveList, Color us, Square from, Square to) {

    *moveList++ = make<T>(from, to);

    // Gating moves
    if (pos.seirawan_gating() && (pos.gates(us) & from))
        for (PieceType pt_gating : pos.piece_types())
            if (pos.count_in_hand(us, pt_gating) && (pos.drop_region(us, pt_gating) & from))
                *moveList++ = make_gating<T>(from, to, pt_gating, from);
    if (pos.seirawan_gating() && T == CASTLING && (pos.gates(us) & to))
        for (PieceType pt_gating : pos.piece_types())
            if (pos.count_in_hand(us, pt_gating) && (pos.drop_region(us, pt_gating) & to))
                *moveList++ = make_gating<T>(from, to, pt_gating, to);

    return moveList;
  }

  template<Color c, GenType Type, Direction D>
  ExtMove* make_promotions(const Position& pos, ExtMove* moveList, Square to) {

    if (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS)
    {
        for (PieceType pt : pos.promotion_piece_types())
            if (!pos.promotion_limit(pt) || pos.promotion_limit(pt) > pos.count(c, pt))
                *moveList++ = make<PROMOTION>(to - D, to, pt);
        PieceType pt = pos.promoted_piece_type(PAWN);
        if (pt && !(pos.piece_promotion_on_capture() && pos.empty(to)))
            *moveList++ = make<PIECE_PROMOTION>(to - D, to);
    }

    return moveList;
  }

  template<Color Us, bool Checks>
  ExtMove* generate_drops(const Position& pos, ExtMove* moveList, PieceType pt, Bitboard b) {
    if (pos.count_in_hand(Us, pt))
    {
        // Restrict to valid target
        b &= pos.drop_region(Us, pt);

        // Add to move list
        if (pos.drop_promoted() && pos.promoted_piece_type(pt))
        {
            Bitboard b2 = b;
            if (Checks)
                b2 &= pos.check_squares(pos.promoted_piece_type(pt));
            while (b2)
                *moveList++ = make_drop(pop_lsb(&b2), pt, pos.promoted_piece_type(pt));
        }
        if (Checks)
            b &= pos.check_squares(pt);
        while (b)
            *moveList++ = make_drop(pop_lsb(&b), pt, pt);
    }

    return moveList;
  }

  template<Color Us, GenType Type>
  ExtMove* generate_pawn_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

    // Compute some compile time parameters relative to the white side
    constexpr Color     Them     = (Us == WHITE ? BLACK      : WHITE);
    constexpr Direction Up       = pawn_push(Us);
    constexpr Direction Down     = -pawn_push(Us);
    constexpr Direction UpRight  = (Us == WHITE ? NORTH_EAST : SOUTH_WEST);
    constexpr Direction UpLeft   = (Us == WHITE ? NORTH_WEST : SOUTH_EAST);

    const Square ksq = pos.count<KING>(Them) ? pos.square<KING>(Them) : SQ_NONE;

    Bitboard TRank8BB = pos.mandatory_pawn_promotion() ? rank_bb(relative_rank(Us, pos.promotion_rank(), pos.max_rank()))
                                                       : promotion_zone_bb(Us, pos.promotion_rank(), pos.max_rank());
    Bitboard TRank7BB = shift<Down>(TRank8BB);
    // Define squares a pawn can pass during a double step
    Bitboard  TRank3BB = rank_bb(relative_rank(Us, Rank(pos.double_step_rank() + 1), pos.max_rank()));
    if (pos.first_rank_double_steps())
        TRank3BB |= rank_bb(relative_rank(Us, RANK_2, pos.max_rank()));

    Bitboard emptySquares;

    Bitboard pawnsOn7    = pos.pieces(Us, PAWN) &  TRank7BB;
    Bitboard pawnsNotOn7 = pos.pieces(Us, PAWN) & (pos.mandatory_pawn_promotion() ? ~TRank7BB : AllSquares);

    Bitboard enemies = (Type == EVASIONS ? pos.pieces(Them) & target:
                        Type == CAPTURES ? target : pos.pieces(Them));

    // Single and double pawn pushes, no promotions
    if (Type != CAPTURES)
    {
        emptySquares = (Type == QUIETS || Type == QUIET_CHECKS ? target : ~pos.pieces() & pos.board_bb(Us, PAWN));

        Bitboard b1 = shift<Up>(pawnsNotOn7)   & emptySquares;
        Bitboard b2 = pos.double_step_enabled() ? shift<Up>(b1 & TRank3BB) & emptySquares : Bitboard(0);

        if (Type == EVASIONS) // Consider only blocking squares
        {
            b1 &= target;
            b2 &= target;
        }

        if (Type == QUIET_CHECKS && pos.count<KING>(Them))
        {
            b1 &= pos.attacks_from<PAWN>(ksq, Them);
            b2 &= pos.attacks_from<PAWN>(ksq, Them);

            // Add pawn pushes which give discovered check. This is possible only
            // if the pawn is not on the same file as the enemy king, because we
            // don't generate captures. Note that a possible discovery check
            // promotion has been already generated amongst the captures.
            Bitboard dcCandidateQuiets = pos.blockers_for_king(Them) & pawnsNotOn7;
            if (dcCandidateQuiets)
            {
                Bitboard dc1 = shift<Up>(dcCandidateQuiets) & emptySquares & ~file_bb(ksq);
                Bitboard dc2 = pos.double_step_enabled() ? shift<Up>(dc1 & TRank3BB) & emptySquares : Bitboard(0);

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
    if (pawnsOn7)
    {
        if (Type == CAPTURES)
            emptySquares = ~pos.pieces() & pos.board_bb(Us, PAWN);

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

    // Sittuyin promotions
    if (pos.sittuyin_promotion() && (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS))
    {
        Bitboard pawns = pos.pieces(Us, PAWN);
        // Pawns need to be on diagonals on opponent's half if there is more than one pawn
        if (pos.count<PAWN>(Us) > 1)
            pawns &=  (  PseudoAttacks[Us][BISHOP][make_square(FILE_A, relative_rank(Us, RANK_1, pos.max_rank()))]
                       | PseudoAttacks[Us][BISHOP][make_square(pos.max_file(), relative_rank(Us, RANK_1, pos.max_rank()))])
                    & forward_ranks_bb(Us, relative_rank(Us, Rank((pos.max_rank() - 1) / 2), pos.max_rank()));
        while (pawns)
        {
            Square from = pop_lsb(&pawns);
            for (PieceType pt : pos.promotion_piece_types())
            {
                if (pos.promotion_limit(pt) && pos.promotion_limit(pt) <= pos.count(Us, pt))
                    continue;
                Bitboard b = (pos.attacks_from(Us, pt, from) & ~pos.pieces()) | from;
                if (Type == EVASIONS)
                    b &= target;

                while (b)
                    *moveList++ = make<PROMOTION>(from, pop_lsb(&b), pt);
            }
        }
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
            assert(rank_of(pos.ep_square()) == relative_rank(Them, Rank(pos.double_step_rank() + 1), pos.max_rank()));

            // An en passant capture can be an evasion only if the checking piece
            // is the double pushed pawn and so is in the target. Otherwise this
            // is a discovery check and we are forced to do otherwise.
            if (Type == EVASIONS && !(target & (pos.ep_square() - Up)))
                return moveList;

            b1 = pawnsNotOn7 & pos.attacks_from<PAWN>(pos.ep_square(), Them);

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

        Bitboard b1 = (  (pos.attacks_from(us, pt, from) & pos.pieces())
                       | (pos.moves_from(us, pt, from) & ~pos.pieces())) & target;
        // Xiangqi soldier
        if (pt == SOLDIER && pos.unpromoted_soldier(us, from))
            b1 &= file_bb(file_of(from));
        if (pt == JANGGI_CANNON)
        {
            b1 &= ~pos.pieces(pt);
            b1 &= attacks_bb(us, pt, from, pos.pieces() ^ pos.pieces(pt));
        }
        PieceType prom_pt = pos.promoted_piece_type(pt);
        Bitboard b2 = prom_pt && (!pos.promotion_limit(prom_pt) || pos.promotion_limit(prom_pt) > pos.count(us, prom_pt)) ? b1 : Bitboard(0);
        Bitboard b3 = pos.piece_demotion() && pos.is_promoted(from) ? b1 : Bitboard(0);

        if (Checks)
        {
            b1 &= pos.check_squares(pt);
            if (b2)
                b2 &= pos.check_squares(pos.promoted_piece_type(pt));
            if (b3)
                b3 &= pos.check_squares(type_of(pos.unpromoted_piece_on(from)));
        }

        // Restrict target squares considering promotion zone
        if (b2 | b3)
        {
            Bitboard promotion_zone = promotion_zone_bb(us, pos.promotion_rank(), pos.max_rank());
            if (pos.mandatory_piece_promotion())
                b1 &= (promotion_zone & from ? Bitboard(0) : ~promotion_zone) | (pos.piece_promotion_on_capture() ? ~pos.pieces() : Bitboard(0));
            // Exclude quiet promotions/demotions
            if (pos.piece_promotion_on_capture())
            {
                b2 &= pos.pieces();
                b3 &= pos.pieces();
            }
            // Consider promotions/demotions into promotion zone
            if (!(promotion_zone & from))
            {
                b2 &= promotion_zone;
                b3 &= promotion_zone;
            }
        }

        while (b1)
            moveList = make_move_and_gating<NORMAL>(pos, moveList, us, from, pop_lsb(&b1));

        // Shogi-style piece promotions
        while (b2)
            *moveList++ = make<PIECE_PROMOTION>(from, pop_lsb(&b2));

        // Piece demotions
        while (b3)
            *moveList++ = make<PIECE_DEMOTION>(from, pop_lsb(&b3));
    }

    return moveList;
  }


  template<Color Us, GenType Type>
  ExtMove* generate_all(const Position& pos, ExtMove* moveList, Bitboard target) {

    constexpr CastlingRights OO  = Us & KING_SIDE;
    constexpr CastlingRights OOO = Us & QUEEN_SIDE;
    constexpr bool Checks = Type == QUIET_CHECKS; // Reduce template instantations

    moveList = generate_pawn_moves<Us, Type>(pos, moveList, target);
    for (PieceType pt : pos.piece_types())
        if (pt != PAWN && pt != KING)
            moveList = generate_moves<Checks>(pos, moveList, Us, pt, target);
    // generate drops
    if (pos.piece_drops() && Type != CAPTURES && pos.count_in_hand(Us, ALL_PIECES))
        for (PieceType pt : pos.piece_types())
            moveList = generate_drops<Us, Checks>(pos, moveList, pt, target & ~pos.pieces(~Us));

    if (Type != QUIET_CHECKS && Type != EVASIONS && pos.count<KING>(Us))
    {
        Square ksq = pos.square<KING>(Us);
        Bitboard b = (  (pos.attacks_from(Us, KING, ksq) & pos.pieces())
                      | (pos.moves_from(Us, KING, ksq) & ~pos.pieces())) & target;
        while (b)
            moveList = make_move_and_gating<NORMAL>(pos, moveList, Us, ksq, pop_lsb(&b));

        // Passing move by king
        if (pos.pass_on_stalemate())
            *moveList++ = make<SPECIAL>(ksq, ksq);

        if (Type != CAPTURES && pos.can_castle(CastlingRights(OO | OOO)))
        {
            if (!pos.castling_impeded(OO) && pos.can_castle(OO))
                moveList = make_move_and_gating<CASTLING>(pos, moveList, Us, ksq, pos.castling_rook_square(OO));

            if (!pos.castling_impeded(OOO) && pos.can_castle(OOO))
                moveList = make_move_and_gating<CASTLING>(pos, moveList, Us, ksq, pos.castling_rook_square(OOO));
        }
    }

    // Castling with non-king piece
    if (!pos.count<KING>(Us) && Type != CAPTURES && pos.can_castle(CastlingRights(OO | OOO)))
    {
        Square from = make_square(FILE_E, pos.castling_rank(Us));
        if (!pos.castling_impeded(OO) && pos.can_castle(OO))
            moveList = make_move_and_gating<CASTLING>(pos, moveList, Us, from, pos.castling_rook_square(OO));

        if (!pos.castling_impeded(OOO) && pos.can_castle(OOO))
            moveList = make_move_and_gating<CASTLING>(pos, moveList, Us, from, pos.castling_rook_square(OOO));
    }

    // Special moves
    if (pos.cambodian_moves() && pos.gates(Us))
    {
        if (Type != CAPTURES && Type != EVASIONS && (pos.pieces(Us, KING) & pos.gates(Us)))
        {
            Square from = pos.square<KING>(Us);
            Bitboard b = PseudoAttacks[WHITE][KNIGHT][from] & rank_bb(rank_of(from + (Us == WHITE ? NORTH : SOUTH)))
                        & target & ~pos.pieces();
            while (b)
                moveList = make_move_and_gating<SPECIAL>(pos, moveList, Us, from, pop_lsb(&b));
        }

        Bitboard b = pos.pieces(Us, FERS) & pos.gates(Us);
        while (b)
        {
            Square from = pop_lsb(&b);
            Square to = from + 2 * (Us == WHITE ? NORTH : SOUTH);
            if (is_ok(to) && (target & to))
                moveList = make_move_and_gating<SPECIAL>(pos, moveList, Us, from, to);
        }
    }

    // Janggi palace moves
    if (pos.diagonal_lines())
    {
        Bitboard diags = pos.pieces(Us) & pos.diagonal_lines();
        while (diags)
        {
            Square from = pop_lsb(&diags);
            PieceType pt = type_of(pos.piece_on(from));
            PieceType movePt = pt == KING ? pos.king_type() : pt;
            Bitboard b = 0;
            PieceType diagType = movePt == WAZIR ? FERS : movePt == SOLDIER ? PAWN : movePt == ROOK ? BISHOP : NO_PIECE_TYPE;
            if (diagType)
                b |= attacks_bb(Us, diagType, from, pos.pieces());
            else if (movePt == JANGGI_CANNON)
                // TODO: fix for longer diagonals
                b |= attacks_bb(Us, ALFIL, from, pos.pieces()) & ~attacks_bb(Us, ELEPHANT, from, pos.pieces() ^ pos.pieces(JANGGI_CANNON));
            b &= pos.board_bb(Us, pt) & target & pos.diagonal_lines();
            while (b)
                moveList = make_move_and_gating<SPECIAL>(pos, moveList, Us, from, pop_lsb(&b));
        }
    }

    return moveList;
  }

} // namespace


/// <CAPTURES>     Generates all pseudo-legal captures and queen promotions
/// <QUIETS>       Generates all pseudo-legal non-captures and underpromotions
/// <NON_EVASIONS> Generates all pseudo-legal captures and non-captures
///
/// Returns a pointer to the end of the move list.

template<GenType Type>
ExtMove* generate(const Position& pos, ExtMove* moveList) {

  static_assert(Type == CAPTURES || Type == QUIETS || Type == NON_EVASIONS, "Unsupported type in generate()");
  assert(!pos.checkers());

  Color us = pos.side_to_move();

  Bitboard target =  Type == CAPTURES     ?  pos.pieces(~us)
                   : Type == QUIETS       ? ~pos.pieces()
                   : Type == NON_EVASIONS ? ~pos.pieces(us) : Bitboard(0);
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

     if (pt == KING && pos.king_type() == KING)
         b &= ~PseudoAttacks[~us][QUEEN][pos.square<KING>(~us)];

     while (b)
         moveList = make_move_and_gating<NORMAL>(pos, moveList, us, from, pop_lsb(&b));
  }

  return us == WHITE ? generate_all<WHITE, QUIET_CHECKS>(pos, moveList, ~pos.pieces() & pos.board_bb())
                     : generate_all<BLACK, QUIET_CHECKS>(pos, moveList, ~pos.pieces() & pos.board_bb());
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

  // Consider all evasion moves for special pieces
  if (sliders & (pos.pieces(CANNON, BANNER) | pos.pieces(HORSE, ELEPHANT) | pos.pieces(JANGGI_CANNON, JANGGI_ELEPHANT)))
  {
      Bitboard target = pos.board_bb() & ~pos.pieces(us);
      Bitboard b = (  (pos.attacks_from(us, KING, ksq) & pos.pieces())
                    | (pos.moves_from(us, KING, ksq) & ~pos.pieces())) & target;
      while (b)
          moveList = make_move_and_gating<NORMAL>(pos, moveList, us, ksq, pop_lsb(&b));
      return us == WHITE ? generate_all<WHITE, EVASIONS>(pos, moveList, target)
                         : generate_all<BLACK, EVASIONS>(pos, moveList, target);
  }

  // Find all the squares attacked by slider checkers. We will remove them from
  // the king evasions in order to skip known illegal moves, which avoids any
  // useless legality checks later on.
  while (sliders)
  {
      Square checksq = pop_lsb(&sliders);
      sliderAttacks |=  attacks_bb(~us, type_of(pos.piece_on(checksq)), checksq, pos.pieces() ^ ksq);
  }

  // Generate evasions for king, capture and non capture moves
  Bitboard b = (  (pos.attacks_from(us, KING, ksq) & pos.pieces())
                | (pos.moves_from(us, KING, ksq) & ~pos.pieces())) & ~pos.pieces(us) & ~sliderAttacks;
  while (b)
      moveList = make_move_and_gating<NORMAL>(pos, moveList, us, ksq, pop_lsb(&b));

  // Janggi king palace moves
  if (pos.diagonal_lines() & ksq)
  {
      PieceType movePt = pos.king_type();
      PieceType diagType = movePt == WAZIR ? FERS : movePt == SOLDIER ? PAWN : movePt == ROOK ? BISHOP : NO_PIECE_TYPE;
      if (diagType)
      {
          b = attacks_bb(us, diagType, ksq, pos.pieces()) & pos.board_bb(us, KING) & pos.diagonal_lines() & ~pos.pieces(us) & ~sliderAttacks;
          while (b)
              moveList = make_move_and_gating<SPECIAL>(pos, moveList, us, ksq, pop_lsb(&b));
      }
  }

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

  if (pos.is_immediate_game_end())
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
