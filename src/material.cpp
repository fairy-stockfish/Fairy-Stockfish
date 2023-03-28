/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2022 The Stockfish developers (see AUTHORS file)

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
#include <cstring>   // For std::memset

#include "material.h"
#include "thread.h"

using namespace std;

namespace Stockfish {

namespace {
  #define S(mg, eg) make_score(mg, eg)

  // Polynomial material imbalance parameters

  // One Score parameter for each pair (our piece, another of our pieces)
  constexpr Score QuadraticOurs[][PIECE_TYPE_NB] = {
    // OUR PIECE 2
    // bishop pair    pawn         knight       bishop       rook           queen
    {S(1419, 1455)                                                                  }, // Bishop pair
    {S( 101,   28), S( 37,  39)                                                     }, // Pawn
    {S(  57,   64), S(249, 187), S(-49, -62)                                        }, // Knight      OUR PIECE 1
    {S(   0,    0), S(118, 137), S( 10,  27), S(  0,   0)                           }, // Bishop
    {S( -63,  -68), S( -5,   3), S(100,  81), S(132, 118), S(-246, -244)            }, // Rook
    {S(-210, -211), S( 37,  14), S(147, 141), S(161, 105), S(-158, -174), S(-9,-31) }  // Queen
  };

  // One Score parameter for each pair (our piece, their piece)
  constexpr Score QuadraticTheirs[][PIECE_TYPE_NB] = {
    // THEIR PIECE
    // bishop pair   pawn         knight       bishop       rook         queen
    {                                                                               }, // Bishop pair
    {S(  33,  30)                                                                   }, // Pawn
    {S(  46,  18), S(106,  84)                                                      }, // Knight      OUR PIECE
    {S(  75,  35), S( 59,  44), S( 60,  15)                                         }, // Bishop
    {S(  26,  35), S(  6,  22), S( 38,  39), S(-12,  -2)                            }, // Rook
    {S(  97,  93), S(100, 163), S(-58, -91), S(112, 192), S(276, 225)               }  // Queen
  };

  #undef S

  // Endgame evaluation and scaling functions are accessed directly and not through
  // the function maps because they correspond to more than one material hash key.
  Endgame<KFsPsK> EvaluateKFsPsK[] = { Endgame<KFsPsK>(WHITE), Endgame<KFsPsK>(BLACK) };
  Endgame<KXK>    EvaluateKXK[] = { Endgame<KXK>(WHITE),    Endgame<KXK>(BLACK) };

  Endgame<KBPsK>  ScaleKBPsK[]  = { Endgame<KBPsK>(WHITE),  Endgame<KBPsK>(BLACK) };
  Endgame<KQKRPs> ScaleKQKRPs[] = { Endgame<KQKRPs>(WHITE), Endgame<KQKRPs>(BLACK) };
  Endgame<KPsK>   ScaleKPsK[]   = { Endgame<KPsK>(WHITE),   Endgame<KPsK>(BLACK) };
  Endgame<KPKP>   ScaleKPKP[]   = { Endgame<KPKP>(WHITE),   Endgame<KPKP>(BLACK) };

  // Helper used to detect a given material distribution
  bool is_KFsPsK(const Position& pos, Color us) {
    return    (pos.promotion_piece_types(us) == piece_set(FERS))
          && !more_than_one(pos.pieces(~us))
          && (pos.count<FERS>(us) || pos.count<PAWN>(us))
          && !(pos.count<ALL_PIECES>(us) - pos.count<FERS>(us) - pos.count<PAWN>(us) - pos.count<KING>(us));
  }

  bool is_KXK(const Position& pos, Color us) {
    return  !more_than_one(pos.pieces(~us))
          && pos.non_pawn_material(us) >= std::min(RookValueMg, 2 * SilverValueMg);
  }

  bool is_KBPsK(const Position& pos, Color us) {
    return   pos.non_pawn_material(us) == BishopValueMg
          && pos.count<PAWN>(us) >= 1;
  }

  bool is_KQKRPs(const Position& pos, Color us) {
    return  !pos.count<PAWN>(us)
          && pos.non_pawn_material(us) == QueenValueMg
          && pos.count<ROOK>(~us) == 1
          && pos.count<PAWN>(~us) >= 1;
  }


