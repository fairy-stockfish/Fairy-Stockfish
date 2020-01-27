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

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "evaluate.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "timeman.h"
#include "tt.h"
#include "uci.h"
#include "xboard.h"
#include "syzygy/tbprobe.h"

using namespace std;

extern vector<string> setup_bench(const Position&, istream&);

namespace {

  // position() is called when engine receives the "position" UCI command.
  // The function sets up the position described in the given FEN string ("fen")
  // or the starting position ("startpos") and then makes the moves given in the
  // following move list ("moves").

  void position(Position& pos, istringstream& is, StateListPtr& states) {

    Move m;
    string token, fen;

    is >> token;
    // Parse as SFEN if specified
    bool sfen = token == "sfen";

    if (token == "startpos")
    {
        fen = variants.find(Options["UCI_Variant"])->second->startFen;
        is >> token; // Consume "moves" token if any
    }
    else if (token == "fen" || token == "sfen")
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        return;

    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one
    pos.set(variants.find(Options["UCI_Variant"])->second, fen, Options["UCI_Chess960"], &states->back(), Threads.main(), sfen);

    // Parse move list (if any)
    while (is >> token && (m = UCI::to_move(pos, token)) != MOVE_NONE)
    {
        states->emplace_back();
        pos.do_move(m, states->back());
    }
  }


  // setoption() is called when engine receives the "setoption" UCI command. The
  // function updates the UCI option ("name") to the given value ("value").

  void setoption(istringstream& is) {

    string token, name, value;

    is >> token; // Consume "name" token

    if (Options["Protocol"] == "ucci")
        name = token;
    else
    // Read option name (can contain spaces)
    while (is >> token && token != "value")
        name += (name.empty() ? "" : " ") + token;

    // Read option value (can contain spaces)
    while (is >> token)
        value += (value.empty() ? "" : " ") + token;

    if (Options.count(name))
        Options[name] = value;
    else if (Options["Protocol"] == "ucci" && (std::replace(name.begin(), name.end(), '_', ' '), Options.count(name)))
        Options[name] = value;
    else
        sync_cout << "No such option: " << name << sync_endl;
  }


  // go() is called when engine receives the "go" UCI command. The function sets
  // the thinking time and other parameters from the input string, then starts
  // the search.

  void go(Position& pos, istringstream& is, StateListPtr& states, const std::vector<Move>& banmoves = {}) {

    Search::LimitsType limits;
    string token;
    bool ponderMode = false;

    limits.startTime = now(); // As early as possible!

    limits.banmoves = banmoves;

    while (is >> token)
        if (token == "searchmoves")
            while (is >> token)
                limits.searchmoves.push_back(UCI::to_move(pos, token));

        else if (token == "wtime")     is >> limits.time[WHITE];
        else if (token == "btime")     is >> limits.time[BLACK];
        else if (token == "winc")      is >> limits.inc[WHITE];
        else if (token == "binc")      is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth")     is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;
        else if (token == "mate")      is >> limits.mate;
        else if (token == "perft")     is >> limits.perft;
        else if (token == "infinite")  limits.infinite = 1;
        else if (token == "ponder")    ponderMode = true;
        // UCCI commands
        else if (token == "time")      is >> limits.time[pos.side_to_move()];
        else if (token == "opptime")   is >> limits.time[~pos.side_to_move()];
        else if (token == "increment") is >> limits.inc[pos.side_to_move()];
        else if (token == "oppinc")    is >> limits.inc[~pos.side_to_move()];

    Threads.start_thinking(pos, states, limits, ponderMode);
  }

  // bench() is called when engine receives the "bench" command. Firstly
  // a list of UCI commands is setup according to bench parameters, then
  // it is run one by one printing a summary at the end.

  void bench(Position& pos, istream& args, StateListPtr& states) {

    string token;
    uint64_t num, nodes = 0, cnt = 1;

    vector<string> list = setup_bench(pos, args);
    num = count_if(list.begin(), list.end(), [](string s) { return s.find("go ") == 0 || s.find("eval") == 0; });

    TimePoint elapsed = now();

    for (const auto& cmd : list)
    {
        istringstream is(cmd);
        is >> skipws >> token;

        if (token == "go" || token == "eval")
        {
            cerr << "\nPosition: " << cnt++ << '/' << num << endl;
            if (token == "go")
            {
               go(pos, is, states);
               Threads.main()->wait_for_search_finished();
               nodes += Threads.nodes_searched();
            }
            else
               sync_cout << "\n" << Eval::trace(pos) << sync_endl;
        }
        else if (token == "setoption")  setoption(is);
        else if (token == "position")   position(pos, is, states);
        else if (token == "ucinewgame") { Search::clear(); elapsed = now(); } // Search::clear() may take some while
    }

    elapsed = now() - elapsed + 1; // Ensure positivity to avoid a 'divide by zero'

    dbg_print(); // Just before exiting

    cerr << "\n==========================="
         << "\nTotal time (ms) : " << elapsed
         << "\nNodes searched  : " << nodes
         << "\nNodes/second    : " << 1000 * nodes / elapsed << endl;
  }

