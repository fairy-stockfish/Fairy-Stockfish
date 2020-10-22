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

#include <cassert>
#include <cstring>   // For std::memset

#include "material.h"
#include "thread.h"

using namespace std;

namespace {

  // Polynomial material imbalance parameters

  constexpr int QuadraticOurs[][PIECE_TYPE_NB] = {
    //            OUR PIECES
    // pair pawn knight bishop rook queen
    {1438                               }, // Bishop pair
    {  40,   38                         }, // Pawn
    {  32,  255, -62                    }, // Knight      OUR PIECES
    {   0,  104,   4,    0              }, // Bishop
    { -26,   -2,  47,   105,  -208      }, // Rook
    {-189,   24, 117,   133,  -134, -6  }  // Queen
  };

  constexpr int QuadraticTheirs[][PIECE_TYPE_NB] = {
    //           THEIR PIECES
    // pair pawn knight bishop rook queen
    {                                   }, // Bishop pair
    {  36,                              }, // Pawn
    {   9,   63,                        }, // Knight      OUR PIECES
    {  59,   65,  42,                   }, // Bishop
    {  46,   39,  24,   -24,            }, // Rook
    {  97,  100, -42,   137,  268,      }  // Queen
  };

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
    return    pos.promotion_piece_types().size() == 1
          &&  pos.promotion_piece_types().find(FERS) != pos.promotion_piece_types().end()
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
          && pos.count<PAWN  >(us) >= 1;
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
  int imbalance(const Position& pos, const int pieceCount[][PIECE_TYPE_NB]) {

    constexpr Color Them = ~Us;

    int bonus = 0;

    // Second-degree polynomial material imbalance, by Tord Romstad
    for (int pt1 = NO_PIECE_TYPE; pt1 <= QUEEN; ++pt1)
    {
        if (!pieceCount[Us][pt1] || (pos.extinction_value() == VALUE_MATE && pt1 != KNIGHT))
            continue;

        int v = QuadraticOurs[pt1][pt1] * pieceCount[Us][pt1];

        for (int pt2 = NO_PIECE_TYPE; pt2 < pt1; ++pt2)
            v +=  QuadraticOurs[pt1][pt2] * pieceCount[Us][pt2] * (pos.must_capture() && pt1 == KNIGHT && pt2 == PAWN ? 2 : pos.check_counting() && pt1 <= BISHOP ? 0 : 1)
                + QuadraticTheirs[pt1][pt2] * pieceCount[Them][pt2];

        bonus += pieceCount[Us][pt1] * v;
    }

    return bonus * (1 + pos.must_capture());
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
      for (PieceType pt : pos.piece_types())
          npm2 += (pos.count_in_hand(WHITE, pt) + pos.count_in_hand(BLACK, pt)) * PieceValue[MG][make_piece(WHITE, pt)];
      e->gamePhase = Phase(PHASE_MIDGAME * npm / std::max(int(npm + npm2), 1));
      int countAll = pos.count_with_hand(WHITE, ALL_PIECES) + pos.count_with_hand(BLACK, ALL_PIECES);
      e->materialDensity = (npm + npm2 + pos.count<PAWN>() * PawnValueMg) * countAll / ((pos.max_file() + 1) * (pos.max_rank() + 1));
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

  e->value = int16_t((imbalance<WHITE>(pos, pieceCount) - imbalance<BLACK>(pos, pieceCount)) / 16);
  return e;
}

} // namespace Material
