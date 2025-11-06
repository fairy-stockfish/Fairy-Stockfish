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

#include "Evaluator.h"
#include "../position.h"
#include "../movegen.h"
#include "../evaluate.h"
#include <algorithm>
#include <cmath>

namespace Stockfish {
namespace FogOfWar {

/// normalize_value() converts centipawn evaluation to [-1, +1]
/// Mate scores map to +/-1, material scores are clamped
float normalize_value(Value v) {
    // Handle mate scores
    if (v >= VALUE_MATE_IN_MAX_PLY)
        return 1.0f;
    if (v <= VALUE_MATED_IN_MAX_PLY)
        return -1.0f;

    // Normalize material scores using sigmoid-like curve
    // Map roughly [-1000, +1000] centipawns to [-1, +1]
    constexpr float scale = 1000.0f;
    float normalized = float(v) / scale;

    // Clamp to [-1, +1]
    return std::max(-1.0f, std::min(1.0f, normalized));
}

/// evaluate_children() evaluates all legal child positions
/// Implements the depth-1 MultiPV evaluation described in Appendix B.3.4
std::vector<ChildEvaluation> evaluate_children(Position& pos) {
    std::vector<ChildEvaluation> evaluations;
    StateInfo st;

    // Generate all legal moves
    for (const auto& m : MoveList<LEGAL>(pos))
    {
        // Make the move
        pos.do_move(m, st);

        // Evaluate the resulting position
        Value eval = Eval::evaluate(pos);

        // Flip sign since we evaluated from opponent's perspective
        eval = -eval;

        // Undo the move
        pos.undo_move(m);

        // Store normalized evaluation
        ChildEvaluation ce;
        ce.move = m;
        ce.value = normalize_value(eval);
        evaluations.push_back(ce);
    }

    return evaluations;
}

/// get_best_child() returns the move with the highest evaluation
Move get_best_child(const std::vector<ChildEvaluation>& evals) {
    if (evals.empty())
        return MOVE_NONE;

    auto best = std::max_element(evals.begin(), evals.end(),
        [](const ChildEvaluation& a, const ChildEvaluation& b) {
            return a.value < b.value;
        });

    return best->move;
}

} // namespace FogOfWar
} // namespace Stockfish
