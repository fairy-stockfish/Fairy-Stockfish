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
    // Armageddon Chess
    // https://en.wikipedia.org/wiki/Fast_chess#Armageddon
    Variant* armageddon_variant() {
        Variant* v = fairy_variant_base();
        v->materialCounting = BLACK_DRAW_ODDS;
        return v;
    }
    Variant* fairy_variant() {
        Variant* v = chess_variant();
        v->add_piece(SILVER, 's');
        v->add_piece(FERS, 'f');
        return v;
    }
    // Makruk (Thai Chess)
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
    // Makpong (Defensive Chess)
    // A Makruk variant used for tie-breaks
    // https://www.mayhematics.com/v/vol8/vc64b.pdf, p. 177
    Variant* makpong_variant() {
        Variant* v = makruk_variant();
        v->makpongRule = true;
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
        v->pieceToCharTable = "PN.R.QB....Kpn.r.qb....k";
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(ALFIL, 'b');
        v->add_piece(FERS, 'q');
        v->startFen = "rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w - - 0 1";
        v->promotionPieceTypes = {FERS};
        v->doubleStep = false;
        v->castling = false;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionClaim = true;
        v->extinctionPieceTypes = {ALL_PIECES};
        v->extinctionPieceCount = 1;
        v->extinctionOpponentPieceCount = 2;
        v->stalemateValue = -VALUE_MATE;
        v->nMoveRule = 70;
        return v;
    }
    // Chaturanga
    // https://en.wikipedia.org/wiki/Chaturanga
    // Rules as used on chess.com
    Variant* chaturanga_variant() {
        Variant* v = shatranj_variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->extinctionValue = VALUE_NONE;
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
        v->whiteFlag = (Rank4BB | Rank5BB) & (FileDBB | FileEBB);
        v->blackFlag = (Rank4BB | Rank5BB) & (FileDBB | FileEBB);
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
        v->castlingKingPiece = KNIGHT;
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP};
        return v;
    }
    Variant* losers_variant() {
        Variant* v = fairy_variant_base();
        v->checkmateValue = VALUE_MATE;
        v->stalemateValue = VALUE_MATE;
        v->extinctionValue = VALUE_MATE;
        v->extinctionPieceTypes = {ALL_PIECES};
        v->extinctionPieceCount = 1;
        v->mustCapture = true;
        return v;
    }
    Variant* giveaway_variant() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "giveaway";
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece = COMMONER;
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
        v->castlingKingPiece = COMMONER;
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
    // Three Kings Chess
    // https://github.com/cutechess/cutechess/blob/master/projects/lib/src/board/threekingsboard.h
    Variant* threekings_variant() {
        Variant* v = fairy_variant_base();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece = COMMONER;
        v->startFen = "knbqkbnk/pppppppp/8/8/8/8/PPPPPPPP/KNBQKBNK w - - 0 1";
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {COMMONER};
        v->extinctionPieceCount = 2;
        return v;
    }
    // Horde chess
    // https://en.wikipedia.org/wiki/Dunsany%27s_chess#Horde_chess
    Variant* horde_variant() {
        Variant* v = fairy_variant_base();
        v->startFen = "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq - 0 1";
        v->doubleStepRankMin = RANK_1;
        v->enPassantRegion = Rank3BB | Rank6BB; // exclude en passant on second rank
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {ALL_PIECES};
        return v;
    }
    // Atomic chess without checks (ICC rules)
    // https://www.chessclub.com/help/atomic
    Variant* nocheckatomic_variant() {
        Variant* v = fairy_variant_base();
        v->variantTemplate = "atomic";
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {COMMONER};
        v->blastOnCapture = true;
        return v;
    }
    // Atomic chess
    // https://en.wikipedia.org/wiki/Atomic_chess
    Variant* atomic_variant() {
        Variant* v = nocheckatomic_variant();
        v->extinctionPseudoRoyal = true;
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
        v->stalemateValue = -VALUE_MATE;
        return v;
    }
    // Koedem (Bughouse variant)
    // http://schachclub-oetigheim.de/wp-content/uploads/2016/04/Koedem-rules.pdf
    Variant* koedem_variant() {
        Variant* v = bughouse_variant();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece = COMMONER;
        v->mustDrop = true;
        v->mustDropType = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {COMMONER};
        v->extinctionOpponentPieceCount = 2; // own all kings/commoners
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
        v->dropNoDoubled = SHOGI_PAWN;
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
        v->dropNoDoubled = NO_PIECE_TYPE;
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
        v->dropNoDoubled = NO_PIECE_TYPE;
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
        v->extinctionValue = VALUE_DRAW; // Robado
        v->extinctionPieceTypes = {ALL_PIECES};
        v->extinctionPieceCount = 1;
        v->shatarMateRule = true;
        return v;
    }
    Variant* coregal_variant() {
        Variant* v = fairy_variant();
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {QUEEN};
        v->extinctionPseudoRoyal = true;
        v->extinctionPieceCount = 64; // no matter how many queens, all are royal
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
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece = BREAKTHROUGH_PIECE;
        v->whiteFlag = Rank8BB;
        v->blackFlag = Rank1BB;
        return v;
    }
    Variant* ataxx_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "P.................p.................";
        v->maxRank = RANK_7;
        v->maxFile = FILE_G;
        v->reset_pieces();
        v->add_piece(ATAXX_PIECE, 'p');
        v->startFen = "P5p/7/7/7/7/7/p5P[PPPPPPPPPPPPPPPPPPPPPPPPPppppppppppppppppppppppppp] w 0 1";
        v->promotionPieceTypes = {};
        v->pieceDrops = true;
        v->doubleStep = false;
        v->castling = false;
        v->immobilityIllegal = false;
        v->stalemateValue = -VALUE_MATE;
        v->stalematePieceCount = true;
        v->passOnStalemate = true;
        v->enclosingDrop = ATAXX;
        v->flipEnclosedPieces = ATAXX;
        v->materialCounting = UNWEIGHTED_MATERIAL;
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
        v->mobilityRegion[WHITE][KING] = (Rank1BB | Rank2BB | Rank3BB) & (FileCBB | FileDBB | FileEBB);
        v->mobilityRegion[BLACK][KING] = (Rank5BB | Rank6BB | Rank7BB) & (FileCBB | FileDBB | FileEBB);
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
        v->pieceToCharTable = "PNBRQ............J...Kpnbrq............j...k";
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
        v->extinctionValue = -VALUE_MATE;
        v->extinctionClaim = true;
        v->extinctionPieceTypes = {ALL_PIECES};
        v->extinctionPieceCount = 1;
        v->extinctionOpponentPieceCount = 2;
        v->stalemateValue = -VALUE_MATE;
        return v;
    }
    Variant* grand_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "PNBRQ..AC............Kpnbrq..ac............k";
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
        v->doubleStepRankMin = RANK_3;
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
        v->doubleStepRankMin = RANK_3;
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
#ifdef ALLVARS
    // Game of the Amazons
    // https://en.wikipedia.org/wiki/Game_of_the_Amazons
    Variant* amazons_variant() {
        Variant* v = fairy_variant_base();
        v->pieceToCharTable = "P...Q.................p...q.................";
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->reset_pieces();
        v->add_piece(QUIET_QUEEN, 'q');
        v->add_piece(IMMOBILE_PIECE, 'p');
        v->startFen = "3q2q3/10/10/q8q/10/10/Q8Q/10/10/3Q2Q3[PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPpppppppppppppppppppppppppppppppppppppppppppppp] w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->arrowGating = true;
        return v;
    }
