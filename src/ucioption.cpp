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

#include <algorithm>
#include <cassert>
#include <ostream>
#include <sstream>
#include <iostream>

#include "evaluate.h"
#include "misc.h"
#include "piece.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "variant.h"
#include "syzygy/tbprobe.h"

using std::string;

namespace Stockfish {

UCI::OptionsMap Options; // Global object

namespace PSQT {
  void init(const Variant* v);
}

namespace UCI {

// standard variants of XBoard/WinBoard
std::set<string> standard_variants = {
    "normal", "nocastle", "fischerandom", "knightmate", "3check", "makruk", "shatranj",
    "asean", "seirawan", "crazyhouse", "bughouse", "suicide", "giveaway", "losers", "atomic",
    "capablanca", "gothic", "janus", "caparandom", "grand", "shogi", "xiangqi"
};

void init_variant(const Variant* v) {
    pieceMap.init(v);
    Bitboards::init_pieces();
}

/// 'On change' actions, triggered by an option's value change
void on_clear_hash(const Option&) { Search::clear(); }
void on_hash_size(const Option& o) { TT.resize(size_t(o)); }
void on_logger(const Option& o) { start_logger(o); }
void on_threads(const Option& o) { Threads.set(size_t(o)); }
void on_tb_path(const Option& o) { Tablebases::init(o); }

void on_use_NNUE(const Option& ) { Eval::NNUE::init(); }
void on_eval_file(const Option& ) { Eval::NNUE::init(); }

void on_variant_path(const Option& o) {
    std::stringstream ss((std::string)o);
    std::string path;

    while (std::getline(ss, path, SepChar))
        variants.parse<false>(path);

    Options["UCI_Variant"].set_combo(variants.get_keys());
}
void on_variant_set(const Option &o) {
    // Re-initialize NNUE
    Eval::NNUE::init();

    const Variant* v = variants.find(o)->second;
    init_variant(v);
    PSQT::init(v);
}
void on_variant_change(const Option &o) {
    // Variant initialization
    on_variant_set(o);

    const Variant* v = variants.find(o)->second;
    // Do not send setup command for known variants
    if (standard_variants.find(o) != standard_variants.end())
        return;
    int pocketsize = v->pieceDrops ? (v->pocketSize ? v->pocketSize : v->pieceTypes.size()) : 0;
    if (Options["Protocol"] == "xboard")
    {
        // Overwrite setup command for Janggi variants
        auto itJanggi = variants.find("janggi");
        if (   itJanggi != variants.end()
            && v->variantTemplate == itJanggi->second->variantTemplate
            && v->startFen == itJanggi->second->startFen
            && v->pieceToCharTable == itJanggi->second->pieceToCharTable)
        {
            sync_cout << "setup (PH.R.AE..K.C.ph.r.ae..k.c.) 9x10+0_janggi "
                      << "rhea1aehr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RHEA1AEHR w - - 0 1"
                      << sync_endl;
            return;
        }
        // Send setup command
        sync_cout << "setup (" << v->pieceToCharTable << ") "
                  << v->maxFile + 1 << "x" << v->maxRank + 1
                  << "+" << pocketsize << "_" << v->variantTemplate
                  << " " << v->startFen
                  << sync_endl;
        // Send piece command with Betza notation
        // https://www.gnu.org/software/xboard/Betza.html
        for (PieceType pt : v->pieceTypes)
        {
            string suffix =   pt == PAWN && v->doubleStep     ? "ifmnD"
                            : pt == KING && v->cambodianMoves ? "ismN"
                            : pt == FERS && v->cambodianMoves ? "ifD"
                                                              : "";
            // Janggi palace moves
            if (v->diagonalLines)
            {
                PieceType pt2 = pt == KING ? v->kingType : pt;
                if (pt2 == WAZIR)
                    suffix += "F";
                else if (pt2 == SOLDIER)
                    suffix += "fF";
                else if (pt2 == ROOK)
                    suffix += "B";
                else if (pt2 == JANGGI_CANNON)
                    suffix += "pB";
            }
            // Castling
            if (pt == KING && v->castling)
                 suffix += "O" + std::to_string((v->castlingKingsideFile - v->castlingQueensideFile) / 2);
            // Drop region
            if (v->pieceDrops)
            {
                if (pt == PAWN && !v->firstRankPawnDrops)
                    suffix += "j";
                else if (pt == v->dropNoDoubled)
                    suffix += std::string(v->dropNoDoubledCount, 'f');
                else if (pt == BISHOP && v->dropOppositeColoredBishop)
                    suffix += "s";
                suffix += "@" + std::to_string(pt == PAWN && !v->promotionZonePawnDrops ? v->promotionRank : v->maxRank + 1);
            }
            sync_cout << "piece " << v->pieceToChar[pt] << "& " << pieceMap.find(pt == KING ? v->kingType : pt)->second->betza << suffix << sync_endl;
            PieceType promType = v->promotedPieceType[pt];
            if (promType)
                sync_cout << "piece +" << v->pieceToChar[pt] << "& " << pieceMap.find(promType)->second->betza << sync_endl;
        }
    }
    else
        sync_cout << "info string variant " << (std::string)o
                << " files " << v->maxFile + 1
                << " ranks " << v->maxRank + 1
                << " pocket " << pocketsize
                << " template " << v->variantTemplate
                << " startpos " << v->startFen
                << sync_endl;
}


/// Our case insensitive less() function as required by UCI protocol
bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {

  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
         [](char c1, char c2) { return tolower(c1) < tolower(c2); });
}


/// UCI::init() initializes the UCI options to their hard-coded default values

void init(OptionsMap& o) {

  constexpr int MaxHashMB = Is64Bit ? 33554432 : 2048;

  o["Protocol"]              << Option("uci", {"uci", "usi", "ucci", "ucicyclone", "xboard"});
  o["Debug Log File"]        << Option("", on_logger);
  o["Threads"]               << Option(1, 1, 512, on_threads);
  o["Hash"]                  << Option(16, 1, MaxHashMB, on_hash_size);
  o["Clear Hash"]            << Option(on_clear_hash);
  o["Ponder"]                << Option(false);
  o["MultiPV"]               << Option(1, 1, 500);
  o["Skill Level"]           << Option(20, -20, 20);
  o["Move Overhead"]         << Option(10, 0, 5000);
  o["Slow Mover"]            << Option(100, 10, 1000);
  o["nodestime"]             << Option(0, 0, 10000);
  o["UCI_Chess960"]          << Option(false);
  o["UCI_Variant"]           << Option("chess", variants.get_keys(), on_variant_change);
  o["UCI_AnalyseMode"]       << Option(false);
  o["UCI_LimitStrength"]     << Option(false);
  o["UCI_Elo"]               << Option(1350, 500, 2850);
  o["UCI_ShowWDL"]           << Option(false);
  o["SyzygyPath"]            << Option("<empty>", on_tb_path);
  o["SyzygyProbeDepth"]      << Option(1, 1, 100);
  o["Syzygy50MoveRule"]      << Option(true);
  o["SyzygyProbeLimit"]      << Option(7, 0, 7);
  o["Use NNUE"]              << Option(true, on_use_NNUE);
#ifndef NNUE_EMBEDDING_OFF
  o["EvalFile"]              << Option((std::string(EvalFileDefaultName) + UCI::SepChar + EvalFile2DefaultName).c_str(), on_eval_file);
#else
  o["EvalFile"]              << Option("<empty>", on_eval_file);
#endif
  o["TsumeMode"]             << Option(false);
  o["VariantPath"]           << Option("<empty>", on_variant_path);
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

  if (Options["Protocol"] == "xboard")
  {
      for (size_t idx = 0; idx < om.size(); ++idx)
          for (const auto& it : om)
              if (it.second.idx == idx && it.first != "Protocol" && it.first != "UCI_Variant"
                                       && it.first != "Threads" && it.first != "Hash")
              {
                  const Option& o = it.second;
                  os << "\nfeature option=\"" << it.first << " -" << o.type;

                  if (o.type == "string" || o.type == "combo")
                      os << " " << o.defaultValue;
                  else if (o.type == "check")
                      os << " " << int(o.defaultValue == "true");

                  if (o.type == "combo")
                      for (string value : o.comboValues)
                          if (value != o.defaultValue)
                              os << " /// " << value;

                  if (o.type == "spin")
                      os << " " << int(stof(o.defaultValue))
                         << " " << o.min
                         << " " << o.max;

                  os << "\"";

                  break;
              }
  }
  else

  for (size_t idx = 0; idx < om.size(); ++idx)
      for (const auto& it : om)
          if (it.second.idx == idx)
          {
              const Option& o = it.second;
              // UCI dialects do not allow spaces
              if (Options["Protocol"] == "ucci" || Options["Protocol"] == "usi")
              {
                  string name = it.first;
                  std::replace(name.begin(), name.end(), ' ', '_');
                  // UCCI skips "name"
                  os << "\noption " << (Options["Protocol"] == "ucci" ? "" : "name ") << name << " type " << o.type;
              }
              else
                  os << "\noption name " << it.first << " type " << o.type;

              if (o.type == "string" || o.type == "check" || o.type == "combo")
                  os << " default " << o.defaultValue;

              if (o.type == "combo")
                  for (string value : o.comboValues)
                      os << " var " << value;

              if (o.type == "spin")
                  os << " default " << int(stof(o.defaultValue))
                     << " min "     << o.min
                     << " max "     << o.max;

              break;
          }

  return os;
}


/// Option class constructors and conversion operators

Option::Option(const char* v, OnChange f) : type("string"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(const char* v, const std::vector<std::string>& values, OnChange f) : type("combo"), min(0), max(0), comboValues(values), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(bool v, OnChange f) : type("check"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = (v ? "true" : "false"); }

Option::Option(OnChange f) : type("button"), min(0), max(0), on_change(f)
{}

Option::Option(double v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(f)
{ defaultValue = currentValue = std::to_string(v); }

Option::operator double() const {
  assert(type == "check" || type == "spin");
  return (type == "spin" ? stof(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
  assert(type == "string" || type == "combo");
  return currentValue;
}

bool Option::operator==(const char* s) const {
  assert(type == "combo");
  return   !CaseInsensitiveLess()(currentValue, s)
        && !CaseInsensitiveLess()(s, currentValue);
}

bool Option::operator!=(const char* s) const {
  assert(type == "combo");
  return !(*this == s);
}


/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

  static size_t insert_order = 0;

  *this = o;
  idx = insert_order++;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value
/// from the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

  assert(!type.empty());

  if (   (type != "button" && v.empty())
      || (type == "check" && v != "true" && v != "false")
      || (type == "combo" && (std::find(comboValues.begin(), comboValues.end(), v) == comboValues.end()))
      || (type == "spin" && (stof(v) < min || stof(v) > max)))
      return *this;

  if (type == "combo")
  {
      OptionsMap comboMap; // To have case insensitive compare
      for (string token : comboValues)
          comboMap[token] << Option();
      if (!comboMap.count(v) || v == "var")
          return *this;
  }

  if (type != "button")
      currentValue = v;

  if (on_change)
      on_change(*this);

  return *this;
}

void Option::set_combo(std::vector<std::string> newComboValues) {
    comboValues = newComboValues;
}

void Option::set_default(std::string newDefault) {
    defaultValue = currentValue = newDefault;

    // When changing the variant default, suppress variant definition output,
    // but still do the essential re-initialization of the variant
    if (on_change)
        (on_change == on_variant_change ? on_variant_set : on_change)(*this);
}

const std::string Option::get_type() const {
    return type;
}

} // namespace UCI

} // namespace Stockfish