  // load() is called when engine receives the "load" command.
  // The function reads variant configuration files.

  void load(istringstream& is) {

    string token;
    while (is >> token)
        Options["VariantPath"] = token;
  }

} // namespace


/// UCI::loop() waits for a command from stdin, parses it and calls the appropriate
/// function. Also intercepts EOF from stdin to ensure gracefully exiting if the
/// GUI dies unexpectedly. When called with some command line arguments, e.g. to
/// run 'bench', once the command is executed the function returns immediately.
/// In addition to the UCI ones, also some additional debug commands are supported.

void UCI::loop(int argc, char* argv[]) {

  Position pos;
  string token, cmd;
  StateListPtr states(new std::deque<StateInfo>(1));

  assert(variants.find(Options["UCI_Variant"])->second != nullptr);
  pos.set(variants.find(Options["UCI_Variant"])->second, variants.find(Options["UCI_Variant"])->second->startFen, false, &states->back(), Threads.main());

  for (int i = 1; i < argc; ++i)
      cmd += std::string(argv[i]) + " ";

  // XBoard state machine
  XBoard::StateMachine xboardStateMachine;
  // UCCI banmoves state
  std::vector<Move> banmoves = {};

  do {
      if (argc == 1 && !getline(cin, cmd)) // Block here waiting for input or EOF
          cmd = "quit";

      istringstream is(cmd);

      token.clear(); // Avoid a stale if getline() returns empty or blank line
      is >> skipws >> token;

      if (    token == "quit"
          ||  token == "stop")
          Threads.stop = true;

      // The GUI sends 'ponderhit' to tell us the user has played the expected move.
      // So 'ponderhit' will be sent if we were told to ponder on the same move the
      // user has played. We should continue searching but switch from pondering to
      // normal search.
      else if (token == "ponderhit")
          Threads.main()->ponder = false; // Switch to normal search

      else if (token == "uci" || token == "usi" || token == "ucci" || token == "xboard")
      {
          Options["Protocol"].set_default(token);
          string defaultVariant = string(  token == "usi"  ? "shogi"
                                         : token == "ucci" ? "xiangqi"
                                                           : "chess");
          Options["UCI_Variant"].set_default(defaultVariant);
          if (token != "xboard")
              sync_cout << "id name " << engine_info(true)
                          << "\n" << Options
                          << "\n" << token << "ok"  << sync_endl;
      }

      else if (Options["Protocol"] == "xboard")
          xboardStateMachine.process_command(pos, token, is, states);

      else if (token == "setoption")  setoption(is);
      // UCCI-specific banmoves command
      else if (token == "banmoves")
          while (is >> token)
              banmoves.push_back(UCI::to_move(pos, token));
      else if (token == "go")         go(pos, is, states, banmoves);
      else if (token == "position")   position(pos, is, states), banmoves.clear();
      else if (token == "ucinewgame" || token == "usinewgame" || token == "uccinewgame") Search::clear();
      else if (token == "isready")    sync_cout << "readyok" << sync_endl;

      // Additional custom non-UCI commands, mainly for debugging.
      // Do not use these commands during a search!
      else if (token == "flip")     pos.flip();
      else if (token == "bench")    bench(pos, is, states);
      else if (token == "d")        sync_cout << pos << sync_endl;
      else if (token == "eval")     sync_cout << Eval::trace(pos) << sync_endl;
      else if (token == "compiler") sync_cout << compiler_info() << sync_endl;
      else if (token == "load")     { load(is); argc = 1; } // continue reading stdin
      else
          sync_cout << "Unknown command: " << cmd << sync_endl;

  } while (token != "quit" && argc == 1); // Command line args are one-shot
}


/// UCI::value() converts a Value to a string suitable for use with the UCI
/// protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in y moves, not plies. If the engine is getting mated
///           use negative values for y.

