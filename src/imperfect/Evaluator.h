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

#ifndef EVALUATOR_H_INCLUDED
#define EVALUATOR_H_INCLUDED

#include <vector>
#include <utility>
#include "../types.h"

namespace Stockfish {

class Position;
class StateInfo;

namespace FogOfWar {

/// ChildEvaluation represents the evaluation of a single child position
struct ChildEvaluation {
    Move move;
    float value;  // Normalized to [-1, +1] range
};

/// evaluate_children() evaluates all legal moves from a position
/// Returns depth-1 evaluations clamped to [-1, +1] (Appendix B.3.4 of paper)
/// This is used as the leaf heuristic for expanded nodes
std::vector<ChildEvaluation> evaluate_children(Position& pos);

/// normalize_value() converts a Value to [-1, +1] range
float normalize_value(Value v);

/// get_best_child() returns the best child from evaluations
Move get_best_child(const std::vector<ChildEvaluation>& evals);

} // namespace FogOfWar
} // namespace Stockfish

#endif // #ifndef EVALUATOR_H_INCLUDED
