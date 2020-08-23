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


void initializeStockfish(std::string& uciVariant) {
  pieceMap.init();
  variants.init();
  UCI::init(Options);
  PSQT::init(variants.find(uciVariant)->second);
  Bitboards::init();
  Position::init();
  Bitbases::init();
}

class Board {
  // note: we can't use references for strings here due to conversion to JavaScript
private:
  const Variant* v;
  StateListPtr states;
  Position pos;
  Thread* thread;
  std::vector<Move> moveStack;
 static bool sfInitialized;
  bool is960;

public:
  Board():
    Board("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" , false) {
  }

  Board(std::string uciVariant) {
    init(uciVariant, "", is960);
  }

  Board(std::string uciVariant, std::string fen):
    Board(uciVariant, fen , false) {
  }

  Board(std::string uciVariant, std::string fen, bool is960) {
    init(uciVariant, fen, is960);
  }

  std::string legal_moves() {
    std::string moves = "";
    bool first = true;
    for (const ExtMove& move : MoveList<LEGAL>(this->pos)) {
        if (first) {
          moves = UCI::move(this->pos, move);
          first = false;
        }
        else
          moves += " "  + UCI::move(this->pos, move);
    }
    return moves;
  }

  std::string legal_moves_san() {
    std::string movesSan = "";
    bool first = true;
    for (const ExtMove& move : MoveList<LEGAL>(this->pos)) {
        if (first) {
          movesSan = move_to_san(this->pos, move, NOTATION_SAN);
          first = false;
        }
        else
          movesSan += " "  + move_to_san(this->pos, move, NOTATION_SAN);
    }
    return movesSan;
  }

  int number_legal_moves() {
    return MoveList<LEGAL>(pos).size();
  }

  void push(std::string uciMove) {
    do_move(UCI::to_move(this->pos, uciMove));
  }

  // TODO: This is a naive implementation which compares all legal SAN moves with the requested string.
  // If the SAN move wasn't found the position remains unchanged. Alternatively, implement a direct conversion.
   void push_san(std::string sanMove) {
     Move foundMove = MOVE_NONE;
     for (const ExtMove& move : MoveList<LEGAL>(pos)) {
        if (sanMove == move_to_san(this->pos, move, NOTATION_SAN)) {
          foundMove = move;
          break;
        }
     }
     if (foundMove != MOVE_NONE)
        do_move(foundMove);
   }


  void pop() {
    pos.undo_move(this->moveStack.back());
    moveStack.pop_back();
    states->pop_back();
  }

   void reset() {
     set_fen(v->startFen);
   }

   bool is_960() {
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

  // returns true for WHITE and false for BLACK
  bool turn() {
    return !pos.side_to_move();
  }

  int halfmove_clock() {
    return pos.rule50_count();
  }

  int game_ply() {
    return pos.game_ply();
  }

  bool is_game_over() {
    for (const ExtMove& move : MoveList<LEGAL>(pos)) {
        return false;
    }
    return true;
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

  void init(std::string& uciVariant, std::string fen, bool is960) {
   if (!Board::sfInitialized) {
        initializeStockfish(uciVariant);
       Board::sfInitialized = true;
   }
    this->v = variants.find(uciVariant)->second;
    this->resetStates();
    if (fen == "")
      fen = v->startFen;
    this->pos.set(this->v, fen, is960, &this->states->back(), this->thread);
    this->is960 = is960;
  }
};

// returns the version of the Fairy-Stockfish binary
std::string info() {
   return engine_info();
}

bool Board::sfInitialized = false;

// binding code
EMSCRIPTEN_BINDINGS(ffish_js) {
  function("info", &info);
  class_<Board>("Board")
    .constructor<>()
    .constructor<std::string>()
    .constructor<std::string, std::string>()
    .constructor<std::string, std::string, bool>()
    .function("legalMoves", &Board::legal_moves)
    .function("legalMovesSan", &Board::legal_moves_san)
    .function("numberLegalMoves", &Board::number_legal_moves)
    .function("push", &Board::push)
    .function("pushSan", &Board::push_san)
    .function("pop", &Board::pop)
    .function("reset", &Board::reset)
    .function("is960", &Board::is_960)
    .function("fen", &Board::fen)
    .function("setFen", &Board::set_fen)
    .function("sanMove", &Board::san_move)
    .function("turn", &Board::turn)
    .function("halfmoveClock", &Board::halfmove_clock)
    .function("gamePly", &Board::game_ply)
    .function("isGameOver", &Board::is_game_over);
    // TODO: enable to string conversion method
    // .class_function("getStringFromInstance", &Board::get_string_from_instance);
}
