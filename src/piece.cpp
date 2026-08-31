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

#include <map>
#include <string>
#include <utility>

#include "types.h"
#include "piece.h"

namespace Stockfish {

PieceMap pieceMap; // Global object


namespace {
  const std::map<char, std::vector<std::pair<int, int>>> leaperAtoms = {
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
  const std::map<char, std::vector<std::pair<int, int>>> riderAtoms = {
      {'R', {std::make_pair(1, 0)}},
      {'B', {std::make_pair(1, 1)}},
      {'Q', {std::make_pair(1, 0), std::make_pair(1, 1)}},
  };
  const std::string verticals = "fbvh";
  const std::string horizontals = "rlsh";
  // The eight queen directions in rotational order, as (file, rank) offsets:
  // one step along this table is a 45 degree turn. Must stay in the same order
  // as QueenDirections in bitboard.h, which indexes RayBB the same way.
  constexpr int BentRotation[8][2] = { {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1} };

  int bent_direction_index(int df, int dr) {
      for (int i = 0; i < 8; i++)
          if (BentRotation[i][0] == df && BentRotation[i][1] == dr)
              return i;
      return -1;
  }

  // Splits a bent rider's first leg into a direction and a length, so that a
  // leg of more than one square is turned as easily as a step: the osprey
  // leaps two squares orthogonally (yD) before riding out diagonally, and
  // turning that leap by 45 degrees is turning the direction it points in.
  // Returns -1 for a leg that is not a multiple of a queen direction, a
  // knight's for instance, where a 45 degree turn means nothing.
  int bent_leg(int df, int dr, int* length) {
      int len = std::max(std::abs(df), std::abs(dr));
      if (!len || (df && dr && std::abs(df) != std::abs(dr)))
          return -1;
      *length = len;
      return bent_direction_index(df / len, dr / len);
  }

  // Which of the two continuations of a bent leg survive the filter. Bit 1 is
  // the direction 45 degrees clockwise from the leg, bit 2 the one 45 degrees
  // counter-clockwise; filter 0 keeps both, 1 keeps only continuations running
  // along a file, 2 only those running along a rank. The leg length is packed
  // above them, so that one int carries the whole shape of the leg.
  int continuation_mask(int df, int dr, int filter) {
      int len = 0;
      int i = bent_leg(df, dr, &len);
      if (i < 0)
          return 0;
      int mask = 0;
      for (int k = 0; k < 2; k++)
      {
          const int* cont = BentRotation[(i + (k ? 7 : 1)) & 7];
          if (   filter == 0
              || (filter == 1 && cont[0] == 0)
              || (filter == 2 && cont[1] == 0))
              mask |= 1 << k;
      }
      return mask ? (mask | (len << 2)) : 0;
  }
  // from_betza creates a piece by parsing Betza notation
  // https://en.wikipedia.org/wiki/Betza%27s_funny_notation
  PieceInfo* from_betza(const std::string& betza, const std::string& name) {
      PieceInfo* p = new PieceInfo();
      p->name = name;
      p->betza = betza;
      std::vector<MoveModality> moveModalities = {};
      bool hopper = false;
      bool rider = false;
      bool lame = false;
      bool initial = false;
      bool bent = false;
      int contFilter = 0;
      int distance = 0;
      std::vector<std::string> prelimDirections = {};
      for (std::string::size_type i = 0; i < betza.size(); i++)
      {
          char c = betza[i];
          // Modality
          if (c == 'm' || c == 'c')
              moveModalities.push_back(c == 'c' ? MODALITY_CAPTURE : MODALITY_QUIET);
          // Hopper
          else if (c == 'p' || c == 'g')
          {
              hopper = true;
              // Grasshopper
              if (c == 'g')
                  distance = 1;
          }
          // Lame leaper
          else if (c == 'n')
              lame = true;
          // Bent rider: the atom gives the first leg, on which the piece may
          // stop, and the ride then continues without limit 45 degrees off it.
          else if (c == 'y')
          {
              bent = true;
              // Accept the "afs" spelling XBoard/Winboard uses for the second
              // leg, so that a piece written yafsF there parses unchanged here.
              if (betza.compare(i + 1, 3, "afs") == 0)
                  i += 3;
              // A single v or h straight after the y restricts the ride to the
              // continuations running along a file, or along a rank. That is
              // what separates the Ship (yvF) from the Griffon (yF).
              // Directional modifiers written *before* the y keep their usual
              // meaning and restrict the first leg instead, hence vyW for the
              // Snake against yW for the Rhino.
              else if (i + 1 < betza.size() && (betza[i+1] == 'v' || betza[i+1] == 'h'))
                  contFilter = betza[++i] == 'v' ? 1 : 2;
          }
          // Initial move
          else if (c == 'i')
              initial = true;
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
                      distance = betza[i+1] - '0';
                  i++;
              }
              if (!rider && lame)
                  distance = -1;
              // No modality qualifier means m+c
              if (moveModalities.size() == 0)
              {
                  moveModalities.push_back(MODALITY_QUIET);
                  moveModalities.push_back(MODALITY_CAPTURE);
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
                  for (auto modality : moveModalities)
                  {
                      auto& v = hopper ? p->hopper[initial][modality]
                               : rider ? p->slider[initial][modality]
                                       : p->steps[initial][modality];
                      auto has_dir = [&](std::string s) {
                        return std::find(directions.begin(), directions.end(), s) != directions.end();
                      };
                      // Records one direction as an ordinary move and, for a
                      // bent rider, as a bent leg as well, carrying the mask of
                      // the continuations that survived the yv/yh filter. The
                      // ordinary move is what lets the piece stop on the corner
                      // square, which is how every bent rider in use behaves.
                      auto add_dir = [&](int df, int dr) {
                          Direction d = Direction(dr * FILE_NB + df);
                          v[d] = distance;
                          if (bent && !rider && !hopper)
                          {
                              int mask = continuation_mask(df, dr, contFilter);
                              if (mask)
                                  p->bent[initial][modality][d] = mask;
                          }
                      };
                      if (directions.size() == 0 || has_dir("ff") || has_dir("vv") || has_dir("rf") || has_dir("rv") || has_dir("fh") || has_dir("rh") || has_dir("hr"))
                          add_dir(atom.second, atom.first);
                      if (directions.size() == 0 || has_dir("bb") || has_dir("vv") || has_dir("lb") || has_dir("lv") || has_dir("bh") || has_dir("lh") || has_dir("hr"))
                          add_dir(-atom.second, -atom.first);
                      if (directions.size() == 0 || has_dir("rr") || has_dir("ss") || has_dir("br") || has_dir("bs") || has_dir("bh") || has_dir("rh") || has_dir("hr"))
                          add_dir(atom.first, -atom.second);
                      if (directions.size() == 0 || has_dir("ll") || has_dir("ss") || has_dir("fl") || has_dir("fs") || has_dir("fh") || has_dir("lh") || has_dir("hr"))
                          add_dir(-atom.first, atom.second);
                      if (directions.size() == 0 || has_dir("rr") || has_dir("ss") || has_dir("fr") || has_dir("fs") || has_dir("fh") || has_dir("rh") || has_dir("hl"))
                          add_dir(atom.first, atom.second);
                      if (directions.size() == 0 || has_dir("ll") || has_dir("ss") || has_dir("bl") || has_dir("bs") || has_dir("bh") || has_dir("lh") || has_dir("hl"))
                          add_dir(-atom.first, -atom.second);
                      if (directions.size() == 0 || has_dir("bb") || has_dir("vv") || has_dir("rb") || has_dir("rv") || has_dir("bh") || has_dir("rh") || has_dir("hl"))
                          add_dir(atom.second, -atom.first);
                      if (directions.size() == 0 || has_dir("ff") || has_dir("vv") || has_dir("lf") || has_dir("lv") || has_dir("fh") || has_dir("lh") || has_dir("hl"))
                          add_dir(-atom.second, atom.first);
                  }
              }
              // Reset state
              moveModalities.clear();
              prelimDirections.clear();
              hopper = false;
              rider = false;
              lame = false;
              initial = false;
              bent = false;
              contFilter = 0;
              distance = 0;
          }
      }
      return p;
  }
  // Special multi-leg betza description for Janggi elephant
  PieceInfo* janggi_elephant_piece() {
      PieceInfo* p = from_betza("nZ", "janggiElephant");
      p->betza = "mafsmafW"; // for compatibility with XBoard/Winboard
      return p;
  }
}

void PieceMap::init(const Variant* v) {
  clear_all();
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
  add(GOLD, from_betza("WfF", "gold"));
  add(DRAGON_HORSE, from_betza("BW", "dragonHorse"));
  add(CLOBBER_PIECE, from_betza("cW", "clobber"));
  add(BREAKTHROUGH_PIECE, from_betza("fmWfF", "breakthrough"));
  add(IMMOBILE_PIECE, from_betza("", "immobile"));
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
  // Add custom pieces
  for (PieceType pt = CUSTOM_PIECES; pt <= CUSTOM_PIECES_END; ++pt)
      add(pt, from_betza(v != nullptr ? v->customPiece[pt - CUSTOM_PIECES] : "", ""));
}

void PieceMap::add(PieceType pt, const PieceInfo* p) {
  insert(std::pair<PieceType, const PieceInfo*>(pt, p));
}

void PieceMap::clear_all() {
  for (auto const& element : *this)
      delete element.second;
  clear();
}

} // namespace Stockfish
