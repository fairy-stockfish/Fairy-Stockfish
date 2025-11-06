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

#include "Selection.h"
#include <algorithm>
#include <random>
#include <numeric>

namespace Stockfish {
namespace FogOfWar {

bool is_in_resolve(const Subgame& subgame) {
    // In Resolve when gadget type is RESOLVE and opponent hasn't entered
    return subgame.get_gadget_type() == GadgetType::RESOLVE &&
           !subgame.has_resolve_entered();
}

std::vector<float> ActionSelection::compute_margins(const InfosetNode* infoset) {
    if (!infoset)
        return {};

    // Simplified implementation: use Q-values as margins
    // Full implementation would compute actual Maxmargin margins
    std::vector<float> margins(infoset->actions.size());

    // Find best Q-value
    float bestQ = *std::max_element(infoset->qValues.begin(), infoset->qValues.end());

    // Margins are differences from best
    for (size_t i = 0; i < infoset->qValues.size(); ++i)
        margins[i] = infoset->qValues[i] - bestQ;

    return margins;
}

std::vector<float> ActionSelection::purify_strategy(const std::vector<float>& strategy,
                                                      const std::vector<float>& margins,
                                                      bool inResolve) {
    if (strategy.empty())
        return {};

    // If in Resolve, play deterministically (best action)
    if (inResolve) {
        std::vector<float> purified(strategy.size(), 0.0f);
        size_t bestIdx = std::distance(strategy.begin(),
                                       std::max_element(strategy.begin(), strategy.end()));
        purified[bestIdx] = 1.0f;
        return purified;
    }

    // Otherwise, apply purification with MaxSupport constraint
    // Keep only top MaxSupport actions with non-negative margins

    // Create pairs of (index, probability, margin)
    std::vector<std::tuple<size_t, float, float>> actions;
    for (size_t i = 0; i < strategy.size(); ++i) {
        if (strategy[i] > 0.0f && check_stability(margins[i]))
            actions.emplace_back(i, strategy[i], margins[i]);
    }

    // Sort by probability (descending)
    std::sort(actions.begin(), actions.end(),
              [](const auto& a, const auto& b) {
                  return std::get<1>(a) > std::get<1>(b);
              });

    // Keep only top MaxSupport actions
    std::vector<float> purified(strategy.size(), 0.0f);
    float totalProb = 0.0f;

    size_t numToKeep = std::min(static_cast<size_t>(maxSupport), actions.size());
    for (size_t i = 0; i < numToKeep; ++i) {
        size_t idx = std::get<0>(actions[i]);
        purified[idx] = std::get<1>(actions[i]);
        totalProb += purified[idx];
    }

    // Renormalize
    if (totalProb > 0.0f) {
        for (float& p : purified)
            p /= totalProb;
    } else {
        // Fallback: uniform over all actions
        std::fill(purified.begin(), purified.end(), 1.0f / purified.size());
    }

    return purified;
}

Move ActionSelection::select_deterministic(const InfosetNode* infoset) {
    if (!infoset || infoset->actions.empty())
        return MOVE_NONE;

    // Select action with highest strategy weight
    size_t bestIdx = 0;
    float bestProb = infoset->strategy[0];

    for (size_t i = 1; i < infoset->strategy.size(); ++i) {
        if (infoset->strategy[i] > bestProb) {
            bestProb = infoset->strategy[i];
            bestIdx = i;
        }
    }

    return infoset->actions[bestIdx];
}

Move ActionSelection::select_stochastic(const InfosetNode* infoset,
                                         const std::vector<float>& purifiedStrategy) {
    if (!infoset || infoset->actions.empty() || purifiedStrategy.empty())
        return MOVE_NONE;

    // Sample from purified strategy
    std::discrete_distribution<size_t> dist(purifiedStrategy.begin(),
                                             purifiedStrategy.end());
    std::mt19937 rng(std::random_device{}());
    size_t selectedIdx = dist(rng);

    return infoset->actions[selectedIdx];
}

Move ActionSelection::select_move(const InfosetNode* rootInfoset,
                                   const Subgame& subgame) {
    if (!rootInfoset || rootInfoset->actions.empty())
        return MOVE_NONE;

    // Compute margins for stability check
    std::vector<float> margins = compute_margins(rootInfoset);

    // Check if we're in Resolve
    bool inResolve = is_in_resolve(subgame);

    // Apply purification
    std::vector<float> purifiedStrategy = purify_strategy(rootInfoset->strategy,
                                                           margins,
                                                           inResolve);

    // If deterministic (in Resolve or only one action has support), select best
    int supportSize = std::count_if(purifiedStrategy.begin(), purifiedStrategy.end(),
                                     [](float p) { return p > 0.0f; });

    if (inResolve || supportSize <= 1)
        return select_deterministic(rootInfoset);

    // Otherwise sample from purified strategy
    return select_stochastic(rootInfoset, purifiedStrategy);
}

} // namespace FogOfWar
} // namespace Stockfish
