/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

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

#include <cstdlib>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "benchmark.h"
#include "engine.h"
#include "memory.h"
#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "timeman.h"
#include "tt.h"
#include "uci.h"
#include "xboard.h"
#include "syzygy/tbprobe.h"

using namespace std;

namespace Stockfish {

constexpr auto BenchmarkCommand = "speedtest";

// position() is called when engine receives the "position" UCI command.
// The function sets up the position described in the given FEN string ("fen")
// or the starting position ("startpos") and then makes the moves given in the
// following move list ("moves").

void UCIEngine::position(istringstream& is) {

    string token, fen;

    is >> token;
    // Parse as SFEN if specified
    bool sfen = token == "sfen";

    if (token == "startpos")
    {
        fen = variants.find(Options["UCI_Variant"])->second->startFen;
        is >> token;  // Consume "moves" token if any
    }
    else if (token == "fen" || token == "sfen")
        while (is >> token && token != "moves")
            fen += token + " ";
    else
        return;

    std::vector<std::string> moves;

    while (is >> token)
        moves.push_back(token);

    engine->set_position(fen, moves, sfen);
}


// Called when the engine receives the "setoption" UCI command.
// The function updates the UCI option ("name") to the given value ("value").

static void setoption(istringstream& is) {

    // Options must not be modified while a search is reading them
    if (mainEngine)
        mainEngine->wait_for_search_finished();

    string token, name, value;

    is >> token;  // Consume the "name" token

    if (CurrentProtocol == UCCI)
        name = token;
    else
        // Read option name (can contain spaces)
        while (is >> token && token != "value")
            name += (name.empty() ? "" : " ") + token;

    // Read the option value (can contain spaces)
    while (is >> token)
        value += (value.empty() ? "" : " ") + token;

    if (Options.count(name))
        Options[name] = value;
    // Deal with option name aliases in UCI dialects
    else if (is_valid_option(Options, name))
        Options[name] = value;
    else
        sync_cout << "No such option: " << name << sync_endl;
}


// Called when the engine receives the "go" UCI command. The function sets the
// thinking time and other parameters from the input string then stars with a search

void UCIEngine::go(istringstream& is, const std::vector<Move>& banmoves) {

    const Position&    pos = engine->position();
    Search::LimitsType limits;
    string             token;
    bool               ponderMode = false;

    limits.startTime = now();  // The search starts as early as possible

    limits.banmoves    = banmoves;
    bool isUsi         = CurrentProtocol == USI;
    int  secResolution = Options["usemillisec"] ? 1 : 1000;

    while (is >> token)
        if (token == "searchmoves")  // Needs to be the last command on the line
            while (is >> token)
                limits.searchmoves.push_back(UCI::to_move(pos, token));

        else if (token == "wtime")
            is >> limits.time[isUsi ? BLACK : WHITE];
        else if (token == "btime")
            is >> limits.time[isUsi ? WHITE : BLACK];
        else if (token == "winc")
            is >> limits.inc[isUsi ? BLACK : WHITE];
        else if (token == "binc")
            is >> limits.inc[isUsi ? WHITE : BLACK];
        else if (token == "movestogo")
            is >> limits.movestogo;
        else if (token == "depth")
            is >> limits.depth;
        else if (token == "nodes")
            is >> limits.nodes;
        else if (token == "movetime")
            is >> limits.movetime;
        else if (token == "mate")
            is >> limits.mate;
        else if (token == "perft")
            is >> limits.perft;
        else if (token == "infinite")
            limits.infinite = 1;
        else if (token == "ponder")
            ponderMode = true;
        // UCCI commands
        else if (token == "time")
            is >> limits.time[pos.side_to_move()], limits.time[pos.side_to_move()] *= secResolution;
        else if (token == "opptime")
            is >> limits.time[~pos.side_to_move()],
              limits.time[~pos.side_to_move()] *= secResolution;
        else if (token == "increment")
            is >> limits.inc[pos.side_to_move()], limits.inc[pos.side_to_move()] *= secResolution;
        else if (token == "oppincrement")
            is >> limits.inc[~pos.side_to_move()], limits.inc[~pos.side_to_move()] *= secResolution;
        // USI commands
        else if (token == "byoyomi")
        {
            int byoyomi = 0;
            is >> byoyomi;
            limits.inc[WHITE] = limits.inc[BLACK] = byoyomi;
            limits.time[WHITE] += byoyomi;
            limits.time[BLACK] += byoyomi;
        }

    if (limits.perft)
    {
        engine->perft(limits.perft);
        return;
    }

    limits.ponderMode = ponderMode;
    engine->go(limits);
}

// bench() is called when engine receives the "bench" command. Firstly
// a list of UCI commands is setup according to bench parameters, then
// it is run one by one printing a summary at the end.

void UCIEngine::bench(istream& args) {

    Position& pos = engine->position();

    string   token;
    uint64_t num, nodes = 0, cnt = 1;

    vector<string> list = setup_bench(pos, args);
    num                 = count_if(list.begin(), list.end(),
                                   [](const string& s) { return s.find("go ") == 0 || s.find("eval") == 0; });

    TimePoint elapsed = now();

    for (const auto& cmd : list)
    {
        istringstream is(cmd);
        is >> skipws >> token;

        if (token == "go" || token == "eval")
        {
            cerr << "\nPosition: " << cnt++ << '/' << num << " (" << pos.fen() << ")" << endl;
            if (token == "go")
            {
                go(is);
                engine->wait_for_search_finished();
                nodes += Threads.nodes_searched();
            }
            else
                engine->trace_eval();
        }
        else if (token == "setoption")
            setoption(is);
        else if (token == "position")
            position(is);
        else if (token == "ucinewgame")
        {
            engine->search_clear();
            elapsed = now();
        }  // Search::clear() may take a while
    }

    elapsed = now() - elapsed + 1;  // Ensure positivity to avoid a 'divide by zero'

    dbg_print();

    cerr << "\n==========================="
         << "\nTotal time (ms) : " << elapsed << "\nNodes searched  : " << nodes
         << "\nNodes/second    : " << 1000 * nodes / elapsed << endl;
}

// Runs the "speedtest" command: a fixed set of chess games is searched with time
// limits, all threads and a large hash table to measure the speed in a realistic way.
void UCIEngine::benchmark(std::istream& args) {
    // Probably not very important for a test this long, but include for completeness and sanity.
    static constexpr int NUM_WARMUP_POSITIONS = 3;

    std::string token;
    uint64_t    cnt = 1;

    engine->set_on_update_full([](const auto&) {});
    engine->set_on_iter([](const auto&) {});
    engine->set_on_update_no_moves([](const auto&) {});
    engine->set_on_bestmove([](const auto&, const auto&) {});
    engine->set_on_verify_networks([](const auto&) {});

    const std::string previousVariant = Options["UCI_Variant"];

    BenchmarkSetup setup = setup_benchmark(args);

    const auto numGoCommands = count_if(setup.commands.begin(), setup.commands.end(),
                                        [](const std::string& c) { return c.find("go ") == 0; });

    // Set options once at the start. The positions are chess positions.
    auto ss = std::istringstream("name Threads value " + std::to_string(setup.threads));
    setoption(ss);
    ss = std::istringstream("name Hash value " + std::to_string(setup.ttSize));
    setoption(ss);
    ss = std::istringstream("name UCI_Variant value chess");
    setoption(ss);
    ss = std::istringstream("name UCI_Chess960 value false");
    setoption(ss);

    // Warmup
    for (const auto& cmd : setup.commands)
    {
        std::istringstream is(cmd);
        is >> token;

        if (token == "go")
        {
            // One new line is produced by the search, so omit it here
            std::cerr << "\rWarmup position " << cnt++ << '/' << NUM_WARMUP_POSITIONS;

            go(is);
            engine->wait_for_search_finished();
        }
        else if (token == "position")
            position(is);
        else if (token == "ucinewgame")
        {
            engine->search_clear();  // search_clear may take a while
        }

        if (cnt > NUM_WARMUP_POSITIONS)
            break;
    }

    std::cerr << "\n";

    cnt = 1;

    int           numHashfullReadings = 0;
    constexpr int hashfullAges[]      = {0, 999};  // Only normal hashfull and touched hash.
    constexpr int hashfullAgeCount    = std::size(hashfullAges);
    int           totalHashfull[hashfullAgeCount] = {0};
    int           maxHashfull[hashfullAgeCount]   = {0};

    auto updateHashfullReadings = [&]() {
        numHashfullReadings += 1;

        for (int i = 0; i < hashfullAgeCount; ++i)
        {
            const int hashfull = engine->get_hashfull(hashfullAges[i]);
            maxHashfull[i]     = std::max(maxHashfull[i], hashfull);
            totalHashfull[i] += hashfull;
        }
    };

    engine->search_clear();  // search_clear may take a while

    using Clock = std::chrono::steady_clock;
    Clock::time_point elapsed;
    Clock::duration   totalTime(0);

    uint64_t nodes = 0, nodesSearched = 0;

    engine->set_on_update_full([&](const Search::InfoFull& i) { nodesSearched = i.nodes; });

    engine->set_on_start([&elapsed, &nodesSearched]() {
        elapsed       = Clock::now();
        nodesSearched = 0;
    });

    engine->set_on_bestmove(
      [&totalTime, &elapsed, &nodes, &nodesSearched](const auto&, const auto&) {
          totalTime += Clock::now() - elapsed;
          nodes += nodesSearched;
      });

    for (const auto& cmd : setup.commands)
    {
        std::istringstream is(cmd);
        is >> token;

        if (token == "go")
        {
            // One new line is produced by the search, so omit it here
            std::cerr << "\rPosition " << cnt++ << '/' << numGoCommands;

            go(is);
            engine->wait_for_search_finished();

            updateHashfullReadings();
        }
        else if (token == "position")
            position(is);
        else if (token == "ucinewgame")
        {
            engine->search_clear();  // search_clear may take a while
        }
    }

    // Ensure positivity to avoid a 'divide by zero'
    const auto totalTimeMs = std::max<int64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count(), 1LL);

