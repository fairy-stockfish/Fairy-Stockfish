/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2022 The Stockfish developers (see AUTHORS file)

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

#ifndef TIMEMAN_H_INCLUDED
#define TIMEMAN_H_INCLUDED

#include "misc.h"
#include "search.h"
#include "thread.h"

namespace Stockfish {

/// The TimeManagement class computes the optimal time to think depending on
/// the maximum available time, the game move number and other parameters.

class TimeManagement {
public:
  void init(const Position& pos, Search::LimitsType& limits, Color us, int ply);
  TimePoint optimum() const { return optimumTime; }
  TimePoint maximum() const { return maximumTime; }
  TimePoint elapsed() const { return Search::Limits.npmsec ?
                                     TimePoint(Threads.nodes_searched()) : now() - startTime; }

  int64_t availableNodes; // When in 'nodes as time' mode

private:
  TimePoint startTime;
  TimePoint optimumTime;
  TimePoint maximumTime;
};

extern TimeManagement Time;

} // namespace Stockfish

#endif // #ifndef TIMEMAN_H_INCLUDED
