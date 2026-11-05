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

//Definition of input features HalfKAv2 of NNUE evaluation function

#ifndef NNUE_FEATURES_HALF_KA_V2_VARIANTS_H_INCLUDED
#define NNUE_FEATURES_HALF_KA_V2_VARIANTS_H_INCLUDED

#include "../nnue_common.h"

#include "../../evaluate.h"
#include "../../misc.h"
#include "../../variant.h"

namespace Stockfish {
class Position;
}

namespace Stockfish::Eval::NNUE::Features {

// Feature HalfKAv2: Combination of the position of own king
// and the position of pieces
class HalfKAv2Variants {

    // Orient a square according to perspective (rotates by 180 for black)
    static Square orient(Color perspective, Square s, const Position& pos);

    // Index of a feature for a given king position and another piece on some square
    static IndexType make_index(const NnueLayout& layout,
                                Color             perspective,
                                Square            s,
                                Piece             pc,
                                Square            ksq,
                                const Position&   pos);

    // Index of a feature for a given king position and another piece in hand
    static IndexType make_index(const NnueLayout& layout,
                                Color             perspective,
                                int               handCount,
                                Piece             pc,
                                Square            ksq,
                                const Position&   pos);

   public:
    // Feature name
    static constexpr const char* Name = "HalfKAv2(Friend)";

    // Hash value embedded in the evaluation file
    static constexpr std::uint32_t HashValue = 0x5f234cb8u;

    // The square of the king of the layout, if any
    static Square king_square(const Position& pos, const NnueLayout& layout, Color c);

    // Returns whether the position has the kings required by the layout
    static bool applicable(const Position& pos, const NnueLayout& layout);

    // The feature indices depend on the layout of the variant
    using Layout = NnueLayout;

    static IndexType dimensions(const Layout& layout) { return IndexType(layout.dimensions); }

    // The piece type the features are relative to, if any
    static PieceType king(const Layout& layout) { return layout.king; }

    // Returns the layout of the variant with the given number of dimensions, if any
    static const Layout* find_layout(const Variant* v, std::size_t dimensions) {
        for (const Layout& layout : v->nnueLayouts)
            if (std::size_t(layout.dimensions) == dimensions)
                return &layout;
        return nullptr;
    }

    using IndexList  = FeatureIndexList;
    using PieceState = NNUE::PieceState;

    // Get a list of indices for active features
    static void append_active_indices(const Position&   pos,
                                      const NnueLayout& layout,
                                      Color             perspective,
                                      IndexList&        active);

    // Get a list of indices for recently changed features
    static void append_changed_indices(const NnueLayout& layout,
                                       Square            ksq,
                                       const DirtyPiece& dp,
                                       Color             perspective,
                                       IndexList&        removed,
                                       IndexList&        added,
                                       const Position&   pos);

    // Get the lists of indices that differ between a piece state
    // and the position, and update the piece state to the position
    static void append_changed_indices(const Position&   pos,
                                       const NnueLayout& layout,
                                       Color             perspective,
                                       PieceState&       state,
                                       IndexList&        removed,
                                       IndexList&        added);

    // Returns whether the change stored in this DirtyPiece means
    // that a full accumulator refresh is required.
    static bool requires_refresh(const NnueLayout& layout,
                                 const DirtyPiece& dp,
                                 Color             perspective,
                                 const Position&   pos);
};

}  // namespace Stockfish::Eval::NNUE::Features

#endif  // #ifndef NNUE_FEATURES_HALF_KA_V2_VARIANTS_H_INCLUDED
