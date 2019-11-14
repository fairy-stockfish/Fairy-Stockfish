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

#ifndef VARIANT_H_INCLUDED
#define VARIANT_H_INCLUDED

#include <set>
#include <map>
#include <vector>
#include <string>
#include <functional>

#include "types.h"
#include "bitboard.h"


/// Variant struct stores information needed to determine the rules of a variant.

struct Variant {
  std::string variantTemplate = "fairy";
  int pocketSize = 0;
  Rank maxRank = RANK_8;
  File maxFile = FILE_H;
  bool chess960 = false;
  std::set<PieceType> pieceTypes = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
  std::string pieceToChar =  " PNBRQ" + std::string(KING - QUEEN - 1, ' ') + "K" + std::string(PIECE_TYPE_NB - KING - 1, ' ')
                           + " pnbrq" + std::string(KING - QUEEN - 1, ' ') + "k" + std::string(PIECE_TYPE_NB - KING - 1, ' ');
  std::string pieceToCharSynonyms = std::string(PIECE_NB, ' ');
  std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  Bitboard mobilityRegion[COLOR_NB][PIECE_TYPE_NB] = {};
  Rank promotionRank = RANK_8;
  std::set<PieceType, std::greater<PieceType> > promotionPieceTypes = { QUEEN, ROOK, BISHOP, KNIGHT };
  bool sittuyinPromotion = false;
  uint8_t promotionLimit[PIECE_TYPE_NB] = {}; // 0 means unlimited
  PieceType promotedPieceType[PIECE_TYPE_NB] = {};
  bool piecePromotionOnCapture = false;
  bool mandatoryPawnPromotion = true;
  bool mandatoryPiecePromotion = false;
  bool pieceDemotion = false;
  bool endgameEval = false;
  bool doubleStep = true;
  Rank doubleStepRank = RANK_2;
  bool firstRankDoubleSteps = false;
  bool castling = true;
  bool castlingDroppedPiece = false;
  File castlingKingsideFile = FILE_G;
  File castlingQueensideFile = FILE_C;
  Rank castlingRank = RANK_1;
  PieceType castlingRookPiece = ROOK;
  bool checking = true;
  bool mustCapture = false;
  bool mustDrop = false;
  bool pieceDrops = false;
  bool dropLoop = false;
  bool capturesToHand = false;
  bool firstRankDrops = false;
  bool dropOnTop = false;
  Bitboard whiteDropRegion = AllSquares;
  Bitboard blackDropRegion = AllSquares;
  bool sittuyinRookDrop = false;
  bool dropOppositeColoredBishop = false;
  bool dropPromoted = false;
  bool shogiDoubledPawn = true;
  bool immobilityIllegal = false;
  bool gating = false;
  bool seirawanGating = false;
  bool cambodianMoves = false;
  bool flyingGeneral = false;
  bool xiangqiGeneral = false;
  bool xiangqiSoldier = false;
  // game end
  int nMoveRule = 50;
  int nFoldRule = 3;
  Value nFoldValue = VALUE_DRAW;
  bool nFoldValueAbsolute = false;
  bool perpetualCheckIllegal = false;
  Value stalemateValue = VALUE_DRAW;
  Value checkmateValue = -VALUE_MATE;
  bool shogiPawnDropMateIllegal = false;
  bool shatarMateRule = false;
  Value bareKingValue = VALUE_NONE;
  Value extinctionValue = VALUE_NONE;
  bool bareKingMove = false;
  std::set<PieceType> extinctionPieceTypes = {};
  PieceType flagPiece = NO_PIECE_TYPE;
  Bitboard whiteFlag = 0;
  Bitboard blackFlag = 0;
  bool flagMove = false;
  bool checkCounting = false;
  int connectN = 0;
  CountingRule countingRule = NO_COUNTING;

  void add_piece(PieceType pt, char c, char c2 = ' ') {
      pieceToChar[make_piece(WHITE, pt)] = toupper(c);
      pieceToChar[make_piece(BLACK, pt)] = tolower(c);
      pieceToCharSynonyms[make_piece(WHITE, pt)] = toupper(c2);
      pieceToCharSynonyms[make_piece(BLACK, pt)] = tolower(c2);
      pieceTypes.insert(pt);
  }

  void remove_piece(PieceType pt) {
      pieceToChar[make_piece(WHITE, pt)] = ' ';
      pieceToChar[make_piece(BLACK, pt)] = ' ';
      pieceToCharSynonyms[make_piece(WHITE, pt)] = ' ';
      pieceToCharSynonyms[make_piece(BLACK, pt)] = ' ';
      pieceTypes.erase(pt);
  }

  void reset_pieces() {
      pieceToChar = std::string(PIECE_NB, ' ');
      pieceToCharSynonyms = std::string(PIECE_NB, ' ');
      pieceTypes.clear();
  }
};

class VariantMap : public std::map<std::string, const Variant*> {
public:
  void init();
  void parse(std::string path);
  void clear_all();
  std::vector<std::string> get_keys();

private:
  void add(std::string s, const Variant* v);
};

extern VariantMap variants;

#endif // #ifndef VARIANT_H_INCLUDED
