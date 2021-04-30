/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2021 Fabian Fichter

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


/// PieceInfo struct stores information about the piece movements.

struct PieceInfo {
  std::string name = "";
  std::string betza = "";
  std::map<Direction, int> stepsQuiet = {};
  std::map<Direction, int> stepsCapture = {};
  std::map<Direction, int> sliderQuiet = {};
  std::map<Direction, int> sliderCapture = {};
  std::map<Direction, int> hopperQuiet = {};
  std::map<Direction, int> hopperCapture = {};
};

struct PieceMap : public std::map<PieceType, const PieceInfo*> {
  void init(const Variant* v = nullptr);
  void add(PieceType pt, const PieceInfo* v);
  void clear_all();
};

extern PieceMap pieceMap;

#endif // #ifndef PIECE_H_INCLUDED
