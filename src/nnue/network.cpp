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
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

#include "../evaluate.h"
#define INCBIN_SILENCE_BITCODE_WARNING
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
const unsigned char                         gEmbeddedNNUEData[1] = {0x0};
[[maybe_unused]] const unsigned char* const gEmbeddedNNUEEnd     = &gEmbeddedNNUEData[1];
const unsigned int                          gEmbeddedNNUESize    = 1;
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

// Creates a network of the architecture matching the header of a network file
static std::unique_ptr<NetworkBase> create_network(std::uint32_t version, std::uint32_t hash) {

    if (version == VariantArchitecture::Version && hash == NetworkImpl<VariantArchitecture>::hash)
        return std::make_unique<NetworkImpl<VariantArchitecture>>();

#ifdef LARGEBOARDS
    if (version == ShogiArchitecture256::Version && hash == NetworkImpl<ShogiArchitecture256>::hash)
        return std::make_unique<NetworkImpl<ShogiArchitecture256>>();

    if (version == ShogiArchitecture768::Version && hash == NetworkImpl<ShogiArchitecture768>::hash)
        return std::make_unique<NetworkImpl<ShogiArchitecture768>>();
#endif

    return nullptr;
}

}  // namespace Detail


// NetworkImpl

template<typename Arch>
std::unique_ptr<NetworkBase> NetworkImpl<Arch>::clone() const {
    return std::make_unique<NetworkImpl<Arch>>(*this);
}

// The layer stack is selected by the piece count relative to the variant
template<typename Arch>
std::size_t NetworkImpl<Arch>::bucket(const Position& pos) const {
    if constexpr (Arch::LayerStacks == 1)
        return 0;
    else
        return std::min((pos.count<ALL_PIECES>() - 1) * 8 / var->nnueMaxPieces, 7);
}

template<typename Arch>
NetworkOutput NetworkImpl<Arch>::evaluate(const Position&    pos,
                                          AccumulatorStack&  accumulatorStack,
                                          AccumulatorCaches& cache) const {
    // We manually align the arrays on the stack because with gcc < 9.3
    // overaligning stack variables with alignas() doesn't work correctly.

    constexpr uint64_t alignment = CacheLineSize;

#if defined(ALIGNAS_ON_STACK_VARIABLES_BROKEN)
    TransformedFeatureType
      transformedFeaturesUnaligned[Transformer::BufferSize
                                   + alignment / sizeof(TransformedFeatureType)];

    auto* transformedFeatures = align_ptr_up<alignment>(&transformedFeaturesUnaligned[0]);
#else
    alignas(alignment) TransformedFeatureType transformedFeatures[Transformer::BufferSize];
#endif

    ASSERT_ALIGNED(transformedFeatures, alignment);

    const std::size_t b = bucket(pos);
    const auto        psqt =
      featureTransformer.transform(pos, accumulatorStack, cache, transformedFeatures, b);
    const auto positional = network[b].propagate(transformedFeatures);

    return {psqt, positional};
}

template<typename Arch>
NnueEvalTrace NetworkImpl<Arch>::trace_evaluate(const Position&    pos,
                                                AccumulatorStack&  accumulatorStack,
                                                AccumulatorCaches& cache) const {
    // We manually align the arrays on the stack because with gcc < 9.3
    // overaligning stack variables with alignas() doesn't work correctly.

    constexpr uint64_t alignment = CacheLineSize;

#if defined(ALIGNAS_ON_STACK_VARIABLES_BROKEN)
    TransformedFeatureType
      transformedFeaturesUnaligned[Transformer::BufferSize
                                   + alignment / sizeof(TransformedFeatureType)];

    auto* transformedFeatures = align_ptr_up<alignment>(&transformedFeaturesUnaligned[0]);
#else
    alignas(alignment) TransformedFeatureType transformedFeatures[Transformer::BufferSize];
#endif

    ASSERT_ALIGNED(transformedFeatures, alignment);

    NnueEvalTrace t{};
    t.layerStacks   = Arch::LayerStacks;
    t.correctBucket = bucket(pos);
    for (std::size_t b = 0; b < Arch::LayerStacks; ++b)
    {
        const auto materialist =
          featureTransformer.transform(pos, accumulatorStack, cache, transformedFeatures, b);
        const auto positional = network[b].propagate(transformedFeatures);

        // Not yet divided by the output scale
        t.psqt[b]       = static_cast<Value>(materialist);
        t.positional[b] = static_cast<Value>(positional);
    }

    return t;
}

template<typename Arch>
void NetworkImpl<Arch>::clear(AccumulatorCaches& cache) const {
    cache.clear(featureTransformer.biases, Transformer::HalfDimensions);
}

template<typename Arch>
bool NetworkImpl<Arch>::applicable(const Position& pos) const {
    return Arch::FeatureSet::applicable(pos, featureTransformer.layout());
}

template<typename Arch>
PieceType NetworkImpl<Arch>::king() const {
    return Arch::FeatureSet::king(featureTransformer.layout());
}

