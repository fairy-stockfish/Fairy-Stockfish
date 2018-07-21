/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#include <string>

#include "variant.h"

using std::string;

VariantMap variants; // Global object

void VariantMap::init() {
    // Define variant rules
    const Variant* chess = [&]{
        Variant* v = new Variant();
        v->endgameEval = true;
        return v;
    } ();
    const Variant* makruk = [&]{
        Variant* v = new Variant();
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 's');
        v->add_piece(MET, 'm');
        v->startFen = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {MET};
        v->endgameEval = true;
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* asean = [&]{
        Variant* v = new Variant();
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 'b');
        v->add_piece(MET, 'q');
        v->startFen = "rnbqkbnr/8/pppppppp/8/8/PPPPPPPP/8/RNBQKBNR w - - 0 1";
        v->promotionPieceTypes = {ROOK, KNIGHT, KHON, MET};
        v->endgameEval = true;
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* aiwok = [&]{
        Variant* v = new Variant();
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(KHON, 's');
        v->add_piece(AIWOK, 'a');
        v->startFen = "rnsaksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKASNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {AIWOK};
        v->endgameEval = true;
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* shatranj = [&]{
        Variant* v = new Variant();
        v->remove_piece(BISHOP);
        v->remove_piece(QUEEN);
        v->add_piece(ALFIL, 'b');
        v->add_piece(FERS, 'q');
        v->startFen = "rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w - - 0 1";
        v->promotionPieceTypes = {FERS};
        v->endgameEval = true;
        v->doubleStep = false;
        v->castling = false;
        v->bareKingValue = -VALUE_MATE;
        v->bareKingMove = true;
        v->stalemateValue = -VALUE_MATE;
        return v;
    } ();
    const Variant* amazon = [&]{
        Variant* v = new Variant();
        v->remove_piece(QUEEN);
        v->add_piece(AMAZON, 'a');
        v->startFen = "rnbakbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBAKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {AMAZON, ROOK, BISHOP, KNIGHT};
        v->endgameEval = true;
        return v;
    } ();
    const Variant* hoppelpoppel = [&]{
        Variant* v = new Variant();
        v->remove_piece(KNIGHT);
        v->remove_piece(BISHOP);
        v->add_piece(KNIBIS, 'n');
        v->add_piece(BISKNI, 'b');
        v->promotionPieceTypes = {QUEEN, ROOK, BISKNI, KNIBIS};
        v->endgameEval = true;
        return v;
    } ();
    const Variant* kingofthehill = [&]{
        Variant* v = new Variant();
        v->whiteFlag = make_bitboard(SQ_D4, SQ_E4, SQ_D5, SQ_E5);
        v->blackFlag = make_bitboard(SQ_D4, SQ_E4, SQ_D5, SQ_E5);
        v->flagMove = false;
        return v;
    } ();
    const Variant* racingkings = [&]{
        Variant* v = new Variant();
        v->startFen = "8/8/8/8/8/8/krbnNBRK/qrbnNBRQ w - - 0 1";
        v->whiteFlag = Rank8BB;
        v->blackFlag = Rank8BB;
        v->flagMove = true;
        v->castling = false;
        v->checking = false;
        return v;
    } ();
    const Variant* losers = [&]{
        Variant* v = new Variant();
        v->checkmateValue = VALUE_MATE;
        v->stalemateValue = VALUE_MATE;
        v->bareKingValue = VALUE_MATE;
        v->bareKingMove = false;
        v->mustCapture = true;
        return v;
    } ();
    const Variant* giveaway = [&]{
        Variant* v = new Variant();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT};
        v->stalemateValue = VALUE_MATE;
        v->extinctionValue = VALUE_MATE;
        v->extinctionPieceTypes = {ALL_PIECES};
        v->mustCapture = true;
        return v;
    } ();
    const Variant* antichess = [&]{
        Variant* v = new Variant();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT};
        v->stalemateValue = VALUE_MATE;
        v->extinctionValue = VALUE_MATE;
        v->extinctionPieceTypes = {ALL_PIECES};
        v->castling = false;
        v->mustCapture = true;
        return v;
    } ();
    const Variant* extinction = [&]{
        Variant* v = new Variant();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT};
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {COMMONER, QUEEN, ROOK, BISHOP, KNIGHT, PAWN};
        return v;
    } ();
    const Variant* kinglet = [&]{
        Variant* v = new Variant();
        v->remove_piece(KING);
        v->add_piece(COMMONER, 'k');
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {COMMONER};
        v->extinctionValue = -VALUE_MATE;
        v->extinctionPieceTypes = {PAWN};
        return v;
    } ();
    const Variant* threecheck = [&]{
        Variant* v = new Variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+3 0 1";
        v->maxCheckCount = CheckCount(3);
        return v;
    } ();
    const Variant* fivecheck = [&]{
        Variant* v = new Variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 5+5 0 1";
        v->maxCheckCount = CheckCount(5);
        return v;
    } ();
    const Variant* crazyhouse = [&]{
        Variant* v = new Variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        return v;
    } ();
    const Variant* loop = [&]{
        Variant* v = new Variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        v->dropLoop = true;
        return v;
    } ();
    const Variant* chessgi = [&]{
        Variant* v = new Variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 0 1";
        v->pieceDrops = true;
        v->dropLoop = true;
        v->capturesToHand = true;
        v->firstRankDrops = true;
        return v;
    } ();
    const Variant* pocketknight = [&]{
        Variant* v = new Variant();
        v->startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[Nn] w KQkq - 0 1";
        v->pieceDrops = true;
        v->capturesToHand = false;
        return v;
    } ();
    const Variant* euroshogi = [&]{
        Variant* v = new Variant();
        v->reset_pieces();
        v->add_piece(SHOGI_PAWN, 'p');
        v->add_piece(EUROSHOGI_KNIGHT, 'n');
        v->add_piece(GOLD, 'g');
        v->add_piece(BISHOP, 'b');
        v->add_piece(HORSE, 'h');
        v->add_piece(ROOK, 'r');
        v->add_piece(KING, 'k');
        v->add_piece(DRAGON, 'd');
        v->startFen = "1nbgkgn1/1r4b1/pppppppp/8/8/PPPPPPPP/1B4R1/1NGKGBN1[-] w 0 1";
        v->pieceDrops = true;
        v->capturesToHand = true;
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {};
        v->doubleStep = false;
        v->castling = false;
        v->promotedPieceType[SHOGI_PAWN]       = GOLD;
        v->promotedPieceType[EUROSHOGI_KNIGHT] = GOLD;
        v->promotedPieceType[SILVER]           = GOLD;
        v->promotedPieceType[BISHOP]           = HORSE;
        v->promotedPieceType[ROOK]             = DRAGON;
        v->mandatoryPiecePromotion = true;
        return v;
    } ();
    const Variant* minishogi = [&]{
        Variant* v = new Variant();
        v->maxRank = RANK_5;
        v->maxFile = FILE_E;
        v->reset_pieces();
        v->add_piece(SHOGI_PAWN, 'p');
        v->add_piece(SILVER, 's');
        v->add_piece(GOLD, 'g');
        v->add_piece(BISHOP, 'b');
        v->add_piece(HORSE, 'h');
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
        v->promotedPieceType[BISHOP]     = HORSE;
        v->promotedPieceType[ROOK]       = DRAGON;
        return v;
    } ();
    const Variant* losalamos = [&]{
        Variant* v = new Variant();
        v->maxRank = RANK_6;
        v->maxFile = FILE_F;
        v->remove_piece(BISHOP);
        v->startFen = "rnqknr/pppppp/6/6/PPPPPP/RNQKNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {QUEEN, ROOK, KNIGHT};
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* almost = [&]{
        Variant* v = new Variant();
        v->remove_piece(QUEEN);
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rnbckbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBCKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {CHANCELLOR, ROOK, BISHOP, KNIGHT};
        v->endgameEval = true;
        return v;
    } ();
    const Variant* chigorin = [&]{
        Variant* v = new Variant();
        v->add_piece(CHANCELLOR, 'c');
        v->startFen = "rbbqkbbr/pppppppp/8/8/8/8/PPPPPPPP/RNNCKNNR w KQkq - 0 1";
        v->promotionPieceTypes = {QUEEN, CHANCELLOR, ROOK, BISHOP, KNIGHT};
        v->endgameEval = true;
        return v;
    } ();
    const Variant* shatar = [&]{
        Variant* v = new Variant();
        v->remove_piece(QUEEN);
        v->add_piece(BERS, 'j');
        v->startFen = "rnbjkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBJKBNR w - - 0 1";
        v->promotionPieceTypes = {BERS};
        v->endgameEval = true;
        v->doubleStep = false;
        v->castling = false;
        v->bareKingValue = VALUE_DRAW;
        // TODO: illegal checkmates
        return v;
    } ();

    // Add to UCI_Variant option
    add("chess", chess);
    add("standard", chess);
    add("makruk", makruk);
    add("asean", asean);
    add("ai-wok", aiwok);
    add("shatranj", shatranj);
    add("amazon", amazon);
    add("hoppelpoppel", hoppelpoppel);
    add("kingofthehill", kingofthehill);
    add("racingkings", racingkings);
    add("losers", losers);
    add("giveaway", giveaway);
    add("antichess", antichess);
    add("extinction", extinction);
    add("kinglet", kinglet);
    add("3check", threecheck);
    add("5check", fivecheck);
    add("crazyhouse", crazyhouse);
    add("loop", loop);
    add("chessgi", chessgi);
    add("pocketknight", pocketknight);
    add("euroshogi", euroshogi);
    add("minishogi", minishogi);
    add("losalamos", losalamos);
    add("almost", almost);
    add("chigorin", chigorin);
    add("shatar", shatar);
}

void VariantMap::add(std::string s, const Variant* v) {
  insert(std::pair<std::string, const Variant*>(s, v));
}

void VariantMap::clear_all() {
  std::set<const Variant*> deleted_vars;
  for (auto const& element : *this) {
      // Delete duplicated variants (synonyms) only once
      if (deleted_vars.find(element.second) == deleted_vars.end())
      {
          delete element.second;
          deleted_vars.insert(element.second);
      }
  }
  clear();
}

std::vector<std::string> VariantMap::get_keys() {
  std::vector<std::string> keys;
  for (auto const& element : *this) {
      keys.push_back(element.first);
  }
  return keys;
}
