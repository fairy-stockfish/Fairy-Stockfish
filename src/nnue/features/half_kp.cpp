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

//Definition of input features HalfKP of NNUE evaluation function

#include "half_kp.h"
#include "index_list.h"

namespace Eval::NNUE::Features {

  // Map square to numbering on 8x8 board
  constexpr Square map_to_standard_board(Square s) {
    return Square(s - rank_of(s) * (FILE_MAX - FILE_H));
  }

  // Orient a square according to perspective (rotates by 180 for black)
  inline Square orient(const Position& pos, Color perspective, Square s) {
    return map_to_standard_board(  perspective == WHITE || (pos.capture_the_flag(BLACK) & Rank8BB) ? s
                                 : flip_rank(flip_file(s, pos.max_file()), pos.max_rank()));
  }

  // Index of a feature for a given king position and another piece on some square
  inline IndexType make_index(const Position& pos, Color perspective, Square s, Piece pc, Square ksq) {
    return IndexType(orient(pos, perspective, s) + kpp_board_index[perspective][pc] + PS_END * ksq);
  }

  // Get a list of indices for active features
  template <Side AssociatedKing>
  void HalfKP<AssociatedKing>::AppendActiveIndices(
      const Position& pos, Color perspective, IndexList* active) {

    Square ksq = orient(pos, perspective, pos.square<KING>(perspective));
    Bitboard bb = pos.pieces() & ~pos.pieces(KING);
    while (bb) {
      Square s = pop_lsb(&bb);
      active->push_back(make_index(pos, perspective, s, pos.piece_on(s), ksq));
    }
  }

  // Get a list of indices for recently changed features
  template <Side AssociatedKing>
  void HalfKP<AssociatedKing>::AppendChangedIndices(
      const Position& pos, const DirtyPiece& dp, Color perspective,
      IndexList* removed, IndexList* added) {

    Square ksq = orient(pos, perspective, pos.square<KING>(perspective));
    for (int i = 0; i < dp.dirty_num; ++i) {
      Piece pc = dp.piece[i];
      if (type_of(pc) == KING) continue;
      if (dp.from[i] != SQ_NONE)
        removed->push_back(make_index(pos, perspective, dp.from[i], pc, ksq));
      if (dp.to[i] != SQ_NONE)
        added->push_back(make_index(pos, perspective, dp.to[i], pc, ksq));
    }
  }

  template class HalfKP<Side::kFriend>;

}  // namespace Eval::NNUE::Features
