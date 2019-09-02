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

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "variant.h"

class VariantParser {
public:
    VariantParser(const std::map<std::string, std::string>& c) : config (c) {};
    Variant* parse();
    Variant* parse(Variant* v);

private:
    std::map<std::string, std::string> config;
    template <class T> void parse_attribute(const std::string& key, T& target);
};

#endif // #ifndef PARSER_H_INCLUDED