/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2022 Fabian Fichter

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

namespace Stockfish {

namespace {

  const Search::LimitsType analysisLimits = []{
    Search::LimitsType limits;
    limits.infinite = 1;
    return limits;
  }();

} // namespace

namespace XBoard {

  StateMachine* stateMachine = nullptr;

  // go() starts the search for game play, analysis, or perft.

  void StateMachine::go(Search::LimitsType searchLimits, bool ponder) {

    searchLimits.startTime = now(); // As early as possible!

    Threads.start_thinking(pos, states, searchLimits, ponder);
  }

  // ponder() starts a ponder search

  void StateMachine::ponder() {

    sync_cout << "Hint: " << UCI::move(pos, ponderMove) << sync_endl;
    ponderHighlight = highlight(UCI::square(pos, from_sq(ponderMove)));
    do_move(ponderMove);
    ponderMove = MOVE_NONE;
    go(limits, true);
  }

  // stop() stops an ongoing search (if any)
  // and does not print/apply a move if aborted

  void StateMachine::stop(bool abort) {

    if (abort)
        Threads.abort = true;
    Threads.stop = true;
    Threads.main()->wait_for_search_finished();
    // Ensure that current position does not get out of sync with GUI
    if (Threads.main()->ponder)
    {
        assert(moveList.size());
        undo_move();
        Threads.main()->ponder = false;
    }
  }

  // setboard() is called when engine receives the "setboard" XBoard command.

  void StateMachine::setboard(std::string fen) {

    if (fen.empty())
        fen = variants.find(Options["UCI_Variant"])->second->startFen;

    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one
    moveList.clear();
    pos.set(variants.find(Options["UCI_Variant"])->second, fen, Options["UCI_Chess960"], &states->back(), Threads.main());
  }

  // do_move() is called when engine needs to apply a move when using XBoard protocol.

