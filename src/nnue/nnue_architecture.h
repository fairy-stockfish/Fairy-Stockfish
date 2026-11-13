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

// Input features and network structure used in NNUE evaluation function

#ifndef NNUE_ARCHITECTURE_H_INCLUDED
#define NNUE_ARCHITECTURE_H_INCLUDED

#include "nnue_common.h"

#include "features/half_ka_v2_variants.h"
#include "features/half_kp_shogi.h"

#include "layers/affine_transform.h"
#include "layers/affine_transform_sparse_input.h"
#include "layers/clipped_relu.h"

#include <cstring>

namespace Stockfish::Eval::NNUE {

// A stack of fully connected layers on top of the transformed features
template<IndexType InputDimensions, int L2, int L3>
struct LayerStack {
    static constexpr int FC_0_OUTPUTS = L2;
    static constexpr int FC_1_OUTPUTS = L3;

    Layers::AffineTransformSparseInput<InputDimensions, FC_0_OUTPUTS> fc_0;
    Layers::ClippedReLU<FC_0_OUTPUTS>                                 ac_0;
    Layers::AffineTransform<FC_0_OUTPUTS, FC_1_OUTPUTS>               fc_1;
    Layers::ClippedReLU<FC_1_OUTPUTS>                                 ac_1;
    Layers::AffineTransform<FC_1_OUTPUTS, 1>                          fc_2;

    // Hash value embedded in the evaluation file
    static constexpr std::uint32_t get_hash_value() {
        // input slice hash
        std::uint32_t hashValue = 0xEC42E90Du;
        hashValue ^= InputDimensions;

        hashValue = decltype(fc_0)::get_hash_value(hashValue);
        hashValue = decltype(ac_0)::get_hash_value(hashValue);
        hashValue = decltype(fc_1)::get_hash_value(hashValue);
        hashValue = decltype(ac_1)::get_hash_value(hashValue);
        hashValue = decltype(fc_2)::get_hash_value(hashValue);

        return hashValue;
    }

    // Read network parameters
    bool read_parameters(std::istream& stream) {
        return fc_0.read_parameters(stream) && ac_0.read_parameters(stream)
            && fc_1.read_parameters(stream) && ac_1.read_parameters(stream)
            && fc_2.read_parameters(stream);
    }

    // Write network parameters
    bool write_parameters(std::ostream& stream) const {
        return fc_0.write_parameters(stream) && ac_0.write_parameters(stream)
            && fc_1.write_parameters(stream) && ac_1.write_parameters(stream)
            && fc_2.write_parameters(stream);
    }

    std::int32_t propagate(const TransformedFeatureType* transformedFeatures) const {
        struct alignas(CacheLineSize) Buffer {
            alignas(CacheLineSize) typename decltype(fc_0)::OutputBuffer fc_0_out;
            alignas(CacheLineSize) typename decltype(ac_0)::OutputBuffer ac_0_out;
            alignas(CacheLineSize) typename decltype(fc_1)::OutputBuffer fc_1_out;
            alignas(CacheLineSize) typename decltype(ac_1)::OutputBuffer ac_1_out;
            alignas(CacheLineSize) typename decltype(fc_2)::OutputBuffer fc_2_out;

            Buffer() { std::memset(this, 0, sizeof(*this)); }
        };

#if defined(__clang__) && (__APPLE__)
        // workaround for a bug reported with xcode 12
        static thread_local auto tlsBuffer = std::make_unique<Buffer>();
        // Access TLS only once, cache result.
        Buffer& buffer = *tlsBuffer;
#else
        alignas(CacheLineSize) static thread_local Buffer buffer;
#endif

        fc_0.propagate(transformedFeatures, buffer.fc_0_out);
        ac_0.propagate(buffer.fc_0_out, buffer.ac_0_out);
        fc_1.propagate(buffer.ac_0_out, buffer.fc_1_out);
        ac_1.propagate(buffer.fc_1_out, buffer.ac_1_out);
        fc_2.propagate(buffer.ac_1_out, buffer.fc_2_out);

        return buffer.fc_2_out[0];
    }
};

// The architecture of the variant networks: HalfKAv2 features with a layout
// depending on the variant, 512x2-16-32-1 with 8 layer stacks and PSQT buckets
struct VariantArchitecture {
    using FeatureSet = Features::HalfKAv2Variants;

    static constexpr IndexType     TransformedFeatureDimensions = 512;
    static constexpr IndexType     PSQTBuckets                  = 8;
    static constexpr IndexType     LayerStacks                  = 8;
    static constexpr std::uint32_t Version                      = 0x7AF32F20u;

    using LayerStackType = LayerStack<TransformedFeatureDimensions * 2, 16, 32>;

    // The output is scaled like the networks of Stockfish
    static constexpr bool StockfishScaling = true;
    static constexpr int  OutputScale      = NNUE::OutputScale;
};

#ifdef LARGEBOARDS

// The architectures of the networks of USI shogi engines in the format of YaneuraOu:
// HalfKP features, L1x2-L2-L3-1 without layer stacks and PSQT. The divisor of the
// output (FV_SCALE in YaneuraOu) is fixed per architecture.
template<IndexType L1, int L2, int L3, int Scale>
struct ShogiArchitecture {
    using FeatureSet = Features::HalfKPShogi;

    static constexpr IndexType     TransformedFeatureDimensions = L1;
    static constexpr IndexType     PSQTBuckets                  = 0;
    static constexpr IndexType     LayerStacks                  = 1;
    static constexpr std::uint32_t Version                      = 0x7AF32F16u;

    using LayerStackType = LayerStack<TransformedFeatureDimensions * 2, L2, L3>;

    static constexpr bool StockfishScaling = false;
    static constexpr int  OutputScale      = Scale;
};

// The standard architecture, e.g. Suisho5 (FV_SCALE 24), Hao and Li (20)
using ShogiArchitecture256 = ShogiArchitecture<256, 32, 32, 24>;
// E.g. AobaNNUE (FV_SCALE 40)
using ShogiArchitecture768 = ShogiArchitecture<768, 16, 64, 40>;

static_assert(ShogiArchitecture768::TransformedFeatureDimensions
              <= MaxTransformedFeatureDimensions);

static_assert(VariantArchitecture::TransformedFeatureDimensions <= MaxTransformedFeatureDimensions);

#endif  // LARGEBOARDS

}  // namespace Stockfish::Eval::NNUE

#endif  // #ifndef NNUE_ARCHITECTURE_H_INCLUDED
