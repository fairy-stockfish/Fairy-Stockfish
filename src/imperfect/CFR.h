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

#ifndef CFR_H_INCLUDED
#define CFR_H_INCLUDED

#include <vector>
#include <atomic>
#include "../types.h"
#include "Subgame.h"

namespace Stockfish {
namespace FogOfWar {

/// CFRSolver implements PCFR+ (Predictive CFR+) with PRM+ (Positive Regret Matching)
/// Follows the paper's algorithm (Figure 10, Appendix B.3.6)
/// Uses last-iterate play (does not average strategies at runtime)
class CFRSolver {
public:
    CFRSolver() : iterations(0), running(false) {}

    /// run_iteration() performs one PCFR+ iteration
    /// Implements RunCFRIteration from Figure 10 of the paper
    void run_iteration(Subgame& subgame);

    /// run_continuous() runs CFR iterations continuously until stopped
    void run_continuous(Subgame& subgame);

    /// stop() signals the solver to stop
    void stop() { running = false; }

    /// is_running() checks if solver is active
    bool is_running() const { return running; }

    /// get_iterations() returns the number of completed iterations
    int get_iterations() const { return iterations; }

    /// reset() resets the solver state
    void reset() { iterations = 0; }

private:
    std::atomic<int> iterations;
    std::atomic<bool> running;

    /// Compute counterfactual values (CFV) for a node
    float compute_cfv(GameTreeNode* node, Subgame& subgame,
                      const std::vector<float>& reach_probs, Color player);

    /// Update regrets using PRM+ (Positive Regret Matching Plus)
    void update_regrets(InfosetNode* infoset, const std::vector<float>& actionValues,
                        float nodeValue, float reachProb);

    /// Compute current strategy from regrets using PRM+
    void compute_strategy(InfosetNode* infoset);

    /// Handle Resolve/Maxmargin gadget switching (Figure 10, lines 6-13)
    void handle_gadget_switching(Subgame& subgame);

    /// Add alternative values for Resolve gadget (Figure 10, lines 18-19)
    float add_alternative_value(float cfv, InfosetNode* infoset,
                                const std::vector<float>& currentX,
                                const std::vector<float>& currentY);
};

/// regret_matching() converts regrets to strategy using positive regret matching
std::vector<float> regret_matching(const std::vector<float>& regrets);

/// positive_regret_matching_plus() implements PRM+ update rule
std::vector<float> positive_regret_matching_plus(const std::vector<float>& regrets,
                                                   const std::vector<float>& oldStrategy,
                                                   float discountFactor = 1.0f);

} // namespace FogOfWar
} // namespace Stockfish

#endif // #ifndef CFR_H_INCLUDED
