/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2022 Fabian Fichter

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

#ifndef PIECE_H_INCLUDED
#define PIECE_H_INCLUDED

#include <string>
#include <map>

#include "types.h"
#include "variant.h"

namespace Stockfish {

enum MoveModality {MODALITY_QUIET, MODALITY_CAPTURE, MOVE_MODALITY_NB};

/// PieceInfo struct stores information about the piece movements.

struct PieceInfo {
  std::string name = "";
  std::string betza = "";
  std::map<Direction, int> steps[2][MOVE_MODALITY_NB] = {};
  std::map<Direction, int> slider[2][MOVE_MODALITY_NB] = {};
  std::map<Direction, int> hopper[2][MOVE_MODALITY_NB] = {};
  // Bent riders. The key is the direction of the *first* leg. The value is a
  // mask of which continuations are taken: 1 for the direction 45 degrees
  // clockwise from the first leg, 2 for 45 degrees counter-clockwise, 3 for
  // both. So {NE:3, SE:3, SW:3, NW:3} is a Griffon, {N:3, E:3, S:3, W:3} a
  // Rhino, {N:3, S:3} the half-Rhino known as the Snake, and the four
  // diagonals each keeping only their file-parallel continuation the
  // half-Griffon known as the Ship.
  // The corner square is not a destination here; the parser records it as an
  // ordinary step alongside, so a Griffon is "F" plus the four bent legs.
  std::map<Direction, int> bent[2][MOVE_MODALITY_NB] = {};
};

struct PieceMap : public std::map<PieceType, const PieceInfo*> {
  void init(const Variant* v = nullptr);
  void add(PieceType pt, const PieceInfo* v);
  void clear_all();
};

extern PieceMap pieceMap;

inline std::string piece_name(PieceType pt) {
  return is_custom(pt) ? "customPiece" + std::to_string(pt - CUSTOM_PIECES + 1)
                       : pieceMap.find(pt)->second->name;
}

} // namespace Stockfish

#endif // #ifndef PIECE_H_INCLUDED
