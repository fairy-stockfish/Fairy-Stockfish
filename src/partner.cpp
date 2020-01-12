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

#include <iostream>
#include <sstream>
#include <string>

#include "partner.h"
#include "thread.h"
#include "uci.h"

PartnerHandler Partner; // Global object

void PartnerHandler::parse_partner(std::istringstream& is) {
    std::string token;
    if (is >> token)
        sync_cout << "tellics ptell partner Fairy-Stockfish is an engine. Ask it 'help' for supported commands." << sync_endl;
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
            sync_cout << "tellics ptell I listen to the commands help, sit, go, and move. Ptell 'help sit', etc. for details." << sync_endl;
        else if (token == "sit")
            sync_cout << "tellics ptell After receiving 'sit', I stop moving. Also see 'go'." << sync_endl;
        else if (token == "go")
            sync_cout << "tellics ptell After receiving 'go', I will no longer sit." << sync_endl;
        else if (token == "move")
        {
            sync_cout << "tellics ptell After receiving 'move', I will move immediately."  << sync_endl;
            sync_cout << "tellics ptell If you specify a valid move, e.g., 'move e2e4', I will play it." << sync_endl;
        }
    }
    else if (!pos.two_boards())
        return;
    else if (token == "sit")
    {
        sitRequested = true;
        sync_cout << "tellics ptell I sit, tell me 'go' to continue" << sync_endl;
    }
    else if (token == "go")
    {
        sitRequested = false;
        Threads.stop = true;
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
                sync_cout << "tellics ptell sorry, not possible" << sync_endl;
        }
        else
            Threads.stop = true;
    }
}
