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

//Definition of input features HalfKP of NNUE evaluation function

#include "half_kp.h"

#include "../../position.h"

namespace Stockfish::Eval::NNUE::Features {

  // Map square to numbering on 8x8 board
  constexpr Square to_chess_square(Square s) {
    return Square(s - rank_of(s) * (FILE_MAX - FILE_H));
  }

  // Orient a square according to perspective (rotates by 180 for black)
  inline Square HalfKPChess::orient(Color perspective, Square s) {
    return Square(int(to_chess_square(s)) ^ (bool(perspective) * 63));
  }

  // Index of a feature for a given king position and another piece on some square
  inline IndexType HalfKPChess::make_index(Color perspective, Square s, Piece pc, Square ksq) {
    return IndexType(orient(perspective, s) + PieceSquareIndex[perspective][pc] + PS_NB * ksq);
  }

  // Get a list of indices for active features
  void HalfKPChess::append_active_indices(
    const Position& pos,
    Color perspective,
    ValueListInserter<IndexType> active
  ) {
    Square ksq = orient(perspective, pos.square(perspective, pos.nnue_king()));
    Bitboard bb = pos.pieces() & ~pos.pieces(pos.nnue_king());
    while (bb)
    {
      Square s = pop_lsb(bb);
      active.push_back(make_index(perspective, s, pos.piece_on(s), ksq));
    }
  }


  // append_changed_indices() : get a list of indices for recently changed features

  void HalfKPChess::append_changed_indices(
    Square ksq,
    StateInfo* st,
    Color perspective,
    ValueListInserter<IndexType> removed,
    ValueListInserter<IndexType> added,
    const Position& pos
  ) {
    const auto& dp = st->dirtyPiece;
    Square oriented_ksq = orient(perspective, ksq);
    for (int i = 0; i < dp.dirty_num; ++i) {
      Piece pc = dp.piece[i];
      if (type_of(pc) == pos.nnue_king()) continue;
      if (dp.from[i] != SQ_NONE)
        removed.push_back(make_index(perspective, dp.from[i], pc, oriented_ksq));
      if (dp.to[i] != SQ_NONE)
        added.push_back(make_index(perspective, dp.to[i], pc, oriented_ksq));
    }
  }

  int HalfKPChess::update_cost(StateInfo* st) {
    return st->dirtyPiece.dirty_num;
  }

  int HalfKPChess::refresh_cost(const Position& pos) {
    return pos.count<ALL_PIECES>() - 2;
  }

  bool HalfKPChess::requires_refresh(StateInfo* st, Color perspective, const Position& pos) {
    return st->dirtyPiece.piece[0] == make_piece(perspective, pos.nnue_king());
  }

}  // namespace Stockfish::Eval::NNUE::Features
