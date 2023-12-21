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

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "parser.h"
#include "piece.h"
#include "variant.h"

using std::string;

namespace Stockfish {

VariantMap variants; // Global object

namespace {
    // Base variant
    Variant* variant_base() {
        Variant* v = new Variant();
        return v;
    }
    // Base for all fairy variants
    Variant* chess_variant_base() {
        Variant* v = variant_base()->init();
        v->pieceToCharTable = "PNBRQ................Kpnbrq................k";
        return v;
    }
    // Standard chess
    // https://en.wikipedia.org/wiki/Chess
    Variant* chess_variant() {
        Variant* v = chess_variant_base()->init();
        v->nnueAlias = "nn-";
        return v;
    }
    // Chess960 aka Fischer random chess
    // https://en.wikipedia.org/wiki/Fischer_random_chess
    Variant* chess960_variant() {
        Variant* v = chess_variant()->init();
        v->chess960 = true;
        v->nnueAlias = "nn-";
        return v;
    }
    // Standard chess without castling
    Variant* nocastle_variant() {
        Variant* v = chess_variant()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->castling = false;
        return v;
    }
    // Armageddon Chess
    // https://en.wikipedia.org/wiki/Fast_chess#Armageddon
    Variant* armageddon_variant() {
        Variant* v = chess_variant()->init();
        v->materialCounting = BLACK_DRAW_ODDS;
        return v;
    }
    // Torpedo Chess
    // https://arxiv.org/abs/2009.04374
    Variant* torpedo_variant() {
        Variant* v = chess_variant_base()->init();
        v->doubleStepRegion[WHITE] = AllSquares;
        v->doubleStepRegion[BLACK] = AllSquares;
        return v;
    }
    // Berolina Chess
    // https://www.chessvariants.com/dpieces.dir/berlin.html
    Variant* berolina_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "mfFcfeWimfnA");
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        return v;
    }
    // Pawnsideways
    // https://arxiv.org/abs/2009.04374
    Variant* pawnsideways_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "fsmWfceFifmnD");
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        return v;
    }
    // Pawnback
    // https://arxiv.org/abs/2009.04374
    Variant* pawnback_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "fbmWfceFifmnD");
        v->mobilityRegion[WHITE][CUSTOM_PIECE_1] = (Rank2BB | Rank3BB | Rank4BB | Rank5BB | Rank6BB | Rank7BB | Rank8BB);
        v->mobilityRegion[BLACK][CUSTOM_PIECE_1] = (Rank7BB | Rank6BB | Rank5BB | Rank4BB | Rank3BB | Rank2BB | Rank1BB);
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = NO_PIECE_SET; // backwards pawn moves are reversible
        return v;
    }
    // Legan Chess
    // https://en.wikipedia.org/wiki/Legan_chess
    Variant* legan_variant() {
        Variant* v =  chess_variant_base()->init();
        v->remove_piece(PAWN);
        v->add_piece(CUSTOM_PIECE_1, 'p', "mflFcflW");
        v->promotionRegion[WHITE] = make_bitboard(SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_A7, SQ_A6, SQ_A5);
        v->promotionRegion[BLACK] = make_bitboard(SQ_E1, SQ_F1, SQ_G1, SQ_H1, SQ_H2, SQ_H3, SQ_H4);
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->startFen = "knbrp3/bqpp4/npp5/rp1p3P/p3P1PR/5PPN/4PPQB/3PRBNK w - - 0 1";
        v->doubleStep = false;
        return v;
    }
    // Pseudo-variant only used for endgame initialization
    Variant* fairy_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(SILVER, 's');
        v->add_piece(FERS, 'f');
        return v;
    }
      // Raazuva (Maldivian Chess)
    Variant* raazuvaa_variant() {
        Variant* v = chess_variant_base()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->castling = false;
        v->doubleStep = false;
        return v;
    }
    // Makruk (Thai Chess)
    // https://en.wikipedia.org/wiki/Makruk
    Variant* makruk_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "makruk";
        v->pieceToCharTable = "PN.R.M....SKpn.r.m....sk";
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 's');
        v->add_piece(MET, 'm');
        v->startFen = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - - 0 1";
        v->promotionRegion[WHITE] = Rank6BB | Rank7BB | Rank8BB;
        v->promotionRegion[BLACK] = Rank3BB | Rank2BB | Rank1BB;
        v->promotionPieceTypes[WHITE] = piece_set(MET);
        v->promotionPieceTypes[BLACK] = piece_set(MET);
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
        Variant* v = makruk_variant()->init();
        v->makpongRule = true;
        return v;
    }
    // Ouk Chatrang, Cambodian chess
    // https://en.wikipedia.org/wiki/Makruk#Cambodian_chess
    Variant* cambodian_variant() {
        Variant* v = makruk_variant()->init();
        v->startFen = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w DEde - 0 1";
        v->gating = true;
        v->cambodianMoves = true;
        v->countingRule = CAMBODIAN_COUNTING;
        v->nnueAlias = "makruk";
        return v;
    }
    // Kar Ouk
    // A variant of Cambodian chess where the first check wins
    // https://en.wikipedia.org/wiki/Makruk#Ka_Ouk
    Variant* karouk_variant() {
        Variant* v = cambodian_variant()->init();
        v->checkCounting = true;
        return v;
    }
    // ASEAN chess
    // A simplified version of south-east asian variants
    // https://aseanchess.org/laws-of-asean-chess/
    Variant* asean_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 'b');
        v->add_piece(MET, 'q');
        v->startFen = "rnbqkbnr/8/pppppppp/8/8/PPPPPPPP/8/RNBQKBNR w - - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ROOK) | KNIGHT | KHON | MET;
        v->promotionPieceTypes[BLACK] = piece_set(ROOK) | KNIGHT | KHON | MET;
        v->doubleStep = false;
        v->castling = false;
        v->countingRule = ASEAN_COUNTING;
        return v;
    }
    // Ai-wok
    // A makruk variant where the met is replaced by a super-piece moving as rook, knight, or met
    Variant* aiwok_variant() {
        Variant* v = makruk_variant()->init();
        v->pieceToCharTable = "PN.R...A..SKpn.r...a..sk";
        v->remove_piece(MET);
        v->add_piece(AIWOK, 'a');
        v->startFen = "rnsaksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKASNR w - - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AIWOK);
        v->promotionPieceTypes[BLACK] = piece_set(AIWOK);
        return v;
    }
    // Shatranj
    // The medieval form of chess, originating from chaturanga
    // https://en.wikipedia.org/wiki/Shatranj
    Variant* shatranj_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "shatranj";
        v->pieceToCharTable = "PN.R.QB....Kpn.r.qb....k";
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(ALFIL, 'b');
        v->add_piece(FERS, 'q');
        v->startFen = "rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w - - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(FERS);
        v->promotionPieceTypes[BLACK] = piece_set(FERS);
        v->doubleStep = false;
        v->castling = false;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionClaim = true;
        v->extinctionPieceTypes = piece_set(ALL_PIECES);
        v->extinctionPieceCount = 1;
        v->extinctionOpponentPieceCount = 2;
        v->stalemateValue = -VALUE_MATE;
        v->nMoveRule = 70;
        return v;
    }
    // Chaturanga
    // The actual rules of the game are not known. This reflects the rules as used on chess.com.
    // https://en.wikipedia.org/wiki/Chaturanga
    Variant* chaturanga_variant() {
        Variant* v = shatranj_variant()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->extinctionValue = VALUE_NONE;
        v->nnueAlias = "shatranj";
        return v;
    }
    // Amazon chess
    // The queen has the additional power of moving like a knight.
    // https://www.chessvariants.com/diffmove.dir/amazone.html
    Variant* amazon_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR..............AKpnbr..............ak";
        v->remove_piece(QUEEN);
        v->add_piece(AMAZON, 'a');
        v->startFen = "rnbakbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBAKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AMAZON) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(AMAZON) | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Nightrider chess
    // Knights are replaced by nightriders.
    // https://en.wikipedia.org/wiki/Nightrider_(chess)
    Variant* nightrider_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KNIGHT);
        v->add_piece(CUSTOM_PIECE_1, 'n', "NN");
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | BISHOP | CUSTOM_PIECE_1;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP | CUSTOM_PIECE_1;
        return v;
    }
    // Grasshopper chess
    // https://en.wikipedia.org/wiki/Grasshopper_chess
    Variant* grasshopper_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(CUSTOM_PIECE_1, 'g', "gQ");
        v->promotionPieceTypes[WHITE] |= CUSTOM_PIECE_1;
        v->promotionPieceTypes[BLACK] |= CUSTOM_PIECE_1;
        v->startFen = "rnbqkbnr/gggggggg/pppppppp/8/8/PPPPPPPP/GGGGGGGG/RNBQKBNR w KQkq - 0 1";
        v->doubleStep = false;
        return v;
    }
    // Hoppel-Poppel
    // A variant from Germany where knights capture like bishops and vice versa
    // https://www.chessvariants.com/diffmove.dir/hoppel-poppel.html
    Variant* hoppelpoppel_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KNIGHT);
        v->remove_piece(BISHOP);
        v->add_piece(KNIBIS, 'n');
        v->add_piece(BISKNI, 'b');
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | BISKNI | KNIBIS;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISKNI | KNIBIS;
        return v;
    }
    // New Zealand
    // Knights capture like rooks and vice versa.
    Variant* newzealand_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(ROOK);
        v->remove_piece(KNIGHT);
        v->add_piece(ROOKNI, 'r');
        v->add_piece(KNIROO, 'n');
        v->castlingRookPieces[WHITE] = v->castlingRookPieces[BLACK] = piece_set(ROOKNI);
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOKNI | BISHOP | KNIROO;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOKNI | BISHOP | KNIROO;
        return v;
    }
    // King of the Hill
    // https://lichess.org/variant/kingOfTheHill
    Variant* kingofthehill_variant() {
        Variant* v = chess_variant_base()->init();
        v->flagPiece[WHITE] = v->flagPiece[BLACK] = KING;
        v->flagRegion[WHITE] = (Rank4BB | Rank5BB) & (FileDBB | FileEBB);
        v->flagRegion[BLACK] = (Rank4BB | Rank5BB) & (FileDBB | FileEBB);
        v->flagMove = false;
        return v;
    }
    // Racing Kings
    // https://lichess.org/variant/racingKings
    Variant* racingkings_variant() {
        Variant* v = chess_variant_base()->init();
        v->startFen = "8/8/8/8/8/8/krbnNBRK/qrbnNBRQ w - - 0 1";
        v->flagPiece[WHITE] = v->flagPiece[BLACK] = KING;
        v->flagRegion[WHITE] = Rank8BB;
        v->flagRegion[BLACK] = Rank8BB;
        v->flagMove = true;
        v->castling = false;
        v->checking = false;
        return v;
    }
    // Knightmate
    // https://www.chessvariants.com/diffobjective.dir/knightmate.html
    Variant* knightmate_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(COMMONER, 'm');
        v->remove_piece(KNIGHT);
        v->startFen = "rmbqkbmr/pppppppp/8/8/8/8/PPPPPPPP/RMBQKBMR w KQkq - 0 1";
        v->kingType = KNIGHT;
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = KING;
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP;
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP;
        return v;
    }
    // Losers chess
    // https://www.chessclub.com/help/Wild17
    Variant* losers_variant() {
        Variant* v = chess_variant_base()->init();
        v->checkmateValue = VALUE_MATE;
        v->stalemateValue = VALUE_MATE;
        v->extinctionValue = VALUE_MATE;
        v->extinctionPieceTypes = piece_set(ALL_PIECES);
        v->extinctionPieceCount = 1;
        v->mustCapture = true;
        return v;
    }
    // Giveaway chess
    // Antichess with castling.
    // https://www.chessvariants.com/diffobjective.dir/giveaway.old.html
    Variant* giveaway_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "giveaway";
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->stalemateValue = VALUE_MATE;
        v->extinctionValue = VALUE_MATE;
        v->extinctionPieceTypes = piece_set(ALL_PIECES);
        v->mustCapture = true;
        v->nnueAlias = "antichess";
        return v;
    }
    // Antichess
    // https://lichess.org/variant/antichess
    Variant* antichess_variant() {
        Variant* v = giveaway_variant()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->castling = false;
        return v;
    }
    // Suicide chess
    // Antichess with modified stalemate adjudication.
    // https://www.freechess.org/Help/HelpFiles/suicide_chess.html
    Variant* suicide_variant() {
        Variant* v = antichess_variant()->init();
        v->stalematePieceCount = true;
        v->nnueAlias = "antichess";
        return v;
    }
    // Codrus
    // Lose the king to win. Captures are mandatory.
    // http://www.binnewirtz.com/Schlagschach1.htm
    Variant* codrus_variant() {
        Variant* v = giveaway_variant()->init();
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP | KNIGHT;
        v->extinctionPieceTypes = piece_set(COMMONER);
        return v;
    }
    // Extinction chess
    // https://en.wikipedia.org/wiki/Extinction_chess
    Variant* extinction_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT | PAWN;
        return v;
    }
    // Kinglet
    // https://en.wikipedia.org/wiki/V._R._Parton#Kinglet_chess
    Variant* kinglet_variant() {
        Variant* v = extinction_variant()->init();
        v->promotionPieceTypes[WHITE] = piece_set(COMMONER);
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER);
        v->extinctionPieceTypes = piece_set(PAWN);
        return v;
    }
    // Three Kings Chess
    // https://github.com/cutechess/cutechess/blob/master/projects/lib/src/board/threekingsboard.h
    Variant* threekings_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->startFen = "knbqkbnk/pppppppp/8/8/8/8/PPPPPPPP/KNBQKBNK w - - 0 1";
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->extinctionPieceCount = 2;
        return v;
    }
    // Horde chess
    // https://en.wikipedia.org/wiki/Dunsany%27s_chess#Horde_chess
    Variant* horde_variant() {
        Variant* v = chess_variant_base()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq - 0 1";
        v->doubleStepRegion[WHITE] |= Rank1BB;
        v->enPassantRegion = Rank3BB | Rank6BB; // exclude en passant on second rank
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(ALL_PIECES);
        return v;
    }
    // Petrified
    // Sideways pawns + petrification on capture
    // https://www.chess.com/variants/petrified
    Variant* petrified_variant() {
        Variant* v = pawnsideways_variant()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->extinctionPseudoRoyal = true;
        v->petrifyOnCaptureTypes = piece_set(COMMONER) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Atomic chess without checks (ICC rules)
    // https://www.chessclub.com/help/atomic
    Variant* nocheckatomic_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "atomic";
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->blastOnCapture = true;
        v->nnueAlias = "atomic";
        return v;
    }
    // Atomic chess
    // https://en.wikipedia.org/wiki/Atomic_chess
    Variant* atomic_variant() {
        Variant* v = nocheckatomic_variant()->init();
        v->extinctionPseudoRoyal = true;
        return v;
    }

    // Atomar chess
    // https://web.archive.org/web/20230519082613/https://chronatog.com/wp-content/uploads/2021/09/atomar-chess-rules.pdf
    Variant* atomar_variant() {
        Variant* v = nocheckatomic_variant()->init();
        v->blastImmuneTypes = piece_set(COMMONER);
        v->mutuallyImmuneTypes = piece_set(COMMONER);
        return v;
    }