    dbg_print();

    std::cerr << "\n";

    static_assert(
      std::size(hashfullAges) == 2 && hashfullAges[0] == 0 && hashfullAges[1] == 999,
      "Hardcoded for display. Would complicate the code needlessly in the current state.");

    std::string threadBinding = engine->thread_binding_information_as_string();
    if (threadBinding.empty())
        threadBinding = "none";

    // clang-format off

    std::cerr << "==========================="
              << "\nVersion                    : "
              << engine_info(false, true)
              // "\nCompiled by                : "
              << compiler_info()
              << "Large pages                : " << (has_large_pages() ? "yes" : "no")
              << "\nUser invocation            : " << BenchmarkCommand << " "
              << setup.originalInvocation << "\nFilled invocation          : " << BenchmarkCommand
              << " " << setup.filledInvocation
              << "\nAvailable processors       : " << engine->get_numa_config_as_string()
              << "\nThread count               : " << setup.threads
              << "\nThread binding             : " << threadBinding
              << "\nTT size [MiB]              : " << setup.ttSize
              << "\nHash max, avg [per mille]  : "
              << "\n    single search          : " << maxHashfull[0] << ", "
              << totalHashfull[0] / numHashfullReadings
              << "\n    single game            : " << maxHashfull[1] << ", "
              << totalHashfull[1] / numHashfullReadings
              << "\nTotal nodes searched       : " << nodes
              << "\nTotal search time [s]      : " << totalTimeMs / 1000.0
              << "\nNodes/second               : " << 1000 * nodes / totalTimeMs << std::endl;

