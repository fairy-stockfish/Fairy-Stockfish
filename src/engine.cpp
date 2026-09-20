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

#include "engine.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

#include "evaluate.h"
#include "misc.h"
#include "perft.h"
#include "position.h"
#include "search.h"
#include "syzygy/tbprobe.h"
#include "types.h"
#include "uci.h"
#include "variant.h"
#include "nnue/network.h"

namespace Stockfish {

Engine* mainEngine = nullptr;

constexpr NumaAutoPolicy DefaultNumaPolicy = BundledL3Policy{32};

Engine::Engine() :
    numaContext(NumaConfig::from_system(DefaultNumaPolicy)),
    states(new std::deque<StateInfo>(1)),
    options(Options),
    threads(Threads),
    tt(TT),
    // The network is large, so it is created on the heap instead of the stack
    networks(numaContext,
             std::move(*std::make_unique<Eval::NNUE::Network>(
               Eval::NNUE::EvalFile{EvalFileDefaultName, "None", ""}))) {
    const Variant* v = variants.find(options["UCI_Variant"])->second;
    pos.set(v, v->startFen, options["UCI_Chess960"], &states->back(), nullptr);
    mainEngine = this;

    // Default no-op callback, the UCI loop and the bindings may replace it
    updateContext.onStart = []() {};
}

std::uint64_t Engine::perft(Depth depth) {
    verify_networks();

    uint64_t nodes = Stockfish::perft<true>(pos, depth);
    sync_cout << "\nNodes searched: " << nodes << "\n" << sync_endl;
    return nodes;
}

void Engine::go(Search::LimitsType& limits) {
    assert(limits.perft == 0);
    verify_networks();

    threads.start_thinking(options, pos, states, limits);
}
void Engine::stop() { threads.stop = true; }

void Engine::search_clear() {
    wait_for_search_finished();

    tt.clear(threads);
    threads.clear();

    // @TODO wont work with multiple instances
    Tablebases::init(options["SyzygyPath"]);  // Free mapped files
}

void Engine::set_on_update_no_moves(std::function<void(const Engine::InfoShort&)>&& f) {
    updateContext.onUpdateNoMoves = std::move(f);
}

void Engine::set_on_update_full(std::function<void(const Engine::InfoFull&)>&& f) {
    updateContext.onUpdateFull = std::move(f);
}

void Engine::set_on_iter(std::function<void(const Engine::InfoIter&)>&& f) {
    updateContext.onIter = std::move(f);
}

void Engine::set_on_start(std::function<void()>&& f) { updateContext.onStart = std::move(f); }

void Engine::set_on_bestmove(std::function<void(std::string_view, std::string_view)>&& f) {
    updateContext.onBestmove = std::move(f);
}

void Engine::wait_for_search_finished() {
    // The thread pool may already be destroyed at program exit
    if (threads.size())
        threads.main_thread()->wait_for_search_finished();
}

void Engine::set_position(const std::string&              fen,
                          const std::vector<std::string>& moves,
                          bool                            sfen) {
    // Drop the old state and create a new one
    states = StateListPtr(new std::deque<StateInfo>(1));
    pos.set(variants.find(options["UCI_Variant"])->second, fen, options["UCI_Chess960"],
            &states->back(), threads.main_thread()->worker.get(), sfen);

    for (const auto& move : moves)
    {
        std::string moveStr = move;
        auto        m       = UCI::to_move(pos, moveStr);

        if (m == Move::none())
            break;

        states->emplace_back();
        pos.do_move(m, states->back());
    }
}

// modifiers

bool Engine::set_numa_config_from_option(const std::string& o) {
    if (o == "auto" || o == "system")
    {
        numaContext.set_numa_config(NumaConfig::from_system(DefaultNumaPolicy));
    }
    else if (o == "hardware")
    {
        // Don't respect affinity set in the system.
        numaContext.set_numa_config(NumaConfig::from_system(DefaultNumaPolicy, false));
    }
    else if (o == "none")
    {
        numaContext.set_numa_config(NumaConfig{});
    }
    else
    {
        auto parsed = NumaConfig::from_string(o);
        if (!parsed.has_value())
            return false;
        numaContext.set_numa_config(std::move(*parsed));
    }

    // Force reallocation of threads in case affinities need to change.
    resize_threads();
    return true;
}

void Engine::resize_threads() {
    threads.wait_for_search_finished();
    threads.set(numaContext.get_numa_config(),
                Search::SharedState(options, threads, tt, sharedHistories, networks),
                updateContext);
    threads.ensure_network_replicated();

    // Reallocate the hash with the new threadpool size
    set_tt_size(options["Hash"]);
}

void Engine::set_tt_size(size_t mb) {
    wait_for_search_finished();
    tt.resize(mb, threads);
}

void Engine::set_ponderhit(bool b) { threads.main_manager()->ponder = b; }

// network related

void Engine::set_on_verify_networks(std::function<void(std::string_view)>&& f) {
    onVerifyNetworks = std::move(f);
}

void Engine::verify_networks() const {
    auto print = [this](std::string_view msg) {
        if (onVerifyNetworks)
            onVerifyNetworks(msg);
        else if (CurrentProtocol != XBOARD)
        {
            // Every line of a message is sent as an info string
            std::stringstream ss{std::string(msg)};
            for (std::string line; std::getline(ss, line);)
                sync_cout << "info string " << line << sync_endl;
        }
    };

    if (Eval::useNNUE)
        networks->verify(std::string(options["EvalFile"]), print);
    else
        print("classical evaluation enabled");
}

// Tries to load a NNUE network at startup time, or when the engine
// receives a UCI command "setoption name EvalFile value nn-[a-z0-9]{12}.nnue"
// The name of the NNUE network is always retrieved from the EvalFile option.
// We search the given network in three locations: internally (the default
// network may be embedded in the binary), in the active working directory and
// in the engine directory. Distro packagers may define the DEFAULT_NNUE_DIRECTORY
// variable to have the engine search in a special directory in their distro.
void Engine::load_networks() {

    // The evaluation settings must not be modified while a search is using them
    wait_for_search_finished();

    Eval::useNNUE = options["Use NNUE"];
    if (!Eval::useNNUE)
        return;

    std::string eval_file = std::string(options["EvalFile"]);

    // Restrict NNUE usage to corresponding variant
    // Support multiple variant networks separated by semicolon(Windows)/colon(Unix)
    std::stringstream ss(eval_file);
    std::string       variant = std::string(options["UCI_Variant"]);
    Eval::useNNUE             = false;
    while (std::getline(ss, eval_file, UCI::SepChar))
    {
        std::string basename  = eval_file.substr(eval_file.find_last_of("\\/") + 1);
        std::string nnueAlias = variants.find(variant)->second->nnueAlias;
        if (basename.rfind(variant, 0) != std::string::npos
            || (!nnueAlias.empty() && basename.rfind(nnueAlias, 0) != std::string::npos))
        {
            Eval::useNNUE = true;
            break;
        }
    }
    if (!Eval::useNNUE)
        return;

    // The feature layout of the network depends on the variant
    const Variant* v = variants.find(variant)->second;

    networks.modify_and_replicate([&](Eval::NNUE::Network& network) {
        network.load(CommandLine::binaryDirectory, eval_file, v);
    });
    threads.ensure_network_replicated();

    // The accumulator caches of the workers depend on the network
    threads.clear();
}

void Engine::save_network(const std::optional<std::string>& filename) { networks->save(filename); }

// utility functions

void Engine::trace_eval() const {
    StateListPtr trace_states(new std::deque<StateInfo>(1));
    Position     p;
    p.set(pos.variant(), pos.fen(), options["UCI_Chess960"], &trace_states->back(),
          threads.main_thread()->worker.get());

    verify_networks();

    sync_cout << "\n" << Eval::trace(p, *networks) << sync_endl;
}

const OptionsMap& Engine::get_options() const { return options; }
OptionsMap&       Engine::get_options() { return options; }

std::string Engine::fen() const { return pos.fen(); }

void Engine::flip() { pos.flip(); }

std::string Engine::visualize() const {
    std::stringstream ss;
    ss << pos;
    return ss.str();
}

int Engine::get_hashfull(int maxAge) const { return tt.hashfull(maxAge); }

std::vector<std::pair<size_t, size_t>> Engine::get_bound_thread_count_by_numa_node() const {
    auto                                   counts = threads.get_bound_thread_count_by_numa_node();
    const NumaConfig&                      cfg    = numaContext.get_numa_config();
    std::vector<std::pair<size_t, size_t>> ratios;
    NumaIndex                              n = 0;
    for (; n < counts.size(); ++n)
        ratios.emplace_back(counts[n], cfg.num_cpus_in_numa_node(n));
    if (!counts.empty())
        for (; n < cfg.num_numa_nodes(); ++n)
            ratios.emplace_back(0, cfg.num_cpus_in_numa_node(n));
    return ratios;
}

std::string Engine::get_numa_config_as_string() const {
    return numaContext.get_numa_config().to_string();
}

std::string Engine::numa_config_information_as_string() const {
    auto cfgStr = get_numa_config_as_string();
    return "Available processors: " + cfgStr;
}

std::string Engine::thread_binding_information_as_string() const {
    auto              boundThreadsByNode = get_bound_thread_count_by_numa_node();
    std::stringstream ss;
    if (boundThreadsByNode.empty())
        return ss.str();

    bool isFirst = true;

    for (auto&& [current, total] : boundThreadsByNode)
    {
        if (!isFirst)
            ss << ":";
        ss << current << "/" << total;
        isFirst = false;
    }

    return ss.str();
}

std::string Engine::thread_allocation_information_as_string() const {
    std::stringstream ss;

    size_t threadsSize = threads.size();
    ss << "Using " << threadsSize << (threadsSize > 1 ? " threads" : " thread");

    auto boundThreadsByNodeStr = thread_binding_information_as_string();
    if (boundThreadsByNodeStr.empty())
        return ss.str();

    ss << " with NUMA node thread binding: ";
    ss << boundThreadsByNodeStr;

    return ss.str();
}
}
