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
  // leg of more than one square turns as easily as a step: the osprey leaps
  // two squares orthogonally, [D?B], before riding out diagonally, and turning
  // that leap is turning the direction it points in. Returns -1 for a leg that
  // is not a multiple of a queen direction, a knight's for instance, where a
  // 45 degree turn means nothing.
  int bent_leg(int df, int dr, int* length) {
      int len = std::max(std::abs(df), std::abs(dr));
      if (!len || (df && dr && std::abs(df) != std::abs(dr)))
          return -1;
      *length = len;
      return bent_direction_index(df / len, dr / len);
  }
  // Which continuations of a bent leg survive, given the atom the second leg
  // rides with and the filter from any modifier written outside the brackets.
  //
  // The second atom decides how far the move turns. Riding with the other
  // family - [F?R], [W?B], [D?B] - is a 45 degree turn, and it forks: bit 1
  // for the direction 45 degrees clockwise from the leg, bit 2 for the one
  // counter-clockwise. Riding with the same family - [F-B], the Tamerlane
  // Picket, or [W-R], a rook that must move at least two squares - carries
  // straight on, recorded as bit 4.
  //
  // filter 0 keeps every continuation, 1 keeps only those running along a
  // file (v[F?R], the ship), 2 only those running along a rank. The leg
  // length is packed above the mask, so one int carries the whole leg.
  int continuation_mask(int df, int dr, int filter, char rideAtom) {
      int len = 0;
      int i = bent_leg(df, dr, &len);
      if (i < 0)
          return 0;
      bool legDiagonal = i & 1;
      bool rideDiagonal = rideAtom == 'B';
      int mask = 0;
      if (legDiagonal == rideDiagonal)
      {
          // straight on: the ride keeps the leg's own direction
          if (filter == 0 || (filter == 1 && !BentRotation[i][0]) || (filter == 2 && !BentRotation[i][1]))
              mask = 4;
      }
      else
          for (int k = 0; k < 2; k++)
          {
              const int* cont = BentRotation[(i + (k ? 7 : 1)) & 7];
              if (   filter == 0
                  || (filter == 1 && cont[0] == 0)
                  || (filter == 2 && cont[1] == 0))
                  mask |= 1 << k;
          }
      return mask ? (mask | (len << 3)) : 0;
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
      // 'bent' is cleared once the leg's atom has been handled, like every
      // other qualifier; this one has to survive until the group closes.
      bool inBracket = false;
      bool bentStop = true;
      char bentRide = 0;
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
          // Bent riders, in the bracket notation of XBetza: [A?B] is a leg
          // with atom A, which the piece may stop on, followed by a ride with
          // atom B in the outward direction; [A-B] is the same with the leg
          // square only passed through. Modifiers written inside the brackets
          // restrict the leg as usual, so the snake is [vW?B]. Modifiers
          // written outside apply to the move as a whole, the brackets
          // standing in for an oblique atom, so the ship is v[F?R].
          //
          // Everything but the leg is read here, when the group opens; the
          // leg itself then goes through the ordinary atom handling below,
          // which is what gives it its directional modifiers for free.
          else if (c == '[')
          {
              std::string::size_type close = betza.find(']', i);
              std::string::size_type sep = betza.find_first_of("?-", i);
              if (close == std::string::npos || sep == std::string::npos || sep > close)
                  continue;
              // The second leg carrying its own directional modifiers, [W?sR],
              // would be a turn of something other than 45 degrees, which is
              // not supported; leave the group alone rather than read it as
              // something it is not.
              if (close - sep != 2)
                  continue;
              bent = inBracket = true;
              bentStop = betza[sep] == '?';
              bentRide = betza[close - 1];
              for (auto s : prelimDirections)
                  contFilter = s == "vv" ? 1 : s == "ss" ? 2 : contFilter;
              prelimDirections.clear();
          }
          // End of a bracket group: the ride atom was taken when it opened
          else if ((c == '?' || c == '-') && inBracket)
          {
              i = betza.find(']', i);
              bent = inBracket = false;
              bentStop = true;
              bentRide = 0;
              contFilter = 0;
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
                      // bent rider, as a bent leg as well, carrying the mask
                      // of the continuations that survived the outer filter.
                      // A leg written with ? is also an ordinary destination,
                      // one written with - only a square passed through.
                      auto add_dir = [&](int df, int dr) {
                          Direction d = Direction(dr * FILE_NB + df);
                          int mask = bent && !rider && !hopper
                                   ? continuation_mask(df, dr, contFilter, bentRide) : 0;
                          if (!bent || bentStop)
                              v[d] = distance;
                          if (mask)
                              p->bent[initial][modality][d] = mask;
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
