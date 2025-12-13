/*
  ffish.dll/.so - C API for Fairy-Stockfish (move generation / move validation)
  Derived from ffish.js, but without Emscripten bindings.
*/

#include <algorithm>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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

using namespace Stockfish;

namespace {

constexpr const char* DELIM = " ";

void initialize_stockfish() {
  pieceMap.init();
  variants.init();
  UCI::init(Options);
  Bitboards::init();
  Position::init();
  Bitbases::init();
}

inline void save_pop_back(std::string& s) {
  if (!s.empty())
    s.pop_back();
}

const Variant* get_variant(const std::string& uciVariant) {
  if (uciVariant.empty() || uciVariant == "Standard" || uciVariant == "standard")
    return variants.find("chess")->second;
  auto it = variants.find(uciVariant);
  if (it != variants.end())
    return it->second;
  return variants.find("chess")->second;
}

template <bool isUCI>
inline bool is_move_none(Move move, const std::string& strMove, const Position& pos) {
  if (move == MOVE_NONE) {
    std::cerr << "The given " << (isUCI ? "uciMove" : "sanMove")
              << " '" << strMove << "' for position '" << pos.fen()
              << "' is invalid." << std::endl;
    return true;
  }
  return false;
}

char* to_cstr(const std::string& s) {
  char* out = new char[s.size() + 1];
  std::memcpy(out, s.c_str(), s.size() + 1);
  return out;
}

Notation to_notation(int n) {
  switch (n) {
  case NOTATION_SAN: return NOTATION_SAN;
  case NOTATION_LAN: return NOTATION_LAN;
  case NOTATION_SHOGI_HOSKING: return NOTATION_SHOGI_HOSKING;
  case NOTATION_SHOGI_HODGES: return NOTATION_SHOGI_HODGES;
  case NOTATION_SHOGI_HODGES_NUMBER: return NOTATION_SHOGI_HODGES_NUMBER;
  case NOTATION_JANGGI: return NOTATION_JANGGI;
  case NOTATION_XIANGQI_WXF: return NOTATION_XIANGQI_WXF;
  case NOTATION_THAI_SAN: return NOTATION_THAI_SAN;
  case NOTATION_THAI_LAN: return NOTATION_THAI_LAN;
  default: return NOTATION_DEFAULT;
  }
}

}  // namespace

class Board {
private:
  const Variant* v;
  StateListPtr states;
  Position pos;
  Thread* thread = nullptr;
  std::vector<Move> moveStack;
  bool is960;

public:
  static bool sfInitialized;

