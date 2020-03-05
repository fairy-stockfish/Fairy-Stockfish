/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2020 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include "bitboard.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "endgame.h"
#include "piece.h"
#include "variant.h"
#include "syzygy/tbprobe.h"

namespace PSQT {
  void init(const Variant* v);
}

int main(int argc, char* argv[]) {

  std::cout << engine_info() << std::endl;

  pieceMap.init();
  std::cout << "piece map initialized" << std::endl;
  variants.init();
  std::cout << "variants initialized" << std::endl;
  UCI::init(Options);
  std::cout << "uci initialized" << std::endl;
  PSQT::init(variants.find(Options["UCI_Variant"])->second);
  std::cout << "psqt initialized" << std::endl;
  Bitboards::init();
  std::cout << "bitboard initialized" << std::endl;
  Position::init();
  std::cout << "position initialized" << std::endl;
  Bitbases::init();
  std::cout << "bitbases initialized" << std::endl;
  Endgames::init();
  std::cout << "endgames initialized" << std::endl;
  /*Threads.set(Options["Threads"]);
  std::cout << "threads initialized" << std::endl;
  Search::clear(); // After threads are up
  std::cout << "search cleared" << std::endl;*/

  /*UCI::loop(argc, argv);

  Threads.set(0);*/
  variants.clear_all();
  std::cout << "variants cleard" << std::endl;
  pieceMap.clear_all();
  std::cout << "piece map cleared" << std::endl;
  return 0;
}
