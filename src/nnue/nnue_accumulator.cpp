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

#include "nnue_accumulator.h"

#include <cassert>
#include <cstring>

#include "../position.h"
#include "nnue_feature_transformer.h"

namespace Stockfish::Eval::NNUE {

#ifdef VECTOR
using SIMD::psqt_vec_t;
using SIMD::vec_t;

constexpr int NumRegs     = FeatureTransformer::NumRegs;
constexpr int NumPsqtRegs = FeatureTransformer::NumPsqtRegs;
#endif

namespace {

enum IncUpdateDirection {
    FORWARD,
    BACKWARDS
};

void update_accumulator_incremental(Color                     perspective,
                                    IncUpdateDirection        direction,
                                    const FeatureTransformer& featureTransformer,
                                    const Square              ksq,
                                    const Position&           pos,
                                    AccumulatorState&         target_state,
                                    const AccumulatorState&   computed);

void update_accumulator_refresh_cache(Color                     perspective,
                                      const FeatureTransformer& featureTransformer,
                                      const Position&           pos,
                                      AccumulatorState&         accumulatorState,
                                      AccumulatorCaches&        cache);

}

const AccumulatorState& AccumulatorStack::latest() const noexcept { return accumulators[size - 1]; }

AccumulatorState& AccumulatorStack::mut_latest() noexcept { return accumulators[size - 1]; }

void AccumulatorStack::reset() noexcept {
    accumulators[0].reset();
    accumulators[0].dirtyPiece.dirty_num = 0;
    accumulators[0].dirtyPiece.piece[0]  = NO_PIECE;
    size                                 = 1;
}

DirtyPiece& AccumulatorStack::push() noexcept {
    assert(size < MaxSize);
    accumulators[size].reset();
    return accumulators[size++].dirtyPiece;
}

void AccumulatorStack::pop() noexcept {
    assert(size > 1);
    size--;
}

void AccumulatorStack::evaluate(const Position&           pos,
                                const FeatureTransformer& featureTransformer,
                                AccumulatorCaches&        cache) noexcept {
    evaluate_side(WHITE, pos, featureTransformer, cache);
    evaluate_side(BLACK, pos, featureTransformer, cache);
}

void AccumulatorStack::evaluate_side(Color                     perspective,
                                     const Position&           pos,
                                     const FeatureTransformer& featureTransformer,
                                     AccumulatorCaches&        cache) noexcept {

    const auto last_usable_accum =
      find_last_usable_accumulator(perspective, pos, featureTransformer);

    if (accumulators[last_usable_accum].computed[perspective])
        forward_update_incremental(perspective, pos, featureTransformer, last_usable_accum);

    else
    {
        update_accumulator_refresh_cache(perspective, featureTransformer, pos, mut_latest(), cache);
        backward_update_incremental(perspective, pos, featureTransformer, last_usable_accum);
    }
}

// Find the earliest usable accumulator, this can either be a computed accumulator or the
// accumulator state just before a change that requires full refresh.
std::size_t AccumulatorStack::find_last_usable_accumulator(
  Color                     perspective,
  const Position&           pos,
  const FeatureTransformer& featureTransformer) const noexcept {

    for (std::size_t curr_idx = size - 1; curr_idx > 0; curr_idx--)
    {
        if (accumulators[curr_idx].computed[perspective])
            return curr_idx;

        if (FeatureSet::requires_refresh(featureTransformer.layout(),
                                         accumulators[curr_idx].dirtyPiece, perspective, pos))
            return curr_idx;
    }

    return 0;
}

void AccumulatorStack::forward_update_incremental(Color                     perspective,
                                                  const Position&           pos,
                                                  const FeatureTransformer& featureTransformer,
                                                  const std::size_t         begin) noexcept {

    assert(begin < accumulators.size());
    assert(accumulators[begin].computed[perspective]);

    const Square ksq = FeatureSet::king_square(pos, featureTransformer.layout(), perspective);

    for (std::size_t next = begin + 1; next < size; next++)
        update_accumulator_incremental(perspective, FORWARD, featureTransformer, ksq, pos,
                                       accumulators[next], accumulators[next - 1]);

    assert(latest().computed[perspective]);
}

void AccumulatorStack::backward_update_incremental(Color                     perspective,
                                                   const Position&           pos,
                                                   const FeatureTransformer& featureTransformer,
                                                   const std::size_t         end) noexcept {

    assert(end < accumulators.size());
    assert(end < size);
    assert(latest().computed[perspective]);

    const Square ksq = FeatureSet::king_square(pos, featureTransformer.layout(), perspective);

    for (std::int64_t next = std::int64_t(size) - 2; next >= std::int64_t(end); next--)
        update_accumulator_incremental(perspective, BACKWARDS, featureTransformer, ksq, pos,
                                       accumulators[next], accumulators[next + 1]);

    assert(accumulators[end].computed[perspective]);
}

namespace {

// Applies the removed and added features to an accumulation. The
// input and the output accumulation may be the same.
void apply_feature_updates(const FeatureTransformer&    featureTransformer,
                           const FeatureSet::IndexList& removed,
                           const FeatureSet::IndexList& added,
                           const BiasType*              accIn,
                           const PSQTWeightType*        psqtIn,
                           BiasType*                    accOut,
                           PSQTWeightType*              psqtOut) {

    constexpr IndexType HalfDimensions = FeatureTransformer::HalfDimensions;

#ifdef VECTOR
    constexpr IndexType TileHeight     = FeatureTransformer::TileHeight;
    constexpr IndexType PsqtTileHeight = FeatureTransformer::PsqtTileHeight;

    // Gcc-10.2 unnecessarily spills AVX2 registers if this array
    // is defined in the VECTOR code below, once in each branch
    vec_t      acc[NumRegs];
    psqt_vec_t psqt[NumPsqtRegs];

    for (IndexType j = 0; j < HalfDimensions / TileHeight; ++j)
    {
        // Load accumulator
        auto accTileIn = reinterpret_cast<const vec_t*>(&accIn[j * TileHeight]);
        for (IndexType k = 0; k < NumRegs; ++k)
            acc[k] = vec_load(&accTileIn[k]);

        // Difference calculation for the deactivated features
        for (const auto index : removed)
        {
            const IndexType offset = HalfDimensions * index + j * TileHeight;
            auto column = reinterpret_cast<const vec_t*>(&featureTransformer.weights[offset]);
            for (IndexType k = 0; k < NumRegs; ++k)
                acc[k] = vec_sub_16(acc[k], column[k]);
        }

        // Difference calculation for the activated features
        for (const auto index : added)
        {
            const IndexType offset = HalfDimensions * index + j * TileHeight;
            auto column = reinterpret_cast<const vec_t*>(&featureTransformer.weights[offset]);
            for (IndexType k = 0; k < NumRegs; ++k)
                acc[k] = vec_add_16(acc[k], column[k]);
        }

        // Store accumulator
        auto accTileOut = reinterpret_cast<vec_t*>(&accOut[j * TileHeight]);
        for (IndexType k = 0; k < NumRegs; ++k)
            vec_store(&accTileOut[k], acc[k]);
    }

    for (IndexType j = 0; j < PSQTBuckets / PsqtTileHeight; ++j)
    {
        // Load accumulator
        auto accTilePsqtIn = reinterpret_cast<const psqt_vec_t*>(&psqtIn[j * PsqtTileHeight]);
        for (std::size_t k = 0; k < NumPsqtRegs; ++k)
            psqt[k] = vec_load_psqt(&accTilePsqtIn[k]);

        // Difference calculation for the deactivated features
        for (const auto index : removed)
        {
            const IndexType offset = PSQTBuckets * index + j * PsqtTileHeight;
            auto            columnPsqt =
              reinterpret_cast<const psqt_vec_t*>(&featureTransformer.psqtWeights[offset]);
            for (std::size_t k = 0; k < NumPsqtRegs; ++k)
                psqt[k] = vec_sub_psqt_32(psqt[k], columnPsqt[k]);
        }

        // Difference calculation for the activated features
        for (const auto index : added)
        {
            const IndexType offset = PSQTBuckets * index + j * PsqtTileHeight;
            auto            columnPsqt =
              reinterpret_cast<const psqt_vec_t*>(&featureTransformer.psqtWeights[offset]);
            for (std::size_t k = 0; k < NumPsqtRegs; ++k)
                psqt[k] = vec_add_psqt_32(psqt[k], columnPsqt[k]);
        }

        // Store accumulator
        auto accTilePsqtOut = reinterpret_cast<psqt_vec_t*>(&psqtOut[j * PsqtTileHeight]);
        for (std::size_t k = 0; k < NumPsqtRegs; ++k)
            vec_store_psqt(&accTilePsqtOut[k], psqt[k]);
    }

#else
    if (accOut != accIn)
        std::memcpy(accOut, accIn, HalfDimensions * sizeof(BiasType));

    for (std::size_t k = 0; k < PSQTBuckets; ++k)
        psqtOut[k] = psqtIn[k];

    // Difference calculation for the deactivated features
    for (const auto index : removed)
    {
        const IndexType offset = HalfDimensions * index;

        for (IndexType j = 0; j < HalfDimensions; ++j)
            accOut[j] -= featureTransformer.weights[offset + j];

        for (std::size_t k = 0; k < PSQTBuckets; ++k)
            psqtOut[k] -= featureTransformer.psqtWeights[index * PSQTBuckets + k];
    }

    // Difference calculation for the activated features
    for (const auto index : added)
    {
        const IndexType offset = HalfDimensions * index;

        for (IndexType j = 0; j < HalfDimensions; ++j)
            accOut[j] += featureTransformer.weights[offset + j];

        for (std::size_t k = 0; k < PSQTBuckets; ++k)
            psqtOut[k] += featureTransformer.psqtWeights[index * PSQTBuckets + k];
    }
#endif

#if defined(USE_MMX)
    _mm_empty();
#endif
}


void update_accumulator_incremental(Color                     perspective,
                                    IncUpdateDirection        direction,
                                    const FeatureTransformer& featureTransformer,
                                    const Square              ksq,
                                    const Position&           pos,
                                    AccumulatorState&         target_state,
                                    const AccumulatorState&   computed) {

    assert(computed.computed[perspective]);
    assert(!target_state.computed[perspective]);

    // The board changes of the move between both states are stored in the later one
    FeatureSet::IndexList removed, added;
    if (direction == FORWARD)
        FeatureSet::append_changed_indices(featureTransformer.layout(), ksq,
                                           target_state.dirtyPiece, perspective, removed, added,
                                           pos);
    else
        FeatureSet::append_changed_indices(featureTransformer.layout(), ksq, computed.dirtyPiece,
                                           perspective, added, removed, pos);

    apply_feature_updates(featureTransformer, removed, added, computed.accumulation[perspective],
                          computed.psqtAccumulation[perspective],
                          target_state.accumulation[perspective],
                          target_state.psqtAccumulation[perspective]);

    target_state.computed[perspective] = true;
}

void update_accumulator_refresh_cache(Color                     perspective,
                                      const FeatureTransformer& featureTransformer,
                                      const Position&           pos,
                                      AccumulatorState&         accumulatorState,
                                      AccumulatorCaches&        cache) {

    const NnueLayout& layout = featureTransformer.layout();

    auto& entry = cache[FeatureSet::king_square(pos, layout, perspective)][perspective];

    // Bring the cached accumulation of this king square up to date
    FeatureSet::IndexList removed, added;
    FeatureSet::append_changed_indices(pos, layout, perspective, entry.pieceState, removed, added);

    if (removed.size() || added.size())
        apply_feature_updates(featureTransformer, removed, added, entry.accumulation,
                              entry.psqtAccumulation, entry.accumulation, entry.psqtAccumulation);

    std::memcpy(accumulatorState.accumulation[perspective], entry.accumulation,
                sizeof(entry.accumulation));
    std::memcpy(accumulatorState.psqtAccumulation[perspective], entry.psqtAccumulation,
                sizeof(entry.psqtAccumulation));

    accumulatorState.computed[perspective] = true;
}

}

}  // namespace Stockfish::Eval::NNUE
