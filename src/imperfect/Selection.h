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

#ifndef SELECTION_H_INCLUDED
#define SELECTION_H_INCLUDED

#include <vector>
#include "../types.h"
#include "Subgame.h"

namespace Stockfish {
namespace FogOfWar {

/// ActionSelection handles move selection and purification
/// Implements the purification strategy from Step 5 and Appendix B.3.7
class ActionSelection {
public:
    ActionSelection() : maxSupport(3) {}

    /// select_move() chooses a move using purified strategy
    /// Implements Figure 8, lines 12-21 (purification)
    Move select_move(const InfosetNode* rootInfoset,
                     const Subgame& subgame);

    /// purify_strategy() applies purification to limit mixing
    /// MaxSupport = 3 by default (allow up to 3 actions)
    /// Only mix among "stable" actions with non-negative margins
    std::vector<float> purify_strategy(const std::vector<float>& strategy,
                                        const std::vector<float>& margins,
                                        bool inResolve);

    /// set_max_support() sets the maximum number of actions to mix
    void set_max_support(int maxSup) { maxSupport = maxSup; }

    /// compute_margins() computes margins for each action under Maxmargin gadget
    std::vector<float> compute_margins(const InfosetNode* infoset);

private:
    int maxSupport; // Maximum support size for mixed strategy (default: 3)

    /// check_stability() verifies if action is stable (non-negative margin)
    bool check_stability(float margin) const { return margin >= 0.0f; }

    /// select_deterministic() selects best action deterministically
    Move select_deterministic(const InfosetNode* infoset);

    /// select_stochastic() samples from purified strategy
    Move select_stochastic(const InfosetNode* infoset,
                           const std::vector<float>& purifiedStrategy);
};

/// is_in_resolve() checks if we're in Resolve gadget (p_max == 0)
bool is_in_resolve(const Subgame& subgame);

} // namespace FogOfWar
} // namespace Stockfish

#endif // #ifndef SELECTION_H_INCLUDED
