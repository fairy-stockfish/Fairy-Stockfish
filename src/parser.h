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

#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <iostream>

#include "variant.h"

class Config : public std::map<std::string, std::string> {
public:
    Config::iterator find (const std::string& s) {
        constexpr bool PrintOptions = false; // print config options?
        if (PrintOptions)
            std::cout << s << std::endl;
        consumedKeys.insert(s);
        return std::map<std::string, std::string>::find(s);
    }
    const std::set<std::string>& get_comsumed_keys() {
        return consumedKeys;
    }
private:
    std::set<std::string> consumedKeys = {};
};

template <bool DoCheck>
class VariantParser {
public:
    VariantParser(const Config& c) : config (c) {};
    Variant* parse();
    Variant* parse(Variant* v);

private:
    Config config;
    template <class T> void parse_attribute(const std::string& key, T& target);
    void parse_attribute(const std::string& key, PieceType& target, std::string pieceToChar);
};

#endif // #ifndef PARSER_H_INCLUDED