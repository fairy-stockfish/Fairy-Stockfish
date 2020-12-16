/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2020 The Stockfish developers (see AUTHORS file)

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

#include <algorithm>
#include <cassert>

#include "bitboard.h"
#include "pawns.h"
#include "position.h"
#include "thread.h"

namespace {

  #define V Value
  #define S(mg, eg) make_score(mg, eg)

  // Pawn penalties
  constexpr Score Backward      = S( 8, 25);
  constexpr Score Doubled       = S(10, 55);
  constexpr Score Isolated      = S( 3, 15);
  constexpr Score WeakLever     = S( 3, 55);
  constexpr Score WeakUnopposed = S(13, 25);

  // Bonus for blocked pawns at 5th or 6th rank
  constexpr Score BlockedPawn[RANK_NB - 5] = { S(-13, -4), S(-5, 2) };

  constexpr Score BlockedStorm[RANK_NB] = {
    S(0, 0), S(0, 0), S(76, 78), S(-10, 15), S(-7, 10), S(-4, 6), S(-1, 2)
  };

  // Connected pawn bonus
  constexpr int Connected[RANK_NB] = { 0, 5, 7, 11, 24, 48, 86 };

  // Strength of pawn shelter for our king by [distance from edge][rank].
  // RANK_1 = 0 is used for files where we have no pawn, or pawn is behind our king.
  constexpr Value ShelterStrength[int(FILE_NB) / 2][RANK_NB] = {
    { V( -5), V( 82), V( 92), V( 54), V( 36), V( 22), V(  28) },
    { V(-44), V( 63), V( 33), V(-50), V(-30), V(-12), V( -62) },
    { V(-11), V( 77), V( 22), V( -6), V( 31), V(  8), V( -45) },
    { V(-39), V(-12), V(-29), V(-50), V(-43), V(-68), V(-164) }
  };

  // Danger of enemy pawns moving toward our king by [distance from edge][rank].
  // RANK_1 = 0 is used for files where the enemy has no pawn, or their pawn
  // is behind our king. Note that UnblockedStorm[0][1-2] accommodate opponent pawn
  // on edge, likely blocked by our king.
  constexpr Value UnblockedStorm[int(FILE_NB) / 2][RANK_NB] = {
    { V( 87), V(-288), V(-168), V( 96), V( 47), V( 44), V( 46) },
    { V( 42), V( -25), V( 120), V( 45), V( 34), V( -9), V( 24) },
    { V( -8), V(  51), V( 167), V( 35), V( -4), V(-16), V(-12) },
    { V(-17), V( -13), V( 100), V(  4), V(  9), V(-16), V(-31) }
  };

  // KingOnFile[semi-open Us][semi-open Them] contains bonuses/penalties
  // for king when the king is on a semi-open or open file.
  constexpr Score KingOnFile[2][2] = {{ S(-19,12), S(-6, 7)  },
                                     {  S(  0, 2), S( 6,-5) }};


  // Variant bonuses
  constexpr int HordeConnected[2][RANK_NB] = {{   5, 10,  20, 55, 55, 100, 80 },
                                              { -10,  5, -10,  5, 25,  40, 30 }};

  #undef S
  #undef V


  /// evaluate() calculates a score for the static pawn structure of the given position.
  /// We cannot use the location of pieces or king in this function, as the evaluation
  /// of the pawn structure will be stored in a small cache for speed reasons, and will
  /// be re-used even when the pieces have moved.

