/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2019 Fabian Fichter

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

#ifndef XBOARD_H_INCLUDED
#define XBOARD_H_INCLUDED

#include <sstream>
#include <string>

#include "types.h"

class Position;

namespace XBoard {

/// StateMachine class maintains the states required by XBoard protocol

class StateMachine {
public:
  StateMachine() {
    moveList = std::deque<Move>();
    moveAfterSearch = false;
    playColor = COLOR_NB;
  }
  void process_command(Position& pos, std::string token, std::istringstream& is, StateListPtr& states);

private:
  std::deque<Move> moveList;
  Search::LimitsType limits;
  bool moveAfterSearch;
  Color playColor;
};

} // namespace XBoard

#endif // #ifndef XBOARD_H_INCLUDED
