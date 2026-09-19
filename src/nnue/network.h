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
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "../types.h"
#include "nnue_accumulator.h"
#include "nnue_architecture.h"
#include "nnue_feature_transformer.h"
#include "nnue_misc.h"

namespace Stockfish {
class Position;
}

namespace Stockfish::Eval::NNUE {

// Material (PSQT) and positional output of the network,
// both not yet divided by OutputScale
using NetworkOutput = std::tuple<std::int32_t, std::int32_t>;

// A network of the variant architecture. The input dimensions of the
// feature transformer depend on the variant the network was loaded for.
class Network {
   public:
    Network(EvalFile file) :
        evalFile(file) {}

    Network(const Network& other)            = default;
    Network(Network&& other)                 = default;
    Network& operator=(const Network& other) = default;
    Network& operator=(Network&& other)      = default;

    void load(const std::string& rootDirectory, std::string evalfilePath);
    bool save(const std::optional<std::string>& filename) const;

    NetworkOutput evaluate(const Position& pos, AccumulatorStack& accumulatorStack) const;

    void verify(std::string evalfilePath, const std::function<void(std::string_view)>&) const;
    NnueEvalTrace trace_evaluate(const Position& pos, AccumulatorStack& accumulatorStack) const;

    const EvalFile& eval_file() const { return evalFile; }

   private:
    void load_user_net(const std::string&, const std::string&);
    void load_internal();

    bool save(std::ostream&, const std::string&, const std::string&) const;
    bool load(std::istream&, const std::string&);

    bool read_header(std::istream&, std::uint32_t*, std::string*) const;
    bool write_header(std::ostream&, std::uint32_t, const std::string&) const;

    bool read_parameters(std::istream&, std::string&);
    bool write_parameters(std::ostream&, const std::string&) const;

    static std::size_t bucket(const Position& pos);

    // Input feature converter
    FeatureTransformer featureTransformer;

    // Evaluation function
    NetworkArchitecture network[LayerStacks];

    EvalFile evalFile;

    // Hash value of evaluation function structure
    static constexpr std::uint32_t hash =
      FeatureTransformer::get_hash_value() ^ NetworkArchitecture::get_hash_value();
};

}  // namespace Stockfish::Eval::NNUE

#endif