    // clang-format on

    ss = std::istringstream("name UCI_Variant value " + previousVariant);
    setoption(ss);

    init_search_update_listeners();
}

// The win rate model returns the probability of winning (in per mille units) given an
// eval and a game ply. It fits the LTC fishtest statistics rather accurately.
static int win_rate_model(Value v, int ply) {

    // The model captures only up to 240 plies, so limit input (and rescale)
    double m = std::min(240, ply) / 64.0;

    // Coefficients of a 3rd order polynomial fit based on fishtest data
    // for two parameters needed to transform eval to the argument of a
    // logistic function.
    double as[] = {-1.17202460e-01, 5.94729104e-01, 1.12065546e+01, 1.22606222e+02};
    double bs[] = {-1.79066759, 11.30759193, -17.43677612, 36.47147479};
    double a    = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b    = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    // Transform eval to centipawns with limited range
    double x = std::clamp(double(100 * v) / PawnValueEg, -2000.0, 2000.0);

    // Return win rate in per mille (rounded to nearest)
    return int(0.5 + 1000 / (1 + std::exp((a - x) / b)));
}

// load() is called when engine receives the "load" or "check" command.
// The function reads variant configuration files.

static void load(istringstream& is, bool check = false) {

    string token;
    std::getline(is >> std::ws, token);

    // The argument to load either is a here-doc or a file path
    if (token.rfind("<<", 0) == 0)
    {
        // Trim the EOF marker
        if (!(stringstream(token.substr(2)) >> token))
            token = "";

        // Parse variant config till EOF marker
        stringstream ss;
        std::string  line;
        while (std::getline(cin, line) && line != token)
            ss << line << std::endl;
        if (check)
            variants.parse_istream<true>(ss);
        else
        {
            variants.parse_istream<false>(ss);
            Options["UCI_Variant"].set_combo(variants.get_keys());
        }
    }
    else
    {
        // store path if non-empty after trimming
        std::size_t end = token.find_last_not_of(' ');
        if (end != std::string::npos)
        {
            if (check)
                variants.parse<true>(token.erase(end + 1));
            else
                Options["VariantPath"] = token.erase(end + 1);
        }
    }
}