  Board() : Board("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false) {}
  Board(std::string uciVariant) : Board(uciVariant, "", false) {}
  Board(std::string uciVariant, std::string fen) : Board(uciVariant, fen, false) {}
  Board(std::string uciVariant, std::string fen, bool is960) { init(uciVariant, fen, is960); }

  std::string legal_moves() {
    std::string moves;
    for (const ExtMove& move : MoveList<LEGAL>(pos)) {
      moves += UCI::move(pos, move);
      moves += DELIM;
    }
    save_pop_back(moves);
    return moves;
  }

  std::string legal_moves_san() {
    std::string movesSan;
    for (const ExtMove& move : MoveList<LEGAL>(pos)) {
      movesSan += SAN::move_to_san(pos, move, NOTATION_SAN);
      movesSan += DELIM;
    }
    save_pop_back(movesSan);
    return movesSan;
  }

  int number_legal_moves() const { return MoveList<LEGAL>(pos).size(); }

  bool push(std::string uciMove) {
    const Move move = UCI::to_move(pos, uciMove);
    if (is_move_none<true>(move, uciMove, pos))
      return false;
    do_move(move);
    return true;
  }

  bool push_san(std::string sanMove) { return push_san(sanMove, NOTATION_SAN); }

  bool push_san(std::string sanMove, Notation notation) {
    Move foundMove = MOVE_NONE;
    for (const ExtMove& move : MoveList<LEGAL>(pos)) {
      if (sanMove == SAN::move_to_san(pos, move, notation)) {
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
    pos.undo_move(moveStack.back());
    moveStack.pop_back();
    states->pop_back();
  }

  void reset() { set_fen(v->startFen); }

  bool is_960() const { return is960; }

  std::string fen() const { return pos.fen(); }
  std::string fen(bool showPromoted) const { return pos.fen(false, showPromoted); }
  std::string fen(bool showPromoted, int countStarted) const { return pos.fen(false, showPromoted, countStarted); }

  void set_fen(std::string fenStr) {
    resetStates();
    moveStack.clear();
    pos.set(v, fenStr, is960, &states->back(), thread);
  }

  std::string san_move(std::string uciMove) { return san_move(uciMove, NOTATION_SAN); }

  std::string san_move(std::string uciMove, Notation notation) {
    const Move move = UCI::to_move(pos, uciMove);
    if (is_move_none<true>(move, uciMove, pos))
      return "";
    return SAN::move_to_san(pos, move, notation);
  }

  std::string variation_san(std::string uciMoves, Notation notation, bool moveNumbers) {
    std::stringstream ss(uciMoves);
    std::vector<Move> moves;
    std::string variationSan;
    std::string uciMove;
    bool first = true;

    while (std::getline(ss, uciMove, ' ')) {
      const Move move = UCI::to_move(pos, uciMove);
      if (is_move_none<true>(move, uciMove, pos))
        return "";
      moves.emplace_back(move);
      if (first) {
        first = false;
        if (moveNumbers) {
          variationSan = std::to_string(fullmove_number());
          variationSan += (pos.side_to_move() == WHITE) ? ". " : "...";
        }
      } else if (moveNumbers && pos.side_to_move() == WHITE) {
        variationSan += DELIM;
        variationSan += std::to_string(fullmove_number());
        variationSan += ".";
      }
      variationSan += (variationSan.empty() ? "" : DELIM);
      variationSan += SAN::move_to_san(pos, moves.back(), notation);
      states->emplace_back();
      pos.do_move(moves.back(), states->back());
    }

    for (auto rIt = std::rbegin(moves); rIt != std::rend(moves); ++rIt)
      pos.undo_move(*rIt);

    return variationSan;
  }

  bool turn() const { return !pos.side_to_move(); }
  int fullmove_number() const { return pos.game_ply() / 2 + 1; }
  int halfmove_clock() const { return pos.rule50_count(); }
  int game_ply() const { return pos.game_ply(); }

  bool has_insufficient_material(bool turnColor) const {
    return Stockfish::has_insufficient_material(turnColor ? WHITE : BLACK, pos);
  }

  bool is_insufficient_material() const {
    return Stockfish::has_insufficient_material(WHITE, pos) && Stockfish::has_insufficient_material(BLACK, pos);
  }

  bool is_game_over(bool claim_draw) const {
    if (is_insufficient_material())
      return true;
    if (claim_draw && pos.is_optional_game_end())
      return true;
    return MoveList<LEGAL>(pos).size() == 0;
  }

  std::string result(bool claim_draw) const {
    Value res;
    bool gameEnd = pos.is_immediate_game_end(res);
    if (!gameEnd && is_insufficient_material()) {
      gameEnd = true;
      res = VALUE_DRAW;
    }
    if (!gameEnd && MoveList<LEGAL>(pos).size() == 0) {
      gameEnd = true;
      res = pos.checkers() ? pos.checkmate_value() : pos.stalemate_value();
    }
    if (!gameEnd && claim_draw)
      gameEnd = pos.is_optional_game_end(res);

    if (!gameEnd)
      return "*";
    if (res == 0) {
      if (pos.material_counting())
        res = pos.material_counting_result();
      if (res == 0)
        return "1/2-1/2";
    }
    if (pos.side_to_move() == BLACK)
      res = -res;
    return (res > 0) ? "1-0" : "0-1";
  }

  std::string checked_pieces() const {
    Bitboard checked = Stockfish::checked(pos);
    std::string squares;
    while (checked) {
      Square sr = pop_lsb(checked);
      squares += UCI::square(pos, sr);
      squares += DELIM;
    }
    save_pop_back(squares);
    return squares;
  }

  bool is_check() const { return Stockfish::checked(pos); }
  bool is_bikjang() const { return pos.bikjang(); }
  bool is_capture(std::string uciMove) const { return pos.capture(UCI::to_move(pos, uciMove)); }

  std::string move_stack() const {
    std::string moves;
    for (auto m : moveStack) {
      moves += UCI::move(pos, m);
      moves += DELIM;
    }
    save_pop_back(moves);
    return moves;
  }

  void push_moves(std::string uciMoves) {
    std::stringstream ss(uciMoves);
    std::string uciMove;
    while (std::getline(ss, uciMove, ' '))
      push(uciMove);
  }

  void push_san_moves(std::string sanMoves, Notation notation = NOTATION_SAN) {
    std::stringstream ss(sanMoves);
    std::string sanMove;
    while (std::getline(ss, sanMove, ' '))
      push_san(sanMove, notation);
  }

  std::string pocket(bool color) {
    const Color c = Color(!color);
    std::string pocketStr;
    for (PieceType pt = KING; pt >= PAWN; --pt)
      for (int i = 0; i < pos.count_in_hand(c, pt); ++i)
        pocketStr += std::string(1, pos.piece_to_char()[make_piece(BLACK, pt)]);
    return pocketStr;
  }

  std::string to_string() {
    std::string stringBoard;
    for (Rank r = pos.max_rank(); r >= RANK_1; --r) {
      for (File f = FILE_A; f <= pos.max_file(); ++f) {
        if (f != FILE_A)
          stringBoard += " ";
        const Piece p = pos.piece_on(make_square(f, r));
        stringBoard += (p == NO_PIECE) ? '.' : pos.piece_to_char()[p];
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
    for (auto it = variants.begin(); it != variants.end(); ++it)
      if (it->second == v)
        return it->first;
    return "unknown";
  }

private:
  void resetStates() { states = StateListPtr(new std::deque<StateInfo>(1)); }

  void do_move(Move move) {
    states->emplace_back();
    pos.do_move(move, states->back());
    moveStack.emplace_back(move);
  }

  void init(std::string uciVariant, std::string fen, bool is960Flag) {
    if (!Board::sfInitialized) {
      initialize_stockfish();
      Board::sfInitialized = true;
    }
    v = get_variant(uciVariant);
    UCI::init_variant(v);
    resetStates();
    if (fen.empty())
      fen = v->startFen;
    pos.set(v, fen, is960Flag, &states->back(), thread);
    is960 = is960Flag;
  }
};

bool Board::sfInitialized = false;

namespace ffish {

std::string info() { return engine_info(); }

template <typename T>
void set_option(std::string name, T value) {
  Options[name] = value;
  Board::sfInitialized = false;
}

std::string available_variants() {
  std::string available;
  for (std::string variant : variants.get_keys()) {
    available += variant;
    available += DELIM;
  }
  save_pop_back(available);
  return available;
}

void load_variant_config(std::string variantInitContent) {
  std::stringstream ss(variantInitContent);
  if (!Board::sfInitialized)
    initialize_stockfish();
  variants.parse_istream<false>(ss);
  Options["UCI_Variant"].set_combo(variants.get_keys());
  Board::sfInitialized = true;
}

bool captures_to_hand(std::string uciVariant) {
  const Variant* v = get_variant(uciVariant);
  return v->capturesToHand;
}

std::string starting_fen(std::string uciVariant) {
  const Variant* v = get_variant(uciVariant);
  return v->startFen;
}

int validate_fen(std::string fen, std::string uciVariant, bool chess960) {
  const Variant* v = get_variant(uciVariant);
  return FEN::validate_fen(fen, v, chess960);
}

}  // namespace ffish

#if defined(_WIN32)
#  define FSF_API __declspec(dllexport)
#else
#  define FSF_API __attribute__((visibility("default")))
#endif

extern "C" {

typedef void* fsf_board;

FSF_API void fsf_init() {
  static std::once_flag init_flag;
  std::call_once(init_flag, []() {
    initialize_stockfish();
    Board::sfInitialized = true;
  });
}

FSF_API fsf_board fsf_new_board(const char* variant, const char* fen, bool is960) {
  fsf_init();
  return new Board(variant ? std::string(variant) : std::string("chess"),
                   fen ? std::string(fen) : std::string(""),
                   is960);
}

FSF_API void fsf_free_board(fsf_board b) { delete static_cast<Board*>(b); }

FSF_API const char* fsf_legal_moves(fsf_board b) { return to_cstr(static_cast<Board*>(b)->legal_moves()); }
FSF_API const char* fsf_legal_moves_san(fsf_board b) { return to_cstr(static_cast<Board*>(b)->legal_moves_san()); }
FSF_API int fsf_number_legal_moves(fsf_board b) { return static_cast<Board*>(b)->number_legal_moves(); }

FSF_API bool fsf_push(fsf_board b, const char* uci) { return static_cast<Board*>(b)->push(uci ? uci : ""); }
FSF_API bool fsf_push_san(fsf_board b, const char* san, int notation) {
  return static_cast<Board*>(b)->push_san(san ? san : "", to_notation(notation));
}
FSF_API void fsf_pop(fsf_board b) { static_cast<Board*>(b)->pop(); }
FSF_API void fsf_reset(fsf_board b) { static_cast<Board*>(b)->reset(); }

FSF_API const char* fsf_fen(fsf_board b, bool showPromoted, int countStarted) {
  return to_cstr(static_cast<Board*>(b)->fen(showPromoted, countStarted));
}

FSF_API void fsf_set_fen(fsf_board b, const char* fen) { static_cast<Board*>(b)->set_fen(fen ? fen : ""); }

FSF_API const char* fsf_san_move(fsf_board b, const char* uciMove, int notation) {
  return to_cstr(static_cast<Board*>(b)->san_move(uciMove ? uciMove : "", to_notation(notation)));
}

FSF_API const char* fsf_variation_san(fsf_board b, const char* uciMoves, int notation, bool moveNumbers) {
  return to_cstr(static_cast<Board*>(b)->variation_san(uciMoves ? uciMoves : "", to_notation(notation), moveNumbers));
}

FSF_API bool fsf_turn(fsf_board b) { return static_cast<Board*>(b)->turn(); }
FSF_API int fsf_fullmove_number(fsf_board b) { return static_cast<Board*>(b)->fullmove_number(); }
FSF_API int fsf_halfmove_clock(fsf_board b) { return static_cast<Board*>(b)->halfmove_clock(); }
FSF_API int fsf_game_ply(fsf_board b) { return static_cast<Board*>(b)->game_ply(); }

FSF_API bool fsf_has_insufficient_material(fsf_board b, bool turnColor) {
  return static_cast<Board*>(b)->has_insufficient_material(turnColor);
}
FSF_API bool fsf_is_insufficient_material(fsf_board b) { return static_cast<Board*>(b)->is_insufficient_material(); }

FSF_API bool fsf_is_game_over(fsf_board b, bool claim_draw) { return static_cast<Board*>(b)->is_game_over(claim_draw); }
FSF_API const char* fsf_result(fsf_board b, bool claim_draw) { return to_cstr(static_cast<Board*>(b)->result(claim_draw)); }

FSF_API const char* fsf_checked_pieces(fsf_board b) { return to_cstr(static_cast<Board*>(b)->checked_pieces()); }
FSF_API bool fsf_is_check(fsf_board b) { return static_cast<Board*>(b)->is_check(); }
FSF_API bool fsf_is_bikjang(fsf_board b) { return static_cast<Board*>(b)->is_bikjang(); }
FSF_API bool fsf_is_capture(fsf_board b, const char* uciMove) {
  return static_cast<Board*>(b)->is_capture(uciMove ? uciMove : "");
}

FSF_API const char* fsf_move_stack(fsf_board b) { return to_cstr(static_cast<Board*>(b)->move_stack()); }
FSF_API void fsf_push_moves(fsf_board b, const char* uciMoves) { static_cast<Board*>(b)->push_moves(uciMoves ? uciMoves : ""); }
FSF_API void fsf_push_san_moves(fsf_board b, const char* sanMoves, int notation) {
  static_cast<Board*>(b)->push_san_moves(sanMoves ? sanMoves : "", to_notation(notation));
}

FSF_API const char* fsf_pocket(fsf_board b, bool color) { return to_cstr(static_cast<Board*>(b)->pocket(color)); }
FSF_API const char* fsf_board_string(fsf_board b) { return to_cstr(static_cast<Board*>(b)->to_string()); }
FSF_API const char* fsf_board_verbose_string(fsf_board b) { return to_cstr(static_cast<Board*>(b)->to_verbose_string()); }
FSF_API const char* fsf_board_variant(fsf_board b) { return to_cstr(static_cast<Board*>(b)->variant()); }

FSF_API const char* fsf_available_variants() { return to_cstr(ffish::available_variants()); }
FSF_API void fsf_load_variant_config(const char* content) {
  ffish::load_variant_config(content ? content : "");
}
FSF_API int fsf_validate_fen(const char* fen, const char* variant, bool is960) {
  return ffish::validate_fen(fen ? fen : "", variant ? variant : "chess", is960);
}
FSF_API const char* fsf_starting_fen(const char* variant) {
  return to_cstr(ffish::starting_fen(variant ? variant : "chess"));
}
FSF_API bool fsf_captures_to_hand(const char* variant) {
  return ffish::captures_to_hand(variant ? variant : "chess");
}

FSF_API const char* fsf_info() { return to_cstr(ffish::info()); }
FSF_API void fsf_set_option_str(const char* name, const char* value) {
  if (name && value) ffish::set_option<std::string>(name, value);
}
FSF_API void fsf_set_option_int(const char* name, int value) {
  if (name) ffish::set_option<int>(name, value);
}
FSF_API void fsf_set_option_bool(const char* name, bool value) {
  if (name) ffish::set_option<bool>(name, value);
}

FSF_API void fsf_free(const char* p) { delete[] p; }

}  // extern "C"