#ifdef ALLVARS
    // Duck chess
    // https://duckchess.com/
    Variant* duck_variant() {
        Variant* v = chess_variant_base()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->wallingRule = DUCK;
        v->stalemateValue = VALUE_MATE;
        return v;
    }
#endif

    Variant* isolation_variant() { //https://boardgamegeek.com/boardgame/1875/isolation
        Variant* v = chess_variant_base()->init();
        v->maxRank = RANK_8;
        v->maxFile = FILE_F;
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'p', "mK"); //move as a King, but can't capture
        v->startFen = "3p2/6/6/6/6/6/6/2P3 w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->wallingRule = STATIC;
        v->wallingRegion[WHITE] = v->wallingRegion[BLACK] = AllSquares ^ make_bitboard(SQ_C1, SQ_D8);
        return v;
    }

    Variant* isolation7x7_variant() {
        Variant* v = isolation_variant()->init();
        v->maxRank = RANK_7;
        v->maxFile = FILE_G;
        v->startFen = "3p3/7/7/7/7/7/3P3 w - - 0 1";
        v->wallingRegion[WHITE] = v->wallingRegion[BLACK] = AllSquares ^ make_bitboard(SQ_D1, SQ_D7);
        return v;
    }

    Variant* snailtrail_variant() { //https://boardgamegeek.com/boardgame/37135/snailtrail
        Variant* v = chess_variant_base()->init();
        v->maxRank = RANK_7;
        v->maxFile = FILE_G;
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'p', "mK"); //move as a King, but can't capture
        v->startFen = "6p/7/7/7/7/7/P6 w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->wallingRule = PAST;
        return v;
    }

    Variant* joust_variant() { //https://www.chessvariants.com/programs.dir/joust.html
        //This page mainly describes a variant where position on home row is randomized, but also a variant where they start in the centre(implemented here)
        Variant* v = chess_variant_base()->init();
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'n', "mN"); //move as a Knight, but can't capture
        v->startFen = "8/8/8/4n3/3N4/8/8/8 w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->wallingRule = PAST;
        return v;
    }

    Variant* fox_and_hounds_variant() { //https://boardgamegeek.com/boardgame/148180/fox-and-hounds
        Variant* v = chess_variant_base()->init();
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'h', "mfF"); //Hound
        v->add_piece(CUSTOM_PIECE_2, 'f', "mF"); //Fox
        v->startFen = "1h1h1h1h/8/8/8/8/8/8/4F3 w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece[WHITE] = CUSTOM_PIECE_2;
        v->flagRegion[WHITE] = Rank8BB;
        return v;
    }

    // Three-check chess
    // Check the king three times to win
    // https://lichess.org/variant/threeCheck
    Variant* threecheck_variant() {
        Variant* v = chess_variant_base()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+3 0 1";
        v->checkCounting = true;
        return v;
    }
    // Five-check chess
    // Check the king five times to win
    Variant* fivecheck_variant() {
        Variant* v = threecheck_variant()->init();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 5+5 0 1";
        v->nnueAlias = "3check";
        return v;
    }
    // Crazyhouse
    // Chess with piece drops
    // https://en.wikipedia.org/wiki/Crazyhouse
    Variant* crazyhouse_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "crazyhouse";
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        return v;
    }
    // Loop chess
    // Variant of crazyhouse where promoted pawns are not demoted when captured
    // https://en.wikipedia.org/wiki/Crazyhouse#Variations
    Variant* loop_variant() {
        Variant* v = crazyhouse_variant()->init();
        v->dropLoop = true;
        v->nnueAlias = "crazyhouse";
        return v;
    }
    // Chessgi
    // Variant of loop chess where pawns can be dropped to the first rank
    // https://en.wikipedia.org/wiki/Crazyhouse#Variations
    Variant* chessgi_variant() {
        Variant* v = loop_variant()->init();
        v->firstRankPawnDrops = true;
        v->nnueAlias = "crazyhouse";
        return v;
    }
    // Bughouse
    // A four player variant where captured pieces are introduced on the other board
    // https://en.wikipedia.org/wiki/Bughouse_chess
    Variant* bughouse_variant() {
        Variant* v = crazyhouse_variant()->init();
        v->variantTemplate = "bughouse";
        v->twoBoards = true;
        v->capturesToHand = false;
        v->stalemateValue = -VALUE_MATE;
        return v;
    }
    // Koedem (Bughouse variant)
    // http://schachclub-oetigheim.de/wp-content/uploads/2016/04/Koedem-rules.pdf
    Variant* koedem_variant() {
        Variant* v = bughouse_variant()->init();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->mustDrop = true;
        v->mustDropType = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->extinctionOpponentPieceCount = 2; // own all kings/commoners
        return v;
    }
    // Pocket Knight chess
    // Each player has an additional knight in hand which can be dropped at any move
    // https://www.chessvariants.com/other.dir/pocket.html
    Variant* pocketknight_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "bughouse";
        v->pocketSize = 2;
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[Nn] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = false;
        return v;
    }
    // Placement/Pre-chess
    // A shuffle variant where the players determine the placing of the back rank pieces
    // https://www.chessvariants.com/link/placement-chess
    Variant* placement_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "bughouse";
        v->startFen = "8/pppppppp/8/8/8/8/PPPPPPPP/8[KQRRBBNNkqrrbbnn] w - - 0 1";
        v->mustDrop = true;
        v->pieceDrops = true;
        v->capturesToHand = false;
        v->whiteDropRegion = Rank1BB;
        v->blackDropRegion = Rank8BB;
        v->dropOppositeColoredBishop = true;
        v->castlingDroppedPiece = true;
        v->nnueAlias = "nn-";
        return v;
    }
    // Sittuyin (Burmese chess)
    // Regional chess variant from Myanmar, similar to Makruk but with a setup phase.
    // https://en.wikipedia.org/wiki/Sittuyin
    Variant* sittuyin_variant() {
        Variant* v = makruk_variant()->init();
        v->variantTemplate = "bughouse";
        v->pieceToCharTable = "PN.R.F....SKpn.r.f....sk";
        v->startFen = "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1";
        v->add_piece(MET, 'f');
        v->mustDrop = true;
        v->pieceDrops = true;
        v->capturesToHand = false;
        v->whiteDropRegion = Rank1BB | Rank2BB | Rank3BB;
        v->blackDropRegion = Rank8BB | Rank7BB | Rank6BB;
        v->sittuyinRookDrop = true;
        v->sittuyinPromotion = true;
        v->promotionRegion[WHITE] = make_bitboard(SQ_A8, SQ_B7, SQ_C6, SQ_D5, SQ_E5, SQ_F6, SQ_G7, SQ_H8);
        v->promotionRegion[BLACK] = make_bitboard(SQ_A1, SQ_B2, SQ_C3, SQ_D4, SQ_E4, SQ_F3, SQ_G2, SQ_H1);
        v->promotionLimit[FERS] = 1;
        v->immobilityIllegal = false;
        v->countingRule = ASEAN_COUNTING;
        v->nMoveRule = 50;
        return v;
    }
    // S-Chess (aka Seirawan-, or SHarper chess)
    // 8x8 variant introducing the knighted pieces from capablanca chess
    // via gating when a piece first moves from its initial square.
    Variant* seirawan_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "seirawan";
        v->pieceToCharTable = "PNBRQ.E..........H...Kpnbrq.e..........h...k";
        v->add_piece(ARCHBISHOP, 'h');
        v->add_piece(CHANCELLOR, 'e');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1";
        v->gating = true;
        v->seirawanGating = true;
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Musketeer Chess
    // https://musketeerchess.net
    // A Seirawan-inspired variant with unique gating mechanics.
    // Pieces are introduced to predefined squares, chosen before game start, this is named Gating Selection = Where the chosen piece is going to be gated
    // Gating of the additional pieces is activated when first-rank pieces move for the first time. Only the additional piece waiting to be gated on that specific square can be introduced.
    // Features a variety of new pieces, thus there is a piece selection step where both players must agree to chose the additional piece combination.
    // In Fairy Stockfish the Piece Selection is determined at the PieceToCharTable, this default combination can be changed in variant.ini
    Variant* musketeer_variant() {
        Variant* v = chess_variant();
        v->variantTemplate = "seirawan";
        v->pieceToCharTable = "PNBRQ.C..........LO..Kpnbrq.c..........lo..k";  // The default piece combo in Musketeer Chess is Leopard L and Musketeer Cannon O
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->add_piece(AMAZON, 'd'); // also called Dragon in Musketeer, but Amazon is the most accurate
        v->add_piece(LEOPARD, 'l');
        v->add_piece(HAWK, 'h');
        v->add_piece(UNICORN, 'u');
        v->add_piece(SPIDER, 's');
        v->add_piece(FORTRESS, 'f');
        v->add_piece(MUSKETEER_ELEPHANT, 'e');
        v->add_piece(MUSKETEER_CANNON, 'o');
        //"********/rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/******** w KQkq - 0 1"
        v->startFen = "********/rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/******** w KQkq - 0 1";
        v->commitGates = true;
        v->promotionPieceTypes = {LEOPARD, MUSKETEER_CANNON, QUEEN, ROOK, BISHOP, KNIGHT}; // refer to PieceToCharTable to know the piece combination that can be promoted apart from the classic chess pieces
        return v;
    }
    
    // S-House
    // A hybrid variant of S-Chess and Crazyhouse.
    // Pieces in the pocket can either be gated or dropped.
    Variant* shouse_variant() {
        Variant* v = seirawan_variant()->init();
        v->variantTemplate = "crazyhouse";
        v->pieceDrops = true;
        v->capturesToHand = true;
        return v;
    }
    // Dragon Chess
    // 8x8 variant invented by Miguel Illescas:
    // https://www.edami.com/dragonchess/
    // Like regular chess, but with an extra piece, the dragon, which moves like
    // an archbishop (i.e. bishop+knight). The dragon can be dropped at an empty
    // square on the back rank instead of making a normal move.
    Variant* dragon_variant() {
        Variant *v = chess_variant_base()->init();
        v->variantTemplate = "bughouse";
        v->pieceToCharTable = "PNBRQ............D...Kpnbrq............d...k";
        v->add_piece(ARCHBISHOP, 'd');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[Dd] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = false;
        v->whiteDropRegion = Rank1BB;
        v->blackDropRegion = Rank8BB;
        return v;
    }
    // Paradigm chess30
    // 8x8 variant with a bishop+horse hybrid piece replacing bishops
    // https://www.chessvariants.com/rules/paradigm-chess30
    Variant* paradigm_variant() {
        Variant *v = chess_variant_base()->init();
        v->remove_piece(BISHOP);
        v->add_piece(CUSTOM_PIECE_1, 'b', "BnN");
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | CUSTOM_PIECE_1 | ROOK | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | CUSTOM_PIECE_1 | ROOK | KNIGHT;
        return v;
    }
    // Base used for most shogi variants
    Variant* minishogi_variant_base() {
        Variant* v = variant_base()->init();
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
        v->promotionRegion[WHITE] = Rank5BB;
        v->promotionRegion[BLACK] = Rank1BB;
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
    // Minishogi
    // 5x5 variant of shogi
    // https://en.wikipedia.org/wiki/Minishogi
    Variant* minishogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->pieceToCharTable = "P.BR.S...G.+.++.+Kp.br.s...g.+.++.+k";
        v->pocketSize = 5;
        v->nFoldValue = -VALUE_MATE;
        v->nFoldValueAbsolute = true;
        v->nnueAlias = "minishogi";
        return v;
    }
    // Kyoto shogi
    // 5x5 variant of shogi with pieces alternating between promotion and demotion
    // https://en.wikipedia.org/wiki/Kyoto_shogi
    Variant* kyotoshogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->add_piece(LANCE, 'l');
        v->add_piece(SHOGI_KNIGHT, 'n');
        v->startFen = "p+nks+l/5/5/5/+LSK+NP[-] w 0 1";
        v->promotionRegion[WHITE] = AllSquares;
        v->promotionRegion[BLACK] = AllSquares;
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
    // Micro shogi
    // 4x5 shogi variant where pieces promoted and demote when capturing
    // https://en.wikipedia.org/wiki/Micro_shogi
    Variant* microshogi_variant() {
        Variant* v = kyotoshogi_variant()->init();
        v->maxFile = FILE_D;
        v->startFen = "kb+r+l/p3/4/3P/+L+RBK[-] w 0 1";
        v->promotionRegion[WHITE] = AllSquares;
        v->promotionRegion[BLACK] = AllSquares;
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
    // Dobutsu
    // Educational shogi variant on a 3x4 board
    // https://en.wikipedia.org/wiki/D%C5%8Dbutsu_sh%C5%8Dgi
    Variant* dobutsu_variant() {
        Variant* v = minishogi_variant_base()->init();
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
        v->promotionRegion[WHITE] = Rank4BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->mandatoryPiecePromotion = true;
        v->immobilityIllegal = false;
        v->shogiPawnDropMateIllegal = false;
        v->flagPiece[WHITE] = v->flagPiece[BLACK] = KING;
        v->flagRegion[WHITE] = Rank4BB;
        v->flagRegion[BLACK] = Rank1BB;
        v->dropNoDoubled = NO_PIECE_TYPE;
        v->nFoldValue = VALUE_DRAW;
        v->perpetualCheckIllegal = false;
        return v;
    }
    // Goro goro shogi
    // https://en.wikipedia.org/wiki/D%C5%8Dbutsu_sh%C5%8Dgi#Variation
    Variant* gorogoroshogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->pieceToCharTable = "P....S...G.+....+Kp....s...g.+....+k";
        v->pocketSize = 3;
        v->maxRank = RANK_6;
        v->maxFile = FILE_E;
        v->startFen = "sgkgs/5/1ppp1/1PPP1/5/SGKGS[-] w 0 1";
        v->promotionRegion[WHITE] = Rank5BB | Rank6BB;
        v->promotionRegion[BLACK] = Rank2BB | Rank1BB;
        return v;
    }
    // Judkins shogi
    // https://en.wikipedia.org/wiki/Judkins_shogi
    Variant* judkinsshogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->pieceToCharTable = "PNBR.S...G.++++.+Kpnbr.s...g.++++.+k";
        v->maxRank = RANK_6;
        v->maxFile = FILE_F;
        v->add_piece(SHOGI_KNIGHT, 'n');
        v->startFen = "rbnsgk/5p/6/6/P5/KGSNBR[-] w 0 1";
        v->promotionRegion[WHITE] = Rank5BB | Rank6BB;
        v->promotionRegion[BLACK] = Rank2BB | Rank1BB;
        v->promotedPieceType[SHOGI_KNIGHT] = GOLD;
        return v;
    }
    // Tori shogi
    // https://en.wikipedia.org/wiki/Tori_shogi
    Variant* torishogi_variant() {
        Variant* v = variant_base()->init();
        v->variantTemplate = "shogi";
        v->pieceToCharTable = "S.....FLR.C+.....+.PKs.....flr.c+.....+.pk";
        v->maxRank = RANK_7;
        v->maxFile = FILE_G;
        v->reset_pieces();
        v->add_piece(SHOGI_PAWN, 's');
        v->add_piece(KING, 'k');
        v->add_piece(CUSTOM_PIECE_1, 'f', "FsfW"); // falcon
        v->add_piece(CUSTOM_PIECE_2, 'c', "FvW"); // crane
        v->add_piece(CUSTOM_PIECE_3, 'l', "fRrbBlbF"); // left quail
        v->add_piece(CUSTOM_PIECE_4, 'r', "fRlbBrbF"); // right quail
        v->add_piece(CUSTOM_PIECE_5, 'p', "bFfD"); // pheasant
        v->add_piece(CUSTOM_PIECE_6, 'g', "fAbD"); // goose
        v->add_piece(CUSTOM_PIECE_7, 'e', "KbRfBbF2"); // eagle
        v->startFen = "rpckcpl/3f3/sssssss/2s1S2/SSSSSSS/3F3/LPCKCPR[-] w 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        v->promotionRegion[WHITE] = Rank6BB | Rank7BB;
        v->promotionRegion[BLACK] = Rank2BB | Rank1BB;
        v->doubleStep = false;
        v->castling = false;
        v->promotedPieceType[SHOGI_PAWN]    = CUSTOM_PIECE_6; // swallow promotes to goose
        v->promotedPieceType[CUSTOM_PIECE_1] = CUSTOM_PIECE_7; // falcon promotes to eagle
        v->mandatoryPiecePromotion = true;
        v->dropNoDoubled = SHOGI_PAWN;
        v->dropNoDoubledCount = 2;
        v->immobilityIllegal = true;
        v->shogiPawnDropMateIllegal = true;
        v->stalemateValue = -VALUE_MATE;
        v->nFoldValue = VALUE_MATE;
        v->nFoldRule = 4;
        v->nMoveRule = 0;
        v->perpetualCheckIllegal = true;
        return v;
    }
    // EuroShogi
    // https://en.wikipedia.org/wiki/EuroShogi
    Variant* euroshogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->pieceToCharTable = "PNBR.....G.++++Kpnbr.....g.++++k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_H;
        v->add_piece(CUSTOM_PIECE_1, 'n', "fNsW");
        v->startFen = "1nbgkgn1/1r4b1/pppppppp/8/8/PPPPPPPP/1B4R1/1NGKGBN1[-] w 0 1";
        v->promotionRegion[WHITE] = Rank6BB | Rank7BB | Rank8BB;
        v->promotionRegion[BLACK] = Rank3BB | Rank2BB | Rank1BB;
        v->promotedPieceType[CUSTOM_PIECE_1] = GOLD;
        v->mandatoryPiecePromotion = true;
        return v;
    }
    // Los Alamos chess
    // https://en.wikipedia.org/wiki/Los_Alamos_chess
    Variant* losalamos_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PN.RQ................Kpn.rq................k";
        v->maxRank = RANK_6;
        v->maxFile = FILE_F;
        v->remove_piece(BISHOP);
        v->startFen = "rnqknr/pppppp/6/6/PPPPPP/RNQKNR w - - 0 1";
        v->promotionRegion[WHITE] = Rank6BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | KNIGHT;
        v->doubleStep = false;
        v->castling = false;
        return v;
    }
    // Gardner's minichess
    // https://en.wikipedia.org/wiki/Minichess#5%C3%975_chess
    Variant* gardner_variant() {
        Variant* v = chess_variant_base()->init();
        v->maxRank = RANK_5;
        v->maxFile = FILE_E;
        v->startFen = "rnbqk/ppppp/5/PPPPP/RNBQK w - - 0 1";
        v->promotionRegion[WHITE] = Rank5BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->doubleStep = false;
        v->castling = false;
        return v;
    }
    // Almost chess
    // Queens are replaced by chancellors
    // https://en.wikipedia.org/wiki/Almost_chess
    Variant* almost_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR............CKpnbr............ck";
        v->remove_piece(QUEEN);
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbckbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBCKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(CHANCELLOR) | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Sort of almost chess
    // One queen is replaced by a chancellor
    // https://en.wikipedia.org/wiki/Almost_chess#Sort_of_almost_chess
    Variant* sortofalmost_variant() {
        Variant* v = chess_variant();
        v->pieceToCharTable = "PNBRQ...........CKpnbrq...........ck";
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBCKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Chigorin chess
    // Asymmetric variant with knight vs. bishop movements
    // https://www.chessvariants.com/diffsetup.dir/chigorin.html
    Variant* chigorin_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR............CKpnbrq............k";
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rbbqkbbr/pppppppp/8/8/8/8/PPPPPPPP/RNNCKNNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | ROOK | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP;
        return v;
    }
    // Perfect chess
    // https://www.chessvariants.com/diffmove.dir/perfectchess.html
    Variant* perfect_variant() {
        Variant* v = chess_variant_base()->init();
        v->add_piece(CHANCELLOR, 'c');
        v->add_piece(ARCHBISHOP, 'm');
        v->add_piece(AMAZON, 'g');
        v->startFen = "cmqgkbnr/pppppppp/8/8/8/8/PPPPPPPP/CMQGKBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AMAZON) | CHANCELLOR | ARCHBISHOP | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(AMAZON) | CHANCELLOR | ARCHBISHOP | QUEEN | ROOK | BISHOP | KNIGHT;
        v->castlingRookPieces[WHITE] = v->castlingRookPieces[BLACK] |= piece_set(CHANCELLOR);
        return v;
    }
    // Spartan chess
    // https://www.chessvariants.com/rules/spartan-chess
    Variant* spartan_variant() {
        Variant* v = threekings_variant()->init();
        v->add_piece(DRAGON, 'g');
        v->add_piece(ARCHBISHOP, 'w');
        v->add_piece(CUSTOM_PIECE_1, 'h', "fmFfcWimA");
        v->add_piece(CUSTOM_PIECE_2, 'l', "FAsmW");
        v->add_piece(CUSTOM_PIECE_3, 'c', "WD");
        v->startFen = "lgkcckwl/hhhhhhhh/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1";
        v->promotionPawnType[BLACK] = CUSTOM_PIECE_1;
        v->promotionPawnTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->nMoveRuleTypes[BLACK] = piece_set(CUSTOM_PIECE_1);
        v->promotionPieceTypes[BLACK] = piece_set(COMMONER) | DRAGON | ARCHBISHOP | CUSTOM_PIECE_2 | CUSTOM_PIECE_3;
        v->promotionLimit[COMMONER] = 2;
        v->enPassantRegion = 0;
        v->extinctionPieceCount = 0;
        v->extinctionPseudoRoyal = true;
        v->dupleCheck = true;
        return v;
    }
    // Shatar (Mongolian chess)
    // https://en.wikipedia.org/wiki/Shatar
    Variant* shatar_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBR..........J......Kpnbr..........j......k";
        v->remove_piece(QUEEN);
        v->add_piece(BERS, 'j');
        v->startFen = "rnbjkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBJKBNR w - - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(BERS);
        v->promotionPieceTypes[BLACK] = piece_set(BERS);
        v->doubleStep = false;
        v->castling = false;
        v->extinctionValue = VALUE_DRAW; // Robado
        v->extinctionPieceTypes = piece_set(ALL_PIECES);
        v->extinctionPieceCount = 1;
        v->shatarMateRule = true;
        return v;
    }
    // Coregal chess
    // Queens are also subject to check and checkmate
    // https://www.chessvariants.com/winning.dir/coregal.html
    Variant* coregal_variant() {
        Variant* v = chess_variant_base()->init();
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(QUEEN);
        v->extinctionPseudoRoyal = true;
        v->extinctionPieceCount = 64; // no matter how many queens, all are royal
        return v;
    }
    // Clobber
    // https://en.wikipedia.org/wiki/Clobber
    Variant* clobber_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "P.................p.................";
        v->maxRank = RANK_6;
        v->maxFile = FILE_E;
        v->reset_pieces();
        v->add_piece(CLOBBER_PIECE, 'p');
        v->startFen = "PpPpP/pPpPp/PpPpP/pPpPp/PpPpP/pPpPp w 0 1";
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->immobilityIllegal = false;
        return v;
    }
    // Breakthrough
    // https://en.wikipedia.org/wiki/Breakthrough_(board_game)
    Variant* breakthrough_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "P.................p.................";
        v->reset_pieces();
        v->add_piece(BREAKTHROUGH_PIECE, 'p');
        v->startFen = "pppppppp/pppppppp/8/8/8/8/PPPPPPPP/PPPPPPPP w 0 1";
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece[WHITE] = v->flagPiece[BLACK] = BREAKTHROUGH_PIECE;
        v->flagRegion[WHITE] = Rank8BB;
        v->flagRegion[BLACK] = Rank1BB;
        return v;
    }
    // Ataxx
    // https://en.wikipedia.org/wiki/Ataxx
    Variant* ataxx_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "P.................p.................";
        v->maxRank = RANK_7;
        v->maxFile = FILE_G;
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'p', "mDmNmA");
        v->startFen = "P5p/7/7/7/7/7/p5P w 0 1";
        v->pieceDrops = true;
        v->doubleStep = false;
        v->castling = false;
        v->immobilityIllegal = false;
        v->stalemateValue = -VALUE_MATE;
        v->stalematePieceCount = true;
        v->passOnStalemate[WHITE] = true;
        v->passOnStalemate[BLACK] = true;
        v->enclosingDrop = ATAXX;
        v->flipEnclosedPieces = ATAXX;
        v->materialCounting = UNWEIGHTED_MATERIAL;
        v->nMoveRule = 0;
        v->freeDrops = true;
        return v;
    }
    // Flipersi
    // https://en.wikipedia.org/wiki/Reversi
    Variant* flipersi_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "P.................p.................";
        v->maxRank = RANK_8;
        v->maxFile = FILE_H;
        v->reset_pieces();
        v->add_piece(IMMOBILE_PIECE, 'p');
        v->startFen = "8/8/8/8/8/8/8/8[PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPpppppppppppppppppppppppppppppppp] w 0 1";
        v->pieceDrops = true;
        v->doubleStep = false;
        v->castling = false;
        v->immobilityIllegal = false;
        v->stalemateValue = -VALUE_MATE;
        v->stalematePieceCount = true;
        v->passOnStalemate[WHITE] = false;
        v->passOnStalemate[BLACK] = false;
        v->enclosingDrop = REVERSI;
        v->enclosingDropStart = make_bitboard(SQ_D4, SQ_E4, SQ_D5, SQ_E5);
        v->flipEnclosedPieces = REVERSI;
        v->materialCounting = UNWEIGHTED_MATERIAL;
        return v;
    }
    // Flipello
    // https://en.wikipedia.org/wiki/Reversi#Othello
    Variant* flipello_variant() {
        Variant* v = flipersi_variant()->init();
        v->startFen = "8/8/8/3pP3/3Pp3/8/8/8[PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPpppppppppppppppppppppppppppppppp] w 0 1";
        v->passOnStalemate[WHITE] = true;
        v->passOnStalemate[BLACK] = true;
        return v;
    }
    // Minixiangqi
    // http://mlwi.magix.net/bg/minixiangqi.htm
    Variant* minixiangqi_variant() {
        Variant* v = chess_variant_base()->init();
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
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        //v->nFoldValue = VALUE_MATE;
        v->perpetualCheckIllegal = true;
        v->flyingGeneral = true;
        return v;
    }