#endif
    Variant* xiangqi_variant() {
        Variant* v = minixiangqi_variant();
        v->pieceToCharTable = "PN.R.AB..K.C..........pn.r.ab..k.c..........";
        v->maxRank = RANK_10;
        v->maxFile = FILE_I;
        v->add_piece(ELEPHANT, 'b', 'e');
        v->add_piece(FERS, 'a');
        v->startFen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";
        v->mobilityRegion[WHITE][KING] = (Rank1BB | Rank2BB | Rank3BB) & (FileDBB | FileEBB | FileFBB);
        v->mobilityRegion[BLACK][KING] = (Rank8BB | Rank9BB | Rank10BB) & (FileDBB | FileEBB | FileFBB);
        v->mobilityRegion[WHITE][FERS] = v->mobilityRegion[WHITE][KING];
        v->mobilityRegion[BLACK][FERS] = v->mobilityRegion[BLACK][KING];
        v->mobilityRegion[WHITE][ELEPHANT] = Rank1BB | Rank2BB | Rank3BB | Rank4BB | Rank5BB;
        v->mobilityRegion[BLACK][ELEPHANT] = Rank6BB | Rank7BB | Rank8BB | Rank9BB | Rank10BB;
        v->soldierPromotionRank = RANK_6;
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
    // Janggi (Korean chess)
    // https://en.wikipedia.org/wiki/Janggi
    // Official tournament rules with bikjang and material counting.
    Variant* janggi_variant() {
        Variant* v = xiangqi_variant();
        v->variantTemplate = "janggi";
        v->pieceToCharTable = ".N.R.AB.P..C.........K.n.r.ab.p..c.........k";
        v->remove_piece(FERS);
        v->remove_piece(CANNON);
        v->remove_piece(ELEPHANT);
        v->add_piece(WAZIR, 'a');
        v->add_piece(JANGGI_CANNON, 'c');
        v->add_piece(JANGGI_ELEPHANT, 'b', 'e');
        v->startFen = "rnba1abnr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RNBA1ABNR w - - 0 1";
        v->mobilityRegion[WHITE][WAZIR] = v->mobilityRegion[WHITE][KING];
        v->mobilityRegion[BLACK][WAZIR] = v->mobilityRegion[BLACK][KING];
        v->soldierPromotionRank = RANK_1;
        v->flyingGeneral = false;
        v->bikjangRule = true;
        v->materialCounting = JANGGI_MATERIAL;
        v->diagonalLines = make_bitboard(SQ_D1, SQ_F1, SQ_E2, SQ_D3, SQ_F3,
                                         SQ_D8, SQ_F8, SQ_E9, SQ_D10, SQ_F10);
        v->pass = true;
        v->nFoldValue = VALUE_DRAW;
        v->perpetualCheckIllegal = true;
        return v;
    }
    // Traditional rules of Janggi, where bikjang is a draw
    Variant* janggi_traditional_variant() {
        Variant* v = janggi_variant();
        v->bikjangRule = true;
        v->materialCounting = NO_MATERIAL_COUNTING;
        return v;
    }
    // Modern rules of Janggi, where bikjang is not considered, but material counting is.
    // The repetition rules are also adjusted for better compatibility with Kakao Janggi.
    Variant* janggi_modern_variant() {
        Variant* v = janggi_variant();
        v->bikjangRule = false;
        v->materialCounting = JANGGI_MATERIAL;
        v->moveRepetitionIllegal = true;
        v->nFoldRule = 4; // avoid nFold being triggered before move repetition
        v->nMoveRule = 100; // avoid adjudication before reaching 200 half-moves
        return v;
    }
    // Casual rules of Janggi, where bikjang and material counting are not considered
    Variant* janggi_casual_variant() {
        Variant* v = janggi_variant();
        v->bikjangRule = false;
        v->materialCounting = NO_MATERIAL_COUNTING;
        return v;
    }
#endif

} // namespace


