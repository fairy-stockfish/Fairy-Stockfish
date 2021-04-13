/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2021 Fabian Fichter

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

#include <map>
#include <string>
#include <utility>

#include "types.h"
#include "piece.h"

PieceMap pieceMap; // Global object


namespace {
  std::map<char, std::vector<std::pair<int, int>>> leaperAtoms = {
      {'W', {std::make_pair(1, 0)}},
      {'F', {std::make_pair(1, 1)}},
      {'D', {std::make_pair(2, 0)}},
      {'N', {std::make_pair(2, 1)}},
      {'A', {std::make_pair(2, 2)}},
      {'H', {std::make_pair(3, 0)}},
      {'L', {std::make_pair(3, 1)}},
      {'C', {std::make_pair(3, 1)}},
      {'J', {std::make_pair(3, 2)}},
      {'Z', {std::make_pair(3, 2)}},
      {'G', {std::make_pair(3, 3)}},
      {'K', {std::make_pair(1, 0), std::make_pair(1, 1)}},
  };
  std::map<char, std::vector<std::pair<int, int>>> riderAtoms = {
      {'R', {std::make_pair(1, 0)}},
      {'B', {std::make_pair(1, 1)}},
      {'Q', {std::make_pair(1, 0), std::make_pair(1, 1)}},
  };
  const std::string verticals = "fbvh";
  const std::string horizontals = "rlsh";
  // from_betza creates a piece by parsing Betza notation
  // https://en.wikipedia.org/wiki/Betza%27s_funny_notation
  PieceInfo* from_betza(const std::string& betza, const std::string& name) {
      PieceInfo* p = new PieceInfo();
      p->name = name;
      p->betza = betza;
      std::vector<char> moveTypes = {};
      bool hopper = false;
      bool rider = false;
      std::vector<std::string> prelimDirections = {};
      for (std::string::size_type i = 0; i < betza.size(); i++)
      {
          char c = betza[i];
          // Move or capture
          if (c == 'm' || c == 'c')
              moveTypes.push_back(c);
          // Hopper
          else if (c == 'p')
              hopper = true;
          // Lame leaper
          else if (c == 'n')
              p->lameLeaper = true;
          // Directional modifiers
          else if (verticals.find(c) != std::string::npos || horizontals.find(c) != std::string::npos)
          {
              if (i + 1 < betza.size())
              {
                  char c2 = betza[i+1];
                  // Can modifiers be combined?
                  if (   c2 == c
                      || (verticals.find(c) != std::string::npos && horizontals.find(c2) != std::string::npos)
                      || (horizontals.find(c) != std::string::npos && verticals.find(c2) != std::string::npos))
                  {
                      prelimDirections.push_back(std::string(1, c) + c2);
                      i++;
                      continue;
                  }
              }
              prelimDirections.push_back(std::string(2, c));
          }
          // Move atom
          else if (leaperAtoms.find(c) != leaperAtoms.end() || riderAtoms.find(c) != riderAtoms.end())
          {
              const auto& atoms = riderAtoms.find(c) != riderAtoms.end() ? riderAtoms.find(c)->second
                                                                         : leaperAtoms.find(c)->second;
              // Check for rider
              if (riderAtoms.find(c) != riderAtoms.end())
                  rider = true;
              if (i + 1 < betza.size() && (isdigit(betza[i+1]) || betza[i+1] == c))
              {
                  rider = true;
                  // limited distance riders
                  if (isdigit(betza[i+1]))
                  {
                      // TODO: not supported
                  }
                  i++;
              }
              // No type qualifier means m+c
              if (moveTypes.size() == 0)
              {
                  moveTypes.push_back('m');
                  moveTypes.push_back('c');
              }
              // Define moves
              for (const auto& atom : atoms)
              {
                  std::vector<std::string> directions = {};
                  // Split directions for orthogonal pieces
                  // This is required e.g. to correctly interpret fsW for soldiers
                  for (auto s : prelimDirections)
                      if (atoms.size() == 1 && atom.second == 0 && s[0] != s[1])
                      {
                          directions.push_back(std::string(2, s[0]));
                          directions.push_back(std::string(2, s[1]));
                      }
                      else
                          directions.push_back(s);
                  // Add moves
                  for (char mt : moveTypes)
                  {
                      std::vector<Direction>& v = hopper ? (mt == 'c' ? p->hopperCapture : p->hopperQuiet)
                                                 : rider ? (mt == 'c' ? p->sliderCapture : p->sliderQuiet)
                                                         : (mt == 'c' ? p->stepsCapture : p->stepsQuiet);
                      auto has_dir = [&](std::string s) {
                        return std::find(directions.begin(), directions.end(), s) != directions.end();
                      };
                      if (directions.size() == 0 || has_dir("ff") || has_dir("vv") || has_dir("rf") || has_dir("rv") || has_dir("fh") || has_dir("rh") || has_dir("hr"))
                          v.push_back(Direction(atom.first * FILE_NB + atom.second));
                      if (directions.size() == 0 || has_dir("bb") || has_dir("vv") || has_dir("lb") || has_dir("lv") || has_dir("bh") || has_dir("lh") || has_dir("hr"))
                          v.push_back(Direction(-atom.first * FILE_NB - atom.second));
                      if (directions.size() == 0 || has_dir("rr") || has_dir("ss") || has_dir("br") || has_dir("bs") || has_dir("bh") || has_dir("lh") || has_dir("hr"))
                          v.push_back(Direction(-atom.second * FILE_NB + atom.first));
                      if (directions.size() == 0 || has_dir("ll") || has_dir("ss") || has_dir("fl") || has_dir("fs") || has_dir("fh") || has_dir("rh") || has_dir("hr"))
                          v.push_back(Direction(atom.second * FILE_NB - atom.first));
                      if (directions.size() == 0 || has_dir("rr") || has_dir("ss") || has_dir("fr") || has_dir("fs") || has_dir("fh") || has_dir("rh") || has_dir("hl"))
                          v.push_back(Direction(atom.second * FILE_NB + atom.first));
                      if (directions.size() == 0 || has_dir("ll") || has_dir("ss") || has_dir("bl") || has_dir("bs") || has_dir("bh") || has_dir("lh") || has_dir("hl"))
                          v.push_back(Direction(-atom.second * FILE_NB - atom.first));
                      if (directions.size() == 0 || has_dir("bb") || has_dir("vv") || has_dir("rb") || has_dir("rv") || has_dir("bh") || has_dir("rh") || has_dir("hl"))
                          v.push_back(Direction(-atom.first * FILE_NB + atom.second));
                      if (directions.size() == 0 || has_dir("ff") || has_dir("vv") || has_dir("lf") || has_dir("lv") || has_dir("fh") || has_dir("lh") || has_dir("hl"))
                          v.push_back(Direction(atom.first * FILE_NB - atom.second));
                  }
              }
              // Reset state
              moveTypes.clear();
              prelimDirections.clear();
              hopper = false;
              rider = false;
          }
      }
      return p;
  }
  // Special multi-leg betza description for Janggi elephant
  PieceInfo* janggi_elephant_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "janggiElephant";
      p->betza = "mafsmafW";
      p->stepsQuiet = {SOUTH + 2 * SOUTH_WEST, SOUTH + 2 * SOUTH_EAST,
                       WEST  + 2 * SOUTH_WEST, EAST  + 2 * SOUTH_EAST,
                       WEST  + 2 * NORTH_WEST, EAST  + 2 * NORTH_EAST,
                       NORTH + 2 * NORTH_WEST, NORTH + 2 * NORTH_EAST};
      p->stepsCapture = {SOUTH + 2 * SOUTH_WEST, SOUTH + 2 * SOUTH_EAST,
                         WEST  + 2 * SOUTH_WEST, EAST  + 2 * SOUTH_EAST,
                         WEST  + 2 * NORTH_WEST, EAST  + 2 * NORTH_EAST,
                         NORTH + 2 * NORTH_WEST, NORTH + 2 * NORTH_EAST};
      p->lameLeaper = true;
      return p;
  }
}

