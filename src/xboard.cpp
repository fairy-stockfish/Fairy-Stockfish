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
#include <string>

#include "evaluate.h"
#include "misc.h"
#include "partner.h"
#include "search.h"
#include "thread.h"
#include "types.h"
#include "uci.h"
#include "xboard.h"

namespace {

  const Search::LimitsType analysisLimits = []{
    Search::LimitsType limits;
    limits.infinite = 1;
    return limits;
  }();

  // go() starts the search for game play, analysis, or perft.

  void go(Position& pos, Search::LimitsType limits, StateListPtr& states) {

    limits.startTime = now(); // As early as possible!

    Threads.start_thinking(pos, states, limits, false);
  }

  // setboard() is called when engine receives the "setboard" XBoard command.

  void setboard(Position& pos, StateListPtr& states, std::string fen = "") {

    if (fen.empty())
        fen = variants.find(Options["UCI_Variant"])->second->startFen;

    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one
    pos.set(variants.find(Options["UCI_Variant"])->second, fen, Options["UCI_Chess960"], &states->back(), Threads.main());
  }

  // do_move() is called when engine needs to apply a move when using XBoard protocol.

  void do_move(Position& pos, std::deque<Move>& moveList, StateListPtr& states, Move m) {

    // transfer states back
    if (Threads.setupStates.get())
        states = std::move(Threads.setupStates);

    if (m == MOVE_NONE)
        return;
    moveList.push_back(m);
    states->emplace_back();
    pos.do_move(m, states->back());
  }

  // undo_move() is called when the engine receives the undo command in XBoard protocol.

  void undo_move(Position& pos, std::deque<Move>& moveList, StateListPtr& states) {

    // transfer states back
    if (Threads.setupStates.get())
        states = std::move(Threads.setupStates);

    pos.undo_move(moveList.back());
    states->pop_back();
    moveList.pop_back();
  }

} // namespace

