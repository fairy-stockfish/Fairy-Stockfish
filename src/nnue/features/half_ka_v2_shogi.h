/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

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

//Definition of input features HalfKAv2 of NNUE evaluation function

#ifndef NNUE_FEATURES_HALF_KA_V2_SHOGI_H_INCLUDED
#define NNUE_FEATURES_HALF_KA_V2_SHOGI_H_INCLUDED

#include "../nnue_common.h"

#include "../../evaluate.h"
#include "../../misc.h"

namespace Stockfish {
  struct StateInfo;
}

namespace Stockfish::Eval::NNUE::Features {

  // Feature HalfKAv2: Combination of the position of own king
  // and the position of pieces
  class HalfKAv2Shogi {

    // unique number for each piece type on each square
    enum {
      PS_NONE             =   0,
      SHOGI_HAND_W_PAWN   =   0,
      SHOGI_HAND_B_PAWN   =  19,
      SHOGI_HAND_W_LANCE  =  38,
      SHOGI_HAND_B_LANCE  =  43,
      SHOGI_HAND_W_KNIGHT =  48,
      SHOGI_HAND_B_KNIGHT =  53,
      SHOGI_HAND_W_SILVER =  58,
      SHOGI_HAND_B_SILVER =  63,
      SHOGI_HAND_W_GOLD   =  68,
      SHOGI_HAND_B_GOLD   =  73,
      SHOGI_HAND_W_BISHOP =  78,
      SHOGI_HAND_B_BISHOP =  81,
      SHOGI_HAND_W_ROOK   =  84,
      SHOGI_HAND_B_ROOK   =  87,
      SHOGI_HAND_END      =  89,

      SHOGI_PS_W_PAWN     =  SHOGI_HAND_END,
      SHOGI_PS_B_PAWN     =  1 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_LANCE    =  2 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_LANCE    =  3 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_KNIGHT   =  4 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_KNIGHT   =  5 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_SILVER   =  6 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_SILVER   =  7 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_GOLD     =  8 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_GOLD     =  9 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_BISHOP   = 10 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_BISHOP   = 11 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_HORSE    = 12 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_HORSE    = 13 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_ROOK     = 14 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_ROOK     = 15 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_W_DRAGON   = 16 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_B_DRAGON   = 17 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_KING       = 18 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
      SHOGI_PS_NB         = 19 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    };

    static constexpr uint32_t PieceSquareIndexShogi[COLOR_NB][PIECE_NB] = {
      // convention: W - us, B - them
      // viewed from other side, W and B are reversed
      {
        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_BISHOP, SHOGI_PS_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, SHOGI_PS_W_SILVER, PS_NONE, SHOGI_PS_W_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_PAWN, SHOGI_PS_W_LANCE, SHOGI_PS_W_KNIGHT, SHOGI_PS_W_GOLD, SHOGI_PS_W_HORSE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_KING,

        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_BISHOP, SHOGI_PS_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, SHOGI_PS_B_SILVER, PS_NONE, SHOGI_PS_B_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_PAWN, SHOGI_PS_B_LANCE, SHOGI_PS_B_KNIGHT, SHOGI_PS_B_GOLD, SHOGI_PS_B_HORSE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_KING
      },

      {
        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_BISHOP, SHOGI_PS_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, SHOGI_PS_B_SILVER, PS_NONE, SHOGI_PS_B_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_PAWN, SHOGI_PS_B_LANCE, SHOGI_PS_B_KNIGHT, SHOGI_PS_B_GOLD, SHOGI_PS_B_HORSE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_KING,

        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_BISHOP, SHOGI_PS_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, SHOGI_PS_W_SILVER, PS_NONE, SHOGI_PS_W_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_PAWN, SHOGI_PS_W_LANCE, SHOGI_PS_W_KNIGHT, SHOGI_PS_W_GOLD, SHOGI_PS_W_HORSE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_KING
      }
    };
    static_assert(PieceSquareIndexShogi[WHITE][make_piece(WHITE, SHOGI_PAWN)] == SHOGI_PS_W_PAWN);
    static_assert(PieceSquareIndexShogi[WHITE][make_piece(WHITE, KING)] == SHOGI_PS_KING);
    static_assert(PieceSquareIndexShogi[WHITE][make_piece(BLACK, SHOGI_PAWN)] == SHOGI_PS_B_PAWN);
    static_assert(PieceSquareIndexShogi[WHITE][make_piece(BLACK, KING)] == SHOGI_PS_KING);

    static constexpr uint32_t PieceSquareIndexShogiHand[COLOR_NB][PIECE_TYPE_NB] = {
      // convention: W - us, B - them
      // viewed from other side, W and B are reversed
      {
        PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_W_BISHOP, SHOGI_HAND_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, SHOGI_HAND_W_SILVER, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_W_PAWN, SHOGI_HAND_W_LANCE, SHOGI_HAND_W_KNIGHT, SHOGI_HAND_W_GOLD, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE
      },

      {
        PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_B_BISHOP, SHOGI_HAND_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, SHOGI_HAND_B_SILVER, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_B_PAWN, SHOGI_HAND_B_LANCE, SHOGI_HAND_B_KNIGHT, SHOGI_HAND_B_GOLD, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE
      }
    };
    static_assert(PieceSquareIndexShogiHand[WHITE][SHOGI_PAWN] == SHOGI_HAND_W_PAWN);
    static_assert(PieceSquareIndexShogiHand[WHITE][GOLD] == SHOGI_HAND_W_GOLD);
    static_assert(PieceSquareIndexShogiHand[BLACK][SHOGI_PAWN] == SHOGI_HAND_B_PAWN);
    static_assert(PieceSquareIndexShogiHand[BLACK][GOLD] == SHOGI_HAND_B_GOLD);

    // Orient a square according to perspective (rotates by 180 for black)
    static Square orient(Color perspective, Square s);

    // Index of a feature for a given king position and another piece on some square
    static IndexType make_index(Color perspective, Square s, Piece pc, Square ksq);

    // Index of a feature for a given king position and a piece in hand
    static IndexType make_index(Color perspective, Color c, int hand_index, PieceType pt, Square ksq);

   public:
    // Feature name
    static constexpr const char* Name = "HalfKAv2(Friend)";

    // Hash value embedded in the evaluation file
    static constexpr std::uint32_t HashValue = 0x5f234cb8u;

    // Number of feature dimensions
    static constexpr IndexType Dimensions =
        static_cast<IndexType>(SQUARE_NB_SHOGI) * static_cast<IndexType>(SHOGI_PS_NB);

    // Maximum number of simultaneously active features.
    static constexpr IndexType MaxActiveDimensions = 40;

    // Get a list of indices for active features
    static void append_active_indices(
      const Position& pos,
      Color perspective,
      ValueListInserter<IndexType> active);

    // Get a list of indices for recently changed features
    static void append_changed_indices(
      Square ksq,
      StateInfo* st,
      Color perspective,
      ValueListInserter<IndexType> removed,
      ValueListInserter<IndexType> added);

    // Returns the cost of updating one perspective, the most costly one.
    // Assumes no refresh needed.
    static int update_cost(StateInfo* st);
    static int refresh_cost(const Position& pos);

    // Returns whether the change stored in this StateInfo means that
    // a full accumulator refresh is required.
    static bool requires_refresh(StateInfo* st, Color perspective);
  };

}  // namespace Stockfish::Eval::NNUE::Features

#endif // #ifndef NNUE_FEATURES_HALF_KA_V2_SHOGI_H_INCLUDED
