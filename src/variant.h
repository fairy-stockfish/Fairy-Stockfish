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
  std::string pieceToCharTable = "-";
  int pocketSize = 0;
  Rank maxRank = RANK_8;
  File maxFile = FILE_H;
  bool chess960 = false;
  bool twoBoards = false;
  std::set<PieceType> pieceTypes = { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
  std::string pieceToChar =  " PNBRQ" + std::string(KING - QUEEN - 1, ' ') + "K" + std::string(PIECE_TYPE_NB - KING - 1, ' ')
                           + " pnbrq" + std::string(KING - QUEEN - 1, ' ') + "k" + std::string(PIECE_TYPE_NB - KING - 1, ' ');
  std::string pieceToCharSynonyms = std::string(PIECE_NB, ' ');
  std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  Bitboard mobilityRegion[COLOR_NB][PIECE_TYPE_NB] = {};
  Rank promotionRank = RANK_8;
  std::set<PieceType, std::greater<PieceType> > promotionPieceTypes = { QUEEN, ROOK, BISHOP, KNIGHT };
  bool sittuyinPromotion = false;
  int promotionLimit[PIECE_TYPE_NB] = {}; // 0 means unlimited
  PieceType promotedPieceType[PIECE_TYPE_NB] = {};
  bool piecePromotionOnCapture = false;
  bool mandatoryPawnPromotion = true;
  bool mandatoryPiecePromotion = false;
  bool pieceDemotion = false;
  bool blastOnCapture = false;
  bool endgameEval = false;
  bool doubleStep = true;
  Rank doubleStepRank = RANK_2;
  Rank doubleStepRankMin = RANK_2;
  Bitboard enPassantRegion = AllSquares;
  bool castling = true;
  bool castlingDroppedPiece = false;
  File castlingKingsideFile = FILE_G;
  File castlingQueensideFile = FILE_C;
  Rank castlingRank = RANK_1;
  File castlingKingFile = FILE_E;
  PieceType castlingKingPiece = KING;
  PieceType castlingRookPiece = ROOK;
  PieceType kingType = KING;
  bool checking = true;
  bool dropChecks = true;
  bool mustCapture = false;
  bool mustDrop = false;
  PieceType mustDropType = ALL_PIECES;
  bool pieceDrops = false;
  bool dropLoop = false;
  bool capturesToHand = false;
  bool firstRankPawnDrops = false;
  bool promotionZonePawnDrops = false;
  bool dropOnTop = false;
  EnclosingRule enclosingDrop = NO_ENCLOSING;
  Bitboard enclosingDropStart = 0;
  Bitboard whiteDropRegion = AllSquares;
  Bitboard blackDropRegion = AllSquares;
  bool sittuyinRookDrop = false;
  bool dropOppositeColoredBishop = false;
  bool dropPromoted = false;
  bool shogiDoubledPawn = true;
  bool immobilityIllegal = false;
  bool gating = false;
  bool arrowGating = false;
  bool seirawanGating = false;
  bool cambodianMoves = false;
  Bitboard diagonalLines = 0;
  bool pass = false;
  bool passOnStalemate = false;
  bool makpongRule = false;
  bool flyingGeneral = false;
  Rank soldierPromotionRank = RANK_1;
  EnclosingRule flipEnclosedPieces = NO_ENCLOSING;
  // game end
  int nMoveRule = 50;
  int nFoldRule = 3;
  Value nFoldValue = VALUE_DRAW;
  bool nFoldValueAbsolute = false;
  bool perpetualCheckIllegal = false;
  bool moveRepetitionIllegal = false;
  Value stalemateValue = VALUE_DRAW;
  bool stalematePieceCount = false; // multiply stalemate value by sign(count(~stm) - count(stm))
  Value checkmateValue = -VALUE_MATE;
  bool shogiPawnDropMateIllegal = false;
  bool shatarMateRule = false;
  bool bikjangRule = false;
  Value extinctionValue = VALUE_NONE;
  bool extinctionClaim = false;
  bool extinctionPseudoRoyal = false; // TODO: implementation incomplete
  std::set<PieceType> extinctionPieceTypes = {};
  int extinctionPieceCount = 0;
  int extinctionOpponentPieceCount = 0;
  PieceType flagPiece = NO_PIECE_TYPE;
  Bitboard whiteFlag = 0;
  Bitboard blackFlag = 0;
  bool flagMove = false;
  bool checkCounting = false;
  int connectN = 0;
  MaterialCounting materialCounting = NO_MATERIAL_COUNTING;
  CountingRule countingRule = NO_COUNTING;

  // Derived properties
  bool fastAttacks = true;
  bool fastAttacks2 = true;
  PieceType nnueKing = KING;

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

  // Pre-calculate derived properties
  Variant* conclude() {
      fastAttacks = std::all_of(pieceTypes.begin(), pieceTypes.end(), [this](PieceType pt) {
                                    return (   pt < FAIRY_PIECES
                                            || pt == COMMONER || pt == IMMOBILE_PIECE
                                            || pt == ARCHBISHOP || pt == CHANCELLOR
                                            || (pt == KING && kingType == KING))
                                          && !(mobilityRegion[WHITE][pt] || mobilityRegion[BLACK][pt]);
                                })
                    && !cambodianMoves
                    && !diagonalLines;
      fastAttacks2 = std::all_of(pieceTypes.begin(), pieceTypes.end(), [this](PieceType pt) {
                                    return (   pt < FAIRY_PIECES
                                            || pt == COMMONER || pt == FERS || pt == WAZIR || pt == BREAKTHROUGH_PIECE
                                            || pt == SHOGI_PAWN || pt == GOLD || pt == SILVER || pt == SHOGI_KNIGHT
                                            || pt == DRAGON || pt == DRAGON_HORSE || pt == LANCE
                                            || (pt == KING && kingType == KING))
                                          && !(mobilityRegion[WHITE][pt] || mobilityRegion[BLACK][pt]);
                                })
                    && !cambodianMoves
                    && !diagonalLines;
      nnueKing =  pieceTypes.find(KING) != pieceTypes.end() ? KING
                : extinctionPieceTypes.find(COMMONER) != extinctionPieceTypes.end() ? COMMONER
                : NO_PIECE_TYPE;
      return this;
  }
};

class VariantMap : public std::map<std::string, const Variant*> {
public:
  void init();
  template <bool DoCheck> void parse(std::string path);
  template <bool DoCheck> void parse_istream(std::istream& file);
  void clear_all();
  std::vector<std::string> get_keys();

private:
  void add(std::string s, const Variant* v);
};

extern VariantMap variants;

#endif // #ifndef VARIANT_H_INCLUDED
