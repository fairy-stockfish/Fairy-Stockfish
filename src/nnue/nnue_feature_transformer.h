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

// A class that converts the input features of the NNUE evaluation function

#ifndef NNUE_FEATURE_TRANSFORMER_H_INCLUDED
#define NNUE_FEATURE_TRANSFORMER_H_INCLUDED

#include "nnue_accumulator.h"
#include "nnue_architecture.h"
#include "nnue_common.h"
#include "simd.h"
#include "../memory.h"
#include "../position.h"

#include <cstring>  // std::memset()

namespace Stockfish::Eval::NNUE {

using BiasType       = std::int16_t;
using WeightType     = std::int16_t;
using PSQTWeightType = std::int32_t;

// Input feature converter
template<typename Arch>
class FeatureTransformer {

   public:
    using FeatureSet = typename Arch::FeatureSet;

    // Number of output dimensions for one side
    static constexpr IndexType HalfDimensions = Arch::TransformedFeatureDimensions;
    static constexpr IndexType PSQTBuckets    = Arch::PSQTBuckets;

    static_assert(PSQTBuckets % 8 == 0,
                  "Per feature PSQT values cannot be processed at granularity lower than 8 at a "
                  "time.");

#ifdef VECTOR
    // If vector instructions are enabled, we update and refresh the
    // accumulator tile by tile such that each tile fits in the CPU's
    // vector registers.
    // Architectures without PSQT never enter the PSQT loops
    using Tiling =
      SIMD::SIMDTiling<HalfDimensions, HalfDimensions, (PSQTBuckets ? PSQTBuckets : 8)>;

    static constexpr int       NumRegs        = Tiling::NumRegs;
    static constexpr int       NumPsqtRegs    = Tiling::NumPsqtRegs;
    static constexpr IndexType TileHeight     = Tiling::TileHeight;
    static constexpr IndexType PsqtTileHeight = Tiling::PsqtTileHeight;
#endif

    // Output type
    using OutputType = TransformedFeatureType;

    // Number of output dimensions. The number of input dimensions
    // depends on the variant the network is loaded for.
    static constexpr IndexType OutputDimensions = HalfDimensions * 2;

    // Size of forward propagation buffer
    static constexpr std::size_t BufferSize = OutputDimensions * sizeof(OutputType);

    // Hash value embedded in the evaluation file
    static constexpr std::uint32_t get_hash_value() {
        return FeatureSet::HashValue ^ OutputDimensions;
    }

    FeatureTransformer() = default;

    FeatureTransformer(const FeatureTransformer& other) { *this = other; }

    FeatureTransformer& operator=(const FeatureTransformer& other) {
        if (this == &other)
            return *this;

        featureLayout = other.featureLayout;
        std::memcpy(biases, other.biases, sizeof(biases));
        allocate(other.inputDimensions);
        if (inputDimensions)
        {
            std::memcpy(weights, other.weights,
                        sizeof(WeightType) * HalfDimensions * inputDimensions);
            std::memcpy(psqtWeights, other.psqtWeights,
                        sizeof(PSQTWeightType) * PSQTBuckets * inputDimensions);
        }
        return *this;
    }

    FeatureTransformer(FeatureTransformer&&)            = default;
    FeatureTransformer& operator=(FeatureTransformer&&) = default;

    // Read network parameters
    bool read_parameters(std::istream& stream) {

        assert(featureLayout);
        allocate(FeatureSet::dimensions(*featureLayout));

        read_little_endian<BiasType>(stream, biases, HalfDimensions);
        read_little_endian<WeightType>(stream, weights, HalfDimensions * inputDimensions);
        read_little_endian<PSQTWeightType>(stream, psqtWeights, PSQTBuckets * inputDimensions);

        return !stream.fail();
    }

    // Write network parameters
    bool write_parameters(std::ostream& stream) const {

        write_little_endian<BiasType>(stream, biases, HalfDimensions);
        write_little_endian<WeightType>(stream, weights, HalfDimensions * inputDimensions);
        write_little_endian<PSQTWeightType>(stream, psqtWeights, PSQTBuckets * inputDimensions);

        return !stream.fail();
    }

