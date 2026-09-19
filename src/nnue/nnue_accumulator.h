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

// Class for difference calculation of NNUE evaluation function

#ifndef NNUE_ACCUMULATOR_H_INCLUDED
#define NNUE_ACCUMULATOR_H_INCLUDED

#include <array>
#include <cstddef>
#include <cstdint>

#include "../types.h"
#include "nnue_architecture.h"
#include "nnue_common.h"

namespace Stockfish {
class Position;
}

namespace Stockfish::Eval::NNUE {

class FeatureTransformer;

// Class that holds the result of affine transformation of input features
struct alignas(CacheLineSize) Accumulator {
    std::int16_t accumulation[COLOR_NB][TransformedFeatureDimensions];
    std::int32_t psqtAccumulation[COLOR_NB][PSQTBuckets];
    bool         computed[COLOR_NB] = {};
};

// An accumulator together with the board changes of the move leading to it
struct AccumulatorState: public Accumulator {
    DirtyPiece dirtyPiece;

    void reset() noexcept { computed[WHITE] = computed[BLACK] = false; }
};

// Stack of the accumulators along the current search line. It is owned by
// the search worker, positions only report what a move changed on the board.
class AccumulatorStack {
   public:
    static constexpr std::size_t MaxSize = MAX_PLY + 1;

    [[nodiscard]] const AccumulatorState& latest() const noexcept;

    void        reset() noexcept;
    DirtyPiece& push() noexcept;
    void        pop() noexcept;

    void evaluate(const Position& pos, const FeatureTransformer& featureTransformer) noexcept;

   private:
    [[nodiscard]] AccumulatorState& mut_latest() noexcept;

    void evaluate_side(Color                     perspective,
                       const Position&           pos,
                       const FeatureTransformer& featureTransformer) noexcept;

    [[nodiscard]] std::size_t find_last_usable_accumulator(Color           perspective,
                                                           const Position& pos) const noexcept;

    void forward_update_incremental(Color                     perspective,
                                    const Position&           pos,
                                    const FeatureTransformer& featureTransformer,
                                    const std::size_t         begin) noexcept;

    void backward_update_incremental(Color                     perspective,
                                     const Position&           pos,
                                     const FeatureTransformer& featureTransformer,
                                     const std::size_t         end) noexcept;

    std::array<AccumulatorState, MaxSize> accumulators;
    std::size_t                           size = 1;
};

}  // namespace Stockfish::Eval::NNUE

#endif  // NNUE_ACCUMULATOR_H_INCLUDED