  template<Color Us>
  Score evaluate(const Position& pos, Pawns::Entry* e) {

    constexpr Color     Them = ~Us;
    constexpr Direction Up   = pawn_push(Us);

    Bitboard neighbours, stoppers, support, phalanx, opposed;
    Bitboard lever, leverPush, blocked;
    Square s;
    bool backward, passed, doubled;
    Score score = SCORE_ZERO;
    const Square* pl = pos.squares<PAWN>(Us);

    Bitboard ourPawns   = pos.pieces(  Us, PAWN);
    Bitboard theirPawns = pos.pieces(Them, PAWN);

    Bitboard doubleAttackThem = pawn_double_attacks_bb<Them>(theirPawns);

    e->passedPawns[Us] = 0;
    e->kingSquares[Us] = SQ_NONE;
    e->pawnAttacks[Us] = e->pawnAttacksSpan[Us] = pawn_attacks_bb<Us>(ourPawns);
    e->blockedCount += popcount(shift<Up>(ourPawns) & (theirPawns | doubleAttackThem));

    // Loop through all pawns of the current color and score each pawn
    while ((s = *pl++) != SQ_NONE)
    {
        assert(pos.piece_on(s) == make_piece(Us, PAWN));

        Rank r = relative_rank(Us, s, pos.max_rank());

        // Flag the pawn
        opposed    = theirPawns & forward_file_bb(Us, s);
        blocked    = is_ok(s + Up) ? theirPawns & (s + Up) : Bitboard(0);
        stoppers   = theirPawns & passed_pawn_span(Us, s);
        lever      = theirPawns & pawn_attacks_bb(Us, s);
        leverPush  = relative_rank(Them, s, pos.max_rank()) > RANK_1 ? theirPawns & pawn_attacks_bb(Us, s + Up) : Bitboard(0);
        doubled    = r > RANK_1 ? ourPawns & (s - Up) : Bitboard(0);
        neighbours = ourPawns   & adjacent_files_bb(s);
        phalanx    = neighbours & rank_bb(s);
        support    = r > RANK_1 ? neighbours & rank_bb(s - Up) : Bitboard(0);

        // A pawn is backward when it is behind all pawns of the same color on
        // the adjacent files and cannot safely advance.
        backward =   is_ok(s + Up)
                  && !(neighbours & forward_ranks_bb(Them, s + Up))
                  && (stoppers & blocked);

        // Compute additional span if pawn is not backward nor blocked
        if (!backward && !blocked)
            e->pawnAttacksSpan[Us] |= pawn_attack_span(Us, s);

        // A pawn is passed if one of the three following conditions is true:
        // (a) there is no stoppers except some levers
        // (b) the only stoppers are the leverPush, but we outnumber them
        // (c) there is only one front stopper which can be levered.
        //     (Refined in Evaluation::passed)
        passed =   !(stoppers ^ lever)
                || (   !(stoppers ^ leverPush)
                    && popcount(phalanx) >= popcount(leverPush))
                || (   stoppers == blocked && r >= RANK_5
                    && (shift<Up>(support) & ~(theirPawns | doubleAttackThem)));

        passed &= !(forward_file_bb(Us, s) & ourPawns);

        // Passed pawns will be properly scored later in evaluation when we have
        // full attack info.
        if (passed && is_ok(s + Up) && (r < pos.promotion_rank() || !pos.mandatory_pawn_promotion()))
            e->passedPawns[Us] |= s;

        // Score this pawn
        if ((support | phalanx) && (r < pos.promotion_rank() || !pos.mandatory_pawn_promotion()))
        {
            int v =  Connected[r] * (2 + bool(phalanx) - bool(opposed)) * (r == RANK_2 && pos.captures_to_hand() ? 3 : 1)
                   + 22 * popcount(support);
            if (pos.count<PAWN>(Us) > popcount(pos.board_bb()) / 4)
                v = popcount(support | phalanx) * HordeConnected[bool(opposed)][r];

            score += make_score(v, v * (r - 2) / 4);
        }

        else if (!neighbours)
        {
            if (     opposed
                &&  (ourPawns & forward_file_bb(Them, s))
                && !(theirPawns & adjacent_files_bb(s)))
                score -= Doubled * (1 + 2 * pos.must_capture());
            else
                score -=  Isolated * (1 + 2 * pos.must_capture())
                        + WeakUnopposed * !opposed;
        }

        else if (backward)
            score -=  Backward
                    + WeakUnopposed * !opposed;

        if (!support)
            score -=  Doubled * doubled
                    + WeakLever * more_than_one(lever);

        if (blocked && r >= RANK_5)
            score += BlockedPawn[r - RANK_5];
    }

    // Double pawn evaluation if there are no non-pawn pieces
    if (pos.count<ALL_PIECES>(Us) == pos.count<PAWN>(Us))
        score = score * 2;

    return score;
  }

} // namespace

