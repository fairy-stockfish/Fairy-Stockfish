/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2024 Fabian Fichter

  Fairy-Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Fairy-Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLANNER_H_INCLUDED
#define PLANNER_H_INCLUDED

#include <thread>
#include <memory>
#include <atomic>
#include "../types.h"
#include "Belief.h"
#include "Subgame.h"
#include "CFR.h"
#include "Expander.h"
#include "Selection.h"

namespace Stockfish {

class Position;

namespace FogOfWar {

/// PlannerConfig holds configuration parameters
struct PlannerConfig {
    int minInfosetSize = 256;     // Sample size for root infoset (Figure 9, line 10)
    int numExpanderThreads = 2;   // Number of expander threads (paper uses 2)
    int numSolverThreads = 1;     // Number of CFR solver threads (paper uses 1)
    float puctConstant = 1.0f;    // PUCT exploration constant C (paper uses 1.0)
    int maxSupport = 3;           // Max actions in purified strategy (paper uses 3)
    int maxTimeMs = 5000;         // Maximum thinking time in milliseconds
    bool enableIncrementalBelief = false; // Use incremental belief update
};

/// Planner is the main coordinator for Obscuro-style FoW search
/// Implements the top-level Move() loop from Figure 8
class Planner {
public:
    Planner();
    ~Planner();

    /// plan_move() is the main entry point for FoW move selection
    /// Implements Figure 8 (Move loop) from the paper
    Move plan_move(Position& pos, const PlannerConfig& config);

    /// set_config() updates configuration
    void set_config(const PlannerConfig& cfg) { config = cfg; }

    /// get_statistics() returns search statistics
    struct Statistics {
        size_t numNodes;
        size_t numInfosets;
        size_t beliefStateSize;
        int averageDepth;
        int cfrIterations;
        int totalExpansions;
        int timeUsedMs;
    };
    Statistics get_statistics() const { return stats; }

private:
    PlannerConfig config;
    Statistics stats;

    // Core components
    ObservationHistory obsHistory;
    BeliefState beliefState;
    std::unique_ptr<Subgame> subgame;
    std::unique_ptr<CFRSolver> solver;
    std::vector<std::unique_ptr<Expander>> expanders;
    std::unique_ptr<ActionSelection> selector;

    // Threading
    std::vector<std::thread> threads;
    std::atomic<bool> stopSearch;

    /// construct_subgame() implements ConstructSubgame from Figure 9
    void construct_subgame(const Position& pos);

    /// launch_threads() starts solver and expander threads (Figure 8, lines 8-10)
    void launch_threads();

    /// stop_threads() stops all threads gracefully
    /// Stops expanders first, then solver (for better last iterate)
    void stop_threads();

    /// update_observation_history() adds new observation
    void update_observation_history(const Position& pos);

    /// update_statistics() collects metrics for reporting
    void update_statistics();
};

/// is_fow_variant() checks if the current variant is Fog-of-War chess
bool is_fow_variant(const Position& pos);

} // namespace FogOfWar
} // namespace Stockfish

#endif // #ifndef PLANNER_H_INCLUDED
