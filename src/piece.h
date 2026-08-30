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
  // Bent riders. The key is the direction of the *first* leg; the ride then
  // continues, without limit, in the two directions obtained by rotating that
  // first leg by +/- 45 degrees, i.e. away from the origin square. Diagonal
  // keys describe a Griffon. The corner square is not a destination here: a
  // piece that may stop there declares the step separately, so that the
  // Griffon is "F" plus the four bent diagonal legs.
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
