/*
  ffish.js, a JavaScript chess variant library derived from Fairy-Stockfish
  Copyright (C) 2020 Fabian Fichter, Johannes Czech

  ffish.js is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ffish.js is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <emscripten.h>
#include <emscripten/bind.h>
#include <vector>
#include <string>
#include <sstream>
#include<iostream>

#include "misc.h"
#include "types.h"
#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "syzygy/tbprobe.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "piece.h"
#include "variant.h"
#include "movegen.h"
#include "apiutil.h"

using namespace emscripten;


void initialize_stockfish() {
  pieceMap.init();
  variants.init();
  UCI::init(Options);
  Bitboards::init();
  Position::init();
  Bitbases::init();
}

#define DELIM " "

inline void save_pop_back(std::string& s) {
  if (s.size() != 0) {
    s.pop_back();
  }
}

const Variant* get_variant(const std::string& uciVariant) {
  if (uciVariant.size() == 0)
    return variants.find("chess")->second;
  return variants.find(uciVariant)->second;
}

class Board {
  // note: we can't use references for strings here due to conversion to JavaScript
private:
  const Variant* v;
  StateListPtr states;
  Position pos;
  Thread* thread;
  std::vector<Move> moveStack;
  bool is960;

public:
  static bool sfInitialized;

  Board():
    Board("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" , false) {
  }

  Board(std::string uciVariant):
    Board(uciVariant, "", false) {
  }

  Board(std::string uciVariant, std::string fen):
    Board(uciVariant, fen, false) {
  }

  Board(std::string uciVariant, std::string fen, bool is960) {
    init(uciVariant, fen, is960);
  }

  std::string legal_moves() {
    std::string moves;
    for (const ExtMove& move : MoveList<LEGAL>(this->pos)) {
      moves += UCI::move(this->pos, move);
      moves += DELIM;
    }
    save_pop_back(moves);
    return moves;
  }

  std::string legal_moves_san() {
    std::string movesSan;
    for (const ExtMove& move : MoveList<LEGAL>(this->pos)) {
      movesSan += move_to_san(this->pos, move, NOTATION_SAN);
      movesSan += DELIM;
    }
    save_pop_back(movesSan);
    return movesSan;
  }

  int number_legal_moves() const {
    return MoveList<LEGAL>(pos).size();
  }

  void push(std::string uciMove) {
    do_move(UCI::to_move(this->pos, uciMove));
  }

  bool push_san(std::string sanMove) {
    return push_san(sanMove, NOTATION_SAN);
  }

  // TODO: This is a naive implementation which compares all legal SAN moves with the requested string.
  // If the SAN move wasn't found the position remains unchanged. Alternatively, implement a direct conversion.
  bool push_san(std::string sanMove, Notation notation) {
    Move foundMove = MOVE_NONE;
    for (const ExtMove& move : MoveList<LEGAL>(pos)) {
      if (sanMove == move_to_san(this->pos, move, notation)) {
        foundMove = move;
        break;
      }
    }
    if (foundMove != MOVE_NONE) {
      do_move(foundMove);
      return true;
    }
    return false;
  }

  void pop() {
    pos.undo_move(this->moveStack.back());
    moveStack.pop_back();
    states->pop_back();
  }

  void reset() {
    set_fen(v->startFen);
  }

  bool is_960() const {
    return is960;
  }

  std::string fen() const {
    return this->pos.fen();
  }

  void set_fen(std::string fen) {
    resetStates();
    moveStack.clear();
    pos.set(v, fen, is960, &states->back(), thread);
  }

  // note: const identifier for pos not possible due to move_to_san()
  std::string san_move(std::string uciMove) {
    return move_to_san(this->pos, UCI::to_move(this->pos, uciMove), NOTATION_SAN);
  }

  std::string san_move(std::string uciMove, Notation notation) {
    return move_to_san(this->pos, UCI::to_move(this->pos, uciMove), notation);
  }
  std::string variation_san(std::string uciMoves) {
    return variation_san(uciMoves, NOTATION_SAN, true);
  }

  std::string variation_san(std::string uciMoves, Notation notation) {
    return variation_san(uciMoves, notation, true);
  }

  std::string variation_san(std::string uciMoves, Notation notation, bool moveNumbers) {
    std::stringstream ss(uciMoves);
    StateListPtr tempStates;
    std::vector<Move> moves;
    std::string variationSan = "";
    std::string uciMove;
    bool first = true;

    while (std::getline(ss, uciMove, ' ')) {
      moves.emplace_back(UCI::to_move(this->pos, uciMove));
      if (first) {
        first = false;
        if (moveNumbers) {
          variationSan = std::to_string(fullmove_number());
          if (pos.side_to_move() == WHITE)
          variationSan += ". ";
          else
          variationSan += "...";
        }
        variationSan += move_to_san(this->pos, moves.back(), Notation(notation));
      }
      else {
        if (moveNumbers && pos.side_to_move() == WHITE) {
          variationSan += DELIM;
          variationSan += std::to_string(fullmove_number());
          variationSan += ".";
        }
        variationSan += DELIM;
        variationSan += move_to_san(this->pos, moves.back(), Notation(notation));
      }
      states->emplace_back();
      pos.do_move(moves.back(), states->back());
    }

    // recover initial state
    for(auto rIt = std::rbegin(moves); rIt != std::rend(moves); ++rIt) {
      pos.undo_move(*rIt);
    }

    return variationSan;
  }

  // returns true for WHITE and false for BLACK
  bool turn() const {
    return !pos.side_to_move();
  }

  int fullmove_number() const {
    return pos.game_ply() / 2 + 1;
  }

  int halfmove_clock() const {
    return pos.rule50_count();
  }

  int game_ply() const {
    return pos.game_ply();
  }

  bool is_game_over() const {
    for (const ExtMove& move: MoveList<LEGAL>(pos))
      return false;
    return true;
  }

  bool is_check() const {
    return pos.checkers();
  }

  bool is_bikjang() const {
    return pos.bikjang();
  }

  std::string move_stack() const {
    std::string moves;
    for(auto it = std::begin(moveStack); it != std::end(moveStack); ++it) {
      moves += UCI::move(pos, *it);
      moves += DELIM;
    }
    save_pop_back(moves);
    return moves;
  }

  void push_moves(std::string uciMoves) {
    std::stringstream ss(uciMoves);
    std::string uciMove;
    while (std::getline(ss, uciMove, ' ')) {
      push(uciMove);
    }
  }

  void push_san_moves(std::string sanMoves) {
    return push_san_moves(sanMoves, NOTATION_SAN);
  }

  void push_san_moves(std::string sanMoves, Notation notation) {
    std::stringstream ss(sanMoves);
    std::string sanMove;
    while (std::getline(ss, sanMove, ' '))
      push_san(sanMove, notation);
  }

  std::string pocket(bool color) {
    const Color c = Color(!color);
    std::string pocket;
    for (PieceType pt = KING; pt >= PAWN; --pt) {
      for (int i = 0; i < pos.count_in_hand(c, pt); ++i) {
        // only create BLACK pieces in order to convert to lower case
        pocket += std::string(1, pos.piece_to_char()[make_piece(BLACK, pt)]);
      }
    }
    return pocket;
  }

  // TODO: return board in ascii notation
  // static std::string get_string_from_instance(const Board& board) {
  // }

private:
  void resetStates() {
    this->states = StateListPtr(new std::deque<StateInfo>(1));
  }

  void do_move(Move move) {
    states->emplace_back();
    this->pos.do_move(move, states->back());
    this->moveStack.emplace_back(move);
  }

  void init(std::string uciVariant, std::string fen, bool is960) {
    if (!Board::sfInitialized) {
      initialize_stockfish();
      Board::sfInitialized = true;
    }
    v = get_variant(uciVariant);
    this->resetStates();
    if (fen == "")
      fen = v->startFen;
    this->pos.set(this->v, fen, is960, &this->states->back(), this->thread);
    this->is960 = is960;
  }
};

bool Board::sfInitialized = false;

namespace ffish {
  // returns the version of the Fairy-Stockfish binary
  std::string info() {
    return engine_info();
  }

  template <typename T>
  void set_option(std::string name, T value) {
    Options[name] = value;
    Board::sfInitialized = false;
  }

  std::string available_variants() {
    std::string availableVariants;
    for (std::string variant : variants.get_keys()) {
      availableVariants += variant;
      availableVariants += DELIM;
    }
    save_pop_back(availableVariants);
    return availableVariants;
  }

  void load_variant_config(std::string variantInitContent) {
    std::stringstream ss(variantInitContent);
    if (!Board::sfInitialized)
      initialize_stockfish();
    variants.parse_istream<false>(ss);
    Options["UCI_Variant"].set_combo(variants.get_keys());
    Board::sfInitialized = true;
  }

  std::string starting_fen(std::string uciVariant) {
    const Variant* v = get_variant(uciVariant);
    return v->startFen;
  }

  int validate_fen(std::string fen, std::string uciVariant) {
    const Variant* v = get_variant(uciVariant);
    return fen::validate_fen(fen, v);
  }

  int validate_fen(std::string fen) {
    return validate_fen(fen, "chess");
  }
}

class Game {
private:
  std::unordered_map<std::string, std::string> header;
  std::unique_ptr<Board> board;
  std::string variant = "chess";
  std::string fen = ""; // start pos
  bool is960 = false;
  bool parsedGame = false;
public:
  std::string header_keys() {
    std::string keys;
    for (auto it = header.begin(); it != header.end(); ++it) {
      keys += it->first;
      keys += DELIM;
    }
    save_pop_back(keys);
    return keys;
  }

  std::string headers(std::string item) {
    auto it = header.find(item);
    if (it == header.end())
      return "";
    return it->second;
  }

  std::string mainline_moves() {
    if (!parsedGame)
    return "";
    return board->move_stack();
  }

  friend Game read_game_pgn(std::string);
};


Game read_game_pgn(std::string pgn) {
  Game game;
  size_t lineStart = 0;
  bool headersParsed = false;

  while(true) {
    size_t lineEnd = pgn.find('\n', lineStart);

    if (lineEnd == std::string::npos)
    lineEnd = pgn.size();

    if (!headersParsed && pgn[lineStart] == '[') {
      // parse header
      // look for item
      size_t headerKeyStart = lineStart+1;
      size_t headerKeyEnd = pgn.find(' ', lineStart);
      size_t headerItemStart = pgn.find('"', headerKeyEnd)+1;
      size_t headerItemEnd = pgn.find('"', headerItemStart);

      // put item into list
      game.header[pgn.substr(headerKeyStart, headerKeyEnd-headerKeyStart)] = pgn.substr(headerItemStart, headerItemEnd-headerItemStart);
    }
    else {
      if (!headersParsed) {
        headersParsed = true;
        auto it = game.header.find("Variant");
        if (it != game.header.end()) {
          game.variant = it->second;
          std::transform(game.variant.begin(), game.variant.end(), game.variant.begin(),
          [](unsigned char c){ return std::tolower(c); });
          game.is960 = it->second.find("960") != std::string::npos;
        }

        it = game.header.find("FEN");
        if (it != game.header.end())
        game.fen = it->second;

        game.board = std::make_unique<Board>(game.variant, game.fen, game.is960);
        game.parsedGame = true;
      }

      // game line
      size_t curIdx = lineStart;
      while (curIdx <= lineEnd) {
        if (pgn[curIdx] == '*')
        return game;

        while (pgn[curIdx] == '{') {
          // skip comment
          curIdx = pgn.find('}', curIdx);
          if (curIdx == std::string::npos) {
            std::cerr << "Missing '}' for move comment while reading pgn." << std::endl;
            return game;
          }
          curIdx += 2;
        }
        while (pgn[curIdx] == '(') {
          // skip comment
          curIdx = pgn.find(')', curIdx);
          if (curIdx == std::string::npos) {
            std::cerr << "Missing ')' for move comment while reading pgn." << std::endl;
            return game;
          }
          curIdx += 2;
        }

        if (pgn[curIdx] >= '0' && pgn[curIdx] <= '9') {
          // we are at a move number -> look for next point
          curIdx = pgn.find('.', curIdx);
          if (curIdx == std::string::npos)
          break;
          ++curIdx;
          // increment if we're at a space
          while (curIdx < pgn.size() && pgn[curIdx] == ' ')
          ++curIdx;
          // increment if we're at a point
          while (curIdx < pgn.size() && pgn[curIdx] == '.')
          ++curIdx;
        }
        // extract sanMove
        size_t sanMoveEnd = std::min(pgn.find(' ', curIdx), lineEnd);
        if (sanMoveEnd > curIdx) {
          std::string sanMove = pgn.substr(curIdx, sanMoveEnd-curIdx);
          // clean possible ? and ! from string
          size_t annotationChar1 = sanMove.find('?');
          size_t annotationChar2 = sanMove.find('!');
          if (annotationChar1 != std::string::npos || annotationChar2 != std::string::npos)
          sanMove = sanMove.substr(0, std::min(annotationChar1, annotationChar2));
          game.board->push_san(sanMove);
        }
        curIdx = sanMoveEnd+1;
      }
    }
    lineStart = lineEnd+1;

    if (lineStart >= pgn.size())
    return game;
  }
  return game;
}


// binding code
EMSCRIPTEN_BINDINGS(ffish_js) {
  class_<Board>("Board")
    .constructor<>()
    .constructor<std::string>()
    .constructor<std::string, std::string>()
    .constructor<std::string, std::string, bool>()
    .function("legalMoves", &Board::legal_moves)
    .function("legalMovesSan", &Board::legal_moves_san)
    .function("numberLegalMoves", &Board::number_legal_moves)
    .function("push", &Board::push)
    .function("pushSan", select_overload<bool(std::string)>(&Board::push_san))
    .function("pushSan", select_overload<bool(std::string, Notation)>(&Board::push_san))
    .function("pop", &Board::pop)
    .function("reset", &Board::reset)
    .function("is960", &Board::is_960)
    .function("fen", &Board::fen)
    .function("setFen", &Board::set_fen)
    .function("sanMove", select_overload<std::string(std::string)>(&Board::san_move))
    .function("sanMove", select_overload<std::string(std::string, Notation)>(&Board::san_move))
    .function("variationSan", select_overload<std::string(std::string)>(&Board::variation_san))
    .function("variationSan", select_overload<std::string(std::string, Notation)>(&Board::variation_san))
    .function("variationSan", select_overload<std::string(std::string, Notation, bool)>(&Board::variation_san))
    .function("turn", &Board::turn)
    .function("fullmoveNumber", &Board::fullmove_number)
    .function("halfmoveClock", &Board::halfmove_clock)
    .function("gamePly", &Board::game_ply)
    .function("isGameOver", &Board::is_game_over)
    .function("isCheck", &Board::is_check)
    .function("isBikjang", &Board::is_bikjang)
    .function("moveStack", &Board::move_stack)
    .function("pushMoves", &Board::push_moves)
    .function("pushSanMoves", select_overload<void(std::string)>(&Board::push_san_moves))
    .function("pushSanMoves", select_overload<void(std::string, Notation)>(&Board::push_san_moves))
    .function("pocket", &Board::pocket);
  class_<Game>("Game")
    .function("headerKeys", &Game::header_keys)
    .function("headers", &Game::headers)
    .function("mainlineMoves", &Game::mainline_moves);
  // usage: e.g. ffish.Notation.DEFAULT
  enum_<Notation>("Notation")
    .value("DEFAULT", NOTATION_DEFAULT)
    .value("SAN", NOTATION_SAN)
    .value("LAN", NOTATION_LAN)
    .value("SHOGI_HOSKING", NOTATION_SHOGI_HOSKING)
    .value("SHOGI_HODGES", NOTATION_SHOGI_HODGES)
    .value("SHOGI_HODGES_NUMBER", NOTATION_SHOGI_HODGES_NUMBER)
    .value("JANGGI", NOTATION_JANGGI)
    .value("XIANGQI_WXF", NOTATION_XIANGQI_WXF);
  function("info", &ffish::info);
  function("setOption", &ffish::set_option<std::string>);
  function("setOptionInt", &ffish::set_option<int>);
  function("setOptionBool", &ffish::set_option<bool>);
  function("readGamePGN", &read_game_pgn);
  function("variants", &ffish::available_variants);
  function("loadVariantConfig", &ffish::load_variant_config);
  function("startingFen", &ffish::starting_fen);
  function("validateFen", select_overload<int(std::string)>(&ffish::validate_fen));
  function("validateFen", select_overload<int(std::string, std::string)>(&ffish::validate_fen));
  // TODO: enable to string conversion method
  // .class_function("getStringFromInstance", &Board::get_string_from_instance);
}
