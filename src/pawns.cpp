/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2019 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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
  constexpr Score Backward = S( 9, 24);
  constexpr Score Doubled  = S(11, 56);
  constexpr Score Isolated = S( 5, 15);

  // Connected pawn bonus by opposed, phalanx, #support and rank
  Score Connected[2][2][3][RANK_NB];

  // Strength of pawn shelter for our king by [distance from edge][rank].
  // RANK_1 = 0 is used for files where we have no pawn, or pawn is behind our king.
  constexpr Value ShelterStrength[int(FILE_NB) / 2][RANK_NB] = {
    { V( -6), V( 81), V( 93), V( 58), V( 39), V( 18), V(  25) },
    { V(-43), V( 61), V( 35), V(-49), V(-29), V(-11), V( -63) },
    { V(-10), V( 75), V( 23), V( -2), V( 32), V(  3), V( -45) },
    { V(-39), V(-13), V(-29), V(-52), V(-48), V(-67), V(-166) }
  };

  // Danger of enemy pawns moving toward our king by [distance from edge][rank].
  // RANK_1 = 0 is used for files where the enemy has no pawn, or their pawn
  // is behind our king.
  constexpr Value UnblockedStorm[int(FILE_NB) / 2][RANK_NB] = {
    { V( 89), V(107), V(123), V(93), V(57), V( 45), V( 51) },
    { V( 44), V(-18), V(123), V(46), V(39), V( -7), V( 23) },
    { V(  4), V( 52), V(162), V(37), V( 7), V(-14), V( -2) },
    { V(-10), V(-14), V( 90), V(15), V( 2), V( -7), V(-16) }
  };

  #undef S
  #undef V

  template<Color Us>
  Score evaluate(const Position& pos, Pawns::Entry* e) {

    constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
    constexpr Direction Up   = (Us == WHITE ? NORTH : SOUTH);

    Bitboard b, neighbours, stoppers, doubled, support, phalanx;
    Bitboard lever, leverPush;
    Square s;
    bool opposed, backward;
    Score score = SCORE_ZERO;
    const Square* pl = pos.squares<PAWN>(Us);

    Bitboard ourPawns   = pos.pieces(  Us, PAWN);
    Bitboard theirPawns = pos.pieces(Them, PAWN);

    e->passedPawns[Us] = e->pawnAttacksSpan[Us] = e->weakUnopposed[Us] = 0;
    e->semiopenFiles[Us] = 0xFF;
    e->kingSquares[Us]   = SQ_NONE;
    e->pawnAttacks[Us]   = pawn_attacks_bb<Us>(ourPawns);
    e->pawnsOnSquares[Us][BLACK] = popcount(ourPawns & DarkSquares);
    e->pawnsOnSquares[Us][WHITE] = pos.count<PAWN>(Us) - e->pawnsOnSquares[Us][BLACK];

    // Loop through all pawns of the current color and score each pawn
    while ((s = *pl++) != SQ_NONE)
    {
        assert(pos.piece_on(s) == make_piece(Us, PAWN));

        File f = file_of(s);

        e->semiopenFiles[Us]   &= ~(1 << f);
        e->pawnAttacksSpan[Us] |= pawn_attack_span(Us, s);

        // Flag the pawn
        opposed    = theirPawns & forward_file_bb(Us, s);
        stoppers   = theirPawns & passed_pawn_mask(Us, s);
        lever      = theirPawns & PseudoAttacks[Us][PAWN][s];
        leverPush  = relative_rank(Them, s, pos.max_rank()) > RANK_1 ? theirPawns & PseudoAttacks[Us][PAWN][s + Up] : 0;
        doubled    = relative_rank(Us, s, pos.max_rank()) > RANK_1 ? ourPawns & (s - Up) : 0;
        neighbours = ourPawns   & adjacent_files_bb(f);
        phalanx    = neighbours & rank_bb(s);
        support    = relative_rank(Us, s, pos.max_rank()) > RANK_1 ? neighbours & rank_bb(s - Up) : 0;

        // A pawn is backward when it is behind all pawns of the same color
        // on the adjacent files and cannot be safely advanced.
        backward =   relative_rank(Them, s, pos.max_rank()) > RANK_1
                  && !(ourPawns & pawn_attack_span(Them, s + Up))
                  && (stoppers & (leverPush | (s + Up)));

        // Passed pawns will be properly scored in evaluation because we need
        // full attack info to evaluate them. Include also not passed pawns
        // which could become passed after one or two pawn pushes when are
        // not attacked more times than defended.
        if (   !(stoppers ^ lever ^ leverPush)
            && popcount(support) >= popcount(lever) - 1
            && popcount(phalanx)   >= popcount(leverPush))
            e->passedPawns[Us] |= s;

        else if (   relative_rank(Them, s, pos.max_rank()) > RANK_1
                 && stoppers == SquareBB[s + Up]
                 && relative_rank(Us, s, pos.max_rank()) >= RANK_5)
        {
            b = shift<Up>(support) & ~theirPawns;
            while (b)
                if (!more_than_one(theirPawns & PseudoAttacks[Us][PAWN][pop_lsb(&b)]))
                    e->passedPawns[Us] |= s;
        }

        // Score this pawn
        if (support | phalanx)
            score += Connected[opposed][bool(phalanx)][popcount(support)][relative_rank(Us, s, pos.max_rank())];

        else if (!neighbours)
            score -= Isolated * (1 + 2 * pos.must_capture()), e->weakUnopposed[Us] += !opposed;

        else if (backward)
            score -= Backward, e->weakUnopposed[Us] += !opposed;

        if (doubled && !support)
            score -= Doubled;
    }

    const Square* pl_shogi = pos.squares<SHOGI_PAWN>(Us);

    ourPawns   = pos.pieces(Us,   SHOGI_PAWN);
    theirPawns = pos.pieces(Them, SHOGI_PAWN);

    // Loop through all shogi pawns of the current color and score each one
    while ((s = *pl_shogi++) != SQ_NONE)
    {
        assert(pos.piece_on(s) == make_piece(Us, SHOGI_PAWN));

        File f = file_of(s);

        e->semiopenFiles[Us] &= ~(1 << f);

        opposed    = theirPawns & forward_file_bb(Us, s);
        neighbours = ourPawns   & adjacent_files_bb(f);
        phalanx    = neighbours & rank_bb(s);

        if (phalanx)
            score += Connected[opposed][bool(phalanx)][0][relative_rank(Us, s, pos.max_rank())] / 2;

        else if (!neighbours)
            score -= Isolated / 2;
    }

    return score;
  }

} // namespace