  /// imbalance() calculates the imbalance by comparing the piece count of each
  /// piece type for both colors.

  template<Color Us>
  Score imbalance(const Position& pos, const int pieceCount[][PIECE_TYPE_NB]) {

    constexpr Color Them = ~Us;

    Score bonus = SCORE_ZERO;

    // Second-degree polynomial material imbalance, by Tord Romstad
    for (int pt1 = NO_PIECE_TYPE; pt1 <= QUEEN; ++pt1)
    {
        if (!pieceCount[Us][pt1])
            continue;

        int v = QuadraticOurs[pt1][pt1] * pieceCount[Us][pt1];

        for (int pt2 = NO_PIECE_TYPE; pt2 < pt1; ++pt2)
            v +=  QuadraticOurs[pt1][pt2] * pieceCount[Us][pt2]
                + QuadraticTheirs[pt1][pt2] * pieceCount[Them][pt2];

        bonus += pieceCount[Us][pt1] * v;
    }

    if (pos.must_capture())
        bonus +=  (make_score(mg_value(QuadraticOurs[PAWN][PAWN]), 0) * pieceCount[Us][PAWN]
                   + 3 * QuadraticOurs[KNIGHT][PAWN] * pieceCount[Us][KNIGHT]
                   + QuadraticOurs[QUEEN][PAWN] * pieceCount[Us][QUEEN]) * pieceCount[Us][PAWN]
                + make_score( mg_value(QuadraticOurs[KNIGHT][KNIGHT]),
                             -eg_value(QuadraticOurs[KNIGHT][KNIGHT])) * pieceCount[Us][KNIGHT] * pieceCount[Us][KNIGHT];
    else if (pos.blast_on_capture())
        bonus -= make_score(mg_value(QuadraticOurs[KNIGHT][PAWN]) * pieceCount[Us][KNIGHT] * pieceCount[Us][PAWN] / 2, 0);
    else if (pos.check_counting())
        bonus -= 2 * QuadraticOurs[PAWN][PAWN] * pieceCount[Us][PAWN] * pieceCount[Us][PAWN];
    else if (pos.captures_to_hand())
        bonus -= bonus / 10;

    return bonus;
  }

} // namespace

namespace Material {


/// Material::probe() looks up the current position's material configuration in
/// the material hash table. It returns a pointer to the Entry if the position
/// is found. Otherwise a new Entry is computed and stored there, so we don't
/// have to recompute all when the same material configuration occurs again.

Entry* probe(const Position& pos) {

  Key key = pos.material_key();
  Entry* e = pos.this_thread()->materialTable[key];

  if (e->key == key)
      return e;

  std::memset(e, 0, sizeof(Entry));
  e->key = key;
  e->factor[WHITE] = e->factor[BLACK] = (uint8_t)SCALE_FACTOR_NORMAL;

  Value npm_w = pos.non_pawn_material(WHITE);
  Value npm_b = pos.non_pawn_material(BLACK);
  Value npm   = std::clamp(npm_w + npm_b, EndgameLimit, MidgameLimit);

  // Map total non-pawn material into [PHASE_ENDGAME, PHASE_MIDGAME]
  if (pos.captures_to_hand() || pos.two_boards())
  {
      Value npm2 = VALUE_ZERO;
      for (PieceSet ps = pos.piece_types(); ps;)
      {
          PieceType pt = pop_lsb(ps);
          npm2 += pos.count_in_hand(pt) * PieceValue[MG][make_piece(WHITE, pt)];
      }
      e->gamePhase = Phase(PHASE_MIDGAME * npm / std::max(int(npm + npm2), 1));
      int countAll = pos.count_with_hand(WHITE, ALL_PIECES) + pos.count_with_hand(BLACK, ALL_PIECES);
      e->materialDensity = (npm + npm2 + pos.count<PAWN>() * PawnValueMg) * countAll / (pos.files() * pos.ranks());
  }
  else
      e->gamePhase = Phase(((npm - EndgameLimit) * PHASE_MIDGAME) / (MidgameLimit - EndgameLimit));

  if (pos.endgame_eval())
  {
  // Let's look if we have a specialized evaluation function for this particular
  // material configuration. Firstly we look for a fixed configuration one, then
  // for a generic one if the previous search failed.
  if ((e->evaluationFunction = Endgames::probe<Value>(key)) != nullptr)
      return e;

  for (Color c : { WHITE, BLACK })
      if (is_KFsPsK(pos, c))
      {
          e->evaluationFunction = &EvaluateKFsPsK[c];
          return e;
      }

  for (Color c : { WHITE, BLACK })
      if (is_KXK(pos, c))
      {
          e->evaluationFunction = &EvaluateKXK[c];
          return e;
      }

  // OK, we didn't find any special evaluation function for the current material
  // configuration. Is there a suitable specialized scaling function?
  const auto* sf = Endgames::probe<ScaleFactor>(key);

  if (sf)
  {
      e->scalingFunction[sf->strongSide] = sf; // Only strong color assigned
      return e;
  }

  // We didn't find any specialized scaling function, so fall back on generic
  // ones that refer to more than one material distribution. Note that in this
  // case we don't return after setting the function.
  for (Color c : { WHITE, BLACK })
  {
    if (is_KBPsK(pos, c))
        e->scalingFunction[c] = &ScaleKBPsK[c];

    else if (is_KQKRPs(pos, c))
        e->scalingFunction[c] = &ScaleKQKRPs[c];
  }

  if (npm_w + npm_b == VALUE_ZERO && pos.pieces(PAWN)) // Only pawns on the board
  {
      if (!pos.count<PAWN>(BLACK))
      {
          assert(pos.count<PAWN>(WHITE) >= 2);

          e->scalingFunction[WHITE] = &ScaleKPsK[WHITE];
      }
      else if (!pos.count<PAWN>(WHITE))
      {
          assert(pos.count<PAWN>(BLACK) >= 2);

          e->scalingFunction[BLACK] = &ScaleKPsK[BLACK];
      }
      else if (pos.count<PAWN>(WHITE) == 1 && pos.count<PAWN>(BLACK) == 1)
      {
          // This is a special case because we set scaling functions
          // for both colors instead of only one.
          e->scalingFunction[WHITE] = &ScaleKPKP[WHITE];
          e->scalingFunction[BLACK] = &ScaleKPKP[BLACK];
      }
  }

  // Zero or just one pawn makes it difficult to win, even with a small material
  // advantage. This catches some trivial draws like KK, KBK and KNK and gives a
  // drawish scale factor for cases such as KRKBP and KmmKm (except for KBBKN).
  if (!pos.count<PAWN>(WHITE) && npm_w - npm_b <= BishopValueMg)
      e->factor[WHITE] = uint8_t(npm_w <  RookValueMg && pos.count<ALL_PIECES>(WHITE) <= 2 ? SCALE_FACTOR_DRAW :
                                 npm_b <= BishopValueMg && pos.count<ALL_PIECES>(WHITE) <= 3 ? 4 : 14);

  if (!pos.count<PAWN>(BLACK) && npm_b - npm_w <= BishopValueMg)
      e->factor[BLACK] = uint8_t(npm_b <  RookValueMg && pos.count<ALL_PIECES>(BLACK) <= 2 ? SCALE_FACTOR_DRAW :
                                 npm_w <= BishopValueMg && pos.count<ALL_PIECES>(BLACK) <= 3 ? 4 : 14);
  }

  // Evaluate the material imbalance. We use PIECE_TYPE_NONE as a place holder
  // for the bishop pair "extended piece", which allows us to be more flexible
  // in defining bishop pair bonuses.
  const int pieceCount[COLOR_NB][PIECE_TYPE_NB] = {
  { pos.count<BISHOP>(WHITE) > 1, pos.count<PAWN>(WHITE), pos.count<KNIGHT>(WHITE),
    pos.count<BISHOP>(WHITE)    , pos.count<ROOK>(WHITE), pos.count<QUEEN >(WHITE) },
  { pos.count<BISHOP>(BLACK) > 1, pos.count<PAWN>(BLACK), pos.count<KNIGHT>(BLACK),
    pos.count<BISHOP>(BLACK)    , pos.count<ROOK>(BLACK), pos.count<QUEEN >(BLACK) } };

  e->score = (imbalance<WHITE>(pos, pieceCount) - imbalance<BLACK>(pos, pieceCount)) / 16;
  return e;
}

} // namespace Material

} // namespace Stockfish
