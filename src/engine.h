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

#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "history.h"
#include "nnue/network.h"
#include "numa.h"
#include "types.h"
#include "uci.h"

namespace Stockfish {

// The Engine class bundles the position, the search threads, the transposition
// table and the options. Fairy-Stockfish keeps the global Options, Threads and TT
// objects for the XBoard protocol, the bughouse partner logic and the bindings,
// so the engine holds references to them instead of owning them.
class Engine {
   public:
    using InfoShort = Search::InfoShort;
    using InfoFull  = Search::InfoFull;
    using InfoIter  = Search::InfoIteration;

    Engine();

    // Cannot be movable due to components holding backreferences to fields
    Engine(const Engine&)            = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(const Engine&) = delete;
    Engine& operator=(Engine&&)      = delete;

    ~Engine() { wait_for_search_finished(); }

    std::uint64_t perft(Depth depth);

    // non blocking call to start searching
    void go(Search::LimitsType&);
    // non blocking call to stop searching
    void stop();

    // blocking call to wait for search to finish
    void wait_for_search_finished();
    // set a new position of the current variant, moves are in UCI format
    void
    set_position(const std::string& fen, const std::vector<std::string>& moves, bool sfen = false);

    // modifiers

    bool set_numa_config_from_option(const std::string& o);
    void resize_threads();
    void set_tt_size(size_t mb);
    void set_ponderhit(bool);
    void search_clear();

    void set_on_update_no_moves(std::function<void(const InfoShort&)>&&);
    void set_on_update_full(std::function<void(const InfoFull&)>&&);
    void set_on_iter(std::function<void(const InfoIter&)>&&);
    void set_on_start(std::function<void()>&&);
    void set_on_bestmove(std::function<void(std::string_view, std::string_view)>&&);
    void set_on_verify_networks(std::function<void(std::string_view)>&&);

    // network related

    void verify_networks() const;
    void load_networks();
    void save_network(const std::optional<std::string>& filename);

    // utility functions

    void trace_eval() const;

    const OptionsMap& get_options() const;
    OptionsMap&       get_options();

    int get_hashfull(int maxAge = 0) const;

    std::string                            fen() const;
    void                                   flip();
    std::string                            visualize() const;
    std::vector<std::pair<size_t, size_t>> get_bound_thread_count_by_numa_node() const;
    std::string                            get_numa_config_as_string() const;
    std::string                            numa_config_information_as_string() const;
    std::string                            thread_allocation_information_as_string() const;
    std::string                            thread_binding_information_as_string() const;

    // Fairy-Stockfish: direct access to the game state for the XBoard
    // state machine and the bindings
    Position&     position() { return pos; }
    StateListPtr& state_list() { return states; }

   private:
    NumaReplicationContext numaContext;

    Position     pos;
    StateListPtr states;

    OptionsMap&         options;
    ThreadPool&         threads;
    TranspositionTable& tt;

    std::map<NumaIndex, SharedHistories> sharedHistories;

    LazyNumaReplicated<Eval::NNUE::Network> networks;

    Search::SearchManager::UpdateContext  updateContext;
    std::function<void(std::string_view)> onVerifyNetworks;
};

// The engine instance created by the UCI loop or the bindings,
// used by option callbacks that need to resize threads or the hash
extern Engine* mainEngine;

}  // namespace Stockfish


#endif  // #ifndef ENGINE_H_INCLUDED
