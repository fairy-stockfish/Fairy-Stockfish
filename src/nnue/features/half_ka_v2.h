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

//Definition of input features HalfKAv2 of NNUE evaluation function

#ifndef NNUE_FEATURES_HALF_KA_V2_H_INCLUDED
#define NNUE_FEATURES_HALF_KA_V2_H_INCLUDED

#include "../nnue_common.h"

#include "../../evaluate.h"
#include "../../misc.h"

namespace Stockfish {
  struct StateInfo;
}

namespace Stockfish::Eval::NNUE::Features {

  // Feature HalfKAv2: Combination of the position of own king
  // and the position of pieces
  class HalfKAv2 {

    // unique number for each piece type on each square
    enum {
      PS_NONE     =  0,
      PS_W_PAWN   =  0,
      PS_B_PAWN   =  1 * SQUARE_NB_CHESS,
      PS_W_KNIGHT =  2 * SQUARE_NB_CHESS,
      PS_B_KNIGHT =  3 * SQUARE_NB_CHESS,
      PS_W_BISHOP =  4 * SQUARE_NB_CHESS,
      PS_B_BISHOP =  5 * SQUARE_NB_CHESS,
      PS_W_ROOK   =  6 * SQUARE_NB_CHESS,
      PS_B_ROOK   =  7 * SQUARE_NB_CHESS,
      PS_W_QUEEN  =  8 * SQUARE_NB_CHESS,
      PS_B_QUEEN  =  9 * SQUARE_NB_CHESS,
      PS_KING     =  10 * SQUARE_NB_CHESS,
      PS_NB = 11 * SQUARE_NB_CHESS
    };

    static constexpr uint32_t PieceSquareIndex[COLOR_NB][PIECE_NB] = {
      // convention: W - us, B - them
      // viewed from other side, W and B are reversed
      {
        PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_KING,

        PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_KING,
      },

      {
        PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_KING,

        PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
        PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_KING,
      }
    };
    // Check that the fragile array definition is correct
    static_assert(PieceSquareIndex[WHITE][make_piece(WHITE, PAWN)] == PS_W_PAWN);
    static_assert(PieceSquareIndex[WHITE][make_piece(WHITE, KING)] == PS_KING);
    static_assert(PieceSquareIndex[WHITE][make_piece(BLACK, PAWN)] == PS_B_PAWN);
    static_assert(PieceSquareIndex[WHITE][make_piece(BLACK, KING)] == PS_KING);


    // Orient a square according to perspective (rotates by 180 for black)
    static Square orient(Color perspective, Square s);

    // Index of a feature for a given king position and another piece on some square
    static IndexType make_index(Color perspective, Square s, Piece pc, Square ksq);

   public:
    // Feature name
    static constexpr const char* Name = "HalfKAv2(Friend)";

    // Hash value embedded in the evaluation file
    static constexpr std::uint32_t HashValue = 0x5f234cb8u;

    // Number of feature dimensions
    static constexpr IndexType Dimensions =
        static_cast<IndexType>(SQUARE_NB_CHESS) * static_cast<IndexType>(PS_NB);

    // Maximum number of simultaneously active features.
    static constexpr IndexType MaxActiveDimensions = 32;

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

#endif // #ifndef NNUE_FEATURES_HALF_KA_V2_H_INCLUDED