    // Convert input features
    std::int32_t transform(const Position&    pos,
                           AccumulatorStack&  accumulatorStack,
                           AccumulatorCaches& cache,
                           OutputType*        output,
                           int                bucket) const {

        accumulatorStack.evaluate(pos, *this, cache);

        const Color perspectives[2]  = {pos.side_to_move(), ~pos.side_to_move()};
        const auto& accumulation     = accumulatorStack.latest().accumulation;
        const auto& psqtAccumulation = accumulatorStack.latest().psqtAccumulation;

        const std::int32_t psqt = PSQTBuckets ? (psqtAccumulation[perspectives[0]][bucket]
                                                 - psqtAccumulation[perspectives[1]][bucket])
                                                  / 2
                                              : 0;


#if defined(USE_AVX512)

        constexpr IndexType NumChunks = HalfDimensions / (SimdWidth * 2);
        static_assert(HalfDimensions % (SimdWidth * 2) == 0);
        const __m512i Control = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);
        const __m512i Zero    = _mm512_setzero_si512();

        for (IndexType p = 0; p < 2; ++p)
        {
            const IndexType offset = HalfDimensions * p;
            auto            out    = reinterpret_cast<__m512i*>(&output[offset]);
            for (IndexType j = 0; j < NumChunks; ++j)
            {
                __m512i sum0 = _mm512_load_si512(
                  &reinterpret_cast<const __m512i*>(accumulation[perspectives[p]])[j * 2 + 0]);
                __m512i sum1 = _mm512_load_si512(
                  &reinterpret_cast<const __m512i*>(accumulation[perspectives[p]])[j * 2 + 1]);

                _mm512_store_si512(
                  &out[j], _mm512_permutexvar_epi64(
                             Control, _mm512_max_epi8(_mm512_packs_epi16(sum0, sum1), Zero)));
            }
        }
        return psqt;

#elif defined(USE_AVX2)

        constexpr IndexType NumChunks = HalfDimensions / SimdWidth;
        constexpr int       Control   = 0b11011000;
        const __m256i       Zero      = _mm256_setzero_si256();

        for (IndexType p = 0; p < 2; ++p)
        {
            const IndexType offset = HalfDimensions * p;
            auto            out    = reinterpret_cast<__m256i*>(&output[offset]);
            for (IndexType j = 0; j < NumChunks; ++j)
            {
                __m256i sum0 = _mm256_load_si256(
                  &reinterpret_cast<const __m256i*>(accumulation[perspectives[p]])[j * 2 + 0]);
                __m256i sum1 = _mm256_load_si256(
                  &reinterpret_cast<const __m256i*>(accumulation[perspectives[p]])[j * 2 + 1]);

                _mm256_store_si256(
                  &out[j], _mm256_permute4x64_epi64(
                             _mm256_max_epi8(_mm256_packs_epi16(sum0, sum1), Zero), Control));
            }
        }
        return psqt;

#elif defined(USE_SSE2)

    #ifdef USE_SSE41
        constexpr IndexType NumChunks = HalfDimensions / SimdWidth;
        const __m128i       Zero      = _mm_setzero_si128();
    #else
        constexpr IndexType NumChunks = HalfDimensions / SimdWidth;
        const __m128i       k0x80s    = _mm_set1_epi8(-128);
    #endif

        for (IndexType p = 0; p < 2; ++p)
        {
            const IndexType offset = HalfDimensions * p;
            auto            out    = reinterpret_cast<__m128i*>(&output[offset]);
            for (IndexType j = 0; j < NumChunks; ++j)
            {
                __m128i sum0 = _mm_load_si128(
                  &reinterpret_cast<const __m128i*>(accumulation[perspectives[p]])[j * 2 + 0]);
                __m128i sum1 = _mm_load_si128(
                  &reinterpret_cast<const __m128i*>(accumulation[perspectives[p]])[j * 2 + 1]);
                const __m128i packedbytes = _mm_packs_epi16(sum0, sum1);

    #ifdef USE_SSE41
                _mm_store_si128(&out[j], _mm_max_epi8(packedbytes, Zero));
    #else
                _mm_store_si128(&out[j], _mm_subs_epi8(_mm_adds_epi8(packedbytes, k0x80s), k0x80s));
    #endif
            }
        }
        return psqt;

#elif defined(USE_MMX)