  void StateMachine::do_move(Move m) {

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

  void StateMachine::undo_move() {

    // transfer states back
    if (Threads.setupStates.get())
        states = std::move(Threads.setupStates);

    pos.undo_move(moveList.back());
    states->pop_back();
    moveList.pop_back();
  }

  std::string StateMachine::highlight(std::string square) {
    Bitboard promotions = 0, captures = 0, quiets = 0;
    // Collect targets
    for (const auto& m : MoveList<LEGAL>(pos))
    {
        Square from = from_sq(m), to = to_sq(m);
        if (is_ok(from) && UCI::square(pos, from) == square && !is_pass(m))
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
    return ss.str();
  }

  // rules() prints the rules of the given variant.

  void rules(std::istringstream& is) {

    std::string token;
    if (is >> token && variants.find(token) != variants.end())
        sync_cout << variants.find(token) << sync_endl;
    else
        sync_cout << "Error (unkown command): " << token << sync_endl;
  }

/// StateMachine::process_command() processes commands of the XBoard protocol.

void StateMachine::process_command(std::string token, std::istringstream& is) {
  if (token == "protover")
  {
      std::string vars = "chess";
      for (std::string v : variants.get_keys())
          if (v != "chess")
              vars += "," + v;
      sync_cout << "feature setboard=1 usermove=1 time=1 memory=1 smp=1 colors=0 draw=0 "
                << "highlight=1 name=0 sigint=0 ping=1 myname=\""
                << engine_info(false, true) << "\" " << "variants=\"" << vars << "\""
                << Options << sync_endl;
      sync_cout << "feature done=1" << sync_endl;
  }
  else if (token == "accepted" || token == "rejected") {}
  else if (token == "hover" || token == "put") {}
  else if (token == "lift")
  {
      if (is >> token)
      {
          if (Threads.main()->ponder)
          {
              if (token == UCI::square(pos, from_sq(moveList.back())))
                  sync_cout << "highlight " << ponderHighlight << sync_endl;
              else
              {
                  Move currentPonderMove = moveList.back();
                  stop();
                  sync_cout << "highlight " << highlight(token) << sync_endl;
                  // Restart ponder search with random guess
                  auto moves = MoveList<LEGAL>(pos);
                  std::vector<Move> filteredMoves;
                  copy_if(moves.begin(), moves.end(), back_inserter(filteredMoves), [&](const Move m) {
                    return is_ok(from_sq(m)) && UCI::square(pos, from_sq(m)) == token;
                  });
                  if (filteredMoves.size())
                  {
                      static PRNG rng(now());
                      ponderMove = filteredMoves.at(rng.rand<unsigned>() % filteredMoves.size());
                  }
                  else
                      ponderMove = currentPonderMove;
                  ponder();
              }
          }
          else
              sync_cout << "highlight " << highlight(token) << sync_endl;
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
      stop();
      Search::clear();
      setboard();
      // play second by default
      playColor = ~pos.side_to_move();
      Threads.sit = false;
      Partner.reset();
  }
  else if (token == "variant")
  {
      stop();
      if (is >> token)
          Options["UCI_Variant"] = token;
      setboard();
  }
  else if (token == "force" || token == "result")
  {
      stop();
      playColor = COLOR_NB;
  }
  else if (token == "?")
  {
      if (!Threads.main()->ponder)
          stop(false);
  }
  else if (token == "go")
  {
      stop();
      playColor = pos.side_to_move();
      go(limits);
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
          limits.time[WHITE] = limits.time[BLACK] = 0;
      }
      // Note: time/otim are in centi-, not milliseconds
      else if (token == "time")
      {
          is >> num;
          Color us = playColor != COLOR_NB ? playColor : pos.side_to_move();
          if (limits.time[us])
              limits.time[us] = num * 10;
      }
      else if (token == "otim")
      {
          is >> num;
          Color them = playColor != COLOR_NB ? ~playColor : ~pos.side_to_move();
          if (limits.time[them])
              limits.time[them] = num * 10;
      }
  }
  else if (token == "setboard")
  {
      stop();
      std::string fen;
      std::getline(is >> std::ws, fen);
      // Check if setboard actually indicates a passing move
      // to avoid unnecessarily clearing the move history
      if (pos.pass(~pos.side_to_move()))
      {
          StateInfo st;
          Position p;
          p.set(pos.variant(), fen, pos.is_chess960(), &st, pos.this_thread());
          Move m;
          std::string passMove = "@@@@";
          if ((m = UCI::to_move(pos, passMove)) != MOVE_NONE)
              do_move(m);
          // apply setboard if passing does not lead to a match
          if (pos.key() != p.key())
              setboard(fen);
      }
      else
          setboard(fen);
      // Winboard sends setboard after passing moves
      if (Options["UCI_AnalyseMode"])
          go(analysisLimits);
      else if (pos.side_to_move() == playColor)
      {
          go(limits);
          moveAfterSearch = true;
      }
  }
  else if (token == "cores")
  {
      stop();
      if (is >> token)
          Options["Threads"] = token;
  }
  else if (token == "memory")
  {
      stop();
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
      stop();
      Options["UCI_AnalyseMode"] = std::string("true");
      go(analysisLimits);
  }
  else if (token == "exit")
  {
      stop();
      Options["UCI_AnalyseMode"] = std::string("false");
  }
  else if (token == "undo")
  {
      stop();
      if (moveList.size())
      {
          undo_move();
          if (Options["UCI_AnalyseMode"])
              go(analysisLimits);
      }
  }
  else if (token == "remove")
  {
      stop();
      if (moveList.size())
      {
          undo_move();
          undo_move();
          if (Options["UCI_AnalyseMode"])
              go(analysisLimits);
      }
  }
  // Bughouse commands
  else if (token == "partner")
      Partner.parse_partner(is);
  else if (token == "ptell")
  {
      Partner.parse_ptell(is, pos);
      // play move requested by partner
      // Partner.moveRequested can only be set if search was successfully aborted
      if (moveAfterSearch && Partner.moveRequested)
      {
          assert(Threads.abort);
          stop();
          sync_cout << "move " << UCI::move(pos, Partner.moveRequested) << sync_endl;
          do_move(Partner.moveRequested);
          moveAfterSearch = false;
          Partner.moveRequested = MOVE_NONE;
      }
  }
  else if (token == "holding")
  {
      stop();
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
          setboard(fen);
      }
      // restart search
      if (moveAfterSearch)
          go(limits);
  }
  // Additional custom non-XBoard commands
  else if (token == "perft")
  {
      stop();
      Search::LimitsType perft_limits;
      is >> perft_limits.perft;
      go(perft_limits);
  }
  else if (token == "d")
      sync_cout << pos << sync_endl;
  else if (token == "eval")
      sync_cout << Eval::trace(pos) << sync_endl;
  else if (token == "rules")
      rules(is);
  // Move strings and unknown commands
  else
  {
      bool isMove = false;

      if (token == "usermove")
      {
          is >> token;
          isMove = true;
      }

      // Handle pondering
      if (Threads.main()->ponder)
      {
          assert(moveList.size());
          if (token == UCI::move(pos, moveList.back()))
          {
              // ponderhit
              moveAfterSearch = true;
              Threads.main()->ponder = false;
              return;
          }
      }
      stop(false);

      // Apply move
      Move m;
      if ((m = UCI::to_move(pos, token)) != MOVE_NONE)
          do_move(m);
      else
          sync_cout << (isMove ? "Illegal move: " : "Error (unknown command): ") << token << sync_endl;

      // Restart search if applicable
      if (Options["UCI_AnalyseMode"])
          go(analysisLimits);
      else if (pos.side_to_move() == playColor)
      {
          moveAfterSearch = true;
          go(limits);
      }
  }
}

} // namespace XBoard

} // namespace Stockfish
