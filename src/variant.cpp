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
#include <iostream>
#include <fstream>
#include <sstream>

#include "parser.h"
#include "piece.h"
#include "variant.h"

using std::string;

VariantMap variants; // Global object

namespace {
    // Define variant rules
    Variant* fairy_variant_base() {
        Variant* v = new Variant();
        v->pieceToCharTable = "PNBRQ................Kpnbrq................k";
        v->endgameEval = false;
        return v;
    }
    Variant* chess_variant() {
        Variant* v = fairy_variant_base();
        v->endgameEval = true;
        return v;
    }
    Variant* chess960_variant() {
        Variant* v = chess_variant();
        v->chess960 = true;
        return v;
    }
    Variant* nocastle_variant() {
        Variant* v = chess_variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->castling = false;
        return v;
    }
    Variant* fairy_variant() {
        Variant* v = chess_variant();
        v->add_piece(SILVER, 's');
        v->add_piece(FERS, 'f');
        return v;
    }
    Variant* makruk_variant() {
        Variant* v = chess_variant();
        v->variantTemplate = "makruk";
        v->pieceToCharTable = "PN.R.M....SKpn.r.m....sk";
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 's');
        v->add_piece(MET, 'm');
        v->startFen = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {MET};
        v->doubleStep = false;
        v->castling = false;
        v->nMoveRule = 0;
        v->countingRule = MAKRUK_COUNTING;
        return v;
    }
    Variant* cambodian_variant() {
        Variant* v = makruk_variant();
        v->startFen = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w DEde - 0 1";
        v->gating = true;
        v->cambodianMoves = true;
        return v;
    }
    Variant* karouk_variant() {
        Variant* v = cambodian_variant();
        v->checkCounting = true;
        return v;
    }
    Variant* asean_variant() {
        Variant* v = chess_variant();
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 'b');
        v->add_piece(MET, 'q');
        v->startFen = "rnbqkbnr/8/pppppppp/8/8/PPPPPPPP/8/RNBQKBNR w - - 0 1";
        v->promotionPieceTypes = {ROOK, KNIGHT, KHON, MET};
        v->doubleStep = false;
        v->castling = false;
        v->countingRule = ASEAN_COUNTING;
        return v;
    }
    Variant* aiwok_variant() {
        Variant* v = makruk_variant();
        v->pieceToCharTable = "PN.R...A..SKpn.r...a..sk";
        v->remove_piece(MET);
        v->add_piece(AIWOK, 'a');
        v->startFen = "rnsaksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKASNR w - - 0 1";
        v->promotionPieceTypes = {AIWOK};
        return v;
    }
    Variant* shatranj_variant() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "shatranj";
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(ALFIL, 'b');
        v->add_piece(FERS, 'q');
        v->startFen = "rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w - - 0 1";
        v->promotionPieceTypes = {FERS};
        v->doubleStep = false;
        v->castling = false;
        v->bareKingValue = -VALUE_MATE;
        v->bareKingMove = true;
        v->stalemateValue = -VALUE_MATE;
        v->nMoveRule = 70;
        return v;
    }
    Variant* amazon_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBR..............AKpnbr..............ak";
        v->remove_piece(QUEEN);
        v->add_piece(AMAZON, 'a');
        v->startFen = "rnbakbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBAKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {AMAZON, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* hoppelpoppel_variant() {
        Variant* v = chess_variant();
        v->remove_piece(KNIGHT);
        v->remove_piece(BISHOP);
        v->add_piece(KNIBIS, 'n');
        v->add_piece(BISKNI, 'b');
        v->promotionPieceTypes = {QUEEN, ROOK, BISKNI, KNIBIS};
        return v;
    }
    Variant* newzealand_variant() {
        Variant* v = chess_variant();
        v->remove_piece(ROOK);
        v->remove_piece(KNIGHT);
        v->add_piece(ROOKNI, 'r');
        v->add_piece(KNIROO, 'n');
        v->castlingRookPiece = ROOKNI;
        v->promotionPieceTypes = {QUEEN, ROOKNI, BISHOP, KNIROO};
        return v;
    }
    Variant* kingofthehill_variant() {
        Variant* v = fairy_variant_base();
        v->flagPiece = KING;
        v->whiteFlag = make_bitboard(SQ_D4, SQ_E4, SQ_D5, SQ_E5);
        v->blackFlag = make_bitboard(SQ_D4, SQ_E4, SQ_D5, SQ_E5);
        v->flagMove = false;
        return v;
    }
    Variant* racingkings_variant() {
        Variant* v = fairy_variant_base();
        v->startFen = "8/8/8/8/8/8/krbnNBRK/qrbnNBRQ w - - 0 1";
        v->flagPiece = KING;
        v->whiteFlag = Rank8BB;
        v->blackFlag = Rank8BB;
        v->flagMove = true;
        v->castling = false;
        v->checking = false;
        return v;
    }
    Variant* knightmate_variant() {
        Variant* v = fairy_variant_base();
        v->add_piece(COMMONER, 'm');
        v->remove_piece(KNIGHT);
        v->startFen = "rmbqkbmr/pppppppp/8/8/8/8/PPPPPPPP/RMBQKBMR w KQkq - 0 1";
        v->kingType = KNIGHT;
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP};
        return v;
    }
    Variant* losers_variant() {
        Variant* v = fairy_variant_base();
        v->checkmateValue = VALUE_MATE;
        v->stalemateValue = VALUE_MATE;
        v->bareKingValue = VALUE_MATE;
        v->bareKingMove = false;
        v->mustCapture = true;
        return v;
    }
    Variant* giveaway_variant() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "giveaway";
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT};
        v->stalemateValue = VALUE_MATE;
        v->extinctionValue = VALUE_MATE;
        v->extinctionPieceTypes = {ALL_PIECES};
        v->mustCapture = true;
        return v;
    }
    Variant* antichess_variant() {
        Variant* v = giveaway_variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->castling = false;
        return v;
    }
    Variant* suicide_variant() {
        Variant* v = antichess_variant();
        v->stalematePieceCount = true;
        return v;
    }
    Variant* codrus_variant() {
        Variant* v = giveaway_variant();
        v->promotionPieceTypes = {QUEEN, ROOK, BISHOP, KNIGHT};
        v->extinctionPieceTypes = {COMMONER};
        return v;
    }
    Variant* extinction_variant() {
        Variant* v = fairy_variant_base();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT};
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
        return v;
    }
    Variant* kinglet_variant() {
        Variant* v = extinction_variant();
        v->promotionPieceTypes = {COMMONER};
        v->extinctionPieceTypes = {PAWN};
        return v;
    }
    Variant* horde_variant() {
        Variant* v = fairy_variant_base();
        v->startFen = "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq - 0 1";
        v->firstRankDoubleSteps = true;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {ALL_PIECES};
        return v;
    }
    Variant* threecheck_variant() {
        Variant* v = fairy_variant_base();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+3 0 1";
        v->checkCounting = true;
        return v;
    }
    Variant* fivecheck_variant() {
        Variant* v = threecheck_variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 5+5 0 1";
        return v;
    }
    Variant* crazyhouse_variant() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "crazyhouse";
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        return v;
    }
    Variant* loop_variant() {
        Variant* v = crazyhouse_variant();
        v->dropLoop = true;
        return v;
    }
    Variant* chessgi_variant() {
        Variant* v = loop_variant();
        v->firstRankPawnDrops = true;
        return v;
    }
    Variant* bughouse_variant() {
        Variant* v = crazyhouse_variant();
        v->variantTemplate = "bughouse";
        v->twoBoards = true;
        v->capturesToHand = false;
        return v;
    }
    Variant* pocketknight_variant() {
        Variant* v = chess_variant();
        v->variantTemplate = "bughouse";
        v->pocketSize = 2;
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[Nn] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = false;
        return v;
    }
    Variant* placement_variant() {
        Variant* v = chess_variant();
        v->variantTemplate = "bughouse";
        v->startFen = "8/pppppppp/8/8/8/8/PPPPPPPP/8[KQRRBBNNkqrrbbnn] w - - 0 1";
        v->mustDrop = true;
        v->pieceDrops = true;
        v->capturesToHand = false;
        v->whiteDropRegion = Rank1BB;
        v->blackDropRegion = Rank8BB;
        v->dropOppositeColoredBishop = true;
        v->castlingDroppedPiece = true;
        return v;
    }
    Variant* sittuyin_variant() {
        Variant* v = makruk_variant();
        v->variantTemplate = "bughouse";
        v->pieceToCharTable = "PN.R.F....SKpn.r.f....sk";
        v->startFen = "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1";
        v->remove_piece(MET);
        v->add_piece(MET, 'f');
        v->mustDrop = true;
        v->pieceDrops = true;
        v->capturesToHand = false;
        v->whiteDropRegion = Rank1BB | Rank2BB | Rank3BB;
        v->blackDropRegion = Rank8BB | Rank7BB | Rank6BB;
        v->sittuyinRookDrop = true;
        v->promotionRank = RANK_1; // no regular promotions
        v->sittuyinPromotion = true;
        v->promotionLimit[FERS] = 1;
        v->immobilityIllegal = false;
        v->countingRule = ASEAN_COUNTING;
        v->nMoveRule = 50;
        return v;
    }
    Variant* seirawan_variant() {
        Variant* v = chess_variant();
        v->variantTemplate = "seirawan";
        v->pieceToCharTable = "PNBRQ.E..........H...Kpnbrq.e..........h...k";
        v->add_piece(ARCHBISHOP, 'h');
        v->add_piece(CHANCELLOR, 'e');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1";
        v->gating = true;
        v->seirawanGating = true;
        v->promotionPieceTypes = {ARCHBISHOP, CHANCELLOR, QUEEN, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* shouse_variant() {
        Variant* v = seirawan_variant();
        v->variantTemplate = "crazyhouse";
        v->pieceDrops = true;
        v->capturesToHand = true;
        return v;
    }
    Variant* minishogi_variant_base() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "shogi";
        v->maxRank = RANK_5;
        v->maxFile = FILE_E;
        v->reset_pieces();
        v->add_piece(SHOGI_PAWN, 'p');
        v->add_piece(SILVER, 's');
        v->add_piece(GOLD, 'g');
        v->add_piece(BISHOP, 'b');
        v->add_piece(DRAGON_HORSE, 'h');
        v->add_piece(ROOK, 'r');
        v->add_piece(DRAGON, 'd');
        v->add_piece(KING, 'k');
        v->startFen = "rbsgk/4p/5/P4/KGSBR[-] w 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        v->promotionRank = RANK_5;
        v->promotionPieceTypes = {};
        v->doubleStep = false;
        v->castling = false;
        v->promotedPieceType[SHOGI_PAWN] = GOLD;
        v->promotedPieceType[SILVER]     = GOLD;
        v->promotedPieceType[BISHOP]     = DRAGON_HORSE;
        v->promotedPieceType[ROOK]       = DRAGON;
        v->shogiDoubledPawn = false;
        v->immobilityIllegal = true;
        v->shogiPawnDropMateIllegal = true;
        v->stalemateValue = -VALUE_MATE;
        v->nFoldRule = 4;
        v->nMoveRule = 0;
        v->perpetualCheckIllegal = true;
        return v;
    }
    Variant* minishogi_variant() {
        Variant* v = minishogi_variant_base();
        v->pieceToCharTable = "P.BR.S...G.+.++.+Kp.br.s...g.+.++.+k";
        v->pocketSize = 5;
        v->nFoldValue = -VALUE_MATE;
        v->nFoldValueAbsolute = true;
        return v;
    }
    Variant* kyotoshogi_variant() {
        Variant* v = minishogi_variant_base();
        v->add_piece(LANCE, 'l');
        v->add_piece(SHOGI_KNIGHT, 'n');
        v->startFen = "p+nks+l/5/5/5/+LSK+NP[-] w 0 1";
        v->promotionRank = RANK_1;
        v->mandatoryPiecePromotion = true;
        v->pieceDemotion = true;
        v->dropPromoted = true;
        v->promotedPieceType[LANCE]        = GOLD;
        v->promotedPieceType[SILVER]       = BISHOP;
        v->promotedPieceType[SHOGI_KNIGHT] = GOLD;
        v->promotedPieceType[SHOGI_PAWN]   = ROOK;
        v->promotedPieceType[GOLD]         = NO_PIECE_TYPE;
        v->promotedPieceType[BISHOP]       = NO_PIECE_TYPE;
        v->promotedPieceType[ROOK]         = NO_PIECE_TYPE;
        v->immobilityIllegal = false;
        v->shogiPawnDropMateIllegal = false;
        v->shogiDoubledPawn = true;
        return v;
    }
    Variant* microshogi_variant() {
        Variant* v = kyotoshogi_variant();
        v->maxFile = FILE_D;
        v->startFen = "kb+r+l/p3/4/3P/+L+RBK[-] w 0 1";
        v->promotionRank = RANK_1;
        v->piecePromotionOnCapture = true;
        v->promotedPieceType[LANCE]        = SILVER;
        v->promotedPieceType[BISHOP]       = GOLD;
        v->promotedPieceType[ROOK]         = GOLD;
        v->promotedPieceType[SHOGI_PAWN]   = SHOGI_KNIGHT;
        v->promotedPieceType[SILVER]       = NO_PIECE_TYPE;
        v->promotedPieceType[GOLD]         = NO_PIECE_TYPE;
        v->promotedPieceType[SHOGI_KNIGHT] = NO_PIECE_TYPE;
        return v;
    }
    Variant* dobutsu_variant() {
        Variant* v = minishogi_variant_base();
        v->pieceToCharTable = "C....E...G.+.....Lc....e...g.+.....l";
        v->pocketSize = 3;
        v->maxRank = RANK_4;
        v->maxFile = FILE_C;
        v->reset_pieces();
        v->add_piece(SHOGI_PAWN, 'c');
        v->add_piece(GOLD, 'h');
        v->add_piece(FERS, 'e');
        v->add_piece(WAZIR, 'g');
        v->add_piece(KING, 'l');
        v->startFen = "gle/1c1/1C1/ELG[-] w 0 1";
        v->promotionRank = RANK_4;
        v->immobilityIllegal = false;
        v->shogiPawnDropMateIllegal = false;
        v->flagPiece = KING;
        v->whiteFlag = Rank4BB;
        v->blackFlag = Rank1BB;
        v->shogiDoubledPawn = true;
        return v;
    }
    Variant* gorogoroshogi_variant() {
        Variant* v = minishogi_variant_base();
        v->pieceToCharTable = "P....S...G.+....+Kp....s...g.+....+k";
        v->pocketSize = 3;
        v->maxRank = RANK_6;
        v->maxFile = FILE_E;
        v->startFen = "sgkgs/5/1ppp1/1PPP1/5/SGKGS[-] w 0 1";
        v->promotionRank = RANK_5;
        return v;
    }
    Variant* judkinsshogi_variant() {
        Variant* v = minishogi_variant_base();
        v->pieceToCharTable = "PNBR.S...G.++++.+Kpnbr.s...g.++++.+k";
        v->maxRank = RANK_6;
        v->maxFile = FILE_F;
        v->add_piece(SHOGI_KNIGHT, 'n');
        v->startFen = "rbnsgk/5p/6/6/P5/KGSNBR[-] w 0 1";
        v->promotionRank = RANK_5;
        v->promotedPieceType[SHOGI_KNIGHT] = GOLD;
        return v;
    }
    Variant* euroshogi_variant() {
        Variant* v = minishogi_variant_base();
        v->pieceToCharTable = "PNBR.....G.++++Kpnbr.....g.++++k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_H;
        v->add_piece(EUROSHOGI_KNIGHT, 'n');
        v->startFen = "1nbgkgn1/1r4b1/pppppppp/8/8/PPPPPPPP/1B4R1/1NGKGBN1[-] w 0 1";
        v->promotionRank = RANK_6;
        v->promotedPieceType[EUROSHOGI_KNIGHT] = GOLD;
        v->mandatoryPiecePromotion = true;
        return v;
    }
    Variant* losalamos_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "PN.RQ................Kpn.rq................k";
        v->maxRank = RANK_6;
        v->maxFile = FILE_F;
        v->remove_piece(BISHOP);
        v->startFen = "rnqknr/pppppp/6/6/PPPPPP/RNQKNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {QUEEN, ROOK, KNIGHT};
        v->doubleStep = false;
        v->castling = false;
        return v;
    }
    Variant* gardner_variant() {
        Variant* v = fairy_variant_base();
        v->maxRank = RANK_5;
        v->maxFile = FILE_E;
        v->startFen = "rnbqk/ppppp/5/PPPPP/RNBQK w - - 0 1";
        v->promotionRank = RANK_5;
        v->doubleStep = false;
        v->castling = false;
        return v;
    }
    Variant* almost_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBR............CKpnbr............ck";
        v->remove_piece(QUEEN);
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbckbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBCKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {CHANCELLOR, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* chigorin_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBR............CKpnbrq............k";
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rbbqkbbr/pppppppp/8/8/8/8/PPPPPPPP/RNNCKNNR w KQkq - 0 1";
        v->promotionPieceTypes = {QUEEN, CHANCELLOR, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* shatar_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBR..........J......Kpnbr..........j......k";
        v->remove_piece(QUEEN);
        v->add_piece(BERS, 'j');
        v->startFen = "rnbjkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBJKBNR w - - 0 1";
        v->promotionPieceTypes = {BERS};
        v->doubleStep = false;
        v->castling = false;
        v->bareKingValue = VALUE_DRAW; // Robado
        v->shatarMateRule = true;
        return v;
    }
    Variant* clobber_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "P.................p.................";
        v->maxRank = RANK_6;
        v->maxFile = FILE_E;
        v->reset_pieces();
        v->add_piece(CLOBBER_PIECE, 'p');
        v->startFen = "PpPpP/pPpPp/PpPpP/pPpPp/PpPpP/pPpPp w 0 1";
        v->promotionPieceTypes = {};
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->immobilityIllegal = false;
        return v;
    }
    Variant* breakthrough_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "P.................p.................";
        v->reset_pieces();
        v->add_piece(BREAKTHROUGH_PIECE, 'p');
        v->startFen = "pppppppp/pppppppp/8/8/8/8/PPPPPPPP/PPPPPPPP w 0 1";
        v->promotionPieceTypes = {};
        v->firstRankDoubleSteps = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece = BREAKTHROUGH_PIECE;
        v->whiteFlag = Rank8BB;
        v->blackFlag = Rank1BB;
        return v;
    }
    Variant* minixiangqi_variant() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "xiangqi";
        v->pieceToCharTable = "PN.R.....K.C.pn.r.....k.c.";
        v->maxRank = RANK_7;
        v->maxFile = FILE_G;
        v->reset_pieces();
        v->add_piece(ROOK, 'r');
        v->add_piece(HORSE, 'n', 'h');
        v->add_piece(KING, 'k');
        v->add_piece(CANNON, 'c');
        v->add_piece(SOLDIER, 'p');
        v->startFen = "rcnkncr/p1ppp1p/7/7/7/P1PPP1P/RCNKNCR w - - 0 1";
        Bitboard white_castle = make_bitboard(SQ_C1, SQ_D1, SQ_E1,
                                              SQ_C2, SQ_D2, SQ_E2,
                                              SQ_C3, SQ_D3, SQ_E3);
        Bitboard black_castle = make_bitboard(SQ_C5, SQ_D5, SQ_E5,
                                              SQ_C6, SQ_D6, SQ_E6,
                                              SQ_C7, SQ_D7, SQ_E7);
        v->mobilityRegion[WHITE][KING] = white_castle;
        v->mobilityRegion[BLACK][KING] = black_castle;
        v->kingType = WAZIR;
        v->promotionPieceTypes = {};
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        //v->nFoldValue = VALUE_MATE;
        v->perpetualCheckIllegal = true;
        v->flyingGeneral = true;
        return v;
    }
