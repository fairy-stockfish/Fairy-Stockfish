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

#ifndef PARTNER_H_INCLUDED
#define PARTNER_H_INCLUDED

#include <atomic>
#include <sstream>

#include "misc.h"
#include "position.h"

/// PartnerHandler manages the communication with the partner
/// in games played on two boards, such as bughouse.

enum PartnerType {
  HUMAN,
  FAIRY,
  ALL_PARTNERS
};

struct PartnerHandler {
    void reset();
    template <PartnerType p = ALL_PARTNERS>
    void ptell(const std::string& message);
    void parse_partner(std::istringstream& is);
    void parse_ptell(std::istringstream& is, const Position& pos);

    std::atomic<bool> isFairy;
    std::atomic<bool> fast, sitRequested, partnerDead, weDead, weWin, weVirtualWin, weVirtualLoss;
    std::atomic<TimePoint> time, opptime;
    Move moveRequested;
};

extern PartnerHandler Partner;

#endif // #ifndef PARTNER_H_INCLUDED