        constexpr IndexType NumChunks = HalfDimensions / SimdWidth;
        const __m64         k0x80s    = _mm_set1_pi8(-128);

        for (IndexType p = 0; p < 2; ++p)
        {
            const IndexType offset = HalfDimensions * p;
            auto            out    = reinterpret_cast<__m64*>(&output[offset]);
            for (IndexType j = 0; j < NumChunks; ++j)
            {
                __m64 sum0 =
                  *(&reinterpret_cast<const __m64*>(accumulation[perspectives[p]])[j * 2 + 0]);
                __m64 sum1 =
                  *(&reinterpret_cast<const __m64*>(accumulation[perspectives[p]])[j * 2 + 1]);
                const __m64 packedbytes = _mm_packs_pi16(sum0, sum1);
                out[j]                  = _mm_subs_pi8(_mm_adds_pi8(packedbytes, k0x80s), k0x80s);
            }
        }
        _mm_empty();
        return psqt;

#elif defined(USE_NEON)

        constexpr IndexType NumChunks = HalfDimensions / (SimdWidth / 2);
        const int8x8_t      Zero      = {0};

        for (IndexType p = 0; p < 2; ++p)
        {
            const IndexType offset = HalfDimensions * p;
            const auto      out    = reinterpret_cast<int8x8_t*>(&output[offset]);
            for (IndexType j = 0; j < NumChunks; ++j)
            {
                int16x8_t sum =
                  reinterpret_cast<const int16x8_t*>(accumulation[perspectives[p]])[j];
                out[j] = vmax_s8(vqmovn_s16(sum), Zero);
            }
        }
        return psqt;

#else

        for (IndexType p = 0; p < 2; ++p)
        {
            const IndexType offset = HalfDimensions * p;
            for (IndexType j = 0; j < HalfDimensions; ++j)
            {
                BiasType sum = accumulation[perspectives[p]][j];
                output[offset + j] =
                  static_cast<OutputType>(std::max<int>(0, std::min<int>(127, sum)));
            }
        }
        return psqt;

#endif

    }  // end of function transform()


    // The feature layout of the variant the parameters are for
    using Layout = typename FeatureSet::Layout;

    void          set_layout(const Layout* l) { featureLayout = l; }
    const Layout& layout() const { return *featureLayout; }

    alignas(CacheLineSize) BiasType biases[HalfDimensions] = {};

    // The weights are sized by the input dimensions of the variant
    WeightType*     weights         = nullptr;
    PSQTWeightType* psqtWeights     = nullptr;
    IndexType       inputDimensions = 0;

   private:
    // Cache line sized blocks keep the weights aligned for SIMD access
    struct alignas(CacheLineSize) Block {
        char data[CacheLineSize];
    };

    static std::size_t blocks(std::size_t bytes) {
        return (bytes + CacheLineSize - 1) / CacheLineSize;
    }

    void allocate(IndexType dimensions) {
        inputDimensions = dimensions;
        weights         = nullptr;
        psqtWeights     = nullptr;
        weightMemory.reset();
        psqtWeightMemory.reset();
        if (dimensions)
        {
            weightMemory = make_unique_large_page<Block[]>(
              blocks(sizeof(WeightType) * HalfDimensions * dimensions));
            psqtWeightMemory = make_unique_large_page<Block[]>(
              blocks(sizeof(PSQTWeightType) * PSQTBuckets * dimensions));
            weights     = reinterpret_cast<WeightType*>(weightMemory.get());
            psqtWeights = reinterpret_cast<PSQTWeightType*>(psqtWeightMemory.get());
        }
    }

    const Layout* featureLayout = nullptr;

    LargePagePtr<Block[]> weightMemory;
    LargePagePtr<Block[]> psqtWeightMemory;
};

}  // namespace Stockfish::Eval::NNUE

#endif  // #ifndef NNUE_FEATURE_TRANSFORMER_H_INCLUDED