/// VariantMap::init() is called at startup to initialize all predefined variants

void VariantMap::init() {
    // Add to UCI_Variant option
    add("chess", chess_variant()->conclude());
    add("normal", chess_variant()->conclude());
    add("fischerandom", chess960_variant()->conclude());
    add("nocastle", nocastle_variant()->conclude());
    add("armageddon", armageddon_variant()->conclude());
    add("fairy", fairy_variant()->conclude()); // fairy variant used for endgame code initialization
    add("makruk", makruk_variant()->conclude());
    add("makpong", makpong_variant()->conclude());
    add("cambodian", cambodian_variant()->conclude());
    add("karouk", karouk_variant()->conclude());
    add("asean", asean_variant()->conclude());
    add("ai-wok", aiwok_variant()->conclude());
    add("shatranj", shatranj_variant()->conclude());
    add("chaturanga", chaturanga_variant()->conclude());
    add("amazon", amazon_variant()->conclude());
    add("hoppelpoppel", hoppelpoppel_variant()->conclude());
    add("newzealand", newzealand_variant()->conclude());
    add("kingofthehill", kingofthehill_variant()->conclude());
    add("racingkings", racingkings_variant()->conclude());
    add("knightmate", knightmate_variant()->conclude());
    add("losers", losers_variant()->conclude());
    add("giveaway", giveaway_variant()->conclude());
    add("antichess", antichess_variant()->conclude());
    add("suicide", suicide_variant()->conclude());
    add("codrus", codrus_variant()->conclude());
    add("extinction", extinction_variant()->conclude());
    add("kinglet", kinglet_variant()->conclude());
    add("threekings", threekings_variant()->conclude());
    add("horde", horde_variant()->conclude());
    add("nocheckatomic", nocheckatomic_variant()->conclude());
    add("atomic", atomic_variant()->conclude());
    add("3check", threecheck_variant()->conclude());
    add("5check", fivecheck_variant()->conclude());
    add("crazyhouse", crazyhouse_variant()->conclude());
    add("loop", loop_variant()->conclude());
    add("chessgi", chessgi_variant()->conclude());
    add("bughouse", bughouse_variant()->conclude());
    add("koedem", koedem_variant()->conclude());
    add("pocketknight", pocketknight_variant()->conclude());
    add("placement", placement_variant()->conclude());
    add("sittuyin", sittuyin_variant()->conclude());
    add("seirawan", seirawan_variant()->conclude());
    add("shouse", shouse_variant()->conclude());
    add("minishogi", minishogi_variant()->conclude());
    add("mini", minishogi_variant()->conclude());
    add("kyotoshogi", kyotoshogi_variant()->conclude());
    add("micro", microshogi_variant()->conclude());
    add("dobutsu", dobutsu_variant()->conclude());
    add("gorogoro", gorogoroshogi_variant()->conclude());
    add("judkins", judkinsshogi_variant()->conclude());
    add("euroshogi", euroshogi_variant()->conclude());
    add("losalamos", losalamos_variant()->conclude());
    add("gardner", gardner_variant()->conclude());
    add("almost", almost_variant()->conclude());
    add("chigorin", chigorin_variant()->conclude());
    add("shatar", shatar_variant()->conclude());
    add("coregal", coregal_variant()->conclude());
    add("clobber", clobber_variant()->conclude());
    add("breakthrough", breakthrough_variant()->conclude());
    add("ataxx", ataxx_variant()->conclude());
    add("minixiangqi", minixiangqi_variant()->conclude());
#ifdef LARGEBOARDS
    add("shogi", shogi_variant()->conclude());
    add("capablanca", capablanca_variant()->conclude());
    add("capahouse", capahouse_variant()->conclude());
    add("caparandom", caparandom_variant()->conclude());
    add("gothic", gothic_variant()->conclude());
    add("janus", janus_variant()->conclude());
    add("modern", modern_variant()->conclude());
    add("chancellor", chancellor_variant()->conclude());
    add("embassy", embassy_variant()->conclude());
    add("centaur", centaur_variant()->conclude());
    add("jesonmor", jesonmor_variant()->conclude());
    add("courier", courier_variant()->conclude());
    add("grand", grand_variant()->conclude());
    add("shako", shako_variant()->conclude());
    add("clobber10", clobber10_variant()->conclude());
#ifdef ALLVARS
    add("amazons", amazons_variant()->conclude());
#endif
    add("xiangqi", xiangqi_variant()->conclude());
    add("manchu", manchu_variant()->conclude());
    add("supply", supply_variant()->conclude());
    add("janggi", janggi_variant()->conclude());
    add("janggitraditional", janggi_traditional_variant()->conclude());
    add("janggimodern", janggi_modern_variant()->conclude());
    add("janggicasual", janggi_casual_variant()->conclude());
#endif
}


/// VariantMap::parse_istream reads variants from an INI-style configuration input stream.

template <bool DoCheck>
void VariantMap::parse_istream(std::istream& file) {
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
                add(variant, v->conclude());
                // In order to allow inheritance, we need to temporarily add configured variants
                // even when only checking them, but we remove them later after parsing is finished.
                if (DoCheck)
                    varsToErase.push_back(variant);
            }
            else
                delete v;
        }
    }
    // Clean up temporary variants
    for (std::string tempVar : varsToErase)
    {
        delete variants[tempVar];
        variants.erase(tempVar);
    }
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
    parse_istream<DoCheck>(file);
    file.close();
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
