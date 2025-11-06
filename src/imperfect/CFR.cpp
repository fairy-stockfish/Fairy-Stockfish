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

#include "CFR.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace Stockfish {
namespace FogOfWar {

/// regret_matching() converts regrets to probabilities
/// Uses positive regret matching: only positive regrets contribute
std::vector<float> regret_matching(const std::vector<float>& regrets) {
    std::vector<float> strategy(regrets.size(), 0.0f);

    // Sum of positive regrets
    float sumPositiveRegret = 0.0f;
    for (float r : regrets)
        if (r > 0.0f)
            sumPositiveRegret += r;

    if (sumPositiveRegret > 0.0f) {
        // Proportional to positive regrets
        for (size_t i = 0; i < regrets.size(); ++i)
            if (regrets[i] > 0.0f)
                strategy[i] = regrets[i] / sumPositiveRegret;
    } else {
        // Uniform if no positive regrets
        float uniform = 1.0f / regrets.size();
        std::fill(strategy.begin(), strategy.end(), uniform);
    }

    return strategy;
}

/// positive_regret_matching_plus() implements PRM+ with discounting
std::vector<float> positive_regret_matching_plus(const std::vector<float>& regrets,
                                                   const std::vector<float>& oldStrategy,
                                                   float discountFactor) {
    // PRM+ uses linear discount on regrets
    std::vector<float> discountedRegrets(regrets.size());
    for (size_t i = 0; i < regrets.size(); ++i)
        discountedRegrets[i] = regrets[i] * discountFactor;

    return regret_matching(discountedRegrets);
}

void CFRSolver::compute_strategy(InfosetNode* infoset) {
    if (!infoset || infoset->regrets.empty())
        return;

    infoset->strategy = regret_matching(infoset->regrets);
}

void CFRSolver::update_regrets(InfosetNode* infoset,
                                const std::vector<float>& actionValues,
                                float nodeValue,
                                float reachProb) {
    if (!infoset || actionValues.size() != infoset->actions.size())
        return;

    // Update regrets: R(a) += reach_prob * (V(a) - V(node))
    for (size_t i = 0; i < actionValues.size(); ++i) {
        float instantRegret = actionValues[i] - nodeValue;
        infoset->regrets[i] += reachProb * instantRegret;

        // PRM+: ensure regrets stay non-negative
        infoset->regrets[i] = std::max(0.0f, infoset->regrets[i]);
    }
}

float CFRSolver::compute_cfv(GameTreeNode* node, Subgame& subgame,
                              const std::vector<float>& reach_probs,
                              Color player) {
    if (!node)
        return 0.0f;

    // Terminal node
    if (node->terminal)
        return node->terminalValue;

    // If not in KLUSS, return cached value
    if (!node->inKLUSS)
        return 0.0f; // Placeholder: should use cached value

    // Get infoset
    // TODO: Determine side to move from FEN or pass as parameter
    // For now, alternate by depth (even=WHITE, odd=BLACK)
    Color nodePlayer = (node->depth % 2 == 0) ? WHITE : BLACK;
    SequenceId seqId = nodePlayer == WHITE ? node->ourSequence : node->theirSequence;
    InfosetNode* infoset = subgame.get_infoset(seqId, nodePlayer);

    if (!infoset || infoset->actions.empty())
        return 0.0f;

    // Compute strategy if needed
    if (infoset->strategy.empty())
        compute_strategy(infoset);

    // Compute value of each action
    std::vector<float> actionValues(infoset->actions.size(), 0.0f);
    float nodeValue = 0.0f;

    for (size_t i = 0; i < infoset->actions.size(); ++i) {
        // Find child corresponding to this action
        // Simplified: assume children match actions in order
        if (i < node->children.size()) {
            GameTreeNode* child = node->children[i].get();
            float childValue = compute_cfv(child, subgame, reach_probs, player);
            actionValues[i] = childValue;
            nodeValue += infoset->strategy[i] * childValue;
        }
    }

    // Update regrets if this is the player's node
    if (nodePlayer == player) {
        float reachProb = reach_probs[player];
        update_regrets(infoset, actionValues, nodeValue, reachProb);
    }

    infoset->value = nodeValue;
    return nodeValue;
}

void CFRSolver::handle_gadget_switching(Subgame& subgame) {
    // Implements Figure 10, lines 6-13
    // Switch between Resolve and Maxmargin based on whether Resolve has been entered

    if (subgame.has_resolve_entered()) {
        // Solve in Maxmargin
        subgame.set_gadget_type(GadgetType::MAXMARGIN);
    } else {
        // Solve in Resolve
        subgame.set_gadget_type(GadgetType::RESOLVE);
    }
}

float CFRSolver::add_alternative_value(float cfv, InfosetNode* infoset,
                                        const std::vector<float>& currentX,
                                        const std::vector<float>& currentY) {
    // When in Resolve gadget, add v_alt to counterfactual values (Figure 10, lines 18-19)
    float altValue = compute_alternative_value(infoset, currentX, currentY);
    return cfv + altValue;
}

void CFRSolver::run_iteration(Subgame& subgame) {
    if (!subgame.root())
        return;

    // Handle gadget switching
    handle_gadget_switching(subgame);

    // Initialize reach probabilities (both players start with prob 1.0)
    std::vector<float> reachProbs(COLOR_NB, 1.0f);

    // Run CFR traversal for both players
    compute_cfv(subgame.root(), subgame, reachProbs, WHITE);
    compute_cfv(subgame.root(), subgame, reachProbs, BLACK);

    // Update all strategies
    for (auto& [seqId, infoset] : subgame.get_infosets())
        compute_strategy(&infoset);

    iterations++;
}

void CFRSolver::run_continuous(Subgame& subgame) {
    running = true;
    while (running) {
        run_iteration(subgame);
    }
}

} // namespace FogOfWar
} // namespace Stockfish