namespace XBoard {

/// StateMachine::process_command() processes commands of the XBoard protocol.

void StateMachine::process_command(Position& pos, std::string token, std::istringstream& is, StateListPtr& states) {
  if (moveAfterSearch && token != "ptell")
  {
      // abort search in bughouse when receiving "holding" command
      bool doMove = token != "holding" || Threads.abort.exchange(true);
      Threads.stop = true;
      Threads.main()->wait_for_search_finished();
      if (doMove)
      {
          do_move(pos, moveList, states, Threads.main()->bestThread->rootMoves[0].pv[0]);
          moveAfterSearch = false;
      }
  }
  if (token == "protover")
  {
      std::string vars = "chess";
      for (std::string v : variants.get_keys())
          if (v != "chess")
              vars += "," + v;
      sync_cout << "feature setboard=1 usermove=1 time=1 memory=1 smp=1 colors=0 draw=0 "
                << "highlight=1 name=0 sigint=0 ping=1 myname=Fairy-Stockfish myversion=\""
                << engine_version() << "\" " << "variants=\"" << vars << "\""
                << Options << sync_endl;
      sync_cout << "feature done=1" << sync_endl;
  }
  else if (token == "accepted" || token == "rejected" || token == "result" || token == "?") {}
  else if (token == "hover" || token == "put") {}
  else if (token == "lift")
  {
      if (is >> token)
      {
          Bitboard promotions = 0, captures = 0, quiets = 0;
          // Collect targets
          for (const auto& m : MoveList<LEGAL>(pos))
          {
              Square from = from_sq(m), to = to_sq(m);
              if (is_ok(from) && UCI::square(pos, from) == token)
              {
                  if (type_of(m) == PROMOTION)
                      promotions |= to;
                  else if (pos.capture(m))
                      captures |= to;
                  else
                  {
                      if (type_of(m) == CASTLING && !pos.is_chess960())
                          to = make_square(to > from ? pos.castling_kingside_file()
                                                     : pos.castling_queenside_file(), rank_of(from));
                      quiets |= to;
                  }
              }
          }
          // Generate color FEN
          int emptyCnt;
          std::ostringstream ss;
          for (Rank r = pos.max_rank(); r >= RANK_1; --r)
          {
              for (File f = FILE_A; f <= pos.max_file(); ++f)
              {
                  for (emptyCnt = 0; f <= pos.max_file() && !((promotions | captures | quiets) & make_square(f, r)); ++f)
                      ++emptyCnt;

                  if (emptyCnt)
                      ss << emptyCnt;

                  if (f <= pos.max_file())
                      ss << (promotions & make_square(f, r) ? "M" : captures & make_square(f, r) ? "R" : "Y");
              }

              if (r > RANK_1)
                  ss << '/';
          }
          sync_cout << "highlight " << ss.str() << sync_endl;
      }
  }
  else if (token == "ping")
  {
      if (!(is >> token))
          token = "";
      sync_cout << "pong " << token << sync_endl;
  }
  else if (token == "new")
  {
      Search::clear();
      setboard(pos, states);
      // play second by default
      playColor = ~pos.side_to_move();
      Threads.sit = false;
      Partner.reset();
  }
  else if (token == "variant")
  {
      if (is >> token)
          Options["UCI_Variant"] = token;
      setboard(pos, states);
  }
  else if (token == "force")
      playColor = COLOR_NB;
  else if (token == "go")
  {
      playColor = pos.side_to_move();
      go(pos, limits, states);
      moveAfterSearch = true;
  }
  else if (token == "level" || token == "st" || token == "sd" || token == "time" || token == "otim")
  {
      int num;
      if (token == "level")
      {
          // moves to go
          is >> limits.movestogo;
          // base time
          is >> token;
          size_t idx = token.find(":");
          if (idx != std::string::npos)
              num = std::stoi(token.substr(0, idx)) * 60 + std::stoi(token.substr(idx + 1));
          else
              num = std::stoi(token) * 60;
          limits.time[WHITE] = num * 1000;
          limits.time[BLACK] = num * 1000;
          // increment
          is >> num;
          limits.inc[WHITE] = num * 1000;
          limits.inc[BLACK] = num * 1000;
      }
      else if (token == "sd")
          is >> limits.depth;
      else if (token == "st")
      {
          is >> num;
          limits.movetime = num * 1000;
      }
      // Note: time/otim are in centi-, not milliseconds
      else if (token == "time")
      {
          is >> num;
          limits.time[playColor != COLOR_NB ? playColor : pos.side_to_move()] = num * 10;
      }
      else if (token == "otim")
      {
          is >> num;
          limits.time[playColor != COLOR_NB ? ~playColor : ~pos.side_to_move()] = num * 10;
      }
  }
  else if (token == "setboard")
  {
      std::string fen;
      std::getline(is >> std::ws, fen);
      // Check if setboard actually indicates a passing move
      // to avoid unnecessarily clearing the move history
      if (pos.pass())
      {
          StateInfo st;
          Position p;
          p.set(pos.variant(), fen, pos.is_chess960(), &st, pos.this_thread());
          Move m;
          std::string passMove = "pass";
          if ((m = UCI::to_move(pos, passMove)) != MOVE_NONE)
              do_move(pos, moveList, states, m);
          // apply setboard if passing does not lead to a match
          if (pos.key() != p.key())
              setboard(pos, states, fen);
      }
      else
          setboard(pos, states, fen);
      // Winboard sends setboard after passing moves
      if (pos.side_to_move() == playColor)
      {
          go(pos, limits, states);
          moveAfterSearch = true;
      }
  }
  else if (token == "cores")
  {
      if (is >> token)
          Options["Threads"] = token;
  }
  else if (token == "memory")
  {
      if (is >> token)
          Options["Hash"] = token;
  }
  else if (token == "hard" || token == "easy")
      Options["Ponder"] = token == "hard";
  else if (token == "option")
  {
      std::string name, value;
      is.get();
      std::getline(is, name, '=');
      std::getline(is, value);
      if (Options.count(name))
      {
          if (Options[name].get_type() == "check")
              value = value == "1" ? "true" : "false";
          Options[name] = value;
      }
  }
  else if (token == "analyze")
  {
      Options["UCI_AnalyseMode"] = std::string("true");
      go(pos, analysisLimits, states);
  }
  else if (token == "exit")
  {
      Threads.stop = true;
      Threads.main()->wait_for_search_finished();
      Options["UCI_AnalyseMode"] = std::string("false");
  }
  else if (token == "undo")
  {
      if (moveList.size())
      {
          if (Options["UCI_AnalyseMode"])
          {
              Threads.stop = true;
              Threads.main()->wait_for_search_finished();
          }
          undo_move(pos, moveList, states);
          if (Options["UCI_AnalyseMode"])
              go(pos, analysisLimits, states);
      }
  }
  // Bughouse commands
  else if (token == "partner")
      Partner.parse_partner(is);
  else if (token == "ptell")
  {
      Partner.parse_ptell(is, pos);
      // play move requested by partner
      if (moveAfterSearch && Partner.moveRequested)
      {
          Threads.stop = true;
          Threads.main()->wait_for_search_finished();
          sync_cout << "move " << UCI::move(pos, Partner.moveRequested) << sync_endl;
          do_move(pos, moveList, states, Partner.moveRequested);
          moveAfterSearch = false;
          Partner.moveRequested = MOVE_NONE;
      }
  }
  else if (token == "holding")
  {
      // holding [<white>] [<black>] <color><piece>
      std::string white_holdings, black_holdings;
      if (   std::getline(is, token, '[') && std::getline(is, white_holdings, ']')
          && std::getline(is, token, '[') && std::getline(is, black_holdings, ']'))
      {
          std::string fen;
          char color, pieceType;
          // Use the obtained holding if available to avoid race conditions
          if (is >> color && is >> pieceType)
          {
              fen = pos.fen();
              fen.insert(fen.find(']'), 1, toupper(color) == 'B' ? tolower(pieceType) : toupper(pieceType));
          }
          else
          {
              std::transform(black_holdings.begin(), black_holdings.end(), black_holdings.begin(), ::tolower);
              fen = pos.fen(false, false, 0, white_holdings + black_holdings);
          }
          setboard(pos, states, fen);
      }
      // restart search
      if (moveAfterSearch)
          go(pos, limits, states);
  }
  // Additional custom non-XBoard commands
  else if (token == "perft")
  {
      Search::LimitsType perft_limits;
      is >> perft_limits.perft;
      go(pos, perft_limits, states);
  }
  else if (token == "d")
      sync_cout << pos << sync_endl;
  else if (token == "eval")
      sync_cout << Eval::trace(pos) << sync_endl;
  // Move strings and unknown commands
  else
  {
      // process move string
      if (token == "usermove")
          is >> token;
      if (Options["UCI_AnalyseMode"])
      {
          Threads.stop = true;
          Threads.main()->wait_for_search_finished();
      }
      Move m;
      if ((m = UCI::to_move(pos, token)) != MOVE_NONE)
          do_move(pos, moveList, states, m);
      else
          sync_cout << "Error (unkown command): " << token << sync_endl;
      if (Options["UCI_AnalyseMode"])
          go(pos, analysisLimits, states);
      else if (pos.side_to_move() == playColor)
      {
          go(pos, limits, states);
          moveAfterSearch = true;
      }
  }
}

} // namespace XBoard
