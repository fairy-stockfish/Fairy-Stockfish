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

#ifndef UCI_H_INCLUDED
#define UCI_H_INCLUDED

#include <map>
#include <string>
#include <vector>

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

/// Custom comparator because UCI options should be case insensitive
struct CaseInsensitiveLess {
  bool operator() (const std::string&, const std::string&) const;
};

/// Our options container is actually a std::map
typedef std::map<std::string, Option, CaseInsensitiveLess> OptionsMap;

/// Option class implements an option as defined by UCI protocol
class Option {

  typedef void (*OnChange)(const Option&);

public:
  Option(OnChange = nullptr);
  Option(bool v, OnChange = nullptr);
  Option(const char* v, OnChange = nullptr);
  Option(const char* v, const std::vector<std::string>& variants, OnChange = nullptr);
  Option(double v, int minv, int maxv, OnChange = nullptr);

  Option& operator=(const std::string&);
  void operator<<(const Option&);
  operator double() const;
  operator std::string() const;
  bool operator==(const char*) const;
  bool operator!=(const char*) const;
  void set_combo(std::vector<std::string> newComboValues);
  void set_default(std::string newDefault);
  const std::string get_type() const;

private:
  friend std::ostream& operator<<(std::ostream&, const OptionsMap&);

  std::string defaultValue, currentValue, type;
  int min, max;
  std::vector<std::string> comboValues;
  size_t idx;
  OnChange on_change;
};

void init(OptionsMap&);
void loop(int argc, char* argv[]);
std::string value(Value v);
std::string square(const Position& pos, Square s);
std::string dropped_piece(const Position& pos, Move m);
std::string move(const Position& pos, Move m);
std::string pv(const Position& pos, Depth depth, Value alpha, Value beta);
std::string wdl(Value v, int ply);
Move to_move(const Position& pos, std::string& str);

std::string option_name(std::string name, std::string protocol);
bool is_valid_option(UCI::OptionsMap& options, std::string& name);

} // namespace UCI

extern UCI::OptionsMap Options;

} // namespace Stockfish

#endif // #ifndef UCI_H_INCLUDED