// Waits for a command from the stdin, parses it, and then calls the appropriate
// function. It also intercepts an end-of-file (EOF) indication from the stdin to ensure a
// graceful exit if the GUI dies unexpectedly. When called with some command-line arguments,
// like running 'bench', the function returns immediately after the command is executed.
// In addition to the UCI ones, some additional debug commands are also supported.
void print_numa_config_information(const Engine& engine) {
    auto cfgStr = engine.get_numa_config_as_string();
    sync_cout << "info string Available Processors: " << cfgStr << sync_endl;
}

void print_thread_binding_information(const Engine& engine) {
    auto boundThreadsByNode = engine.get_bound_thread_count_by_numa_node();
    if (!boundThreadsByNode.empty())
    {
        sync_cout << "info string NUMA Node Thread Binding: ";
        bool isFirst = true;
        for (auto&& [current, total] : boundThreadsByNode)
        {
            if (!isFirst)
                std::cout << ":";
            std::cout << current << "/" << total;
            isFirst = false;
        }
        std::cout << sync_endl;
    }
}

void UCIEngine::loop() {

    Position& pos = engine->position();
    string    token, cmd;
    for (int i = 1; i < argc; ++i)
        cmd += std::string(argv[i]) + " ";

    // XBoard state machine
    XBoard::stateMachine = new XBoard::StateMachine(engine->position(), engine->state_list());
    // UCCI banmoves state
    std::vector<Move> banmoves = {};

    if (argc > 1 && (std::strcmp(argv[1], "noautoload") == 0))
    {
        cmd  = "";
        argc = 1;
    }
    else if (argc == 1 || !(std::strcmp(argv[1], "load") == 0))
    {
        // Check environment for variants.ini file
        char* envVariantPath = std::getenv("FAIRY_STOCKFISH_VARIANT_PATH");
        if (envVariantPath != NULL)
            Options["VariantPath"] = std::string(envVariantPath);
    }

    do
    {
        if (argc == 1
            && !getline(cin, cmd))  // Wait for an input or an end-of-file (EOF) indication
            cmd = "quit";

        istringstream is(cmd);

        token.clear();  // Avoid a stale if getline() returns nothing or a blank line
        is >> skipws >> token;

        if (token == "quit" || token == "stop")
            engine->stop();

        // The GUI sends 'ponderhit' to tell that the user has played the expected move.
        // So, 'ponderhit' is sent if pondering was done on the same move that the user
        // has played. The search should continue, but should also switch from pondering
        // to the normal search.
        else if (token == "ponderhit")
            engine->set_ponderhit(false);  // Switch to the normal search

        else if (token == "uci" || token == "usi" || token == "ucci" || token == "xboard"
                 || token == "ucicyclone")
        {
            CurrentProtocol       = token == "uci"
                                    ? (CurrentProtocol == UCI_CYCLONE ? UCI_CYCLONE : UCI_GENERAL)
                                  : token == "ucicyclone" ? UCI_CYCLONE
                                  : token == "usi"        ? USI
                                  : token == "ucci"       ? UCCI
                                                          : XBOARD;
            string defaultVariant = string(
#ifdef LARGEBOARDS
              CurrentProtocol == USI                                      ? "shogi"
              : CurrentProtocol == UCCI || CurrentProtocol == UCI_CYCLONE ? "xiangqi"
#else
              CurrentProtocol == USI                                      ? "minishogi"
              : CurrentProtocol == UCCI || CurrentProtocol == UCI_CYCLONE ? "minixiangqi"
#endif
                                                                          : "chess");
            Options["UCI_Variant"].set_default(defaultVariant);
            std::istringstream ss("startpos");
            position(ss);
            if (is_uci_dialect(CurrentProtocol) && token != "ucicyclone")
            {
                sync_cout << "id name " << engine_info(true) << "\n" << Options << sync_endl;

                sync_cout << token << "ok" << sync_endl;
            }
            // Allow to enforce protocol at startup
            argc = 1;
        }

        else if (CurrentProtocol == XBOARD)
            XBoard::stateMachine->process_command(token, is);

        else if (token == "setoption")
            setoption(is);
        // UCCI-specific banmoves command
        else if (token == "banmoves")
            while (is >> token)
                banmoves.push_back(UCI::to_move(pos, token));
        else if (token == "go")
        {
            // send info strings after the go command is sent for old GUIs and python-chess
            print_numa_config_information(*engine);
            print_thread_binding_information(*engine);
            go(is, banmoves);
        }
        else if (token == "position")
            position(is), banmoves.clear();
        else if (token == "ucinewgame" || token == "usinewgame" || token == "uccinewgame")
            engine->search_clear();
        else if (token == "isready")
            sync_cout << "readyok" << sync_endl;

        // Add custom non-UCI commands, mainly for debugging purposes.
        // These commands must not be used during a search!
        else if (token == "flip")
            engine->flip();
        else if (token == "bench")
            bench(is);
        else if (token == BenchmarkCommand)
            benchmark(is);
        else if (token == "d")
            sync_cout << engine->visualize() << sync_endl;
        else if (token == "eval")
            engine->trace_eval();
        else if (token == "compiler")
            sync_cout << compiler_info() << sync_endl;
        else if (token == "export_net")
        {
            std::optional<std::string> filename;
            std::string                f;
            if (is >> skipws >> f)
                filename = f;
            engine->save_network(filename);
        }
        else if (token == "load")
        {
            load(is);
            argc = 1;
        }  // continue reading stdin
        else if (token == "check")
            load(is, true);
        // UCI-Cyclone omits the "position" keyword
        else if (token == "fen" || token == "startpos")
        {
#ifdef LARGEBOARDS
            if (CurrentProtocol == UCI_GENERAL && Options["UCI_Variant"] == "chess")
            {
                CurrentProtocol = UCI_CYCLONE;
                Options["UCI_Variant"].set_default("xiangqi");
            }
#endif
            is.seekg(0);
            position(is);
        }
        else if (token == "--help" || token == "help" || token == "--license" || token == "license")
            sync_cout
              << "\nFairy-Stockfish is a powerful chess variant engine for playing and analyzing."
                 "\nIt is released as free software licensed under the GNU GPLv3 License."
                 "\nFairy-Stockfish is normally used with a graphical user interface (GUI) and implements"
                 "\nthe Universal Chess Interface (UCI) protocol and related protocols to communicate"
                 "\nwith a GUI, an API, etc."
                 "\nFor any further information, visit https://github.com/fairy-stockfish/Fairy-Stockfish#readme"
                 "\nor read the corresponding README.md and Copying.txt files distributed along with this program.\n"
              << sync_endl;
        else if (!token.empty() && token[0] != '#')
            sync_cout << "Unknown command: '" << cmd << "'. Type help for more information."
                      << sync_endl;

    } while (token != "quit" && argc == 1);  // The command-line arguments are one-shot
}


