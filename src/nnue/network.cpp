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

#include "network.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "../evaluate.h"
#include "../incbin/incbin.h"
#include "../misc.h"
#include "../position.h"
#include "../types.h"
#include "../variant.h"
#include "nnue_architecture.h"
#include "nnue_common.h"
#include "nnue_misc.h"

// Macro to embed the default efficiently updatable neural network (NNUE) file
// data in the engine binary (using incbin.h, by Dale Weiler).
// This macro invocation will declare the following three variables
//     const unsigned char        gEmbeddedNNUEData[];  // a pointer to the embedded data
//     const unsigned char *const gEmbeddedNNUEEnd;     // a marker to the end
//     const unsigned int         gEmbeddedNNUESize;    // the size of the embedded file
// Note that this does not work in Microsoft Visual Studio.
#if !defined(_MSC_VER) && !defined(NNUE_EMBEDDING_OFF)
INCBIN(EmbeddedNNUE, EvalFileDefaultName);
#else
const unsigned char        gEmbeddedNNUEData[1] = {0x0};
const unsigned char* const gEmbeddedNNUEEnd     = &gEmbeddedNNUEData[1];
const unsigned int         gEmbeddedNNUESize    = 1;
#endif

namespace Stockfish::Eval::NNUE {

namespace Detail {

// Read evaluation function parameters
template<typename T>
bool read_parameters(std::istream& stream, T& reference) {

    std::uint32_t header;
    header = read_little_endian<std::uint32_t>(stream);
    if (!stream || header != T::get_hash_value())
        return false;
    return reference.read_parameters(stream);
}

// Write evaluation function parameters
template<typename T>
bool write_parameters(std::ostream& stream, const T& reference) {

    write_little_endian<std::uint32_t>(stream, T::get_hash_value());
    return reference.write_parameters(stream);
}

}  // namespace Detail

void Network::load(const std::string& rootDirectory, std::string evalfilePath) {
#if defined(DEFAULT_NNUE_DIRECTORY)
    std::vector<std::string> dirs = {"<internal>", "", rootDirectory,
                                     stringify(DEFAULT_NNUE_DIRECTORY)};
#else
    std::vector<std::string> dirs = {"<internal>", "", rootDirectory};
#endif

    if (evalfilePath.empty())
        evalfilePath = evalFile.defaultName;

    for (const auto& directory : dirs)
    {
        if (evalFile.current != evalfilePath)
        {
            if (directory != "<internal>")
                load_user_net(directory, evalfilePath);

            if (directory == "<internal>" && evalfilePath == evalFile.defaultName)
                load_internal();
        }
    }
}

bool Network::save(const std::optional<std::string>& filename) const {
    std::string actualFilename;
    std::string msg;

    if (filename.has_value())
        actualFilename = filename.value();
    else
    {
        if (evalFile.current != evalFile.defaultName)
        {
            msg = "Failed to export a net. "
                  "A non-embedded net can only be saved if the filename is specified";

            sync_cout << msg << sync_endl;
            return false;
        }

        actualFilename = evalFile.defaultName;
    }

    std::ofstream stream(actualFilename, std::ios_base::binary);
    bool          saved = save(stream, evalFile.current, evalFile.netDescription);

    msg = saved ? "Network saved successfully to " + actualFilename : "Failed to export a net";

    sync_cout << msg << sync_endl;
    return saved;
}

// The layer stack is selected by the piece count relative to the variant
std::size_t Network::bucket(const Position& pos) {
    return std::min((pos.count<ALL_PIECES>() - 1) * 8 / currentNnueVariant->nnueMaxPieces, 7);
}

NetworkOutput Network::evaluate(const Position& pos, AccumulatorStack& accumulatorStack) const {
    // We manually align the arrays on the stack because with gcc < 9.3
    // overaligning stack variables with alignas() doesn't work correctly.

    constexpr uint64_t alignment = CacheLineSize;

#if defined(ALIGNAS_ON_STACK_VARIABLES_BROKEN)
    TransformedFeatureType
         transformedFeaturesUnaligned[FeatureTransformer::BufferSize
                                   + alignment / sizeof(TransformedFeatureType)];
    char bufferUnaligned[NetworkArchitecture::BufferSize + alignment];

    auto* transformedFeatures = align_ptr_up<alignment>(&transformedFeaturesUnaligned[0]);
    auto* buffer              = align_ptr_up<alignment>(&bufferUnaligned[0]);
#else
    alignas(alignment) TransformedFeatureType transformedFeatures[FeatureTransformer::BufferSize];
    alignas(alignment) char                   buffer[NetworkArchitecture::BufferSize];
#endif

    ASSERT_ALIGNED(transformedFeatures, alignment);
    ASSERT_ALIGNED(buffer, alignment);

    const std::size_t b = bucket(pos);
    const auto psqt = featureTransformer.transform(pos, accumulatorStack, transformedFeatures, b);
    const auto positional = network[b].propagate(transformedFeatures, buffer)[0];

    return {psqt, positional};
}

void Network::verify(std::string                                  evalfilePath,
                     const std::function<void(std::string_view)>& f) const {
    if (evalfilePath.empty())
        evalfilePath = evalFile.defaultName;

    if (evalfilePath.find(evalFile.current) == std::string::npos)
    {
        if (f)
        {
            std::string msg1 =
              "Network evaluation parameters compatible with the engine must be available.";
            std::string msg2 = "The network file " + evalfilePath + " was not loaded successfully.";
            std::string msg3 = "The UCI option EvalFile might need to specify the full path, "
                               "including the directory name, to the network file.";
            std::string msg4 = "The default net can be downloaded from: "
                               "https://tests.stockfishchess.org/api/nn/"
                             + evalFile.defaultName;
            std::string msg5 = "The engine will be terminated now.";

            std::string msg = "ERROR: " + msg1 + '\n' + "ERROR: " + msg2 + '\n' + "ERROR: " + msg3
                            + '\n' + "ERROR: " + msg4 + '\n' + "ERROR: " + msg5 + '\n';

            f(msg);
        }

        exit(EXIT_FAILURE);
    }

    if (f)
        f("NNUE evaluation using " + evalFile.current + " enabled");
}

NnueEvalTrace Network::trace_evaluate(const Position&   pos,
                                      AccumulatorStack& accumulatorStack) const {
    // We manually align the arrays on the stack because with gcc < 9.3
    // overaligning stack variables with alignas() doesn't work correctly.

    constexpr uint64_t alignment = CacheLineSize;

#if defined(ALIGNAS_ON_STACK_VARIABLES_BROKEN)
    TransformedFeatureType
         transformedFeaturesUnaligned[FeatureTransformer::BufferSize
                                   + alignment / sizeof(TransformedFeatureType)];
    char bufferUnaligned[NetworkArchitecture::BufferSize + alignment];

    auto* transformedFeatures = align_ptr_up<alignment>(&transformedFeaturesUnaligned[0]);
    auto* buffer              = align_ptr_up<alignment>(&bufferUnaligned[0]);
#else
    alignas(alignment) TransformedFeatureType transformedFeatures[FeatureTransformer::BufferSize];
    alignas(alignment) char                   buffer[NetworkArchitecture::BufferSize];
#endif

    ASSERT_ALIGNED(transformedFeatures, alignment);
    ASSERT_ALIGNED(buffer, alignment);

    NnueEvalTrace t{};
    t.correctBucket = bucket(pos);
    for (std::size_t b = 0; b < LayerStacks; ++b)
    {
        const auto materialist =
          featureTransformer.transform(pos, accumulatorStack, transformedFeatures, b);
        const auto positional = network[b].propagate(transformedFeatures, buffer)[0];

        t.psqt[b]       = static_cast<Value>(materialist / OutputScale);
        t.positional[b] = static_cast<Value>(positional / OutputScale);
    }

    return t;
}

void Network::load_user_net(const std::string& dir, const std::string& evalfilePath) {
    std::ifstream stream(dir + evalfilePath, std::ios::binary);

    if (load(stream, evalfilePath))
        evalFile.current = evalfilePath;
}

void Network::load_internal() {
    // C++ way to prepare a buffer for a memory stream
    class MemoryBuffer: public std::basic_streambuf<char> {
       public:
        MemoryBuffer(char* p, size_t n) {
            setg(p, p, p + n);
            setp(p, p + n);
        }
    };

    MemoryBuffer buffer(const_cast<char*>(reinterpret_cast<const char*>(gEmbeddedNNUEData)),
                        size_t(gEmbeddedNNUESize));

    std::istream stream(&buffer);

    if (load(stream, evalFile.defaultName))
        evalFile.current = evalFile.defaultName;
}

bool Network::save(std::ostream&      stream,
                   const std::string& name,
                   const std::string& netDescription) const {
    if (name.empty() || name == "None")
        return false;

    return write_parameters(stream, netDescription);
}

bool Network::load(std::istream& stream, const std::string&) {
    std::string description;

    if (!read_parameters(stream, description))
        return false;

    evalFile.netDescription = description;
    return true;
}

// Read network header
bool Network::read_header(std::istream& stream, std::uint32_t* hashValue, std::string* desc) const {
    std::uint32_t version, size;

    version    = read_little_endian<std::uint32_t>(stream);
    *hashValue = read_little_endian<std::uint32_t>(stream);
    size       = read_little_endian<std::uint32_t>(stream);
    if (!stream || version != Version)
        return false;
    desc->resize(size);
    stream.read(&(*desc)[0], size);
    return !stream.fail();
}

// Write network header
bool Network::write_header(std::ostream&      stream,
                           std::uint32_t      hashValue,
                           const std::string& desc) const {
    write_little_endian<std::uint32_t>(stream, Version);
    write_little_endian<std::uint32_t>(stream, hashValue);
    write_little_endian<std::uint32_t>(stream, std::uint32_t(desc.size()));
    stream.write(&desc[0], desc.size());
    return !stream.fail();
}

// Read network parameters
bool Network::read_parameters(std::istream& stream, std::string& netDescription) {
    std::uint32_t hashValue;
    if (!read_header(stream, &hashValue, &netDescription))
        return false;
    if (hashValue != Network::hash)
        return false;
    if (!Detail::read_parameters(stream, featureTransformer))
        return false;
    for (std::size_t i = 0; i < LayerStacks; ++i)
        if (!Detail::read_parameters(stream, network[i]))
            return false;
    return stream && stream.peek() == std::ios::traits_type::eof();
}

// Write network parameters
bool Network::write_parameters(std::ostream& stream, const std::string& netDescription) const {
    if (!write_header(stream, Network::hash, netDescription))
        return false;
    if (!Detail::write_parameters(stream, featureTransformer))
        return false;
    for (std::size_t i = 0; i < LayerStacks; ++i)
        if (!Detail::write_parameters(stream, network[i]))
            return false;
    return bool(stream);
}

}  // namespace Stockfish::Eval::NNUE