void PieceMap::init() {
  add(PAWN, from_betza("fmWfceF", "pawn"));
  add(KNIGHT, from_betza("N", "knight"));
  add(BISHOP, from_betza("B", "bishop"));
  add(ROOK, from_betza("R", "rook"));
  add(QUEEN, from_betza("Q", "queen"));
  add(FERS, from_betza("F", "fers"));
  add(ALFIL, from_betza("A", "alfil"));
  add(FERS_ALFIL, from_betza("FA", "fersAlfil"));
  add(SILVER, from_betza("FfW", "silver"));
  add(AIWOK, from_betza("RNF", "aiwok"));
  add(BERS, from_betza("RF", "bers"));
  add(ARCHBISHOP, from_betza("BN", "archbishop"));
  add(CHANCELLOR, from_betza("RN", "chancellor"));
  add(AMAZON, from_betza("QN", "amazon"));
  add(KNIBIS, from_betza("mNcB", "knibis"));
  add(BISKNI, from_betza("mBcN", "biskni"));
  add(KNIROO, from_betza("mNcR", "kniroo"));
  add(ROOKNI, from_betza("mRcN", "rookni"));
  add(SHOGI_PAWN, from_betza("fW", "shogiPawn"));
  add(LANCE, from_betza("fR", "lance"));
  add(SHOGI_KNIGHT, from_betza("fN", "shogiKnight"));
  add(EUROSHOGI_KNIGHT, from_betza("fNsW", "euroshogiKnight"));
  add(GOLD, from_betza("WfF", "gold"));
  add(DRAGON_HORSE, from_betza("BW", "dragonHorse"));
  add(CLOBBER_PIECE, from_betza("cW", "clobber"));
  add(BREAKTHROUGH_PIECE, from_betza("fWfFcF", "breakthrough"));
  add(IMMOBILE_PIECE, from_betza("", "immobile"));
  add(ATAXX_PIECE, from_betza("mDmNmA", "ataxx"));
  add(QUIET_QUEEN, from_betza("mQ", "quietQueen"));
  add(CANNON, from_betza("mRcpR", "cannon"));
  add(JANGGI_CANNON, from_betza("pR", "janggiCannon"));
  add(SOLDIER, from_betza("fsW", "soldier"));
  add(HORSE, from_betza("nN", "horse"));
  add(ELEPHANT, from_betza("nA", "elephant"));
  add(JANGGI_ELEPHANT, janggi_elephant_piece());
  add(BANNER, from_betza("RcpRnN", "banner"));
  add(WAZIR, from_betza("W", "wazir"));
  add(COMMONER, from_betza("K", "commoner"));
  add(CENTAUR, from_betza("KN", "centaur"));
  add(KING, from_betza("K", "king"));
}

void PieceMap::add(PieceType pt, const PieceInfo* p) {
  insert(std::pair<PieceType, const PieceInfo*>(pt, p));
}

void PieceMap::clear_all() {
  for (auto const& element : *this)
      delete element.second;
  clear();
}