void UCIEngine::init_search_update_listeners() {
    engine->set_on_iter([this](const auto& i) { on_iter(i); });
    engine->set_on_update_no_moves([this](const auto& i) { on_update_no_moves(i); });
    engine->set_on_update_full([this](const auto& i) { on_update_full(i); });
    engine->set_on_start([]() {});
    engine->set_on_bestmove([this](const auto& bm, const auto& p) { on_bestmove(bm, p); });
    engine->set_on_verify_networks({});
}

UCIEngine::UCIEngine(int argc_, char** argv_) :
    engine(std::make_unique<Engine>()),
    argc(argc_),
    argv(argv_) {

    init_search_update_listeners();

    engine->load_networks();
    engine->resize_threads();
    engine->search_clear();  // After threads are up
}

UCIEngine::~UCIEngine() = default;

void UCIEngine::on_update_no_moves(const Search::InfoShort& info) {
    sync_cout << "info depth " << info.depth << " score " << UCI::value(info.score) << sync_endl;
}

void UCIEngine::on_update_full(const Search::InfoFull& info) {
    std::stringstream ss;

    if (CurrentProtocol == XBOARD)
    {
        ss << info.depth << " " << UCI::value(info.score) << " " << info.timeMs / 10 << " "
           << info.nodes << " " << info.selDepth << " " << info.nps << " " << info.tbHits << "\t";

        // Do not print PVs with virtual drops in bughouse variants
        if (!engine->position().two_boards())
            ss << " " << info.pv;
    }
    else
    {
        ss << "info";
        ss << " depth " << info.depth               //
           << " seldepth " << info.selDepth         //
           << " multipv " << info.multiPV           //
           << " score " << UCI::value(info.score);  //

        if (!info.bound.empty())
            ss << " " << info.bound;

        if (!info.wdl.empty())
            ss << info.wdl;

        ss << " nodes " << info.nodes        //
           << " nps " << info.nps            //
           << " hashfull " << info.hashfull  //
           << " tbhits " << info.tbHits      //
           << " time " << info.timeMs        //
           << " pv " << info.pv;             //
    }

    sync_cout << ss.str() << sync_endl;
}

