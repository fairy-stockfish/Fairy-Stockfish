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

#include <string>

#include "types.h"
#include "piece.h"

PieceMap pieceMap; // Global object

void PieceInfo::merge(const PieceInfo* pi) {
    stepsQuiet.insert(stepsQuiet.end(), pi->stepsQuiet.begin(), pi->stepsQuiet.end());
    stepsCapture.insert(stepsCapture.end(), pi->stepsCapture.begin(), pi->stepsCapture.end());
    sliderQuiet.insert(sliderQuiet.end(), pi->sliderQuiet.begin(), pi->sliderQuiet.end());
    sliderCapture.insert(sliderCapture.end(), pi->sliderCapture.begin(), pi->sliderCapture.end());
}

namespace {
  PieceInfo* pawn_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "pawn";
      p->betza = "fmWfceF";
      p->stepsQuiet = {NORTH};
      p->stepsCapture = {NORTH_WEST, NORTH_EAST};
      return p;
  }
  PieceInfo* knight_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "knight";
      p->betza = "N";
      p->stepsQuiet = {2 * SOUTH + WEST, 2 * SOUTH + EAST, SOUTH + 2 * WEST, SOUTH + 2 * EAST,
                       NORTH + 2 * WEST, NORTH + 2 * EAST, 2 * NORTH + WEST, 2 * NORTH + EAST };
      p->stepsCapture = {2 * SOUTH + WEST, 2 * SOUTH + EAST, SOUTH + 2 * WEST, SOUTH + 2 * EAST,
                         NORTH + 2 * WEST, NORTH + 2 * EAST, 2 * NORTH + WEST, 2 * NORTH + EAST };
      return p;
  }
  PieceInfo* bishop_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "bishop";
      p->betza = "B";
      p->sliderQuiet = {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};
      p->sliderCapture = {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};
      return p;
  }
  PieceInfo* rook_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "rook";
      p->betza = "R";
      p->sliderQuiet = {NORTH, EAST, SOUTH, WEST};
      p->sliderCapture = {NORTH, EAST, SOUTH, WEST};
      return p;
  }
  PieceInfo* queen_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "queen";
      p->betza = "RB";
      p->sliderQuiet = {NORTH, EAST, SOUTH, WEST, NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};
      p->sliderCapture = {NORTH, EAST, SOUTH, WEST, NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};
      return p;
  }
  PieceInfo* king_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "king";
      p->betza = "K";
      p->stepsQuiet = {SOUTH_WEST, SOUTH, SOUTH_EAST, WEST, EAST, NORTH_WEST, NORTH, NORTH_EAST};
      p->stepsCapture = {SOUTH_WEST, SOUTH, SOUTH_EAST, WEST, EAST, NORTH_WEST, NORTH, NORTH_EAST};
      return p;
  }
  PieceInfo* commoner_piece() {
      PieceInfo* p = king_piece();
      p->name = "commoner";
      return p;
  }
  PieceInfo* fers_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "fers";
      p->betza = "F";
      p->stepsQuiet = {SOUTH_WEST, SOUTH_EAST, NORTH_WEST, NORTH_EAST};
      p->stepsCapture = {SOUTH_WEST, SOUTH_EAST, NORTH_WEST, NORTH_EAST};
      return p;
  }
  PieceInfo* wazir_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "wazir";
      p->betza = "W";
      p->stepsQuiet = {SOUTH, WEST, EAST, NORTH};
      p->stepsCapture = {SOUTH, WEST, EAST, NORTH};
      return p;
  }
  PieceInfo* alfil_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "alfil";
      p->betza = "A";
      p->stepsQuiet = {2 * SOUTH_WEST, 2 * SOUTH_EAST, 2 * NORTH_WEST, 2 * NORTH_EAST};
      p->stepsCapture = {2 * SOUTH_WEST, 2 * SOUTH_EAST, 2 * NORTH_WEST, 2 * NORTH_EAST};
      return p;
  }
  PieceInfo* fers_alfil_piece() {
      PieceInfo* p = fers_piece();
      p->name = "fers_alfil";
      p->betza = "FA";
      PieceInfo* p2 = alfil_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
  PieceInfo* silver_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "silver";
      p->betza = "FfW";
      p->stepsQuiet = {SOUTH_WEST, SOUTH_EAST, NORTH_WEST, NORTH, NORTH_EAST};
      p->stepsCapture = {SOUTH_WEST, SOUTH_EAST, NORTH_WEST, NORTH, NORTH_EAST};
      return p;
  }
  PieceInfo* aiwok_piece() {
      PieceInfo* p = rook_piece();
      p->name = "aiwok";
      p->betza = "RNF";
      PieceInfo* p2 = knight_piece();
      PieceInfo* p3 = fers_piece();
      p->merge(p2);
      p->merge(p3);
      delete p2;
      delete p3;
      return p;
  }
  PieceInfo* bers_piece() {
      PieceInfo* p = rook_piece();
      p->name = "bers";
      p->betza = "RF";
      PieceInfo* p2 = fers_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
  PieceInfo* archbishop_piece() {
      PieceInfo* p = bishop_piece();
      p->name = "archbishop";
      p->betza = "BN";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
  PieceInfo* chancellor_piece() {
      PieceInfo* p = rook_piece();
      p->name = "chancellor";
      p->betza = "RN";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
  PieceInfo* amazon_piece() {
      PieceInfo* p = queen_piece();
      p->name = "amazon";
      p->betza = "RBN";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
  PieceInfo* knibis_piece() {
      PieceInfo* p = bishop_piece();
      p->name = "knibis";
      p->betza = "mNcB";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      p->stepsCapture = {};
      p->sliderQuiet = {};
      return p;
  }
  PieceInfo* biskni_piece() {
      PieceInfo* p = bishop_piece();
      p->name = "biskni";
      p->betza = "mBcN";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      p->stepsQuiet = {};
      p->sliderCapture = {};
      return p;
  }
  PieceInfo* kniroo_piece() {
      PieceInfo* p = rook_piece();
      p->name = "kniroo";
      p->betza = "mNcR";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      p->stepsCapture = {};
      p->sliderQuiet = {};
      return p;
  }
  PieceInfo* rookni_piece() {
      PieceInfo* p = rook_piece();
      p->name = "rookni";
      p->betza = "mRcN";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      p->stepsQuiet = {};
      p->sliderCapture = {};
      return p;
  }
  PieceInfo* shogi_pawn_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "shogi_pawn";
      p->betza = "fW";
      p->stepsQuiet = {NORTH};
      p->stepsCapture = {NORTH};
      return p;
  }
  PieceInfo* lance_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "lance";
      p->betza = "fR";
      p->sliderQuiet = {NORTH};
      p->sliderCapture = {NORTH};
      return p;
  }
  PieceInfo* shogi_knight_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "shogi_knight";
      p->betza = "fN";
      p->stepsQuiet = {2 * NORTH + WEST, 2 * NORTH + EAST};
      p->stepsCapture = {2 * NORTH + WEST, 2 * NORTH + EAST};
      return p;
  }
  PieceInfo* euroshogi_knight_piece() {
      PieceInfo* p = shogi_knight_piece();
      p->name = "euroshogi_knight";
      p->betza = "fNsW";
      p->stepsQuiet.push_back(WEST);
      p->stepsQuiet.push_back(EAST);
      p->stepsCapture.push_back(WEST);
      p->stepsCapture.push_back(EAST);
      return p;
  }
  PieceInfo* gold_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "gold";
      p->betza = "WfF";
      p->stepsQuiet = {SOUTH, WEST, EAST, NORTH_WEST, NORTH, NORTH_EAST};
      p->stepsCapture = {SOUTH, WEST, EAST, NORTH_WEST, NORTH, NORTH_EAST};
      return p;
  }
  PieceInfo* dragon_horse_piece() {
      PieceInfo* p = bishop_piece();
      p->name = "dragon_horse";
      p->betza = "BW";
      PieceInfo* p2 = wazir_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
  PieceInfo* clobber_piece() {
      PieceInfo* p = wazir_piece();
      p->name = "clobber";
      p->betza = "cW";
      p->stepsQuiet = {};
      return p;
  }
  PieceInfo* breakthrough_piece() {
      PieceInfo* p = pawn_piece();
      p->name = "breakthrough";
      p->betza = "fWfFcF";
      p->stepsQuiet.push_back(NORTH_WEST);
      p->stepsQuiet.push_back(NORTH_EAST);
      return p;
  }
  PieceInfo* immobile_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "immobile";
      return p;
  }
  PieceInfo* cannon_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "cannon";
      p->betza = "mRcpR";
      p->sliderQuiet = {NORTH, EAST, SOUTH, WEST};
      p->hopperCapture = {NORTH, EAST, SOUTH, WEST};
      return p;
  }
  PieceInfo* janggi_cannon_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "janggi_cannon";
      p->betza = "pR";
      p->hopperQuiet = {NORTH, EAST, SOUTH, WEST};
      p->hopperCapture = {NORTH, EAST, SOUTH, WEST};
      return p;
  }
  PieceInfo* soldier_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "soldier";
      p->betza = "fsW";
      p->stepsQuiet = {NORTH, WEST, EAST};
      p->stepsCapture = {NORTH, WEST, EAST};
      return p;
  }
  PieceInfo* horse_piece() {
      PieceInfo* p = knight_piece();
      p->name = "horse";
      p->betza = "nN";
      p->lameLeaper = true;
      return p;
  }
  PieceInfo* elephant_piece() {
      PieceInfo* p = alfil_piece();
      p->name = "elephant";
      p->betza = "nA";
      p->lameLeaper = true;
      return p;
  }
  PieceInfo* janggi_elephant_piece() {
      PieceInfo* p = new PieceInfo();
      p->name = "janggi_elephant";
      p->betza = "nZ";
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
  PieceInfo* banner_piece() {
      PieceInfo* p = rook_piece();
      p->name = "banner";
      p->betza = "RcpRnN";
      PieceInfo* p2 = horse_piece();
      p->merge(p2);
      delete p2;
      p->hopperCapture = {NORTH, EAST, SOUTH, WEST};
      p->lameLeaper = true;
      return p;
  }
  PieceInfo* centaur_piece() {
      PieceInfo* p = commoner_piece();
      p->name = "centaur";
      p->betza = "KN";
      PieceInfo* p2 = knight_piece();
      p->merge(p2);
      delete p2;
      return p;
  }
}