// Read network parameters
template<typename Arch>
bool NetworkImpl<Arch>::read_parameters(std::istream&  stream,
                                        std::size_t    size,
                                        std::size_t    headerSize,
                                        const Variant* v) {

    // The number of input dimensions is not stored in the file, so derive it from
    // the file size to find out which feature layout of the variant the network uses
    static const std::size_t layerStackBytes = [] {
        auto layerStack = std::make_unique<LayerStackType>();
        std::memset(static_cast<void*>(layerStack.get()), 0, sizeof(LayerStackType));
        std::ostringstream os;
        layerStack->write_parameters(os);
        return os.str().size();
    }();

    constexpr std::size_t HalfDimensions = Transformer::HalfDimensions;
    const std::size_t     fixedBytes     = sizeof(std::uint32_t) + HalfDimensions * sizeof(BiasType)
                                 + Arch::LayerStacks * (sizeof(std::uint32_t) + layerStackBytes);
    constexpr std::size_t bytesPerDimension =
      HalfDimensions * sizeof(WeightType) + Arch::PSQTBuckets * sizeof(PSQTWeightType);

    if (size < headerSize + fixedBytes || (size - headerSize - fixedBytes) % bytesPerDimension)
        return false;

    const auto* layout =
      Arch::FeatureSet::find_layout(v, (size - headerSize - fixedBytes) / bytesPerDimension);

    if (!layout)
        return false;

    var = v;
    featureTransformer.set_layout(layout);

    if (!Detail::read_parameters(stream, featureTransformer))
        return false;
    for (std::size_t i = 0; i < Arch::LayerStacks; ++i)
        if (!Detail::read_parameters(stream, network[i]))
            return false;
    return stream && stream.peek() == std::ios::traits_type::eof();
}

// Write network parameters
template<typename Arch>
bool NetworkImpl<Arch>::write_parameters(std::ostream& stream) const {
    if (!Detail::write_parameters(stream, featureTransformer))
        return false;
    for (std::size_t i = 0; i < Arch::LayerStacks; ++i)
        if (!Detail::write_parameters(stream, network[i]))
            return false;
    return bool(stream);
}

template class NetworkImpl<VariantArchitecture>;
#ifdef LARGEBOARDS
template class NetworkImpl<ShogiArchitecture256>;
template class NetworkImpl<ShogiArchitecture768>;
#endif


// Network

Network::Network(const Network& other) :
    impl(other.impl ? other.impl->clone() : nullptr),
    version(other.version),
    hash(other.hash),
    evalFile(other.evalFile),
    var(other.var) {}

Network& Network::operator=(const Network& other) {
    impl     = other.impl ? other.impl->clone() : nullptr;
    version  = other.version;
    hash     = other.hash;
    evalFile = other.evalFile;
    var      = other.var;
    return *this;
}

void Network::load(const std::string& rootDirectory, std::string evalfilePath, const Variant* v) {
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
        // A network is specific to the variant it was loaded for
        if (evalFile.current != evalfilePath || var != v)
        {
            if (directory != "<internal>")
                load_user_net(directory, evalfilePath, v);

            if (directory == "<internal>" && evalfilePath == evalFile.defaultName)
                load_internal(v);
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

void Network::clear(AccumulatorCaches& cache) const {
    if (impl)
        impl->clear(cache);
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

void Network::load_user_net(const std::string& dir,
                            const std::string& evalfilePath,
                            const Variant*     v) {
    std::ifstream stream(dir + evalfilePath, std::ios::binary);
    if (!stream)
        return;

    stream.seekg(0, std::ios::end);
    const std::size_t size = std::size_t(stream.tellg());
    stream.seekg(0, std::ios::beg);

    if (load(stream, size, v))
        evalFile.current = evalfilePath;
}

void Network::load_internal(const Variant* v) {
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

    if (load(stream, size_t(gEmbeddedNNUESize), v))
        evalFile.current = evalFile.defaultName;
}

bool Network::save(std::ostream&      stream,
                   const std::string& name,
                   const std::string& netDescription) const {
    if (name.empty() || name == "None" || !impl)
        return false;

    return write_header(stream, version, hash, netDescription) && impl->write_parameters(stream);
}

bool Network::load(std::istream& stream, std::size_t size, const Variant* v) {
    std::string description;

    // A failed attempt leaves the network without valid parameters
    evalFile.current = "None";
    var              = nullptr;
    impl.reset();

    if (!read_header(stream, &version, &hash, &description))
        return false;

    // The architecture is determined by the header
    impl = Detail::create_network(version, hash);

    const std::size_t headerSize = 3 * sizeof(std::uint32_t) + description.size();

    if (!impl || !impl->read_parameters(stream, size, headerSize, v))
    {
        impl.reset();
        return false;
    }

    var                     = v;
    evalFile.netDescription = description;
    return true;
}

// Read network header
bool Network::read_header(std::istream&  stream,
                          std::uint32_t* fileVersion,
                          std::uint32_t* hashValue,
                          std::string*   desc) const {
    std::uint32_t size;

    *fileVersion = read_little_endian<std::uint32_t>(stream);
    *hashValue   = read_little_endian<std::uint32_t>(stream);
    size         = read_little_endian<std::uint32_t>(stream);
    if (!stream)
        return false;
    desc->resize(size);
    stream.read(&(*desc)[0], size);
    return !stream.fail();
}

// Write network header
bool Network::write_header(std::ostream&      stream,
                           std::uint32_t      fileVersion,
                           std::uint32_t      hashValue,
                           const std::string& desc) const {
    write_little_endian<std::uint32_t>(stream, fileVersion);
    write_little_endian<std::uint32_t>(stream, hashValue);
    write_little_endian<std::uint32_t>(stream, std::uint32_t(desc.size()));
    stream.write(&desc[0], desc.size());
    return !stream.fail();
}

}  // namespace Stockfish::Eval::NNUE
