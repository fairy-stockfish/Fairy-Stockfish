/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2020 Fabian Fichter

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

#include <iostream>
#include <sstream>
#include <string>

#include "partner.h"
#include "thread.h"
#include "uci.h"

PartnerHandler Partner; // Global object

void PartnerHandler::reset() {
    sitRequested = partnerDead = weDead = false;
}

void PartnerHandler::ptell(const std::string& message) {
    sync_cout << "tellics ptell " << message << sync_endl;
}

void PartnerHandler::parse_partner(std::istringstream& is) {
    std::string token;
    if (is >> token)
        ptell("partner Fairy-Stockfish is an engine. Ask it 'help' for supported commands.");
    else
        isFairy = false;
}

void PartnerHandler::parse_ptell(std::istringstream& is, const Position& pos) {
    std::string token;
    is >> token;
    if (token == "partner")
    {
        if (is >> token && token == "Fairy-Stockfish")
            isFairy = true;
    }
    else if (token == "help")
    {
        if (!(is >> token))
            ptell("I listen to the commands help, sit, go, and move. Ptell 'help sit', etc. for details.");
        else if (token == "sit")
            ptell("After receiving 'sit', I stop moving. Also see 'go'.");
        else if (token == "go")
            ptell("After receiving 'go', I will no longer sit.");
        else if (token == "move")
        {
            ptell("After receiving 'move', I will move immediately." );
            ptell("If you specify a valid move, e.g., 'move e2e4', I will play it.");
        }
    }
    else if (!pos.two_boards())
        return;
    else if (token == "sit")
    {
        sitRequested = true;
        ptell("I sit, tell me 'go' to continue");
    }
    else if (token == "go")
    {
        sitRequested = false;
        Threads.stop = true;
    }
    else if (token == "dead")
    {
        partnerDead = true;
        ptell("I will play fast");
    }
    else if (token == "x")
    {
        partnerDead = false;
        ptell("I will play normally again");
    }
    else if (token == "move")
    {
        if (is >> token)
        {
            // if the given move is valid and we can still abort the search, play it
            Move move = UCI::to_move(pos, token);
            if (move && !Threads.abort.exchange(true))
                moveRequested = move;
            else
                ptell("sorry, not possible");
        }
        else
            Threads.stop = true;
    }
}
