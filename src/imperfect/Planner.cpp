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

#include "Planner.h"
#include "../position.h"
#include "../misc.h"
#include <chrono>
#include <iostream>

namespace Stockfish {
namespace FogOfWar {

Planner::Planner() : stopSearch(false) {
    solver = std::make_unique<CFRSolver>();
    selector = std::make_unique<ActionSelection>();
}

Planner::~Planner() {
    stop_threads();
}

bool is_fow_variant(const Position& pos) {
    // Check if this is Fog-of-War chess
    // For now, we'll check via a variant name or flag
    // This should be properly implemented based on variant system
    return false; // Placeholder
}

void Planner::update_observation_history(const Position& pos) {
    Observation obs = create_observation(pos);
    obsHistory.add_observation(obs);
}

void Planner::construct_subgame(const Position& pos) {
    // Implements ConstructSubgame from Figure 9

    // Step 1: Rebuild belief state P from observations
    if (config.enableIncrementalBelief && !obsHistory.observations().empty()) {
        beliefState.update_incrementally(obsHistory.last());
    } else {
        beliefState.rebuild_from_observations(obsHistory, pos);
    }

    // Step 2: Sample I ⊂ P (default 256 states)
    std::vector<Position> sampledStates = beliefState.sample_states(
        config.minInfosetSize,
        std::chrono::steady_clock::now().time_since_epoch().count()
    );

    // If belief state is empty or too small, use current position
    if (sampledStates.empty()) {
        sampledStates.push_back(pos);
    }

    // Step 3: Construct subgame (2-KLUSS)
    subgame = std::make_unique<Subgame>();
    subgame->construct(sampledStates, config.minInfosetSize);

    // Step 4: Initialize Resolve and Maxmargin gadgets
    // The gadget switching is handled by the solver (Figure 10, lines 6-13)
    subgame->set_gadget_type(GadgetType::RESOLVE);
}

void Planner::launch_threads() {
    stopSearch = false;
    threads.clear();

    // Launch CFR solver thread (1 thread, as per paper)
    for (int i = 0; i < config.numSolverThreads; ++i) {
        threads.emplace_back([this]() {
            solver->run_continuous(*subgame);
        });
    }

    // Launch expander threads (2 threads, as per paper)
    expanders.clear();
    for (int i = 0; i < config.numExpanderThreads; ++i) {
        auto expander = std::make_unique<Expander>();
        expander->set_expander_id(i);
        expander->set_puct_constant(config.puctConstant);

        threads.emplace_back([this, exp = expander.get()]() {
            exp->run_continuous(*subgame);
        });

        expanders.push_back(std::move(expander));
    }
}

void Planner::stop_threads() {
    stopSearch = true;

    // Stop expanders first (paper: allows solver to finish with stable tree)
    for (auto& exp : expanders)
        exp->stop();

    // Small grace period for expanders to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Stop solver
    solver->stop();

    // Join all threads
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }

    threads.clear();
}

void Planner::update_statistics() {
    stats.numNodes = subgame ? subgame->count_nodes() : 0;
    stats.numInfosets = subgame ? subgame->num_infosets() : 0;
    stats.beliefStateSize = beliefState.size();
    stats.averageDepth = subgame ? subgame->average_depth() : 0;
    stats.cfrIterations = solver->get_iterations();

    stats.totalExpansions = 0;
    for (const auto& exp : expanders)
        stats.totalExpansions += exp->get_expansion_count();
}

Move Planner::plan_move(Position& pos, const PlannerConfig& cfg) {
    config = cfg;
    auto startTime = std::chrono::steady_clock::now();

    // Step 1: Update observation history (Figure 8, line 6)
    update_observation_history(pos);

    // Step 2: Construct subgame (Figure 8, line 7; Figure 9)
    construct_subgame(pos);

    // Step 3: Launch threads (Figure 8, lines 8-10)
    launch_threads();

    // Step 4: Run until time limit
    std::this_thread::sleep_for(std::chrono::milliseconds(config.maxTimeMs));

    // Step 5: Stop threads (expanders first, then solver)
    stop_threads();

    // Step 6: Collect statistics
    auto endTime = std::chrono::steady_clock::now();
    stats.timeUsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();
    update_statistics();

    // Step 7: Select move using purified strategy (Figure 8, lines 12-21)
    selector->set_max_support(config.maxSupport);

    if (!subgame || !subgame->root())
        return MOVE_NONE;

    // Get root infoset
    Color us = pos.side_to_move();
    InfosetNode* rootInfoset = subgame->get_infoset(0, us);

    Move selectedMove = selector->select_move(rootInfoset, *subgame);

    // Print statistics
    std::cout << "info string FoW search: "
              << "nodes " << stats.numNodes << " "
              << "infosets " << stats.numInfosets << " "
              << "belief_size " << stats.beliefStateSize << " "
              << "avg_depth " << stats.averageDepth << " "
              << "cfr_iters " << stats.cfrIterations << " "
              << "expansions " << stats.totalExpansions << " "
              << "time_ms " << stats.timeUsedMs
              << std::endl;

    return selectedMove;
}

} // namespace FogOfWar
} // namespace Stockfish
