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
    fast = sitRequested = partnerDead = weDead = false;
    time = opptime = 0;
}

template <PartnerType p>
void PartnerHandler::ptell(const std::string& message) {
    if (p == ALL_PARTNERS || (p == FAIRY && isFairy) || (p == HUMAN && !isFairy))
        sync_cout << "tellics ptell " << message << sync_endl;
}

void PartnerHandler::parse_partner(std::istringstream& is) {
    std::string token;
    if (is >> token)
        // handshake to identify Fairy-Stockfish
        ptell("partner Fairy-Stockfish is an engine. Ask it 'help' for supported commands.");
    else
        isFairy = false;
}

void PartnerHandler::parse_ptell(std::istringstream& is, const Position& pos) {
    std::string token;
    is >> token;
    if (token == "partner")
    {
        // handshake to identify Fairy-Stockfish
        if (is >> token && token == "Fairy-Stockfish")
            isFairy = true;
    }
    else if (token == "help")
    {
        if (!(is >> token))
        {
            ptell<HUMAN>("I listen to the commands help, sit, go, move, fast, slow, dead, x, time, and otim.");
            ptell<HUMAN>("Tell 'help sit', etc. for details.");
        }
        else if (token == "sit")
            ptell<HUMAN>("After receiving 'sit', I stop moving. Also see 'go'.");
        else if (token == "go")
            ptell<HUMAN>("After receiving 'go', I will no longer sit.");
        else if (token == "move")
        {
            ptell<HUMAN>("After receiving 'move', I will move immediately." );
            ptell<HUMAN>("If you specify a valid move, e.g., 'move e2e4', I will play it.");
        }
        else if (token == "fast")
            ptell<HUMAN>("After receiving 'go', I will play fast.");
        else if (token == "slow")
            ptell<HUMAN>("After receiving 'slow', I will play at normal speed.");
        else if (token == "dead")
            ptell<HUMAN>("After receiving 'dead', I assume you are dead and I play fast.");
        else if (token == "x")
            ptell<HUMAN>("After receiving 'x', I assume I can play normally again.");
        else if (token == "time")
        {
            ptell<HUMAN>("'time' together with your time in centiseconds allows me to consider your time.");
            ptell<HUMAN>("E.g., 'time 1000' for 10 seconds.");
        }
        else if (token == "otim")
            ptell<HUMAN>("'otim' together with your opponent's time in centiseconds allows me to consider his time.");
    }
    else if (!pos.two_boards())
        return;
    else if (token == "sit")
    {
        sitRequested = true;
        ptell<HUMAN>("I sit, tell me 'go' to continue");
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
                ptell<HUMAN>("sorry, not possible");
        }
        else
            Threads.stop = true;
    }
    else if (token == "fast")
    {
        fast = true;
        ptell<HUMAN>("I play fast, tell me 'slow' to play normally again");
    }
    else if (token == "slow")
    {
        fast = false;
        ptell<HUMAN>("I play at normal speed again.");
    }
    else if (token == "dead")
    {
        partnerDead = true;
        ptell<HUMAN>("I play fast, tell me 'x' if you are no longer dead.");
    }
    else if (token == "x")
    {
        partnerDead = false;
        ptell<HUMAN>("I play normally again");
    }
    else if (token == "time")
    {
        int value;
        time = (is >> value) ? value : 0;
    }
    else if (token == "otim")
    {
        int value;
        opptime = (is >> value) ? value : 0;
    }
}

template void PartnerHandler::ptell<HUMAN>(const std::string&);
template void PartnerHandler::ptell<FAIRY>(const std::string&);
template void PartnerHandler::ptell<ALL_PARTNERS>(const std::string&);
