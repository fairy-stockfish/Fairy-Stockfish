/*
  ffish.js, a JavaScript chess variant library derived from Fairy-Stockfish
  Copyright (C) 2021 Fabian Fichter, Johannes Czech

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

using namespace Stockfish;

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
  if (uciVariant.size() == 0 || uciVariant == "Standard" || uciVariant == "standard")
    return variants.find("chess")->second;
  return variants.find(uciVariant)->second;
}

template <bool isUCI>
inline bool is_move_none(Move move, const std::string& strMove, const Position& pos) {
  if (move == MOVE_NONE) {
    std::cerr << "The given ";
    isUCI ? std::cerr << "uciMove" : std::cerr << "sanMove";
    std::cerr << " '" << strMove << "' for position '" << pos.fen() << "' is invalid." << std::endl;
    return true;
  }
  return false;
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
      movesSan += SAN::move_to_san(this->pos, move, NOTATION_SAN);
      movesSan += DELIM;
    }
    save_pop_back(movesSan);
    return movesSan;
  }

  int number_legal_moves() const {
    return MoveList<LEGAL>(pos).size();
  }

  bool push(std::string uciMove) {
    const Move move = UCI::to_move(this->pos, uciMove);
    if (is_move_none<true>(move, uciMove, pos))
      return false;
    do_move(move);
    return true;
  }

  bool push_san(std::string sanMove) {
    return push_san(sanMove, NOTATION_SAN);
  }

  // TODO: This is a naive implementation which compares all legal SAN moves with the requested string.
  // If the SAN move wasn't found the position remains unchanged. Alternatively, implement a direct conversion.
  bool push_san(std::string sanMove, Notation notation) {
    Move foundMove = MOVE_NONE;
    for (const ExtMove& move : MoveList<LEGAL>(pos)) {
      if (sanMove == SAN::move_to_san(this->pos, move, notation)) {
        foundMove = move;
        break;
      }
    }
    if (is_move_none<false>(foundMove, sanMove, pos))
      return false;
    do_move(foundMove);
    return true;
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

  // note: const identifier for pos not possible due to SAN::move_to_san()
  std::string san_move(std::string uciMove) {
    return san_move(uciMove, NOTATION_SAN);
  }

  std::string san_move(std::string uciMove, Notation notation) {
    const Move move = UCI::to_move(this->pos, uciMove);
    if (is_move_none<true>(move, uciMove, pos))
      return "";
    return SAN::move_to_san(this->pos, UCI::to_move(this->pos, uciMove), notation);
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
      const Move move = UCI::to_move(this->pos, uciMove);
      if (is_move_none<true>(move, uciMove, pos))
        return "";
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
        variationSan += SAN::move_to_san(this->pos, moves.back(), Notation(notation));
      }
      else {
        if (moveNumbers && pos.side_to_move() == WHITE) {
          variationSan += DELIM;
          variationSan += std::to_string(fullmove_number());
          variationSan += ".";
        }
        variationSan += DELIM;
        variationSan += SAN::move_to_san(this->pos, moves.back(), Notation(notation));
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

  bool has_insufficient_material(bool turn) const {
    return Stockfish::has_insufficient_material(turn ? WHITE : BLACK, pos);
  }

  bool is_insufficient_material() const {
    return Stockfish::has_insufficient_material(WHITE, pos) && Stockfish::has_insufficient_material(BLACK, pos);
  }

  bool is_game_over() const {
    return is_game_over(false);
  }

  bool is_game_over(bool claim_draw) const {
    if (is_insufficient_material())
      return true;
    if (claim_draw && pos.is_optional_game_end())
      return true;
    return MoveList<LEGAL>(pos).size() == 0;
  }

  std::string result() const {
    return result(false);
  }

  std::string result(bool claim_draw) const {
    Value result;
    bool gameEnd = false;
    if (is_insufficient_material()) {
      gameEnd = true;
      if (pos.material_counting())
        result = pos.material_counting_result();
      else
        result = VALUE_DRAW;
    }
    if (!gameEnd)
      gameEnd = pos.is_immediate_game_end(result);
    if (!gameEnd && MoveList<LEGAL>(pos).size() == 0) {
      gameEnd = true;
      result = pos.checkers() ? pos.checkmate_value() : pos.stalemate_value();
    }
    if (!gameEnd && claim_draw)
      gameEnd = pos.is_optional_game_end(result);

    if (!gameEnd)
      return "*";
    if (result == 0)
      return "1/2-1/2";
    if (pos.side_to_move() == BLACK)
      result = -result;
    if (result > 0)
      return "1-0";
    else
      return "0-1";
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

  std::string to_string() {
    std::string stringBoard;
    for (Rank r = pos.max_rank(); r >= RANK_1; --r) {
      for (File f = FILE_A; f <= pos.max_file(); ++f) {
        if (f != FILE_A)
          stringBoard += " ";
        const Piece p = pos.piece_on(make_square(f, r));
        switch(p) {
        case NO_PIECE:
          stringBoard += '.';
          break;
        default:
          stringBoard += pos.piece_to_char()[p];
        }
      }
      if (r != RANK_1)
        stringBoard += "\n";
    }
    return stringBoard;
  }

  std::string to_verbose_string() {
    std::stringstream ss;
    operator<<(ss, pos);
    return ss.str();
  }

  std::string variant() {
    // Iterate through the variants map
    for (auto it = variants.begin(); it != variants.end(); ++it)
      if (it->second == v)
        return it->first;

    std::cerr << "Current variant is not registered." << std::endl;
    return "unknown";
  }

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
    UCI::init_variant(v);
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

  int validate_fen(std::string fen, std::string uciVariant, bool chess960) {
    const Variant* v = get_variant(uciVariant);
    return FEN::validate_fen(fen, v, chess960);
  }

  int validate_fen(std::string fen, std::string uciVariant) {
    return validate_fen(fen, uciVariant, false);
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


bool skip_comment(const std::string& pgn, size_t& curIdx, size_t& lineEnd) {
  curIdx = pgn.find('}', curIdx);
  if (curIdx == std::string::npos) {
    std::cerr << "Missing '}' for move comment while reading pgn." << std::endl;
    return false;
  }
  if (curIdx > lineEnd)
    lineEnd = pgn.find('\n', curIdx);
  return true;
}

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

        if (pgn[curIdx] == '{') {
          if (!skip_comment(pgn, curIdx, lineEnd))
            return game;
          ++curIdx;
        }

        // Movetext RAV (Recursive Annotation Variation)
        size_t openedRAV = 0;
        if (pgn[curIdx] == '(') {
          openedRAV = 1;
          ++curIdx;
        }
        while (openedRAV != 0) {
          switch (pgn[curIdx]) {
            case '(':
              ++openedRAV;
              break;
            case ')':
              --openedRAV;
              break;
            case '{':
              if (!skip_comment(pgn, curIdx, lineEnd))
                return game;
            default: ;  // pass
          }
          ++curIdx;
          if (curIdx > lineEnd)
            lineEnd = pgn.find('\n', curIdx);
        }

        if (pgn[curIdx] == '$') {
          // we are at a glyph
          curIdx = pgn.find(' ', curIdx);
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
          std::cout << sanMove << " ";
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
    .function("hasInsufficientMaterial", &Board::has_insufficient_material)
    .function("isInsufficientMaterial", &Board::is_insufficient_material)
    .function("isGameOver", select_overload<bool() const>(&Board::is_game_over))
    .function("isGameOver", select_overload<bool(bool) const>(&Board::is_game_over))
    .function("result", select_overload<std::string() const>(&Board::result))
    .function("result", select_overload<std::string(bool) const>(&Board::result))
    .function("isCheck", &Board::is_check)
    .function("isBikjang", &Board::is_bikjang)
    .function("moveStack", &Board::move_stack)
    .function("pushMoves", &Board::push_moves)
    .function("pushSanMoves", select_overload<void(std::string)>(&Board::push_san_moves))
    .function("pushSanMoves", select_overload<void(std::string, Notation)>(&Board::push_san_moves))
    .function("pocket", &Board::pocket)
    .function("toString", &Board::to_string)
    .function("toVerboseString", &Board::to_verbose_string)
    .function("variant", &Board::variant);
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
  // usage: e.g. ffish.Termination.CHECKMATE
  enum_<Termination>("Termination")
    .value("ONGOING", ONGOING)
    .value("CHECKMATE", CHECKMATE)
    .value("STALEMATE", STALEMATE)
    .value("INSUFFICIENT_MATERIAL", INSUFFICIENT_MATERIAL)
    .value("N_MOVE_RULE", N_MOVE_RULE)
    .value("N_FOLD_REPETITION", N_FOLD_REPETITION)
    .value("VARIANT_END", VARIANT_END);
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
  function("validateFen", select_overload<int(std::string, std::string, bool)>(&ffish::validate_fen));
  // TODO: enable to string conversion method
  // .class_function("getStringFromInstance", &Board::get_string_from_instance);
}
