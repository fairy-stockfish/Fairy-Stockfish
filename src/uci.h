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

#ifndef UCI_H_INCLUDED
#define UCI_H_INCLUDED

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

#include "search.h"
#include "types.h"

#include "variant.h"

namespace Stockfish {

class Position;

namespace UCI {

#ifndef _WIN32
constexpr char SepChar = ':';
#else
constexpr char SepChar = ';';
#endif

void init_variant(const Variant* v);

class Option;

// Define a custom comparator, because the UCI options should be case-insensitive
struct CaseInsensitiveLess {
    bool operator()(const std::string&, const std::string&) const;
};

// The options container is defined as a std::map
// The options container is a std::map with an additional read-only accessor,
// so that search workers can read options through a const reference.
struct OptionsMap: public std::map<std::string, Option, CaseInsensitiveLess> {
    using std::map<std::string, Option, CaseInsensitiveLess>::operator[];
    const Option& operator[](const std::string& name) const { return this->at(name); }
};

std::ostream& operator<<(std::ostream&, const OptionsMap&);

// The Option class implements each option as specified by the UCI protocol
class Option {

    using OnChange = void (*)(const Option&);

   public:
    Option(OnChange = nullptr);
    Option(bool v, OnChange = nullptr);
    Option(const char* v, OnChange = nullptr);
    Option(const char* v, const char* cur, OnChange = nullptr);
    Option(const char* v, const std::vector<std::string>& variants, OnChange = nullptr);
    Option(double v, int minv, int maxv, OnChange = nullptr);

    Option& operator=(const std::string&);
    void    operator<<(const Option&);
    operator int() const;
    operator std::string() const;
    bool              operator==(const char*) const;
    bool              operator!=(const char*) const;
    void              set_combo(std::vector<std::string> newComboValues);
    void              set_default(std::string newDefault);
    const std::string get_type() const;

   private:
    friend std::ostream& operator<<(std::ostream&, const OptionsMap&);

    std::string              defaultValue, currentValue, type;
    int                      min, max;
    std::vector<std::string> comboValues;
    size_t                   idx;
    OnChange                 on_change;
};

void        init(OptionsMap&);
int         to_cp(Value v);
std::string value(Value v);
std::string square(const Position& pos, Square s);
std::string dropped_piece(const Position& pos, Move m);
std::string move(const Position& pos, Move m);
std::string wdl(Value v, int ply);
Move        to_move(const Position& pos, std::string& str);

std::string option_name(std::string name);
bool        is_valid_option(UCI::OptionsMap& options, std::string& name);

}  // namespace UCI

extern UCI::OptionsMap Options;

enum Protocol {
    UCI_GENERAL,
    USI,
    UCCI,
    UCI_CYCLONE,
    XBOARD,
};

constexpr bool is_uci_dialect(Protocol p) { return p != XBOARD; }

extern Protocol CurrentProtocol;

class Engine;

void print_numa_config_information(const Engine& engine);
void print_thread_binding_information(const Engine& engine);

// The UCIEngine class implements the UCI protocol and its dialects (USI, UCCI,
// UCI-Cyclone) as well as the dispatch to the XBoard state machine on top of an Engine.
class UCIEngine {
   public:
    UCIEngine(int argc, char** argv);
    ~UCIEngine();

    void loop();

    Engine& get_engine() { return *engine; }


   private:
    std::unique_ptr<Engine> engine;
    int                     argc;
    char**                  argv;

    void go(std::istringstream& is, const std::vector<Move>& banmoves = {});
    void bench(std::istream& args);
    void benchmark(std::istream& args);
    void init_search_update_listeners();
    void position(std::istringstream& is);

    void on_update_no_moves(const Search::InfoShort& info);
    void on_update_full(const Search::InfoFull& info);
    void on_iter(const Search::InfoIteration& info);
    void on_bestmove(std::string_view bestmove, std::string_view ponder);
};

}  // namespace Stockfish

#endif  // #ifndef UCI_H_INCLUDED
