/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

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

// Definition of input features HalfKP of the NNUE evaluation functions of USI shogi engines

#ifndef NNUE_FEATURES_HALF_KP_SHOGI_H_INCLUDED
#define NNUE_FEATURES_HALF_KP_SHOGI_H_INCLUDED

#include "../nnue_common.h"

#include "../../misc.h"
#include "../../variant.h"

// Shogi requires a build with support for large boards
#ifdef LARGEBOARDS

namespace Stockfish {
class Position;
}

namespace Stockfish::Eval::NNUE::Features {

// Feature HalfKP: Combination of the position of the own king and the position
// of pieces other than kings, on the board and in hand. The feature indices
// are those of YaneuraOu ("BonaPiece"), which makes the feature set compatible
// with the networks of USI shogi engines. It only applies to standard shogi.
class HalfKPShogi {

    // Number of squares and number of piece features per king square
    static constexpr IndexType Squares = 81;
    static constexpr IndexType FeEnd   = 90 + 18 * Squares;

    // Maximum number of pieces other than kings
    static constexpr int MaxPieces = 38;

    // Index of a square from the point of view of the perspective
    static IndexType orient(Color perspective, Square s);

    // Index of a feature for a given king position and another piece on some square
    static IndexType make_index(Color perspective, Square s, Piece pc, IndexType ksq);

    // Index of a feature for a given king position and another piece in hand
    static IndexType make_index(Color perspective, int handIndex, Piece pc, IndexType ksq);

   public:
    // Feature name
    static constexpr const char* Name = "HalfKP(Friend)";

    // Hash value embedded in the evaluation file
    static constexpr std::uint32_t HashValue = 0x5D69D5B9u ^ 1;

    // The feature indices do not depend on the variant
    struct Layout {};

    static IndexType dimensions(const Layout&) { return Squares * FeEnd; }

    // The piece type the features are relative to
    static PieceType king(const Layout&) { return KING; }

    // Returns the layout if the variant is shogi and the number of dimensions matches
    static const Layout* find_layout(const Variant* v, std::size_t dimensions);

    // The square of the king
    static Square king_square(const Position& pos, const Layout& layout, Color c);

    // Returns whether the position has the kings required by the features
    static bool applicable(const Position& pos, const Layout& layout);

    using IndexList  = FeatureIndexList;
    using PieceState = NNUE::PieceState;

    // Get a list of indices for active features
    static void append_active_indices(const Position& pos,
                                      const Layout&   layout,
                                      Color           perspective,
                                      IndexList&      active);

    // Get a list of indices for recently changed features
    static void append_changed_indices(const Layout&     layout,
                                       Square            ksq,
                                       const DirtyPiece& dp,
                                       Color             perspective,
                                       IndexList&        removed,
                                       IndexList&        added,
                                       const Position&   pos);

    // Get the lists of indices that differ between a piece state
    // and the position, and update the piece state to the position
    static void append_changed_indices(const Position& pos,
                                       const Layout&   layout,
                                       Color           perspective,
                                       PieceState&     state,
                                       IndexList&      removed,
                                       IndexList&      added);

    // Returns whether the change stored in this DirtyPiece means
    // that a full accumulator refresh is required.
    static bool requires_refresh(const Layout&     layout,
                                 const DirtyPiece& dp,
                                 Color             perspective,
                                 const Position&   pos);
};

}  // namespace Stockfish::Eval::NNUE::Features

#endif  // LARGEBOARDS

#endif  // #ifndef NNUE_FEATURES_HALF_KP_SHOGI_H_INCLUDED
