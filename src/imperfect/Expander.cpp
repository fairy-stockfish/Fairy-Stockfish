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

#include "Expander.h"
#include "Evaluator.h"
#include "../variant.h"
#include "../position.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace Stockfish {
namespace FogOfWar {

/// compute_variance() computes variance estimate for an action
/// Uses variance prior of {-1, +1} (paper: Appendix B.3.3)
float compute_variance(const InfosetNode* infoset, size_t actionIdx) {
    if (!infoset || actionIdx >= infoset->variances.size())
        return 2.0f; // Variance of {-1, +1} distribution

    // Return stored variance estimate
    return infoset->variances[actionIdx];
}

float Expander::compute_puct_score(const InfosetNode* infoset, size_t actionIdx) {
    if (!infoset || actionIdx >= infoset->actions.size())
        return 0.0f;

    // Q_bar(I,a) = u(x,y|I,a) + C * sigma(I,a) * sqrt(N(I)) / (1 + N(I,a))
    float qValue = infoset->qValues[actionIdx];
    float variance = compute_variance(infoset, actionIdx);
    float totalVisits = static_cast<float>(infoset->totalVisits);
    float actionVisits = static_cast<float>(infoset->visitCounts[actionIdx]);

    float exploration = puctConstant * std::sqrt(variance) *
                        std::sqrt(totalVisits) / (1.0f + actionVisits);

    return qValue + exploration;
}

size_t Expander::select_action_puct(const InfosetNode* infoset) {
    if (!infoset || infoset->actions.empty())
        return 0;

    // Find action with highest PUCT score
    size_t bestAction = 0;
    float bestScore = compute_puct_score(infoset, 0);

    for (size_t i = 1; i < infoset->actions.size(); ++i) {
        float score = compute_puct_score(infoset, i);
        if (score > bestScore) {
            bestScore = score;
            bestAction = i;
        }
    }

    return bestAction;
}

std::vector<float> Expander::build_exploration_strategy(const InfosetNode* infoset) {
    if (!infoset || infoset->actions.empty())
        return {};

    size_t numActions = infoset->actions.size();
    std::vector<float> strategy(numActions, 0.0f);

    // 50/50 mix of uniform and PUCT
    // (1) Uniform over support of x_t
    float uniformProb = 1.0f / numActions;
    for (size_t i = 0; i < numActions; ++i)
        if (infoset->strategy[i] > 0.0f)
            strategy[i] = uniformProb;

    // (2) PUCT argmax
    size_t puctAction = select_action_puct(infoset);

    // Mix 50/50
    std::vector<float> mixed(numActions);
    for (size_t i = 0; i < numActions; ++i) {
        mixed[i] = 0.5f * strategy[i];
        if (i == puctAction)
            mixed[i] += 0.5f;
    }

    return mixed;
}

GameTreeNode* Expander::select_leaf(GameTreeNode* root, Subgame& subgame) {
    if (!root)
        return nullptr;

    GameTreeNode* current = root;

    // Traverse tree until we reach an unexpanded node
    while (current->expanded && !current->children.empty()) {
        // TODO: Determine side to move from FEN or pass as parameter
        // For now, alternate by depth (even=WHITE, odd=BLACK)
        Color nodePlayer = (current->depth % 2 == 0) ? WHITE : BLACK;
        SequenceId seqId = nodePlayer == WHITE ? current->ourSequence : current->theirSequence;
        InfosetNode* infoset = subgame.get_infoset(seqId, nodePlayer);

        if (!infoset || infoset->actions.empty())
            break;

        // If this is the exploring side, use PUCT; otherwise use current strategy
        size_t actionIdx;
        if (nodePlayer == exploringSide) {
            actionIdx = select_action_puct(infoset);
        } else {
            // Sample from current strategy
            std::discrete_distribution<size_t> dist(infoset->strategy.begin(),
                                                     infoset->strategy.end());
            std::mt19937 rng(std::random_device{}());
            actionIdx = dist(rng);
        }

        // Move to child
        if (actionIdx < current->children.size())
            current = current->children[actionIdx].get();
        else
            break;
    }

    return current;
}

void Expander::initialize_to_best_child(InfosetNode* infoset,
                                         const std::vector<ChildEvaluation>& childEvals) {
    if (!infoset || childEvals.empty())
        return;

    // Find best child (Appendix B.3.4, Figure 12 lines 24-27)
    size_t bestIdx = 0;
    float bestValue = childEvals[0].value;

    for (size_t i = 1; i < childEvals.size(); ++i) {
        if (childEvals[i].value > bestValue) {
            bestValue = childEvals[i].value;
            bestIdx = i;
        }
    }

    // Initialize strategy: all weight on best child
    std::fill(infoset->strategy.begin(), infoset->strategy.end(), 0.0f);
    if (bestIdx < infoset->strategy.size())
        infoset->strategy[bestIdx] = 1.0f;

    // Initialize Q-values from evaluations
    for (size_t i = 0; i < childEvals.size() && i < infoset->qValues.size(); ++i)
        infoset->qValues[i] = childEvals[i].value;
}

void Expander::expand_leaf(GameTreeNode* leaf, Subgame& subgame, Position& pos) {
    if (!leaf || leaf->expanded)
        return;

    // Expand node to get children
    subgame.expand_node(leaf, pos);

    if (leaf->terminal || leaf->children.empty())
        return;

    // Evaluate all children (MultiPV depth 1, clamped to [-1, +1])
    std::vector<ChildEvaluation> childEvals = evaluate_children(pos);

    // Get or create infoset for this node
    Color nodePlayer = pos.side_to_move();
    SequenceId seqId = nodePlayer == WHITE ? leaf->ourSequence : leaf->theirSequence;
    InfosetNode* infoset = subgame.get_infoset(seqId, nodePlayer);

    if (!infoset)
        return;

    // Initialize infoset with actions
    infoset->actions.clear();
    for (const auto& eval : childEvals)
        infoset->actions.push_back(eval.move);

    size_t numActions = infoset->actions.size();
    infoset->regrets.resize(numActions, 0.0f);
    infoset->strategy.resize(numActions, 0.0f);
    infoset->cumulativeStrategy.resize(numActions, 0.0f);
    infoset->visitCounts.resize(numActions, 0);
    infoset->qValues.resize(numActions, 0.0f);
    infoset->variances.resize(numActions, 2.0f); // Variance prior {-1, +1}

    // Initialize to best child (Appendix B.3.4)
    initialize_to_best_child(infoset, childEvals);

    infoset->expanded = true;
}

bool Expander::run_expansion_step(Subgame& subgame) {
    std::lock_guard<std::mutex> lock(expansionMutex);

    if (!subgame.root())
        return false;

    // Select a leaf node
    GameTreeNode* leaf = select_leaf(subgame.root(), subgame);
    if (!leaf || leaf->expanded)
        return false;

    // Expand the leaf
    StateInfo st;
    Position pos;

    // Use the variant stored in the subgame
    const Variant* variant = subgame.get_variant();
    if (!variant)
        return false;

    pos.set(variant, leaf->stateFen, false, &st, nullptr, true);
    expand_leaf(leaf, subgame, pos);

    // Alternate exploring side (Appendix B.3.3)
    alternate_exploring_side();

    expansionCount++;
    return true;
}

void Expander::run_continuous(Subgame& subgame) {
    running = true;
    while (running) {
        if (!run_expansion_step(subgame))
            break; // No more nodes to expand
    }
}

} // namespace FogOfWar
} // namespace Stockfish
