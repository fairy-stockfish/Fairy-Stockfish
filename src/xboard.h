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

#ifndef XBOARD_H_INCLUDED
#define XBOARD_H_INCLUDED

#include <algorithm>
#include <sstream>
#include <string>

#include "types.h"

namespace Stockfish {

class Position;

namespace XBoard {

/// StateMachine class maintains the states required by XBoard protocol

class StateMachine {
public:
  StateMachine(Position& uciPos, StateListPtr& uciPosStates) : pos(uciPos), states(uciPosStates) {
    moveList = std::deque<Move>();
    moveAfterSearch = false;
    playColor = COLOR_NB;
    ponderMove = MOVE_NONE;
    ponderHighlight = "";
  }
  void go(Search::LimitsType searchLimits, bool ponder = false);
  void ponder();
  void stop(bool abort = true);
  void setboard(std::string fen = "");
  void do_move(Move m);
  void undo_move();
  std::string highlight(std::string square);
  void process_command(std::string token, std::istringstream& is);
  bool moveAfterSearch;
  Move ponderMove;

private:
  Position& pos;
  StateListPtr& states;
  std::deque<Move> moveList;
  Search::LimitsType limits;
  Color playColor;
  std::string ponderHighlight;
};

extern StateMachine* stateMachine;

} // namespace XBoard

} // namespace Stockfish

#endif // #ifndef XBOARD_H_INCLUDED