namespace Pawns {

/// Pawns::init() initializes some tables needed by evaluation. Instead of using
/// hard-coded tables, when makes sense, we prefer to calculate them with a formula
/// to reduce independent parameters and to allow easier tuning and better insight.

void init() {

  static constexpr int Seed[RANK_NB] = { 0, 13, 24, 18, 65, 100, 175, 330 };

  for (int opposed = 0; opposed <= 1; ++opposed)
      for (int phalanx = 0; phalanx <= 1; ++phalanx)
          for (int support = 0; support <= 2; ++support)
              for (Rank r = RANK_1; r < RANK_MAX; ++r)
  {
      int v = 17 * support;
      v += (Seed[r] + (phalanx ? (Seed[r + 1] - Seed[r]) / 2 : 0)) >> opposed;

      Connected[opposed][phalanx][support][r] = make_score(v, v * (r - 2) / 4);
  }
}


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
  e->scores[WHITE] = evaluate<WHITE>(pos, e);
  e->scores[BLACK] = evaluate<BLACK>(pos, e);
  e->openFiles = popcount(e->semiopenFiles[WHITE] & e->semiopenFiles[BLACK]);
  e->asymmetry = popcount(  (e->passedPawns[WHITE]   | e->passedPawns[BLACK])
                          | (e->semiopenFiles[WHITE] ^ e->semiopenFiles[BLACK]));

  return e;
}


/// Entry::evaluate_shelter() calculates the shelter bonus and the storm
/// penalty for a king, looking at the king file and the two closest files.

template<Color Us>
Value Entry::evaluate_shelter(const Position& pos, Square ksq) {

  constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
  constexpr Direction Down = (Us == WHITE ? SOUTH : NORTH);
  Bitboard  BlockRanks = rank_bb(relative_rank(Us, RANK_1, pos.max_rank())) | rank_bb(relative_rank(Us, RANK_2, pos.max_rank()));

  Bitboard b = pos.pieces(PAWN, SHOGI_PAWN) & ~forward_ranks_bb(Them, ksq);
  Bitboard ourPawns = b & pos.pieces(Us);
  Bitboard theirPawns = b & pos.pieces(Them);

  Value safety = (shift<Down>(theirPawns) & (FileABB | FileHBB) & BlockRanks & ksq) ?
                 Value(374) : Value(5);

  File center = std::max(FILE_B, std::min(File(pos.max_file() - 1), file_of(ksq)));
  for (File f = File(center - 1); f <= File(center + 1); ++f)
  {
      b = ourPawns & file_bb(f);
      Rank ourRank = b ? relative_rank(Us, backmost_sq(Us, b), pos.max_rank()) : RANK_1;

      b = theirPawns & file_bb(f);
      Rank theirRank = b ? relative_rank(Us, frontmost_sq(Them, b), pos.max_rank()) : RANK_1;

      int d = std::min(std::min(f, File(pos.max_file() - f)), FILE_D);
      // higher weight for pawns on second rank and missing shelter in drop variants
      safety += ShelterStrength[d][ourRank] * (1 + (pos.captures_to_hand() && ourRank <= RANK_2));
      safety -= (ourRank && (ourRank == theirRank - 1)) ? 66 * (theirRank == RANK_3)
                                                        : UnblockedStorm[d][theirRank];
  }

  return safety;
}


/// Entry::do_king_safety() calculates a bonus for king safety. It is called only
/// when king square changes, which is about 20% of total king_safety() calls.

template<Color Us>
Score Entry::do_king_safety(const Position& pos) {

  Square ksq = pos.square<KING>(Us);
  kingSquares[Us] = ksq;
  castlingRights[Us] = pos.castling_rights(Us);
  int minKingPawnDistance = 0;

  Bitboard pawns = pos.pieces(Us, PAWN);
  if (pawns)
      while (!(DistanceRingBB[ksq][++minKingPawnDistance] & pawns)) {}

  Value bonus = evaluate_shelter<Us>(pos, ksq);

  // If we can castle use the bonus after the castling if it is bigger
  if (pos.can_castle(Us | KING_SIDE))
  {
      Square s = make_square(pos.castling_kingside_file(), Us == WHITE ? RANK_1 : pos.max_rank());
      bonus = std::max(bonus, evaluate_shelter<Us>(pos, s));
  }

  if (pos.can_castle(Us | QUEEN_SIDE))
  {
      Square s = make_square(pos.castling_queenside_file(), Us == WHITE ? RANK_1 : pos.max_rank());
      bonus = std::max(bonus, evaluate_shelter<Us>(pos, s));
  }

  return make_score(bonus, -16 * minKingPawnDistance);
}

// Explicit template instantiation
template Score Entry::do_king_safety<WHITE>(const Position& pos);
template Score Entry::do_king_safety<BLACK>(const Position& pos);

} // namespace Pawns