namespace Pawns {


/// Pawns::probe() looks up the current position's pawns configuration in
/// the pawns hash table. It returns a pointer to the Entry if the position
/// is found. Otherwise a new Entry is computed and stored there, so we don't
/// have to recompute all when the same pawns configuration occurs again.

Entry* probe(const Position& pos) {

  Key key = pos.pawn_key();
  Entry* e = pos.this_thread()->pawnsTable[key];

  if (e->key == key && !pos.pieces(SHOGI_PAWN))
      return e;

  e->key = key;
  e->blockedCount = 0;
  e->scores[WHITE] = evaluate<WHITE>(pos, e);
  e->scores[BLACK] = evaluate<BLACK>(pos, e);

  return e;
}


/// Entry::evaluate_shelter() calculates the shelter bonus and the storm
/// penalty for a king, looking at the king file and the two closest files.

template<Color Us>
Score Entry::evaluate_shelter(const Position& pos, Square ksq) const {

  constexpr Color Them = ~Us;

  Bitboard b = pos.pieces(PAWN, SHOGI_PAWN) & ~forward_ranks_bb(Them, ksq);
  Bitboard ourPawns = b & pos.pieces(Us) & ~pawnAttacks[Them];
  Bitboard theirPawns = b & pos.pieces(Them);

  Score bonus = make_score(5, 5);

  File center = std::clamp(file_of(ksq), FILE_B, File(pos.max_file() - 1));
  for (File f = File(center - 1); f <= File(center + 1); ++f)
  {
      b = ourPawns & file_bb(f);
      int ourRank = b ? relative_rank(Us, frontmost_sq(Them, b), pos.max_rank()) : 0;

      b = theirPawns & file_bb(f);
      int theirRank = b ? relative_rank(Us, frontmost_sq(Them, b), pos.max_rank()) : 0;

      int d = std::min(File(edge_distance(f, pos.max_file())), FILE_D);
      bonus += make_score(ShelterStrength[d][ourRank], 0) * (1 + (pos.captures_to_hand() && ourRank <= RANK_2)
                                                               + (pos.check_counting() && d == 0 && ourRank == RANK_2));

      if (ourRank && (ourRank == theirRank - 1))
          bonus -= BlockedStorm[theirRank];
      else
          bonus -= make_score(UnblockedStorm[d][theirRank], 0);
  }

  // King On File
  bonus -= KingOnFile[pos.is_on_semiopen_file(Us, ksq)][pos.is_on_semiopen_file(Them, ksq)];

  return bonus;
}


/// Entry::do_king_safety() calculates a bonus for king safety. It is called only
/// when king square changes, which is about 20% of total king_safety() calls.

template<Color Us>
Score Entry::do_king_safety(const Position& pos) {

  Square ksq = pos.square<KING>(Us);
  kingSquares[Us] = ksq;
  castlingRights[Us] = pos.castling_rights(Us);
  auto compare = [](Score a, Score b) { return mg_value(a) < mg_value(b); };

  Score shelter = evaluate_shelter<Us>(pos, ksq);

  // If we can castle use the bonus after castling if it is bigger

  if (pos.can_castle(Us & KING_SIDE))
      shelter = std::max(shelter, evaluate_shelter<Us>(pos, make_square(pos.castling_kingside_file(), pos.castling_rank(Us))), compare);

  if (pos.can_castle(Us & QUEEN_SIDE))
      shelter = std::max(shelter, evaluate_shelter<Us>(pos, make_square(pos.castling_queenside_file(), pos.castling_rank(Us))), compare);

  // In endgame we like to bring our king near our closest pawn
  Bitboard pawns = pos.pieces(Us, PAWN);
  int minPawnDist = 6;

  if (pawns & attacks_bb<KING>(ksq))
      minPawnDist = 1;
  else while (pawns)
      minPawnDist = std::min(minPawnDist, distance(ksq, pop_lsb(&pawns)));

  return shelter - make_score(0, 16 * minPawnDist);
}

// Explicit template instantiation
template Score Entry::do_king_safety<WHITE>(const Position& pos);
template Score Entry::do_king_safety<BLACK>(const Position& pos);

} // namespace Pawns
