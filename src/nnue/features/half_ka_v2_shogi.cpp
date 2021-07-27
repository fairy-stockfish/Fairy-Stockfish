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

#include "half_ka_v2_shogi.h"

#include "../../position.h"

namespace Stockfish::Eval::NNUE::Features {

  constexpr Square rotate(Square s) {
    return Square(SQUARE_NB_SHOGI - 1 - int(s));
  }

  constexpr Square to_shogi_square(Square s) {
    return Square((8 - s % 12) * 9 + 8 - s / 12);
  }

  // Orient a square according to perspective (rotates by 180 for black)
  inline Square HalfKAv2Shogi::orient(Color perspective, Square s) {
    return perspective == WHITE ? to_shogi_square(s) : rotate(to_shogi_square(s));
  }

  // Index of a feature for a given king position and another piece on some square
  inline IndexType HalfKAv2Shogi::make_index(Color perspective, Square s, Piece pc, Square ksq) {
    return IndexType(orient(perspective, s) + PieceSquareIndexShogi[perspective][pc] + SHOGI_PS_NB * ksq);
  }

  // Index of a feature for a given king position and hand piece
  inline IndexType HalfKAv2Shogi::make_index(Color perspective, Color c, int hand_index, PieceType pt, Square ksq) {
    Color color = (c == perspective) ? WHITE : BLACK;
    return IndexType(hand_index + PieceSquareIndexShogiHand[color][pt] + SHOGI_PS_NB * ksq);
  }

  // Get a list of indices for active features
  void HalfKAv2Shogi::append_active_indices(
    const Position& pos,
    Color perspective,
    ValueListInserter<IndexType> active
  ) {
    Square ksq = orient(perspective, pos.square<KING>(perspective));
    Bitboard bb = pos.pieces();
    while (bb)
    {
      Square s = pop_lsb(bb);
      active.push_back(make_index(perspective, s, pos.piece_on(s), ksq));
    }

    // Indices for pieces in hand
    for (Color c : {WHITE, BLACK})
        for (PieceType pt : pos.piece_types())
            for (int i = 0; i < pos.count_in_hand(c, pt); i++)
                active.push_back(make_index(perspective, c, i, pt, ksq));
  }

  // append_changed_indices() : get a list of indices for recently changed features

  void HalfKAv2Shogi::append_changed_indices(
    Square ksq,
    StateInfo* st,
    Color perspective,
    ValueListInserter<IndexType> removed,
    ValueListInserter<IndexType> added
  ) {
    const auto& dp = st->dirtyPiece;
    Square oriented_ksq = orient(perspective, ksq);
    for (int i = 0; i < dp.dirty_num; ++i) {
      Piece pc = dp.piece[i];
      if (dp.from[i] != SQ_NONE)
        removed.push_back(make_index(perspective, dp.from[i], pc, oriented_ksq));
      else if (dp.dirty_num == 1)
      {
        Piece handPc = dp.handPiece[i];
        removed.push_back(make_index(perspective, color_of(handPc), dp.handCount[i], type_of(handPc), oriented_ksq));
      }
      if (dp.to[i] != SQ_NONE)
        added.push_back(make_index(perspective, dp.to[i], pc, oriented_ksq));
      else if (i == 1)
      {
        Piece handPc = dp.handPiece[i];
        added.push_back(make_index(perspective, color_of(handPc), dp.handCount[i] - 1, type_of(handPc), oriented_ksq));
      }
    }
  }

  int HalfKAv2Shogi::update_cost(StateInfo* st) {
    return st->dirtyPiece.dirty_num;
  }

  int HalfKAv2Shogi::refresh_cost(const Position& pos) {
    return pos.count<ALL_PIECES>();
  }

  bool HalfKAv2Shogi::requires_refresh(StateInfo* st, Color perspective) {
    return st->dirtyPiece.piece[0] == make_piece(perspective, KING);
  }


}  // namespace Stockfish::Eval::NNUE::Features