void PieceMap::init() {
  add(PAWN, pawn_piece());
  add(KNIGHT, knight_piece());
  add(BISHOP, bishop_piece());
  add(ROOK, rook_piece());
  add(QUEEN, queen_piece());
  add(FERS, fers_piece());
  add(ALFIL, alfil_piece());
  add(FERS_ALFIL, fers_alfil_piece());
  add(SILVER, silver_piece());
  add(AIWOK, aiwok_piece());
  add(BERS, bers_piece());
  add(ARCHBISHOP, archbishop_piece());
  add(CHANCELLOR, chancellor_piece());
  add(AMAZON, amazon_piece());
  add(KNIBIS, knibis_piece());
  add(BISKNI, biskni_piece());
  add(KNIROO, kniroo_piece());
  add(ROOKNI, rookni_piece());
  add(SHOGI_PAWN, shogi_pawn_piece());
  add(LANCE, lance_piece());
  add(SHOGI_KNIGHT, shogi_knight_piece());
  add(EUROSHOGI_KNIGHT, euroshogi_knight_piece());
  add(GOLD, gold_piece());
  add(DRAGON_HORSE, dragon_horse_piece());
  add(CLOBBER_PIECE, clobber_piece());
  add(BREAKTHROUGH_PIECE, breakthrough_piece());
  add(IMMOBILE_PIECE, immobile_piece());
  add(CANNON, cannon_piece());
  add(JANGGI_CANNON, janggi_cannon_piece());
  add(SOLDIER, soldier_piece());
  add(HORSE, horse_piece());
  add(ELEPHANT, elephant_piece());
  add(JANGGI_ELEPHANT, janggi_elephant_piece());
  add(BANNER, banner_piece());
  add(WAZIR, wazir_piece());
  add(COMMONER, commoner_piece());
  add(CENTAUR, centaur_piece());
  add(KING, king_piece());
}

void PieceMap::add(PieceType pt, const PieceInfo* p) {
  insert(std::pair<PieceType, const PieceInfo*>(pt, p));
}

void PieceMap::clear_all() {
  for (auto const& element : *this)
      delete element.second;
  clear();
}