void UCIEngine::on_iter(const Search::InfoIteration& info) {
    std::stringstream ss;

    ss << "info";
    ss << " depth " << info.depth                     //
       << " currmove " << info.currmove               //
       << " currmovenumber " << info.currmovenumber;  //

    sync_cout << ss.str() << sync_endl;
}

void UCIEngine::on_bestmove(std::string_view bestmove, std::string_view ponder) {
    sync_cout << "bestmove " << bestmove;
    if (!ponder.empty())
        std::cout << " ponder " << ponder;
    std::cout << sync_endl;
}


/// UCI::value() converts a Value to a string by adhering to the UCI protocol specification:
///
/// cp <x>    The score from the engine's point of view in centipawns.
/// mate <y>  Mate in 'y' moves (not plies). If the engine is getting mated,
///           uses negative values for 'y'.

string UCI::value(Value v) {

    assert(-VALUE_INFINITE < v && v < VALUE_INFINITE);

    stringstream ss;

    if (CurrentProtocol == XBOARD)
    {
        if (abs(v) < VALUE_MATE_IN_MAX_PLY)
            ss << v * 100 / PawnValueEg;
        else
            ss << (v > 0 ? XBOARD_VALUE_MATE + VALUE_MATE - v + 1
                         : -XBOARD_VALUE_MATE - VALUE_MATE - v - 1)
                    / 2;
    }
    else

      if (abs(v) < VALUE_MATE_IN_MAX_PLY)
        ss << (CurrentProtocol == UCCI ? "" : "cp ") << v * 100 / PawnValueEg;
    else if (CurrentProtocol == USI)
        // In USI, mate distance is given in ply
        ss << "mate " << (v > 0 ? VALUE_MATE - v : -VALUE_MATE - v);
    else
        ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v - 1) / 2;

    return ss.str();
}


/// UCI::wdl() reports the win-draw-loss (WDL) statistics given an evaluation
/// and a game ply based on the data gathered for fishtest LTC games.

string UCI::wdl(Value v, int ply) {

    stringstream ss;

    int wdl_w = win_rate_model(v, ply);
    int wdl_l = win_rate_model(-v, ply);
    int wdl_d = 1000 - wdl_w - wdl_l;
    ss << " wdl " << wdl_w << " " << wdl_d << " " << wdl_l;

    return ss.str();
}


/// UCI::square() converts a Square to a string in algebraic notation (g1, a7, etc.)

