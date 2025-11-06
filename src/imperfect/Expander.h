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

#ifndef EXPANDER_H_INCLUDED
#define EXPANDER_H_INCLUDED

#include <atomic>
#include <mutex>
#include "../types.h"
#include "Subgame.h"
#include "Evaluator.h"

namespace Stockfish {

class Position;

namespace FogOfWar {

/// Expander implements one-sided GT-CFR expansion with PUCT selection
/// Follows Figure 12 (RunExpanderThread, DoExpansionStep) from the paper
/// Uses alternating exploring side (Appendix B.3.3)
class Expander {
public:
    Expander() : expanderId(0), running(false), exploringSide(WHITE),
                 puctConstant(1.0f), expansionCount(0) {}

    /// run_expansion_step() performs one expansion (Figure 12)
    /// Returns true if expansion was successful
    bool run_expansion_step(Subgame& subgame);

    /// run_continuous() runs expansion continuously until stopped
    void run_continuous(Subgame& subgame);

    /// stop() signals the expander to stop
    void stop() { running = false; }

    /// is_running() checks if expander is active
    bool is_running() const { return running; }

    /// set_expander_id() sets the ID for this expander thread
    void set_expander_id(int id) { expanderId = id; }

    /// set_puct_constant() sets the PUCT exploration constant C (default: 1.0)
    void set_puct_constant(float c) { puctConstant = c; }

    /// get_expansion_count() returns number of expansions performed
    int get_expansion_count() const { return expansionCount; }

    /// reset() resets the expander state
    void reset() { expansionCount = 0; exploringSide = WHITE; }

private:
    int expanderId;
    std::atomic<bool> running;
    Color exploringSide;
    float puctConstant;
    std::atomic<int> expansionCount;
    std::mutex expansionMutex;

    /// Select a leaf node using one-sided GT-CFR + PUCT
    GameTreeNode* select_leaf(GameTreeNode* root, Subgame& subgame);

    /// Compute PUCT score for action selection (Appendix B.3.3)
    /// Q_bar(I,a) = u(x,y|I,a) + C * sigma(I,a) * sqrt(N(I)) / (1 + N(I,a))
    float compute_puct_score(const InfosetNode* infoset, size_t actionIdx);

    /// Select action using PUCT
    size_t select_action_puct(const InfosetNode* infoset);

    /// Expand a leaf node (Figure 12, lines 13-27)
    /// Evaluates children and initializes regret minimizer to best child
    void expand_leaf(GameTreeNode* leaf, Subgame& subgame, Position& pos);

    /// Initialize new infoset with best child strategy (Appendix B.3.4)
    void initialize_to_best_child(InfosetNode* infoset,
                                   const std::vector<ChildEvaluation>& childEvals);

    /// Alternate exploring side (Appendix B.3.3)
    void alternate_exploring_side() { exploringSide = ~exploringSide; }

    /// Build exploration strategy x_tilde (50/50 mix of uniform + PUCT)
    std::vector<float> build_exploration_strategy(const InfosetNode* infoset);
};

/// compute_variance() computes variance estimate for PUCT
/// Uses variance prior of {-1, +1} initially (Appendix B.3.3)
float compute_variance(const InfosetNode* infoset, size_t actionIdx);

} // namespace FogOfWar
} // namespace Stockfish

#endif // #ifndef EXPANDER_H_INCLUDED