#ifdef LARGEBOARDS
    Variant* shogi_variant() {
        Variant* v = minishogi_variant_base();
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->add_piece(LANCE, 'l');
        v->add_piece(SHOGI_KNIGHT, 'n');
        v->startFen = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w 0 1";
        v->promotionRank = RANK_7;
        v->promotedPieceType[LANCE]        = GOLD;
        v->promotedPieceType[SHOGI_KNIGHT] = GOLD;
        return v;
    }
    Variant* capablanca_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ..AC............Kpnbrq..ac............k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq - 0 1";
        v->promotionPieceTypes = {ARCHBISHOP, CHANCELLOR, QUEEN, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* capahouse_variant() {
        Variant* v = capablanca_variant();
        v->startFen = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        v->endgameEval = false;
        return v;
    }
    Variant* caparandom_variant() {
        Variant* v = capablanca_variant();
        v->chess960 = true;
        return v;
    }
    Variant* gothic_variant() {
        Variant* v = capablanca_variant();
        v->startFen = "rnbqckabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQCKABNR w KQkq - 0 1";
        return v;
    }
    Variant* janus_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ............J...Kpnbrq............J...k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_B;
        v->add_piece(ARCHBISHOP, 'j');
        v->startFen = "rjnbkqbnjr/pppppppppp/10/10/10/10/PPPPPPPPPP/RJNBKQBNJR w KQkq - 0 1";
        v->promotionPieceTypes = {ARCHBISHOP, QUEEN, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* modern_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ..M.............Kpnbrq..m.............k";
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->promotionRank = RANK_9;
        v->castlingKingsideFile = FILE_G;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(ARCHBISHOP, 'm');
        v->startFen = "rnbqkmbnr/ppppppppp/9/9/9/9/9/PPPPPPPPP/RNBMKQBNR w KQkq - 0 1";
        v->promotionPieceTypes = {ARCHBISHOP, QUEEN, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* chancellor_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ...........CKpnbrq...........ck";
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->promotionRank = RANK_9;
        v->castlingKingsideFile = FILE_G;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbqkcnbr/ppppppppp/9/9/9/9/9/PPPPPPPPP/RNBQKCNBR w KQkq - 0 1";
        v->promotionPieceTypes = {CHANCELLOR, QUEEN, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* embassy_variant() {
        Variant* v = capablanca_variant();
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_B;
        v->startFen = "rnbqkcabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQKCABNR w KQkq - 0 1";
        return v;
    }
    Variant* centaur_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ...............CKpnbrq...............ck";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(CENTAUR, 'c');
        v->startFen = "rcnbqkbncr/pppppppppp/10/10/10/10/PPPPPPPPPP/RCNBQKBNCR w KQkq - 0 1";
        v->promotionPieceTypes = {CENTAUR, QUEEN, ROOK, BISHOP, KNIGHT};
        return v;
    }
    Variant* jesonmor_variant() {
        Variant* v = fairy_variant_base();
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->reset_pieces();
        v->add_piece(KNIGHT, 'n');
        v->startFen = "nnnnnnnnn/9/9/9/9/9/9/9/NNNNNNNNN w - - 0 1";
        v->promotionPieceTypes = {};
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece = KNIGHT;
        v->whiteFlag = make_bitboard(SQ_E5);
        v->blackFlag = make_bitboard(SQ_E5);
        v->flagMove = true;
        return v;
    }
    Variant* courier_variant() {
        Variant* v = fairy_variant_base();
        v->maxRank = RANK_8;
        v->maxFile = FILE_L;
        v->remove_piece(QUEEN);
        v->add_piece(ALFIL, 'e');
        v->add_piece(FERS, 'f');
        v->add_piece(COMMONER, 'm');
        v->add_piece(WAZIR, 'w');
        v->startFen = "rnebmk1wbenr/1ppppp1pppp1/6f5/p5p4p/P5P4P/6F5/1PPPPP1PPPP1/RNEBMK1WBENR w - - 0 1";
        v->promotionPieceTypes = {FERS};
        v->doubleStep = false;
        v->castling = false;
        v->bareKingValue = -VALUE_MATE;
        v->bareKingMove = true;
        v->stalemateValue = -VALUE_MATE;
        return v;
    }
    Variant* grand_variant() {
        Variant* v = fairy_variant_base();
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R w - - 0 1";
        v->promotionPieceTypes = {ARCHBISHOP, CHANCELLOR, QUEEN, ROOK, BISHOP, KNIGHT};
        v->promotionRank = RANK_8;
        v->promotionLimit[ARCHBISHOP] = 1;
        v->promotionLimit[CHANCELLOR] = 1;
        v->promotionLimit[QUEEN] = 1;
        v->promotionLimit[ROOK] = 2;
        v->promotionLimit[BISHOP] = 2;
        v->promotionLimit[KNIGHT] = 2;
        v->mandatoryPawnPromotion = false;
        v->immobilityIllegal = true;
        v->doubleStepRank = RANK_3;
        v->castling = false;
        return v;
    }
    Variant* shako_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "PNBRQ.E....C.........Kpnbrq.e....c.........k";
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->add_piece(FERS_ALFIL, 'e');
        v->add_piece(CANNON, 'c');
        v->startFen = "c8c/ernbqkbnre/pppppppppp/10/10/10/10/PPPPPPPPPP/ERNBQKBNRE/C8C w KQkq - 0 1";
        v->promotionPieceTypes = { QUEEN, ROOK, BISHOP, KNIGHT, CANNON, FERS_ALFIL };
        v->promotionRank = RANK_10;
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_D;
        v->castlingRank = RANK_2;
        v->doubleStepRank = RANK_3;
        return v;
    }
    Variant* clobber10_variant() {
        Variant* v = clobber_variant();
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->startFen = "PpPpPpPpPp/pPpPpPpPpP/PpPpPpPpPp/pPpPpPpPpP/PpPpPpPpPp/"
                      "pPpPpPpPpP/PpPpPpPpPp/pPpPpPpPpP/PpPpPpPpPp/pPpPpPpPpP w 0 1";
        return v;
    }
    Variant* xiangqi_variant() {
        Variant* v = minixiangqi_variant();
        v->pieceToCharTable = "PN.R.AB..K.C..........pn.r.ab..k.c..........";
        v->maxRank = RANK_10;
        v->maxFile = FILE_I;
        v->add_piece(ELEPHANT, 'b', 'e');
        v->add_piece(FERS, 'a');
        v->startFen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";
        Bitboard white_castle = make_bitboard(SQ_D1, SQ_E1, SQ_F1,
                                              SQ_D2, SQ_E2, SQ_F2,
                                              SQ_D3, SQ_E3, SQ_F3);
        Bitboard black_castle = make_bitboard(SQ_D8, SQ_E8, SQ_F8,
                                              SQ_D9, SQ_E9, SQ_F9,
                                              SQ_D10, SQ_E10, SQ_F10);
        v->mobilityRegion[WHITE][KING] = white_castle;
        v->mobilityRegion[BLACK][KING] = black_castle;
        v->mobilityRegion[WHITE][FERS] = white_castle;
        v->mobilityRegion[BLACK][FERS] = black_castle;
        v->mobilityRegion[WHITE][ELEPHANT] = Rank1BB | Rank2BB | Rank3BB | Rank4BB | Rank5BB;
        v->mobilityRegion[BLACK][ELEPHANT] = Rank6BB | Rank7BB | Rank8BB | Rank9BB | Rank10BB;
        v->xiangqiSoldier = true;
        return v;
    }
    // Manchu/Yitong chess
    // https://en.wikipedia.org/wiki/Manchu_chess
    Variant* manchu_variant() {
        Variant* v = xiangqi_variant();
        v->pieceToCharTable = "PN.R.AB..K.C....M.....pn.r.ab..k.c..........";
        v->add_piece(BANNER, 'm');
        v->startFen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/9/9/M1BAKAB2 w - - 0 1";
        return v;
    }
    // Supply chess
    // https://en.wikipedia.org/wiki/Xiangqi#Variations
    Variant* supply_variant() {
        Variant* v = xiangqi_variant();
        v->variantTemplate = "bughouse";
        v->startFen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR[] w - - 0 1";
        v->twoBoards = true;
        v->pieceDrops = true;
        v->dropChecks = false;
        v->whiteDropRegion = v->mobilityRegion[WHITE][ELEPHANT];
        v->blackDropRegion = v->mobilityRegion[BLACK][ELEPHANT];
        return v;
    }
#endif

} // namespace


/// VariantMap::init() is called at startup to initialize all predefined variants

void VariantMap::init() {
    // Add to UCI_Variant option
    add("chess", chess_variant());
    add("normal", chess_variant());
    add("fischerandom", chess960_variant());
    add("nocastle", nocastle_variant());
    add("fairy", fairy_variant()); // fairy variant used for endgame code initialization
    add("makruk", makruk_variant());
    add("cambodian", cambodian_variant());
    add("karouk", karouk_variant());
    add("asean", asean_variant());
    add("ai-wok", aiwok_variant());
    add("shatranj", shatranj_variant());
    add("amazon", amazon_variant());
    add("hoppelpoppel", hoppelpoppel_variant());
    add("newzealand", newzealand_variant());
    add("kingofthehill", kingofthehill_variant());
    add("racingkings", racingkings_variant());
    add("knightmate", knightmate_variant());
    add("losers", losers_variant());
    add("giveaway", giveaway_variant());
    add("antichess", antichess_variant());
    add("suicide", suicide_variant());
    add("codrus", codrus_variant());
    add("extinction", extinction_variant());
    add("kinglet", kinglet_variant());
    add("horde", horde_variant());
    add("3check", threecheck_variant());
    add("5check", fivecheck_variant());
    add("crazyhouse", crazyhouse_variant());
    add("loop", loop_variant());
    add("chessgi", chessgi_variant());
    add("bughouse", bughouse_variant());
    add("pocketknight", pocketknight_variant());
    add("placement", placement_variant());
    add("sittuyin", sittuyin_variant());
    add("seirawan", seirawan_variant());
    add("shouse", shouse_variant());
    add("minishogi", minishogi_variant());
    add("mini", minishogi_variant());
    add("kyotoshogi", kyotoshogi_variant());
    add("micro", microshogi_variant());
    add("dobutsu", dobutsu_variant());
    add("gorogoro", gorogoroshogi_variant());
    add("judkins", judkinsshogi_variant());
    add("euroshogi", euroshogi_variant());
    add("losalamos", losalamos_variant());
    add("gardner", gardner_variant());
    add("almost", almost_variant());
    add("chigorin", chigorin_variant());
    add("shatar", shatar_variant());
    add("clobber", clobber_variant());
    add("breakthrough", breakthrough_variant());
    add("minixiangqi", minixiangqi_variant());
#ifdef LARGEBOARDS
    add("shogi", shogi_variant());
    add("capablanca", capablanca_variant());
    add("capahouse", capahouse_variant());
    add("caparandom", caparandom_variant());
    add("gothic", gothic_variant());
    add("janus", janus_variant());
    add("modern", modern_variant());
    add("chancellor", chancellor_variant());
    add("embassy", embassy_variant());
    add("centaur", centaur_variant());
    add("jesonmor", jesonmor_variant());
    add("courier", courier_variant());
    add("grand", grand_variant());
    add("shako", shako_variant());
    add("clobber10", clobber10_variant());
    add("xiangqi", xiangqi_variant());
    add("manchu", manchu_variant());
    add("supply", supply_variant());
#endif
}


/// VariantMap::parse reads variants from an INI-style configuration file.

template <bool DoCheck>
void VariantMap::parse(std::string path) {
    if (path.empty() || path == "<empty>")
        return;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Unable to open file " << path << std::endl;
        return;
    }
    std::string variant, variant_template, key, value, input;
    while (file.peek() != '[' && std::getline(file, input)) {}

    std::vector<std::string> varsToErase = {};
    while (file.get() && std::getline(std::getline(file, variant, ']'), input))
    {
        // Extract variant template, if specified
        if (!std::getline(std::getline(std::stringstream(variant), variant, ':'), variant_template))
            variant_template = "";

        // Read variant rules
        Config attribs = {};
        while (file.peek() != '[' && std::getline(file, input))
        {
            std::stringstream ss(input);
            if (ss.peek() != '#' && std::getline(std::getline(ss, key, '=') >> std::ws, value) && !key.empty())
                attribs[key.erase(key.find_last_not_of(" ") + 1)] = value;
        }

        // Create variant
        if (variants.find(variant) != variants.end())
            std::cerr << "Variant '" << variant << "' already exists." << std::endl;
        else if (!variant_template.empty() && variants.find(variant_template) == variants.end())
            std::cerr << "Variant template '" << variant_template << "' does not exist." << std::endl;
        else
        {
            if (DoCheck)
                std::cerr << "Parsing variant: " << variant << std::endl;
            Variant* v = !variant_template.empty() ? VariantParser<DoCheck>(attribs).parse(new Variant(*variants.find(variant_template)->second))
                                                   : VariantParser<DoCheck>(attribs).parse();
            if (v->maxFile <= FILE_MAX && v->maxRank <= RANK_MAX)
            {
                add(variant, v);
                // In order to allow inheritance, we need to temporarily add configured variants
                // even when only checking them, but we remove them later after parsing is finished.
                if (DoCheck)
                    varsToErase.push_back(variant);
            }
            else
                delete v;
        }
    }
    file.close();
    // Clean up temporary variants
    for (std::string tempVar : varsToErase)
    {
        delete variants[tempVar];
        variants.erase(tempVar);
    }
}

template void VariantMap::parse<true>(std::string path);
template void VariantMap::parse<false>(std::string path);

void VariantMap::add(std::string s, const Variant* v) {
  insert(std::pair<std::string, const Variant*>(s, v));
}

void VariantMap::clear_all() {
  for (auto const& element : *this)
      delete element.second;
  clear();
}

std::vector<std::string> VariantMap::get_keys() {
  std::vector<std::string> keys;
  for (auto const& element : *this)
      keys.push_back(element.first);
  return keys;
}