std::string UCI::square(const Position& pos, Square s) {
#ifdef LARGEBOARDS
    if (CurrentProtocol == USI)
        return rank_of(s) < RANK_10
               ? std::string{char('1' + pos.max_file() - file_of(s)),
                             char('a' + pos.max_rank() - rank_of(s))}
               : std::string{char('0' + (pos.max_file() - file_of(s) + 1) / 10),
                             char('0' + (pos.max_file() - file_of(s) + 1) % 10),
                             char('a' + pos.max_rank() - rank_of(s))};
    else if (pos.max_rank() == RANK_10 && CurrentProtocol != UCI_GENERAL)
        return std::string{char('a' + file_of(s)), char('0' + rank_of(s))};
    else
        return rank_of(s) < RANK_10
               ? std::string{char('a' + file_of(s)), char('1' + (rank_of(s) % 10))}
               : std::string{char('a' + file_of(s)), char('0' + ((rank_of(s) + 1) / 10)),
                             char('0' + ((rank_of(s) + 1) % 10))};
#else
    return CurrentProtocol == USI ? std::string{char('1' + pos.max_file() - file_of(s)),
                                                char('a' + pos.max_rank() - rank_of(s))}
                                  : std::string{char('a' + file_of(s)), char('1' + rank_of(s))};
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
/// The only special case is castling where the e1g1 notation is printed in
/// standard chess mode and in e1h1 notation it is printed in Chess960 mode.
/// Internally, all castling moves are always encoded as 'king captures rook'.

string UCI::move(const Position& pos, Move m) {

    Square from = m.from_sq();
    Square to   = m.to_sq();

    if (m == MOVE_NONE)
        return CurrentProtocol == USI ? "resign" : "(none)";

    if (m == MOVE_NULL)
        return "0000";

    if (is_pass(m) && CurrentProtocol == XBOARD)
        return "@@@@";

    if (is_gating(m) && gating_square(m) == to)
        from = to_sq(m), to = from_sq(m);
    else if (type_of(m) == CASTLING && !pos.is_chess960())
    {
        to = make_square(to > from ? pos.castling_kingside_file() : pos.castling_queenside_file(),
                         rank_of(from));
        // If the castling move is ambiguous with a normal king move, switch to 960 notation
        if (pos.pseudo_legal(make_move(from, to)))
            to = to_sq(m);
    }

    string move =
      (type_of(m) == DROP ? UCI::dropped_piece(pos, m) + (CurrentProtocol == USI ? '*' : '@')
                          : UCI::square(pos, from))
      + UCI::square(pos, to);

    // Wall square
    if (pos.walling() && CurrentProtocol == XBOARD)
        move += "," + UCI::square(pos, to) + UCI::square(pos, gating_square(m));

    if (type_of(m) == PROMOTION)
        move += pos.piece_to_char()[make_piece(BLACK, promotion_type(m))];
    else if (type_of(m) == PIECE_PROMOTION)
        move += '+';
    else if (type_of(m) == PIECE_DEMOTION)
        move += '-';
    else if (is_gating(m))
    {
        move += pos.piece_to_char()[make_piece(BLACK, gating_type(m))];
        if (gating_square(m) != from)
            move += UCI::square(pos, gating_square(m));
    }

    // Wall square
    if (pos.walling() && CurrentProtocol != XBOARD)
        move += "," + UCI::square(pos, to) + UCI::square(pos, gating_square(m));

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
        if (str == UCI::move(pos, m)
            || (is_pass(m) && str == UCI::square(pos, from_sq(m)) + UCI::square(pos, to_sq(m))))
            return m;

    return Move::none();
}

std::string UCI::option_name(std::string name) {
    if (CurrentProtocol == UCCI && name == "Hash")
        return "hashsize";
    if (CurrentProtocol == USI)
    {
        if (name == "Hash" || name == "Ponder" || name == "MultiPV")
            return "USI_" + name;
        if (name.substr(0, 4) == "UCI_")
            name = "USI_" + name.substr(4);
    }
    if (CurrentProtocol == UCCI || CurrentProtocol == USI)
        std::replace(name.begin(), name.end(), ' ', '_');
    return name;
}

bool UCI::is_valid_option(UCI::OptionsMap& options, std::string& name) {
    for (const auto& it : options)
    {
        std::string optionName = option_name(it.first);
        if (!options.key_comp()(optionName, name) && !options.key_comp()(name, optionName))
        {
            name = it.first;
            return true;
        }
    }
    return false;
}

Protocol CurrentProtocol = UCI_GENERAL;  // Global object

}  // namespace Stockfish
