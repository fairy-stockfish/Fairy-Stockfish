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
    const Variant* chess = [&]{
        Variant* v = new Variant();
        return v;
    } ();
    const Variant* makruk = [&]{
        Variant* v = new Variant();
        v->reset_pieces();
        v->set_piece(PAWN, 'p');
        v->set_piece(KNIGHT, 'n');
        v->set_piece(KHON, 's');
        v->set_piece(ROOK, 'r');
        v->set_piece(MET, 'm');
        v->set_piece(KING, 'k');
        v->startFen = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {MET};
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* asean = [&]{
        Variant* v = new Variant();
        v->reset_pieces();
        v->set_piece(PAWN, 'p');
        v->set_piece(KNIGHT, 'n');
        v->set_piece(KHON, 'b');
        v->set_piece(ROOK, 'r');
        v->set_piece(MET, 'q');
        v->set_piece(KING, 'k');
        v->startFen = "rnbqkbnr/8/pppppppp/8/8/PPPPPPPP/8/RNBQKBNR w - - 0 1";
        v->promotionPieceTypes = {ROOK, KNIGHT, KHON, MET};
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* aiwok = [&]{
        Variant* v = new Variant();
        v->reset_pieces();
        v->set_piece(PAWN, 'p');
        v->set_piece(KNIGHT, 'n');
        v->set_piece(KHON, 's');
        v->set_piece(ROOK, 'r');
        v->set_piece(AIWOK, 'a');
        v->set_piece(KING, 'k');
        v->startFen = "rnsaksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKASNR w - - 0 1";
        v->promotionRank = RANK_6;
        v->promotionPieceTypes = {AIWOK};
        v->doubleStep = false;
        v->castling = false;
        return v;
    } ();
    const Variant* shatranj = [&]{
        Variant* v = new Variant();
        v->reset_pieces();
        v->set_piece(PAWN, 'p');
        v->set_piece(KNIGHT, 'n');
        v->set_piece(ALFIL, 'b');
        v->set_piece(ROOK, 'r');
        v->set_piece(FERS, 'q');
        v->set_piece(KING, 'k');
        v->startFen = "rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w - - 0 1";
        v->promotionPieceTypes = {FERS};
        v->doubleStep = false;
        v->castling = false;
        // TODO: bare king, stalemate
        return v;
    } ();
    const Variant* amazon = [&]{
        Variant* v = new Variant();
        v->set_piece(QUEEN, ' ');
        v->set_piece(AMAZON, 'a');
        v->startFen = "rnbakbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBAKBNR w KQkq - 0 1";
        v->promotionPieceTypes = {AMAZON, ROOK, BISHOP, KNIGHT};
        return v;
    } ();
    const Variant* hoppelpoppel = [&]{
        Variant* v = new Variant();
        v->set_piece(KNIGHT, ' ');
        v->set_piece(BISHOP, ' ');
        v->set_piece(KNIBIS, 'n');
        v->set_piece(BISKNI, 'b');
        v->promotionPieceTypes = {QUEEN, ROOK, BISKNI, KNIBIS};
        return v;
    } ();
    insert(std::pair<std::string, const Variant*>(std::string("chess"), chess));
    insert(std::pair<std::string, const Variant*>(std::string("makruk"), makruk));
    insert(std::pair<std::string, const Variant*>(std::string("asean"), asean));
    insert(std::pair<std::string, const Variant*>(std::string("ai-wok"), aiwok));
    insert(std::pair<std::string, const Variant*>(std::string("shatranj"), shatranj));
    insert(std::pair<std::string, const Variant*>(std::string("amazon"), amazon));
    insert(std::pair<std::string, const Variant*>(std::string("hoppelpoppel"), hoppelpoppel));
}

void VariantMap::clear_all() {
  for (auto const& element : *this) {
      delete element.second;
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