string UCI::value(Value v) {

  assert(-VALUE_INFINITE < v && v < VALUE_INFINITE);

  stringstream ss;

  if (Options["Protocol"] == "xboard")
  {
      if (abs(v) < VALUE_MATE - MAX_PLY)
          ss << v * 100 / PawnValueEg;
      else
          ss << (v > 0 ? XBOARD_VALUE_MATE + VALUE_MATE - v + 1 : -XBOARD_VALUE_MATE - VALUE_MATE - v - 1) / 2;
  } else

  if (abs(v) < VALUE_MATE - MAX_PLY)
      ss << "cp " << v * 100 / PawnValueEg;
  else if (Options["Protocol"] == "usi")
      // In USI, mate distance is given in ply
      ss << "mate " << (v > 0 ? VALUE_MATE - v : -VALUE_MATE - v);
  else
      ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v - 1) / 2;

  return ss.str();
}


/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

std::string UCI::square(const Position& pos, Square s) {
#ifdef LARGEBOARDS
  if (Options["Protocol"] == "usi")
      return rank_of(s) < RANK_10 ? std::string{ char('1' + pos.max_file() - file_of(s)), char('a' + pos.max_rank() - rank_of(s)) }
                                  : std::string{ char('0' + (pos.max_file() - file_of(s) + 1) / 10),
                                                 char('0' + (pos.max_file() - file_of(s) + 1) % 10),
                                                 char('a' + pos.max_rank() - rank_of(s)) };
  else if ((Options["Protocol"] == "xboard" || Options["Protocol"] == "ucci") && pos.max_rank() == RANK_10)
      return std::string{ char('a' + file_of(s)), char('0' + rank_of(s)) };
  else
      return rank_of(s) < RANK_10 ? std::string{ char('a' + file_of(s)), char('1' + (rank_of(s) % 10)) }
                                  : std::string{ char('a' + file_of(s)), char('0' + ((rank_of(s) + 1) / 10)),
                                                 char('0' + ((rank_of(s) + 1) % 10)) };
#else
  return Options["Protocol"] == "usi" ? std::string{ char('1' + pos.max_file() - file_of(s)), char('a' + pos.max_rank() - rank_of(s)) }
                                      : std::string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
#endif
}

/// UCI::dropped_piece() generates a piece label string from a Move.

string UCI::dropped_piece(const Position& pos, Move m) {
  assert(type_of(m) == DROP);
  if (dropped_piece_type(m) == pos.promoted_piece_type(in_hand_piece_type(m)))
      // Dropping as promoted piece
      return std::string{'+', pos.piece_to_char()[in_hand_piece_type(m)]};
  else
      return std::string{pos.piece_to_char()[dropped_piece_type(m)]};
}


/// UCI::move() converts a Move to a string in coordinate notation (g1f3, a7a8q).
/// The only special case is castling, where we print in the e1g1 notation in
/// normal chess mode, and in e1h1 notation in chess960 mode. Internally all
/// castling moves are always encoded as 'king captures rook'.

string UCI::move(const Position& pos, Move m) {

  Square from = from_sq(m);
  Square to = to_sq(m);

  if (m == MOVE_NONE)
      return "(none)";

  if (m == MOVE_NULL)
      return "0000";

  if (is_gating(m) && gating_square(m) == to)
      from = to_sq(m), to = from_sq(m);
  else if (type_of(m) == CASTLING && !pos.is_chess960())
      to = make_square(to > from ? pos.castling_kingside_file() : pos.castling_queenside_file(), rank_of(from));

  string move = (type_of(m) == DROP ? UCI::dropped_piece(pos, m) + (Options["Protocol"] == "usi" ? '*' : '@')
                                    : UCI::square(pos, from)) + UCI::square(pos, to);

  if (type_of(m) == PROMOTION)
      move += pos.piece_to_char()[make_piece(BLACK, promotion_type(m))];
  else if (type_of(m) == PIECE_PROMOTION)
      move += '+';
  else if (type_of(m) == PIECE_DEMOTION)
      move += '-';
  else if (is_gating(m))
      move += pos.piece_to_char()[make_piece(BLACK, gating_type(m))];

  return move;
}


/// UCI::to_move() converts a string representing a move in coordinate notation
/// (g1f3, a7a8q) to the corresponding legal Move, if any.

Move UCI::to_move(const Position& pos, string& str) {

  if (str.length() == 5)
  {
      if (str[4] == '=')
          // shogi moves refraining from promotion might use equals sign
          str.pop_back();
      else
          // Junior could send promotion piece in uppercase
          str[4] = char(tolower(str[4]));
  }

  for (const auto& m : MoveList<LEGAL>(pos))
      if (str == UCI::move(pos, m))
          return m;

  return MOVE_NONE;
}