#ifdef LARGEBOARDS
    // Shogi (Japanese chess)
    // https://en.wikipedia.org/wiki/Shogi
    Variant* shogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->add_piece(LANCE, 'l');
        v->add_piece(SHOGI_KNIGHT, 'n');
        v->startFen = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w 0 1";
        v->promotionRegion[WHITE] = Rank7BB | Rank8BB | Rank9BB;
        v->promotionRegion[BLACK] = Rank3BB | Rank2BB | Rank1BB;
        v->promotedPieceType[LANCE]        = GOLD;
        v->promotedPieceType[SHOGI_KNIGHT] = GOLD;
        return v;
    }
    // Sho-Shogi
    // 16-th century shogi variant with one additional piece and no drops
    // https://en.wikipedia.org/wiki/Sho_shogi
    Variant* shoshogi_variant() {
        Variant* v = shogi_variant()->init();
        v->pieceToCharTable = "PNBRLSE..G.+.++.++Kpnbrlse..g.+.++.++k";
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->add_piece(CUSTOM_PIECE_1, 'e', "FsfW"); // drunk elephant
        v->startFen = "lnsgkgsnl/1r2e2b1/ppppppppp/9/9/9/PPPPPPPPP/1B2E2R1/LNSGKGSNL w 0 1";
        v->capturesToHand = false;
        v->pieceDrops = false;
        v->promotedPieceType[CUSTOM_PIECE_1] = COMMONER;
        v->castlingKingPiece[WHITE] = v->castlingKingPiece[BLACK] = COMMONER;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = piece_set(COMMONER);
        v->extinctionPseudoRoyal = true;
        v->extinctionPieceCount = 0;
        return v;
    }
    // Yari shogi
    // https://en.wikipedia.org/wiki/Yari_shogi
    Variant* yarishogi_variant() {
        Variant* v = variant_base()->init();
        v->variantTemplate = "shogi";
        v->pieceToCharTable = "PNBR.......++++Kpnbr.......++++k";
        v->maxRank = RANK_9;
        v->maxFile = FILE_G;
        v->reset_pieces();
        v->add_piece(KING, 'k');
        v->add_piece(SHOGI_PAWN, 'p');
        v->add_piece(ROOK, 'l');
        v->add_piece(CUSTOM_PIECE_1, 'n', "fRffN"); // Yari knight
        v->add_piece(CUSTOM_PIECE_2, 'b', "fFfR"); // Yari bishop
        v->add_piece(CUSTOM_PIECE_3, 'r', "frlR"); // Yari rook
        v->add_piece(CUSTOM_PIECE_4, 'g', "WfFbR"); // Yari gold
        v->add_piece(CUSTOM_PIECE_5, 's', "fKbR"); // Yari silver
        v->startFen = "rnnkbbr/7/ppppppp/7/7/7/PPPPPPP/7/RBBKNNR[-] w 0 1";
        v->promotionRegion[WHITE] = Rank7BB | Rank8BB | Rank9BB;
        v->promotionRegion[BLACK] = Rank3BB | Rank2BB | Rank1BB;
        v->promotedPieceType[SHOGI_PAWN] = CUSTOM_PIECE_5;
        v->promotedPieceType[CUSTOM_PIECE_1] = CUSTOM_PIECE_4;
        v->promotedPieceType[CUSTOM_PIECE_2] = CUSTOM_PIECE_4;
        v->promotedPieceType[CUSTOM_PIECE_3] = ROOK;
        v->pieceDrops = true;
        v->capturesToHand = true;
        v->doubleStep = false;
        v->castling = false;
        v->dropNoDoubled = SHOGI_PAWN;
        v->immobilityIllegal = true;
        v->shogiPawnDropMateIllegal = false;
        v->stalemateValue = -VALUE_MATE;
        v->nFoldRule = 3;
        v->nMoveRule = 0;
        v->perpetualCheckIllegal = true;
        return v;
    }
    // Okisaki shogi
    // https://en.wikipedia.org/wiki/Okisaki_shogi
    Variant* okisakishogi_variant() {
        Variant* v = minishogi_variant_base()->init();
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->add_piece(CUSTOM_PIECE_1, 'l', "vR"); // Vertical slider
        v->add_piece(KNIGHT, 'n');
        v->add_piece(QUEEN, 'q');
        v->startFen = "lnsgkqgsnl/1r6b1/pppppppppp/10/10/10/10/PPPPPPPPPP/1B6R1/LNSGQKGSNL[-] w 0 1";
        v->promotionRegion[WHITE] = Rank8BB | Rank9BB | Rank10BB;
        v->promotionRegion[BLACK] = Rank3BB | Rank2BB | Rank1BB;
        v->promotedPieceType[CUSTOM_PIECE_1] = GOLD;
        v->promotedPieceType[KNIGHT] = GOLD;
        return v;
    }
    // Capablanca chess
    // https://en.wikipedia.org/wiki/Capablanca_chess
    Variant* capablanca_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ..AC............Kpnbrq..ac............k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Capahouse
    // Capablanca chess with crazyhouse-style piece drops
    // https://www.pychess.org/variant/capahouse
    Variant* capahouse_variant() {
        Variant* v = capablanca_variant()->init();
        v->startFen = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        return v;
    }
    // Capablanca random chess (CRC)
    // Shuffle variant of capablanca chess
    // https://en.wikipedia.org/wiki/Capablanca_random_chess
    Variant* caparandom_variant() {
        Variant* v = capablanca_variant()->init();
        v->chess960 = true;
        v->nnueAlias = "capablanca";
        return v;
    }
    // Gothic chess
    // Capablanca chess with changed starting position
    // https://www.chessvariants.com/large.dir/gothicchess.html
    Variant* gothic_variant() {
        Variant* v = capablanca_variant()->init();
        v->startFen = "rnbqckabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQCKABNR w KQkq - 0 1";
        v->nnueAlias = "capablanca";
        return v;
    }
    // Janus chess
    // 10x8 variant with two archbishops per side
    // https://en.wikipedia.org/wiki/Janus_Chess
    Variant* janus_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ............J...Kpnbrq............j...k";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_B;
        v->add_piece(ARCHBISHOP, 'j');
        v->startFen = "rjnbkqbnjr/pppppppppp/10/10/10/10/PPPPPPPPPP/RJNBKQBNJR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Modern chess
    // 9x9 variant with archbishops
    // https://en.wikipedia.org/wiki/Modern_chess
    Variant* modern_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ..M.............Kpnbrq..m.............k";
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->promotionRegion[WHITE] = Rank9BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->doubleStepRegion[WHITE] = Rank2BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        v->castlingKingsideFile = FILE_G;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(ARCHBISHOP, 'm');
        v->startFen = "rnbqkmbnr/ppppppppp/9/9/9/9/9/PPPPPPPPP/RNBMKQBNR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Chancellor chess
    // 9x9 variant with chancellors
    // https://en.wikipedia.org/wiki/Chancellor_chess
    Variant* chancellor_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ...........CKpnbrq...........ck";
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->promotionRegion[WHITE] = Rank9BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->doubleStepRegion[WHITE] = Rank2BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        v->castlingKingsideFile = FILE_G;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbqkcnbr/ppppppppp/9/9/9/9/9/PPPPPPPPP/RNBQKCNBR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CHANCELLOR) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(CHANCELLOR) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Embassy chess
    // Capablanca chess with different starting position
    // https://en.wikipedia.org/wiki/Embassy_chess
    Variant* embassy_variant() {
        Variant* v = capablanca_variant()->init();
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_B;
        v->startFen = "rnbqkcabnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQKCABNR w KQkq - 0 1";
        v->nnueAlias = "capablanca";
        return v;
    }
    // Centaur chess (aka Royal Court)
    // 10x8 variant with a knight+commoner compound
    // https://www.chessvariants.com/large.dir/contest/royalcourt.html
    Variant* centaur_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ...............CKpnbrq...............ck";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_C;
        v->add_piece(CENTAUR, 'c');
        v->startFen = "rcnbqkbncr/pppppppppp/10/10/10/10/PPPPPPPPPP/RCNBQKBNCR w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(CENTAUR) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(CENTAUR) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Gustav III chess
    // 10x8 variant with an amazon piece and wall squares
    // https://www.chessvariants.com/play/gustav-iiis-chess
    Variant* gustav3_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ.............AKpnbrq.............ak";
        v->maxRank = RANK_8;
        v->maxFile = FILE_J;
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_D;
        v->add_piece(AMAZON, 'a');
        v->startFen = "arnbqkbnra/*pppppppp*/*8*/*8*/*8*/*8*/*PPPPPPPP*/ARNBQKBNRA w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(AMAZON) | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(AMAZON) | QUEEN | ROOK | BISHOP | KNIGHT;
        return v;
    }
    // Jeson mor
    // Mongolian chess variant with knights only and a king of the hill like goal
    // https://en.wikipedia.org/wiki/Jeson_Mor
    Variant* jesonmor_variant() {
        Variant* v = chess_variant_base()->init();
        v->maxRank = RANK_9;
        v->maxFile = FILE_I;
        v->reset_pieces();
        v->add_piece(KNIGHT, 'n');
        v->startFen = "nnnnnnnnn/9/9/9/9/9/9/9/NNNNNNNNN w - - 0 1";
        v->doubleStep = false;
        v->castling = false;
        v->stalemateValue = -VALUE_MATE;
        v->flagPiece[WHITE] = v->flagPiece[BLACK] = KNIGHT;
        v->flagRegion[WHITE] = make_bitboard(SQ_E5);
        v->flagRegion[BLACK] = make_bitboard(SQ_E5);
        v->flagMove = true;
        return v;
    }
    // Courier chess
    // Medieval variant of Shatranj on a 12x8 board
    // https://en.wikipedia.org/wiki/Courier_chess
    Variant* courier_variant() {
        Variant* v = chess_variant_base()->init();
        v->maxRank = RANK_8;
        v->maxFile = FILE_L;
        v->remove_piece(QUEEN);
        v->add_piece(ALFIL, 'e');
        v->add_piece(FERS, 'f');
        v->add_piece(COMMONER, 'm');
        v->add_piece(WAZIR, 'w');
        v->startFen = "rnebmk1wbenr/1ppppp1pppp1/6f5/p5p4p/P5P4P/6F5/1PPPPP1PPPP1/RNEBMK1WBENR w - - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(FERS);
        v->promotionPieceTypes[BLACK] = piece_set(FERS);
        v->doubleStep = false;
        v->castling = false;
        v->extinctionValue = -VALUE_MATE;
        v->extinctionClaim = true;
        v->extinctionPieceTypes = piece_set(ALL_PIECES);
        v->extinctionPieceCount = 1;
        v->extinctionOpponentPieceCount = 2;
        v->stalemateValue = -VALUE_MATE;
        return v;
    }
    // Grand chess
    // 10x10 variant with chancellors and archbishops
    // https://en.wikipedia.org/wiki/Grand_chess
    Variant* grand_variant() {
        Variant* v = chess_variant_base()->init();
        v->variantTemplate = "grand";
        v->pieceToCharTable = "PNBRQ..AC............Kpnbrq..ac............k";
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R w - - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionRegion[WHITE] = Rank8BB | Rank9BB | Rank10BB;
        v->promotionRegion[BLACK] = Rank3BB | Rank2BB | Rank1BB;
        v->promotionLimit[ARCHBISHOP] = 1;
        v->promotionLimit[CHANCELLOR] = 1;
        v->promotionLimit[QUEEN] = 1;
        v->promotionLimit[ROOK] = 2;
        v->promotionLimit[BISHOP] = 2;
        v->promotionLimit[KNIGHT] = 2;
        v->mandatoryPawnPromotion = false;
        v->immobilityIllegal = true;
        v->doubleStepRegion[WHITE] = Rank3BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        v->castling = false;
        return v;
    }
    // Opulent chess
    // Variant of Grand chess with two extra pieces
    // https://www.chessvariants.com/rules/opulent-chess
    Variant* opulent_variant() {
        Variant* v = grand_variant()->init();
        v->pieceToCharTable = "PNBRQ..AC....W.......LKpnbrq..ac....w.......lk";
        v->remove_piece(KNIGHT);
        v->add_piece(CUSTOM_PIECE_1, 'n', "NW");
        v->add_piece(CUSTOM_PIECE_2, 'w', "CF");
        v->add_piece(CUSTOM_PIECE_3, 'l', "FDH");
        v->startFen = "rw6wr/clbnqknbla/pppppppppp/10/10/10/10/PPPPPPPPPP/CLBNQKNBLA/RW6WR w - - 0 1";
        v->promotionPieceTypes[WHITE] &= ~piece_set(KNIGHT);
        v->promotionPieceTypes[WHITE] |= CUSTOM_PIECE_1;
        v->promotionPieceTypes[WHITE] |= CUSTOM_PIECE_2;
        v->promotionPieceTypes[WHITE] |= CUSTOM_PIECE_3;
        v->promotionPieceTypes[BLACK] = v->promotionPieceTypes[WHITE];
        v->promotionLimit[CUSTOM_PIECE_1] = 2;
        v->promotionLimit[CUSTOM_PIECE_2] = 2;
        v->promotionLimit[CUSTOM_PIECE_3] = 2;
        return v;
    }
    // Tencubed
    // https://www.chessvariants.com/contests/10/tencubedchess.html
    Variant* tencubed_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ.CAM...........WKpnbrq.cam...........wk";
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->startFen = "2cwamwc2/1rnbqkbnr1/pppppppppp/10/10/10/10/PPPPPPPPPP/1RNBQKBNR1/2CWAMWC2 w - - 0 1";
        v->add_piece(ARCHBISHOP, 'a');
        v->add_piece(CHANCELLOR, 'm');
        v->add_piece(CUSTOM_PIECE_1, 'c', "DAW"); // Champion
        v->add_piece(CUSTOM_PIECE_2, 'w', "CF"); // Wizard
        v->promotionPieceTypes[WHITE] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN;
        v->promotionPieceTypes[BLACK] = piece_set(ARCHBISHOP) | CHANCELLOR | QUEEN;
        v->promotionRegion[WHITE] = Rank10BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->doubleStepRegion[WHITE] = Rank3BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        v->castling = false;
        return v;
    }
    // Omicron chess
    // Omega chess on a 12x10 board
    // http://www.eglebbk.dds.nl/program/chess-omicron.html
    Variant* omicron_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ..C.W...........Kpnbrq..c.w...........k";
        v->maxRank = RANK_10;
        v->maxFile = FILE_L;
        v->startFen = "w**********w/*crnbqkbnrc*/*pppppppppp*/*10*/*10*/*10*/*10*/*PPPPPPPPPP*/*CRNBQKBNRC*/W**********W w KQkq - 0 1";
        v->add_piece(CUSTOM_PIECE_1, 'c', "DAW"); // Champion
        v->add_piece(CUSTOM_PIECE_2, 'w', "CF"); // Wizard
        v->castlingKingsideFile = FILE_I;
        v->castlingQueensideFile = FILE_E;
        v->castlingRank = RANK_2;
        v->promotionRegion[WHITE] = Rank9BB | Rank10BB;
        v->promotionRegion[BLACK] = Rank2BB | Rank1BB;
        v->promotionPieceTypes[WHITE] = piece_set(CUSTOM_PIECE_2) | CUSTOM_PIECE_1 | QUEEN | ROOK | BISHOP | KNIGHT;
        v->promotionPieceTypes[BLACK] = piece_set(CUSTOM_PIECE_2) | CUSTOM_PIECE_1 | QUEEN | ROOK | BISHOP | KNIGHT;
        v->doubleStepRegion[WHITE] = Rank3BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        return v;
    }
    // Troitzky Chess
    // https://www.chessvariants.com/play/troitzky-chess
    Variant* troitzky_variant() {
        Variant* v =  chess_variant_base()->init();
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->startFen = "****qk****/**rnbbnr**/*pppppppp*/*8*/10/10/*8*/*PPPPPPPP*/**RNBBNR**/****QK**** w - - 0 1";
        v->promotionRegion[WHITE] = make_bitboard(SQ_A6, SQ_B8, SQ_C9, SQ_D9, SQ_E10, SQ_F10, SQ_G9, SQ_H9, SQ_I8, SQ_J6);
        v->promotionRegion[BLACK] = make_bitboard(SQ_A5, SQ_B3, SQ_C2, SQ_D2, SQ_E1, SQ_F1, SQ_G2, SQ_H2, SQ_I3, SQ_J5);
        v->doubleStepRegion[WHITE] = Rank3BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        v->castling = false;
        return v;
    }
    // Wolf chess
    // https://en.wikipedia.org/wiki/Wolf_chess
    Variant* wolf_variant() {
        Variant* v = chess_variant_base()->init();
        v->maxRank = RANK_10;
        v->remove_piece(KNIGHT);
        v->add_piece(CHANCELLOR, 'w'); // wolf
        v->add_piece(ARCHBISHOP, 'f'); // fox
        v->add_piece(CUSTOM_PIECE_1, 's', "fKifmnD"); // sergeant
        v->add_piece(CUSTOM_PIECE_2, 'n', "NN"); // nightrider
        v->add_piece(CUSTOM_PIECE_3, 'e', "NNQ"); // elephant
        v->startFen = "qwfrbbnk/pssppssp/1pp2pp1/8/8/8/8/1PP2PP1/PSSPPSSP/KNBBRFWQ w - - 0 1";
        v->promotionPawnType[WHITE] = v->promotionPawnType[BLACK] = PAWN;
        v->promotionPawnTypes[WHITE] = v->promotionPawnTypes[BLACK] = piece_set(PAWN) | piece_set(CUSTOM_PIECE_1);
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | CHANCELLOR | ARCHBISHOP | ROOK | BISHOP;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | CHANCELLOR | ARCHBISHOP | ROOK | BISHOP;
        v->promotedPieceType[PAWN] = CUSTOM_PIECE_3;
        v->promotionRegion[WHITE] = Rank10BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->doubleStepRegion[WHITE] = Rank2BB | make_bitboard(SQ_B3, SQ_C3, SQ_F3, SQ_G3);
        v->doubleStepRegion[BLACK] = Rank9BB | make_bitboard(SQ_B8, SQ_C8, SQ_F8, SQ_G8);
        v->enPassantTypes[WHITE] = v->enPassantTypes[BLACK] = piece_set(PAWN);
        v->nMoveRuleTypes[WHITE] = v->nMoveRuleTypes[BLACK] = piece_set(PAWN) | piece_set(CUSTOM_PIECE_1);
        v->castling = false;
        return v;
    }
    // Shako
    // 10x10 variant with cannons by Jean-Louis Cazaux
    // https://www.chessvariants.com/large.dir/shako.html
    Variant* shako_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "PNBRQ.E....C.........Kpnbrq.e....c.........k";
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->add_piece(FERS_ALFIL, 'e');
        v->add_piece(CANNON, 'c');
        v->startFen = "c8c/ernbqkbnre/pppppppppp/10/10/10/10/PPPPPPPPPP/ERNBQKBNRE/C8C w KQkq - 0 1";
        v->promotionPieceTypes[WHITE] = piece_set(QUEEN) | ROOK | BISHOP | KNIGHT | CANNON | FERS_ALFIL ;
        v->promotionPieceTypes[BLACK] = piece_set(QUEEN) | ROOK | BISHOP | KNIGHT | CANNON | FERS_ALFIL ;
        v->promotionRegion[WHITE] = Rank10BB;
        v->promotionRegion[BLACK] = Rank1BB;
        v->castlingKingsideFile = FILE_H;
        v->castlingQueensideFile = FILE_D;
        v->castlingRookKingsideFile = FILE_I;
        v->castlingRookQueensideFile = FILE_B;
        v->castlingRank = RANK_2;
        v->doubleStepRegion[WHITE] = Rank3BB;
        v->doubleStepRegion[BLACK] = Rank8BB;
        return v;
    }
    // Clobber 10x10
    // Clobber on a 10x10 board, mainly played by computers
    // https://en.wikipedia.org/wiki/Clobber
    Variant* clobber10_variant() {
        Variant* v = clobber_variant()->init();
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->startFen = "PpPpPpPpPp/pPpPpPpPpP/PpPpPpPpPp/pPpPpPpPpP/PpPpPpPpPp/"
                      "pPpPpPpPpP/PpPpPpPpPp/pPpPpPpPpP/PpPpPpPpPp/pPpPpPpPpP w 0 1";
        return v;
    }
    // Flipello 10x10
    // Othello on a 10x10 board, mainly played by computers
    // https://en.wikipedia.org/wiki/Reversi
    Variant* flipello10_variant() {
        Variant* v = flipello_variant()->init();
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->startFen = "10/10/10/10/4pP4/4Pp4/10/10/10/10[PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPpppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp] w - - 0 1";
        v->enclosingDropStart = make_bitboard(SQ_E5, SQ_F5, SQ_E6, SQ_F6);
        return v;
    }
