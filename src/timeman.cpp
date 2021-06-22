/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

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

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "partner.h"
#include "search.h"
#include "timeman.h"
#include "uci.h"

namespace Stockfish {

TimeManagement Time; // Our global time management object


/// TimeManagement::init() is called at the beginning of the search and calculates
/// the bounds of time allowed for the current game ply. We currently support:
//      1) x basetime (+ z increment)
//      2) x moves in y seconds (+ z increment)

void TimeManagement::init(const Position& pos, Search::LimitsType& limits, Color us, int ply) {

  TimePoint moveOverhead    = TimePoint(Options["Move Overhead"]);
  TimePoint slowMover       = TimePoint(Options["Slow Mover"]);
  TimePoint npmsec          = TimePoint(Options["nodestime"]);

  // optScale is a percentage of available time to use for the current move.
  // maxScale is a multiplier applied to optimumTime.
  double optScale, maxScale;

  // If we have to play in 'nodes as time' mode, then convert from time
  // to nodes, and use resulting values in time management formulas.
  // WARNING: to avoid time losses, the given npmsec (nodes per millisecond)
  // must be much lower than the real engine speed.
  if (npmsec)
  {
      if (!availableNodes) // Only once at game start
          availableNodes = npmsec * limits.time[us]; // Time is in msec

      // Convert from milliseconds to nodes
      limits.time[us] = TimePoint(availableNodes);
      limits.inc[us] *= npmsec;
      limits.npmsec = npmsec;
  }

  startTime = limits.startTime;

  // Maximum move horizon of 50 moves
  int mtg = limits.movestogo ? std::min(limits.movestogo, 50) : 50;

  // Make sure timeLeft is > 0 since we may use it as a divisor
  TimePoint timeLeft =  std::max(TimePoint(1),
      limits.time[us] + limits.inc[us] * (mtg - 1) - moveOverhead * (2 + mtg));

  // Adjust time management for four-player variants
  if (pos.two_boards())
  {
      if (Partner.partnerDead && Partner.opptime)
          timeLeft -= Partner.opptime;
      else
      {
          timeLeft = std::min(timeLeft, 5000 + std::min(std::abs(limits.time[us] - Partner.opptime), TimePoint(Partner.opptime)));
          if (Partner.fast || Partner.partnerDead)
              timeLeft /= 4;
      }
  }

  // A user may scale time usage by setting UCI option "Slow Mover"
  // Default is 100 and changing this value will probably lose elo.
  timeLeft = slowMover * timeLeft / 100;

  // x basetime (+ z increment)
  // If there is a healthy increment, timeLeft can exceed actual available
  // game time for the current move, so also cap to 20% of available game time.
  if (limits.movestogo == 0)
  {
      optScale = std::min(0.0084 + std::pow(ply + 3.0, 0.5) * 0.0042,
                           0.2 * limits.time[us] / double(timeLeft));
      maxScale = std::min(7.0, 4.0 + ply / 12.0);
  }

  // x moves in y seconds (+ z increment)
  else
  {
      optScale = std::min((0.8 + ply / 128.0) / mtg,
                            0.8 * limits.time[us] / double(timeLeft));
      maxScale = std::min(6.3, 1.5 + 0.11 * mtg);
  }

  // Never use more than 80% of the available time for this move
  optimumTime = TimePoint(optScale * timeLeft);
  maximumTime = TimePoint(std::min(0.8 * limits.time[us] - moveOverhead, maxScale * optimumTime));

  if (Options["Ponder"])
      optimumTime += optimumTime / 4;
}

} // namespace Stockfish
