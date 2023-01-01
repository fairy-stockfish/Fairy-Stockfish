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

#include "half_ka_v2_variants.h"

#include "../../position.h"

namespace Stockfish::Eval::NNUE::Features {

  // Map square to numbering on variant board
  inline Square to_variant_square(Square s, const Position& pos) {
    return Square(s - rank_of(s) * (FILE_MAX - pos.max_file()));
  }

  // Orient a square according to perspective (rotates by 180 for black)
  // Missing kings map to index 0 (SQ_A1)
  inline Square HalfKAv2Variants::orient(Color perspective, Square s, const Position& pos) {
    return s != SQ_NONE ? to_variant_square(  perspective == WHITE || (pos.capture_the_flag(BLACK) & Rank8BB) ? s
                                            : flip_rank(s, pos.max_rank()), pos) : SQ_A1;
  }

  // Index of a feature for a given king position and another piece on some square
  inline IndexType HalfKAv2Variants::make_index(Color perspective, Square s, Piece pc, Square ksq, const Position& pos) {
    return IndexType(orient(perspective, s, pos) + pos.variant()->pieceSquareIndex[perspective][pc] + pos.variant()->kingSquareIndex[ksq]);
  }

  // Index of a feature for a given king position and another piece on some square
  inline IndexType HalfKAv2Variants::make_index(Color perspective, int handCount, Piece pc, Square ksq, const Position& pos) {
    return IndexType(handCount + pos.variant()->pieceHandIndex[perspective][pc] + pos.variant()->kingSquareIndex[ksq]);
  }

  // Get a list of indices for active features
  void HalfKAv2Variants::append_active_indices(
    const Position& pos,
    Color perspective,
    ValueListInserter<IndexType> active
  ) {
    Square oriented_ksq = orient(perspective, pos.nnue_king_square(perspective), pos);
    Bitboard bb = pos.pieces(WHITE) | pos.pieces(BLACK);
    while (bb)
    {
      Square s = pop_lsb(bb);
      active.push_back(make_index(perspective, s, pos.piece_on(s), oriented_ksq, pos));
    }

    // Indices for pieces in hand
    if (pos.nnue_use_pockets())
      for (Color c : {WHITE, BLACK})
          for (PieceType pt : pos.piece_types())
              for (int i = 0; i < pos.count_in_hand(c, pt); i++)
                  active.push_back(make_index(perspective, i, make_piece(c, pt), oriented_ksq, pos));

  }

  // append_changed_indices() : get a list of indices for recently changed features

  void HalfKAv2Variants::append_changed_indices(
    Square ksq,
    StateInfo* st,
    Color perspective,
    ValueListInserter<IndexType> removed,
    ValueListInserter<IndexType> added,
    const Position& pos
  ) {
    const auto& dp = st->dirtyPiece;
    Square oriented_ksq = orient(perspective, ksq, pos);
    for (int i = 0; i < dp.dirty_num; ++i) {
      Piece pc = dp.piece[i];
      if (dp.from[i] != SQ_NONE)
        removed.push_back(make_index(perspective, dp.from[i], pc, oriented_ksq, pos));
      else if (dp.handPiece[i] != NO_PIECE)
        removed.push_back(make_index(perspective, dp.handCount[i] - 1, dp.handPiece[i], oriented_ksq, pos));
      if (dp.to[i] != SQ_NONE)
        added.push_back(make_index(perspective, dp.to[i], pc, oriented_ksq, pos));
      else if (dp.handPiece[i] != NO_PIECE)
        added.push_back(make_index(perspective, dp.handCount[i] - 1, dp.handPiece[i], oriented_ksq, pos));
    }
  }

  int HalfKAv2Variants::update_cost(StateInfo* st) {
    return st->dirtyPiece.dirty_num;
  }

  int HalfKAv2Variants::refresh_cost(const Position& pos) {
    return pos.count<ALL_PIECES>();
  }

  bool HalfKAv2Variants::requires_refresh(StateInfo* st, Color perspective, const Position& pos) {
    return st->dirtyPiece.piece[0] == make_piece(perspective, pos.nnue_king()) || pos.flip_enclosed_pieces();
  }

}  // namespace Stockfish::Eval::NNUE::Features