#ifdef ALLVARS
    // Game of the Amazons
    // https://en.wikipedia.org/wiki/Game_of_the_Amazons
    Variant* amazons_variant() {
        Variant* v = chess_variant_base()->init();
        v->pieceToCharTable = "....Q.....................q.................";
        v->maxRank = RANK_10;
        v->maxFile = FILE_J;
        v->reset_pieces();
        v->add_piece(CUSTOM_PIECE_1, 'q', "mQ");
        v->startFen = "3q2q3/10/10/q8q/10/10/Q8Q/10/10/3Q2Q3 w - - 0 1";
        v->stalemateValue = -VALUE_MATE;
        v->wallingRule = ARROW;
        return v;
    }
#endif
    // Xiangqi (Chinese chess)
    // https://en.wikipedia.org/wiki/Xiangqi
    // Xiangqi base variant for inheriting rules without chasing rules
    Variant* xiangqi_variant_base() {
        Variant* v = minixiangqi_variant()->init();
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
    Variant* xiangqi_variant() {
        Variant* v = xiangqi_variant_base()->init();
        v->chasingRule = AXF_CHASING;
        return v;
    }
    // Manchu/Yitong chess
    // Asymmetric Xiangqi variant with a super-piece
    // https://en.wikipedia.org/wiki/Manchu_chess
    Variant* manchu_variant() {
        Variant* v = xiangqi_variant_base()->init();
        v->pieceToCharTable = "PN.R.AB..K.C....M.....pn.r.ab..k.c..........";
        v->add_piece(BANNER, 'm');
        v->startFen = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/9/9/M1BAKAB2 w - - 0 1";
        return v;
    }
    // Supply chess
    // https://en.wikipedia.org/wiki/Xiangqi#Variations
    Variant* supply_variant() {
        Variant* v = xiangqi_variant_base()->init();
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
        Variant* v = xiangqi_variant_base()->init();
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
        v->pass[WHITE] = true;
        v->pass[BLACK] = true;
        v->nFoldValue = VALUE_DRAW;
        v->perpetualCheckIllegal = true;
        return v;
    }
    // Traditional rules of Janggi, where bikjang is a draw
    Variant* janggi_traditional_variant() {
        Variant* v = janggi_variant()->init();
        v->bikjangRule = true;
        v->materialCounting = NO_MATERIAL_COUNTING;
        v->nnueAlias = "janggi";
        return v;
    }
    // Modern rules of Janggi, where bikjang is not considered, but material counting is.
    // The repetition rules are also adjusted for better compatibility with Kakao Janggi.
    Variant* janggi_modern_variant() {
        Variant* v = janggi_variant()->init();
        v->bikjangRule = false;
        v->materialCounting = JANGGI_MATERIAL;
        v->moveRepetitionIllegal = true;
        v->nFoldRule = 4; // avoid nFold being triggered before move repetition
        v->nMoveRule = 100; // avoid adjudication before reaching 200 half-moves
        v->nnueAlias = "janggi";
        return v;
    }
    // Casual rules of Janggi, where bikjang and material counting are not considered
    Variant* janggi_casual_variant() {
        Variant* v = janggi_variant()->init();
        v->bikjangRule = false;
        v->materialCounting = NO_MATERIAL_COUNTING;
        v->nnueAlias = "janggi";
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
    add("armageddon", armageddon_variant());
    add("torpedo", torpedo_variant());
    add("berolina", berolina_variant());
    add("pawnsideways", pawnsideways_variant());
    add("pawnback", pawnback_variant());
    add("legan", legan_variant());
    add("fairy", fairy_variant()); // fairy variant used for endgame code initialization
    add("makruk", makruk_variant());
    add("makpong", makpong_variant());
    add("cambodian", cambodian_variant());
    add("karouk", karouk_variant());
    add("asean", asean_variant());
    add("ai-wok", aiwok_variant());
    add("shatranj", shatranj_variant());
    add("chaturanga", chaturanga_variant());
    add("amazon", amazon_variant());
    add("nightrider", nightrider_variant());
    add("grasshopper", grasshopper_variant());
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
    add("threekings", threekings_variant());
    add("horde", horde_variant());
    add("petrified", petrified_variant());
    add("nocheckatomic", nocheckatomic_variant());
    add("atomic", atomic_variant());
    add("atomar", atomar_variant());
    add("isolation", isolation_variant());
    add("isolation7x7", isolation7x7_variant());
    add("snailtrail", snailtrail_variant());
    add("fox-and-hounds", fox_and_hounds_variant());
#ifdef ALLVARS
    add("duck", duck_variant());
#endif
    add("joust", joust_variant());
    add("3check", threecheck_variant());
    add("5check", fivecheck_variant());
    add("crazyhouse", crazyhouse_variant());
    add("loop", loop_variant());
    add("chessgi", chessgi_variant());
    add("bughouse", bughouse_variant());
    add("koedem", koedem_variant());
    add("pocketknight", pocketknight_variant());
    add("placement", placement_variant());
    add("sittuyin", sittuyin_variant());
    add("seirawan", seirawan_variant());
    add("musketeer", musketeer_variant());
    add("shouse", shouse_variant());
    add("dragon", dragon_variant());
    add("paradigm", paradigm_variant());
    add("minishogi", minishogi_variant());
    add("mini", minishogi_variant());
    add("kyotoshogi", kyotoshogi_variant());
    add("micro", microshogi_variant());
    add("dobutsu", dobutsu_variant());
    add("gorogoro", gorogoroshogi_variant());
    add("judkins", judkinsshogi_variant());
    add("torishogi", torishogi_variant());
    add("euroshogi", euroshogi_variant());
    add("losalamos", losalamos_variant());
    add("gardner", gardner_variant());
    add("almost", almost_variant());
    add("sortofalmost", sortofalmost_variant());
    add("chigorin", chigorin_variant());
    add("perfect", perfect_variant());
    add("spartan", spartan_variant());
    add("shatar", shatar_variant());
    add("coregal", coregal_variant());
    add("clobber", clobber_variant());
    add("breakthrough", breakthrough_variant());
    add("ataxx", ataxx_variant());
    add("flipersi", flipersi_variant());
    add("flipello", flipello_variant());
    add("minixiangqi", minixiangqi_variant());
    add("raazuvaa", raazuvaa_variant());
#ifdef LARGEBOARDS
    add("shogi", shogi_variant());
    add("shoshogi", shoshogi_variant());
    add("yarishogi", yarishogi_variant());
    add("okisakishogi", okisakishogi_variant());
    add("capablanca", capablanca_variant());
    add("capahouse", capahouse_variant());
    add("caparandom", caparandom_variant());
    add("gothic", gothic_variant());
    add("janus", janus_variant());
    add("modern", modern_variant());
    add("chancellor", chancellor_variant());
    add("embassy", embassy_variant());
    add("centaur", centaur_variant());
    add("gustav3", gustav3_variant());
    add("jesonmor", jesonmor_variant());
    add("courier", courier_variant());
    add("grand", grand_variant());
    add("opulent", opulent_variant());
    add("tencubed", tencubed_variant());
    add("omicron", omicron_variant());
    add("troitzky", troitzky_variant());
    add("wolf", wolf_variant());
    add("shako", shako_variant());
    add("clobber10", clobber10_variant());
    add("flipello10", flipello10_variant());
#ifdef ALLVARS
    add("amazons", amazons_variant());
#endif
    add("xiangqi", xiangqi_variant());
    add("manchu", manchu_variant());
    add("supply", supply_variant());
    add("janggi", janggi_variant());
    add("janggitraditional", janggi_traditional_variant());
    add("janggimodern", janggi_modern_variant());
    add("janggicasual", janggi_casual_variant());
#endif
}


// Pre-calculate derived properties
Variant* Variant::conclude() {
    // Enforce consistency to allow runtime optimizations
    if (!doubleStep)
        doubleStepRegion[WHITE] = doubleStepRegion[BLACK] = 0;
    if (!doubleStepRegion[WHITE] && !doubleStepRegion[BLACK])
        doubleStep = false;

    // Determine optimizations
    bool restrictedMobility = false;
    for (PieceSet ps = pieceTypes; !restrictedMobility && ps;)
    {
        PieceType pt = pop_lsb(ps);
        if (mobilityRegion[WHITE][pt] || mobilityRegion[BLACK][pt])
          restrictedMobility = true;
    }
    fastAttacks =  !(pieceTypes & ~(CHESS_PIECES | COMMON_FAIRY_PIECES))
                  && kingType == KING
                  && !restrictedMobility
                  && !cambodianMoves
                  && !diagonalLines;
    fastAttacks2 =  !(pieceTypes & ~(SHOGI_PIECES | COMMON_STEP_PIECES))
                  && kingType == KING
                  && !restrictedMobility
                  && !cambodianMoves
                  && !diagonalLines;

    // Initialize calculated NNUE properties
    nnueKing =  pieceTypes & KING ? KING
              : extinctionPieceCount == 0 && (extinctionPieceTypes & COMMONER) ? COMMONER
              : NO_PIECE_TYPE;
    // The nnueKing has to present exactly once and must not change in count
    if (nnueKing != NO_PIECE_TYPE)
    {
        // If the nnueKing is involved in promotion, count might change
        if (   ((promotionPawnTypes[WHITE] | promotionPawnTypes[BLACK]) & nnueKing)
            || ((promotionPieceTypes[WHITE] | promotionPieceTypes[BLACK]) & nnueKing)
            || std::find(std::begin(promotedPieceType), std::end(promotedPieceType), nnueKing) != std::end(promotedPieceType))
            nnueKing = NO_PIECE_TYPE;
    }
    if (nnueKing != NO_PIECE_TYPE)
    {
        std::string fenBoard = startFen.substr(0, startFen.find(' '));
        // Switch NNUE from KA to A if there is no unique piece
        if (   std::count(fenBoard.begin(), fenBoard.end(), pieceToChar[make_piece(WHITE, nnueKing)]) != 1
            || std::count(fenBoard.begin(), fenBoard.end(), pieceToChar[make_piece(BLACK, nnueKing)]) != 1)
            nnueKing = NO_PIECE_TYPE;
    }
    // We can not use popcount here yet, as the lookup tables are initialized after the variants
    int nnueSquares = (maxRank + 1) * (maxFile + 1);
    nnueUsePockets = (pieceDrops && (capturesToHand || (!mustDrop && std::bitset<64>(pieceTypes).count() != 1))) || seirawanGating;
    int nnuePockets = nnueUsePockets ? 2 * int(maxFile + 1) : 0;
    int nnueNonDropPieceIndices = (2 * std::bitset<64>(pieceTypes).count() - (nnueKing != NO_PIECE_TYPE)) * nnueSquares;
    int nnuePieceIndices = nnueNonDropPieceIndices + 2 * (std::bitset<64>(pieceTypes).count() - (nnueKing != NO_PIECE_TYPE)) * nnuePockets;
    int i = 0;
    for (PieceSet ps = pieceTypes; ps;)
    {
        // Make sure that the nnueKing type gets the last index, since the NNUE architecture relies on that
        PieceType pt = lsb(ps != piece_set(nnueKing) ? ps & ~piece_set(nnueKing) : ps);
        ps ^= pt;
        assert(pt != nnueKing || !ps);

        for (Color c : { WHITE, BLACK})
        {
            pieceSquareIndex[c][make_piece(c, pt)] = 2 * i * nnueSquares;
            pieceSquareIndex[c][make_piece(~c, pt)] = (2 * i + (pt != nnueKing)) * nnueSquares;
            pieceHandIndex[c][make_piece(c, pt)] = 2 * i * nnuePockets + nnueNonDropPieceIndices;
            pieceHandIndex[c][make_piece(~c, pt)] = (2 * i + 1) * nnuePockets + nnueNonDropPieceIndices;
        }
        i++;
    }

    // Map king squares to enumeration of actually available squares.
    // E.g., for xiangqi map from 0-89 to 0-8.
    // Variants might be initialized before bitboards, so do not rely on precomputed bitboards (like SquareBB).
    // Furthermore conclude() might be called on invalid configuration during validation,
    // therefore skip proper initialization in case of invalid board size.
    int nnueKingSquare = 0;
    if (nnueKing && nnueSquares <= SQUARE_NB)
        for (Square s = SQ_A1; s < nnueSquares; ++s)
        {
            Square bitboardSquare = Square(s + s / (maxFile + 1) * (FILE_MAX - maxFile));
            if (   !mobilityRegion[WHITE][nnueKing] || !mobilityRegion[BLACK][nnueKing]
                || (mobilityRegion[WHITE][nnueKing] & make_bitboard(bitboardSquare))
                || (mobilityRegion[BLACK][nnueKing] & make_bitboard(relative_square(BLACK, bitboardSquare, maxRank))))
            {
                kingSquareIndex[s] = nnueKingSquare++ * nnuePieceIndices;
            }
        }
    else
        kingSquareIndex[SQ_A1] = nnueKingSquare++ * nnuePieceIndices;
    nnueDimensions = nnueKingSquare * nnuePieceIndices;

    // Determine maximum piece count
    std::istringstream ss(startFen);
    ss >> std::noskipws;
    unsigned char token;
    nnueMaxPieces = 0;
    while ((ss >> token) && !isspace(token))
    {
        if (pieceToChar.find(token) != std::string::npos || pieceToCharSynonyms.find(token) != std::string::npos)
            nnueMaxPieces++;
    }
    if (twoBoards)
        nnueMaxPieces *= 2;

    // For endgame evaluation to be applicable, no special win rules must apply.
    // Furthermore, rules significantly changing game mechanics also invalidate it.
    endgameEval = extinctionValue == VALUE_NONE
                  && checkmateValue == -VALUE_MATE
                  && stalemateValue == VALUE_DRAW
                  && !materialCounting
                  && !(flagRegion[WHITE] || flagRegion[BLACK])
                  && !mustCapture
                  && !checkCounting
                  && !makpongRule
                  && !connectN
                  && !blastOnCapture
                  && !petrifyOnCaptureTypes
                  && !capturesToHand
                  && !twoBoards
                  && !restrictedMobility
                  && kingType == KING;

    shogiStylePromotions = false;
    for (PieceType current: promotedPieceType)
        if (current != NO_PIECE_TYPE)
        {
            shogiStylePromotions = true;
            break;
        }

    connect_directions.clear();
    if (connectHorizontal)
    {
        connect_directions.push_back(EAST);
    }
    if (connectVertical)
    {
        connect_directions.push_back(NORTH);
    }
    if (connectDiagonal)
    {
        connect_directions.push_back(NORTH_EAST);
        connect_directions.push_back(SOUTH_EAST);
    }

    return this;
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
            if (!input.empty() && input.back() == '\r')
                input.pop_back();
            std::stringstream ss(input);
            if (ss.peek() != ';' && ss.peek() != '#')
            {
                if (DoCheck && !input.empty() && input.find('=') == std::string::npos)
                    std::cerr << "Invalid syntax: '" << input << "'." << std::endl;
                if (std::getline(std::getline(ss, key, '=') >> std::ws, value) && !key.empty())
                    attribs[key.erase(key.find_last_not_of(" ") + 1)] = value;
            }
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
            Variant* v = !variant_template.empty() ? VariantParser<DoCheck>(attribs).parse((new Variant(*variants.find(variant_template)->second))->init())
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

void VariantMap::add(std::string s, Variant* v) {
  insert(std::pair<std::string, const Variant*>(s, v->conclude()));
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

} // namespace Stockfish
