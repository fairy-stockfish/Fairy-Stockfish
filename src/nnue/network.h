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

#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "../types.h"
#include "../variant.h"
#include "nnue_accumulator.h"
#include "nnue_architecture.h"
#include "nnue_feature_transformer.h"
#include "nnue_misc.h"

namespace Stockfish {
class Position;
}

namespace Stockfish::Eval::NNUE {

// Material (PSQT) and positional output of the network,
// both not yet divided by the output scale
using NetworkOutput = std::tuple<std::int32_t, std::int32_t>;

// Interface of a network of one of the supported architectures
class NetworkBase {
   public:
    virtual ~NetworkBase() = default;

    virtual std::unique_ptr<NetworkBase> clone() const = 0;

    // Reads the parameters following the file header. The size is the total file size.
    virtual bool read_parameters(std::istream&  stream,
                                 std::size_t    size,
                                 std::size_t    headerSize,
                                 const Variant* v)            = 0;
    virtual bool write_parameters(std::ostream& stream) const = 0;

    virtual NetworkOutput evaluate(const Position&    pos,
                                   AccumulatorStack&  accumulatorStack,
                                   AccumulatorCaches& cache) const = 0;

    virtual NnueEvalTrace trace_evaluate(const Position&    pos,
                                         AccumulatorStack&  accumulatorStack,
                                         AccumulatorCaches& cache) const = 0;

    virtual void clear(AccumulatorCaches& cache) const = 0;

    // Returns whether the position has the pieces required by the features
    virtual bool applicable(const Position& pos) const = 0;

    // The piece type the features are relative to, if any
    virtual PieceType king() const = 0;

    // Returns whether the output is blended and scaled like the networks of Stockfish,
    // or just divided by an output scale like in USI shogi engines
    virtual bool stockfish_scaling() const = 0;
    virtual int  output_scale() const      = 0;
};

// A network of a given architecture
template<typename Arch>
class NetworkImpl final: public NetworkBase {
   public:
    using Transformer    = FeatureTransformer<Arch>;
    using LayerStackType = typename Arch::LayerStackType;

    std::unique_ptr<NetworkBase> clone() const override;

    bool read_parameters(std::istream&, std::size_t, std::size_t, const Variant*) override;
    bool write_parameters(std::ostream&) const override;

    NetworkOutput evaluate(const Position&, AccumulatorStack&, AccumulatorCaches&) const override;
    NnueEvalTrace
    trace_evaluate(const Position&, AccumulatorStack&, AccumulatorCaches&) const override;

    void      clear(AccumulatorCaches& cache) const override;
    bool      applicable(const Position& pos) const override;
    PieceType king() const override;

    bool stockfish_scaling() const override { return Arch::StockfishScaling; }
    int  output_scale() const override { return Arch::OutputScale; }

    // Hash value of evaluation function structure
    static constexpr std::uint32_t hash =
      Transformer::get_hash_value() ^ LayerStackType::get_hash_value();

   private:
    std::size_t bucket(const Position& pos) const;

    // Input feature converter
    Transformer featureTransformer;

    // Evaluation function
    LayerStackType network[Arch::LayerStacks];

    const Variant* var = nullptr;
};

// The network used for evaluation. The architecture
// is determined by the header of the network file.
class Network {
   public:
    Network(EvalFile file) :
        evalFile(file) {}

    Network(const Network& other);
    Network(Network&& other) = default;
    Network& operator=(const Network& other);
    Network& operator=(Network&& other) = default;

    // Loads a network for a variant. The network may use any
    // of the feature layouts of the variant.
    void load(const std::string& rootDirectory, std::string evalfilePath, const Variant* v);
    bool save(const std::optional<std::string>& filename) const;

    NetworkOutput evaluate(const Position&    pos,
                           AccumulatorStack&  accumulatorStack,
                           AccumulatorCaches& cache) const {
        return impl->evaluate(pos, accumulatorStack, cache);
    }

    void verify(std::string evalfilePath, const std::function<void(std::string_view)>&) const;
    NnueEvalTrace trace_evaluate(const Position&    pos,
                                 AccumulatorStack&  accumulatorStack,
                                 AccumulatorCaches& cache) const {
        NnueEvalTrace t = impl->trace_evaluate(pos, accumulatorStack, cache);
        for (std::size_t b = 0; b < t.layerStacks; ++b)
        {
            t.psqt[b] /= output_scale();
            t.positional[b] /= output_scale();
        }
        return t;
    }

    void clear(AccumulatorCaches& cache) const;

    const EvalFile& eval_file() const { return evalFile; }

    // The variant the network was loaded for
    const Variant* variant() const { return var; }

    // The divisor of the network output, which is fixed per architecture
    int  output_scale() const { return impl->output_scale(); }
    bool stockfish_scaling() const { return impl->stockfish_scaling(); }

    bool      applicable(const Position& pos) const { return impl && impl->applicable(pos); }
    PieceType king() const { return impl ? impl->king() : NO_PIECE_TYPE; }

   private:
    void load_user_net(const std::string&, const std::string&, const Variant*);
    void load_internal(const Variant*);

    bool save(std::ostream&, const std::string&, const std::string&) const;
    bool load(std::istream&, std::size_t, const Variant*);

    bool read_header(std::istream&, std::uint32_t*, std::uint32_t*, std::string*) const;
    bool write_header(std::ostream&, std::uint32_t, std::uint32_t, const std::string&) const;

    std::unique_ptr<NetworkBase> impl;

    // Version and hash of the loaded network
    std::uint32_t version = 0;
    std::uint32_t hash    = 0;

    EvalFile evalFile;

    const Variant* var = nullptr;
};

}  // namespace Stockfish::Eval::NNUE

#endif
