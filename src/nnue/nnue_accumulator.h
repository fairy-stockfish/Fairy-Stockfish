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
#include <cstring>

#include "../types.h"
#include "nnue_architecture.h"
#include "nnue_common.h"

namespace Stockfish {
class Position;
}

namespace Stockfish::Eval::NNUE {

template<typename Arch>
class FeatureTransformer;

class Network;

// Class that holds the result of affine transformation of input features.
// It is sized for the largest supported architecture.
struct alignas(CacheLineSize) Accumulator {
    std::int16_t accumulation[COLOR_NB][MaxTransformedFeatureDimensions];
    std::int32_t psqtAccumulation[COLOR_NB][MaxPSQTBuckets];
    bool         computed[COLOR_NB] = {};
};

// AccumulatorCaches struct provides per-thread accumulator caches, where each
// cache contains multiple entries for each of the possible king squares.
// When the accumulator needs to be refreshed, the cached entry is used to more
// efficiently update the accumulator, instead of rebuilding it from scratch.
// This idea, was first described by Luecx (author of Koivisto) and
// is commonly referred to as "Finny Tables".
struct AccumulatorCaches {

    AccumulatorCaches(const Network& network) { clear(network); }

    struct alignas(CacheLineSize) Entry {
        BiasType       accumulation[MaxTransformedFeatureDimensions];
        PSQTWeightType psqtAccumulation[MaxPSQTBuckets];
        PieceState     pieceState;

        // To initialize a refresh entry, we set all its pieces empty,
        // so we put the biases in the accumulation, without any weights on top
        void clear(const BiasType* biases, IndexType dimensions) {
            std::memcpy(accumulation, biases, dimensions * sizeof(BiasType));
            std::memset(psqtAccumulation, 0, sizeof(psqtAccumulation));
            std::memset(&pieceState, 0, sizeof(pieceState));
        }
    };

    void clear(const Network& network);

    void clear(const BiasType* biases, IndexType dimensions) {
        for (auto& entries1D : entries)
            for (auto& entry : entries1D)
                entry.clear(biases, dimensions);
    }

    // Variants without a king use the entry of the first square
    std::array<Entry, COLOR_NB>& operator[](Square sq) { return entries[sq == SQ_NONE ? 0 : sq]; }

    std::array<std::array<Entry, COLOR_NB>, SQUARE_NB> entries;
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

    template<typename Arch>
    void evaluate(const Position&                 pos,
                  const FeatureTransformer<Arch>& featureTransformer,
                  AccumulatorCaches&              cache) noexcept;

   private:
    [[nodiscard]] AccumulatorState& mut_latest() noexcept;

    template<typename Arch>
    void evaluate_side(Color                           perspective,
                       const Position&                 pos,
                       const FeatureTransformer<Arch>& featureTransformer,
                       AccumulatorCaches&              cache) noexcept;

    template<typename Arch>
    [[nodiscard]] std::size_t
    find_last_usable_accumulator(Color                           perspective,
                                 const Position&                 pos,
                                 const FeatureTransformer<Arch>& featureTransformer) const noexcept;

    template<typename Arch>
    void forward_update_incremental(Color                           perspective,
                                    const Position&                 pos,
                                    const FeatureTransformer<Arch>& featureTransformer,
                                    const std::size_t               begin) noexcept;

    template<typename Arch>
    void backward_update_incremental(Color                           perspective,
                                     const Position&                 pos,
                                     const FeatureTransformer<Arch>& featureTransformer,
                                     const std::size_t               end) noexcept;

    std::array<AccumulatorState, MaxSize> accumulators;
    std::size_t                           size = 1;
};

}  // namespace Stockfish::Eval::NNUE

#endif  // NNUE_ACCUMULATOR_H_INCLUDED
