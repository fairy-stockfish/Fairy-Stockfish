/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2020 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <deque>
#include <memory> // For std::unique_ptr
#include <string>
#include <functional>

#include "bitboard.h"
#include "types.h"
#include "variant.h"


/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {

  // Copied when making a move
  Key    pawnKey;
  Key    materialKey;
  Value  nonPawnMaterial[COLOR_NB];
  int    castlingRights;
  int    rule50;
  int    pliesFromNull;
  int    countingPly;
  int    countingLimit;
  CheckCount checksRemaining[COLOR_NB];
  Square epSquare;
  Bitboard gatesBB[COLOR_NB];

  // Not copied when making a move (will be recomputed anyhow)
  Key        key;
  Bitboard   checkersBB;
  Piece      capturedPiece;
  Piece      unpromotedCapturedPiece;
  StateInfo* previous;
  Bitboard   blockersForKing[COLOR_NB];
  Bitboard   pinners[COLOR_NB];
  Bitboard   checkSquares[PIECE_TYPE_NB];
  Bitboard   flippedPieces;
  bool       capturedpromoted;
  bool       shak;
  bool       bikjang;
  bool       pass;
  int        repetition;
};

/// A list to keep track of the position states along the setup moves (from the
/// start position to the position just before the search starts). Needed by
/// 'draw by repetition' detection. Use a std::deque because pointers to
/// elements are not invalidated upon list resizing.
typedef std::unique_ptr<std::deque<StateInfo>> StateListPtr;


/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.
class Thread;

class Position {
public:
  static void init();

  Position() = default;
  Position(const Position&) = delete;
  Position& operator=(const Position&) = delete;

  // FEN string input/output
  Position& set(const Variant* v, const std::string& fenStr, bool isChess960, StateInfo* si, Thread* th, bool sfen = false);
  Position& set(const std::string& code, Color c, StateInfo* si);
  const std::string fen(bool sfen = false, bool showPromoted = false, int countStarted = 0, std::string holdings = "-") const;

  // Variant rule properties
  const Variant* variant() const;
  Rank max_rank() const;
  File max_file() const;
  bool two_boards() const;
  Bitboard board_bb() const;
  Bitboard board_bb(Color c, PieceType pt) const;
  const std::set<PieceType>& piece_types() const;
  const std::string& piece_to_char() const;
  const std::string& piece_to_char_synonyms() const;
  Rank promotion_rank() const;
  const std::set<PieceType, std::greater<PieceType> >& promotion_piece_types() const;
  bool sittuyin_promotion() const;
  int promotion_limit(PieceType pt) const;
  PieceType promoted_piece_type(PieceType pt) const;
  bool piece_promotion_on_capture() const;
  bool mandatory_pawn_promotion() const;
  bool mandatory_piece_promotion() const;
  bool piece_demotion() const;
  bool endgame_eval() const;
  bool double_step_enabled() const;
  Rank double_step_rank() const;
  bool first_rank_double_steps() const;
  bool castling_enabled() const;
  bool castling_dropped_piece() const;
  File castling_kingside_file() const;
  File castling_queenside_file() const;
  Rank castling_rank(Color c) const;
  PieceType castling_rook_piece() const;
  PieceType king_type() const;
  bool checking_permitted() const;
  bool drop_checks() const;
  bool must_capture() const;
  bool must_drop() const;
  bool piece_drops() const;
  bool drop_loop() const;
  bool captures_to_hand() const;
  bool first_rank_pawn_drops() const;
  bool drop_on_top() const;
  EnclosingRule enclosing_drop() const;
  Bitboard drop_region(Color c) const;
  Bitboard drop_region(Color c, PieceType pt) const;
  bool sittuyin_rook_drop() const;
  bool drop_opposite_colored_bishop() const;
  bool drop_promoted() const;
  bool shogi_doubled_pawn() const;
  bool immobility_illegal() const;
  bool gating() const;
  bool seirawan_gating() const;
  bool cambodian_moves() const;
  Bitboard diagonal_lines() const;
  bool pass() const;
  bool pass_on_stalemate() const;
  Bitboard promoted_soldiers(Color c) const;
  bool makpong() const;
  EnclosingRule flip_enclosed_pieces() const;
  // winning conditions
  int n_move_rule() const;
  int n_fold_rule() const;
  Value stalemate_value(int ply = 0) const;
  Value checkmate_value(int ply = 0) const;
  Value extinction_value(int ply = 0) const;
  bool extinction_claim() const;
  const std::set<PieceType>& extinction_piece_types() const;
  int extinction_piece_count() const;
  int extinction_opponent_piece_count() const;
  PieceType capture_the_flag_piece() const;
  Bitboard capture_the_flag(Color c) const;
  bool flag_move() const;
  bool check_counting() const;
  int connect_n() const;
  CheckCount checks_remaining(Color c) const;
  MaterialCounting material_counting() const;
  CountingRule counting_rule() const;

  // Variant-specific properties
  int count_in_hand(Color c, PieceType pt) const;
  int count_with_hand(Color c, PieceType pt) const;
  bool bikjang() const;

  // Position representation
  Bitboard pieces(PieceType pt = ALL_PIECES) const;
  Bitboard pieces(PieceType pt1, PieceType pt2) const;
  Bitboard pieces(Color c) const;
  Bitboard pieces(Color c, PieceType pt) const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
  Bitboard major_pieces(Color c) const;
  Piece piece_on(Square s) const;
  Piece unpromoted_piece_on(Square s) const;
  Square ep_square() const;
  Bitboard gates(Color c) const;
  bool empty(Square s) const;
  int count(Color c, PieceType pt) const;
  template<PieceType Pt> int count(Color c) const;
  template<PieceType Pt> int count() const;
  template<PieceType Pt> const Square* squares(Color c) const;
  const Square* squares(Color c, PieceType pt) const;
  template<PieceType Pt> Square square(Color c) const;
  bool is_on_semiopen_file(Color c, Square s) const;

  // Castling
  int castling_rights(Color c) const;
  bool can_castle(CastlingRights cr) const;
  bool castling_impeded(CastlingRights cr) const;
  Square castling_rook_square(CastlingRights cr) const;

  // Checking
  Bitboard checkers() const;
  Bitboard blockers_for_king(Color c) const;
  Bitboard check_squares(PieceType pt) const;
  bool is_discovery_check_on_king(Color c, Move m) const;

  // Attacks to/from a given square
  Bitboard attackers_to(Square s) const;
  Bitboard attackers_to(Square s, Color c) const;
  Bitboard attackers_to(Square s, Bitboard occupied) const;
  Bitboard attackers_to(Square s, Bitboard occupied, Color c) const;
  Bitboard attackers_to(Square s, Bitboard occupied, Color c, Bitboard janggiCannons) const;
  Bitboard attacks_from(Color c, PieceType pt, Square s) const;
  template<PieceType> Bitboard attacks_from(Square s, Color c) const;
  Bitboard moves_from(Color c, PieceType pt, Square s) const;
  Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners, Color c) const;

  // Properties of moves
  bool legal(Move m) const;
  bool pseudo_legal(const Move m) const;
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  bool gives_check(Move m) const;
  bool advanced_pawn_push(Move m) const;
  Piece moved_piece(Move m) const;
  Piece captured_piece() const;

  // Piece specific
  bool pawn_passed(Color c, Square s) const;
  bool opposite_bishops() const;
  bool is_promoted(Square s) const;
  int  pawns_on_same_color_squares(Color c, Square s) const;

  // Doing and undoing moves
  void do_move(Move m, StateInfo& newSt);
  void do_move(Move m, StateInfo& newSt, bool givesCheck);
  void undo_move(Move m);
  void do_null_move(StateInfo& newSt);
  void undo_null_move();

  // Static Exchange Evaluation
  bool see_ge(Move m, Value threshold = VALUE_ZERO) const;

  // Accessing hash keys
  Key key() const;
  Key key_after(Move m) const;
  Key material_key() const;
  Key pawn_key() const;

  // Other properties of the position
  Color side_to_move() const;
  int game_ply() const;
  bool is_chess960() const;
  Thread* this_thread() const;
  bool is_immediate_game_end() const;
  bool is_immediate_game_end(Value& result, int ply = 0) const;
  bool is_optional_game_end() const;
  bool is_optional_game_end(Value& result, int ply = 0, int countStarted = 0) const;
  bool is_game_end(Value& result, int ply = 0) const;
  Value material_counting_result() const;
  bool has_game_cycle(int ply) const;
  bool has_repeated() const;
  int counting_limit() const;
  int counting_ply(int countStarted) const;
  int rule50_count() const;
  Score psq_score() const;
  Value non_pawn_material(Color c) const;
  Value non_pawn_material() const;

  // Position consistency check, for debugging
  bool pos_is_ok() const;
  void flip();

private:
  // Initialization helpers (used while setting up a position)
  void set_castling_right(Color c, Square rfrom);
  void set_state(StateInfo* si) const;
  void set_check_info(StateInfo* si) const;

  // Other helpers
  void put_piece(Piece pc, Square s, bool isPromoted = false, Piece unpromotedPc = NO_PIECE);
  void remove_piece(Square s);
  void move_piece(Square from, Square to);
  template<bool Do>
  void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

  // Data members
  Piece board[SQUARE_NB];
  Piece unpromotedBoard[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  int pieceCount[PIECE_NB];
  Square pieceList[PIECE_NB][64];
  int index[SQUARE_NB];
  int castlingRightsMask[SQUARE_NB];
  Square castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
  int gamePly;
  Color sideToMove;
  Score psq;
  Thread* thisThread;
  StateInfo* st;

  // variant-specific
  const Variant* var;
  bool chess960;
  int pieceCountInHand[COLOR_NB][PIECE_TYPE_NB];
  Bitboard promotedPieces;
  void add_to_hand(Piece pc);
  void remove_from_hand(Piece pc);
  void drop_piece(Piece pc_hand, Piece pc_drop, Square s);
  void undrop_piece(Piece pc_hand, Square s);
};

namespace PSQT {
  extern Score psq[PIECE_NB][SQUARE_NB + 1];
}

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline const Variant* Position::variant() const {
  assert(var != nullptr);
  return var;
}

inline Rank Position::max_rank() const {
  assert(var != nullptr);
  return var->maxRank;
}

inline File Position::max_file() const {
  assert(var != nullptr);
  return var->maxFile;
}

inline bool Position::two_boards() const {
  assert(var != nullptr);
  return var->twoBoards;
}

inline Bitboard Position::board_bb() const {
  assert(var != nullptr);
  return board_size_bb(var->maxFile, var->maxRank);
}

inline Bitboard Position::board_bb(Color c, PieceType pt) const {
  assert(var != nullptr);
  return var->mobilityRegion[c][pt] ? var->mobilityRegion[c][pt] & board_bb() : board_bb();
}

inline const std::set<PieceType>& Position::piece_types() const {
  assert(var != nullptr);
  return var->pieceTypes;
}

inline const std::string& Position::piece_to_char() const {
  assert(var != nullptr);
  return var->pieceToChar;
}

inline const std::string& Position::piece_to_char_synonyms() const {
  assert(var != nullptr);
  return var->pieceToCharSynonyms;
}

inline Rank Position::promotion_rank() const {
  assert(var != nullptr);
  return var->promotionRank;
}

inline const std::set<PieceType, std::greater<PieceType> >& Position::promotion_piece_types() const {
  assert(var != nullptr);
  return var->promotionPieceTypes;
}

inline bool Position::sittuyin_promotion() const {
  assert(var != nullptr);
  return var->sittuyinPromotion;
}

inline int Position::promotion_limit(PieceType pt) const {
  assert(var != nullptr);
  return var->promotionLimit[pt];
}

inline PieceType Position::promoted_piece_type(PieceType pt) const {
  assert(var != nullptr);
  return var->promotedPieceType[pt];
}

inline bool Position::piece_promotion_on_capture() const {
  assert(var != nullptr);
  return var->piecePromotionOnCapture;
}

inline bool Position::mandatory_pawn_promotion() const {
  assert(var != nullptr);
  return var->mandatoryPawnPromotion;
}

inline bool Position::mandatory_piece_promotion() const {
  assert(var != nullptr);
  return var->mandatoryPiecePromotion;
}

inline bool Position::piece_demotion() const {
  assert(var != nullptr);
  return var->pieceDemotion;
}

inline bool Position::endgame_eval() const {
  assert(var != nullptr);
  return var->endgameEval && !count_in_hand(WHITE, ALL_PIECES) && !count_in_hand(BLACK, ALL_PIECES);
}

inline bool Position::double_step_enabled() const {
  assert(var != nullptr);
  return var->doubleStep;
}

inline Rank Position::double_step_rank() const {
  assert(var != nullptr);
  return var->doubleStepRank;
}

inline bool Position::first_rank_double_steps() const {
  assert(var != nullptr);
  return var->firstRankDoubleSteps;
}

inline bool Position::castling_enabled() const {
  assert(var != nullptr);
  return var->castling;
}

inline bool Position::castling_dropped_piece() const {
  assert(var != nullptr);
  return var->castlingDroppedPiece;
}

inline File Position::castling_kingside_file() const {
  assert(var != nullptr);
  return var->castlingKingsideFile;
}

inline File Position::castling_queenside_file() const {
  assert(var != nullptr);
  return var->castlingQueensideFile;
}

inline Rank Position::castling_rank(Color c) const {
  assert(var != nullptr);
  return relative_rank(c, var->castlingRank, max_rank());
}

inline PieceType Position::castling_rook_piece() const {
  assert(var != nullptr);
  return var->castlingRookPiece;
}

inline PieceType Position::king_type() const {
  assert(var != nullptr);
  return var->kingType;
}

inline bool Position::checking_permitted() const {
  assert(var != nullptr);
  return var->checking;
}

inline bool Position::drop_checks() const {
  assert(var != nullptr);
  return var->dropChecks;
}

inline bool Position::must_capture() const {
  assert(var != nullptr);
  return var->mustCapture;
}

inline bool Position::must_drop() const {
  assert(var != nullptr);
  return var->mustDrop;
}

inline bool Position::piece_drops() const {
  assert(var != nullptr);
  return var->pieceDrops;
}

inline bool Position::drop_loop() const {
  assert(var != nullptr);
  return var->dropLoop;
}

inline bool Position::captures_to_hand() const {
  assert(var != nullptr);
  return var->capturesToHand;
}

inline bool Position::first_rank_pawn_drops() const {
  assert(var != nullptr);
  return var->firstRankPawnDrops;
}

inline bool Position::drop_on_top() const {
  assert(var != nullptr);
  return var->dropOnTop;
}

inline EnclosingRule Position::enclosing_drop() const {
  assert(var != nullptr);
  return var->enclosingDrop;
}

inline Bitboard Position::drop_region(Color c) const {
  assert(var != nullptr);
  return c == WHITE ? var->whiteDropRegion : var->blackDropRegion;
}

inline Bitboard Position::drop_region(Color c, PieceType pt) const {
  Bitboard b = drop_region(c) & board_bb(c, pt);

  // Connect4-style drops
  if (drop_on_top())
      b &= shift<NORTH>(pieces()) | Rank1BB;
  // Pawns on back ranks
  if (pt == PAWN)
  {
      if (!var->promotionZonePawnDrops)
          b &= ~promotion_zone_bb(c, promotion_rank(), max_rank());
      if (!first_rank_pawn_drops())
          b &= ~rank_bb(relative_rank(c, RANK_1, max_rank()));
  }
  // Doubled shogi pawns
  if (pt == SHOGI_PAWN && !shogi_doubled_pawn())
      for (File f = FILE_A; f <= max_file(); ++f)
          if (file_bb(f) & pieces(c, pt))
              b &= ~file_bb(f);
  // Sittuyin rook drops
  if (pt == ROOK && sittuyin_rook_drop())
      b &= rank_bb(relative_rank(c, RANK_1, max_rank()));

  // Filter out squares where the drop does not enclose at least one opponent's piece
  if (enclosing_drop())
  {
      // Reversi start
      if (var->enclosingDropStart & ~pieces())
          b &= var->enclosingDropStart;
      else
      {
          if (enclosing_drop() == REVERSI)
          {
              Bitboard theirs = pieces(~c);
              b &=  shift<NORTH     >(theirs) | shift<SOUTH     >(theirs)
                  | shift<NORTH_EAST>(theirs) | shift<SOUTH_WEST>(theirs)
                  | shift<EAST      >(theirs) | shift<WEST      >(theirs)
                  | shift<SOUTH_EAST>(theirs) | shift<NORTH_WEST>(theirs);
              Bitboard b2 = b;
              while (b2)
              {
                  Square s = pop_lsb(&b2);
                  if (!(attacks_bb(c, QUEEN, s, board_bb() & ~pieces(~c)) & ~PseudoAttacks[c][KING][s] & pieces(c)))
                      b ^= s;
              }
          }
          else
          {
              assert(enclosing_drop() == ATAXX);
              Bitboard ours = pieces(c);
              b &=  shift<NORTH     >(ours) | shift<SOUTH     >(ours)
                  | shift<NORTH_EAST>(ours) | shift<SOUTH_WEST>(ours)
                  | shift<EAST      >(ours) | shift<WEST      >(ours)
                  | shift<SOUTH_EAST>(ours) | shift<NORTH_WEST>(ours);
          }
      }
  }

  return b;
}

inline bool Position::sittuyin_rook_drop() const {
  assert(var != nullptr);
  return var->sittuyinRookDrop;
}

inline bool Position::drop_opposite_colored_bishop() const {
  assert(var != nullptr);
  return var->dropOppositeColoredBishop;
}

inline bool Position::drop_promoted() const {
  assert(var != nullptr);
  return var->dropPromoted;
}

inline bool Position::shogi_doubled_pawn() const {
  assert(var != nullptr);
  return var->shogiDoubledPawn;
}

inline bool Position::immobility_illegal() const {
  assert(var != nullptr);
  return var->immobilityIllegal;
}

inline bool Position::gating() const {
  assert(var != nullptr);
  return var->gating;
}

inline bool Position::seirawan_gating() const {
  assert(var != nullptr);
  return var->seirawanGating;
}

inline bool Position::cambodian_moves() const {
  assert(var != nullptr);
  return var->cambodianMoves;
}

inline Bitboard Position::diagonal_lines() const {
  assert(var != nullptr);
  return var->diagonalLines;
}

inline bool Position::pass() const {
  assert(var != nullptr);
  return var->pass || var->passOnStalemate;
}

inline bool Position::pass_on_stalemate() const {
  assert(var != nullptr);
  return var->passOnStalemate;
}

inline Bitboard Position::promoted_soldiers(Color c) const {
  assert(var != nullptr);
  return pieces(c, SOLDIER) & promotion_zone_bb(c, var->soldierPromotionRank, max_rank());
}

inline bool Position::makpong() const {
  assert(var != nullptr);
  return var->makpongRule;
}

inline int Position::n_move_rule() const {
  assert(var != nullptr);
  return var->nMoveRule;
}

inline int Position::n_fold_rule() const {
  assert(var != nullptr);
  return var->nFoldRule;
}

inline EnclosingRule Position::flip_enclosed_pieces() const {
  assert(var != nullptr);
  return var->flipEnclosedPieces;
}

inline Value Position::stalemate_value(int ply) const {
  assert(var != nullptr);
  if (var->stalematePieceCount)
  {
      int c = count<ALL_PIECES>(sideToMove) - count<ALL_PIECES>(~sideToMove);
      return c == 0 ? VALUE_DRAW : convert_mate_value(c < 0 ? var->stalemateValue : -var->stalemateValue, ply);
  }
  return convert_mate_value(var->stalemateValue, ply);
}

inline Value Position::checkmate_value(int ply) const {
  assert(var != nullptr);
  // Check for illegal mate by shogi pawn drop
  if (    var->shogiPawnDropMateIllegal
      && !(checkers() & ~pieces(SHOGI_PAWN))
      && !st->capturedPiece
      &&  st->pliesFromNull > 0
      && (st->materialKey != st->previous->materialKey))
  {
      return mate_in(ply);
  }
  // Check for shatar mate rule
  if (var->shatarMateRule)
  {
      // Mate by knight is illegal
      if (!(checkers() & ~pieces(KNIGHT)))
          return mate_in(ply);

      StateInfo* stp = st;
      while (stp->checkersBB)
      {
          // Return mate score if there is at least one shak in series of checks
          if (stp->shak)
              return convert_mate_value(var->checkmateValue, ply);

          if (stp->pliesFromNull < 2)
              break;

          stp = stp->previous->previous;
      }
      // Niol
      return VALUE_DRAW;
  }
  // Return mate value
  return convert_mate_value(var->checkmateValue, ply);
}

inline Value Position::extinction_value(int ply) const {
  assert(var != nullptr);
  return convert_mate_value(var->extinctionValue, ply);
}

inline bool Position::extinction_claim() const {
  assert(var != nullptr);
  return var->extinctionClaim;
}

inline const std::set<PieceType>& Position::extinction_piece_types() const {
  assert(var != nullptr);
  return var->extinctionPieceTypes;
}

inline int Position::extinction_piece_count() const {
  assert(var != nullptr);
  return var->extinctionPieceCount;
}

inline int Position::extinction_opponent_piece_count() const {
  assert(var != nullptr);
  return var->extinctionOpponentPieceCount;
}

inline PieceType Position::capture_the_flag_piece() const {
  assert(var != nullptr);
  return var->flagPiece;
}

inline Bitboard Position::capture_the_flag(Color c) const {
  assert(var != nullptr);
  return c == WHITE ? var->whiteFlag : var->blackFlag;
}

inline bool Position::flag_move() const {
  assert(var != nullptr);
  return var->flagMove;
}

inline bool Position::check_counting() const {
  assert(var != nullptr);
  return var->checkCounting;
}

inline int Position::connect_n() const {
  assert(var != nullptr);
  return var->connectN;
}

inline CheckCount Position::checks_remaining(Color c) const {
  return st->checksRemaining[c];
}

inline MaterialCounting Position::material_counting() const {
  assert(var != nullptr);
  return var->materialCounting;
}

inline CountingRule Position::counting_rule() const {
  assert(var != nullptr);
  return var->countingRule;
}

inline bool Position::is_immediate_game_end() const {
  Value result;
  return is_immediate_game_end(result);
}

inline bool Position::is_optional_game_end() const {
  Value result;
  return is_optional_game_end(result);
}

inline bool Position::is_game_end(Value& result, int ply) const {
  return is_immediate_game_end(result, ply) || is_optional_game_end(result, ply);
}

inline Color Position::side_to_move() const {
  return sideToMove;
}

inline Piece Position::piece_on(Square s) const {
  assert(is_ok(s));
  return board[s];
}

inline bool Position::empty(Square s) const {
  return piece_on(s) == NO_PIECE;
}

inline Piece Position::unpromoted_piece_on(Square s) const {
  return unpromotedBoard[s];
}

inline Piece Position::moved_piece(Move m) const {
  if (type_of(m) == DROP)
      return make_piece(sideToMove, dropped_piece_type(m));
  return piece_on(from_sq(m));
}

inline Bitboard Position::pieces(PieceType pt) const {
  return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const {
  return pieces(pt1) | pieces(pt2);
}

inline Bitboard Position::pieces(Color c) const {
  return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const {
  return pieces(c) & pieces(pt);
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const {
  return pieces(c) & (pieces(pt1) | pieces(pt2));
}

inline Bitboard Position::major_pieces(Color c) const {
  return pieces(c) & (pieces(QUEEN) | pieces(AIWOK) | pieces(ARCHBISHOP) | pieces(CHANCELLOR) | pieces(AMAZON));
}

inline int Position::count(Color c, PieceType pt) const {
  return pieceCount[make_piece(c, pt)];
}

template<PieceType Pt> inline int Position::count(Color c) const {
  return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt> inline int Position::count() const {
  return count<Pt>(WHITE) + count<Pt>(BLACK);
}

template<PieceType Pt> inline const Square* Position::squares(Color c) const {
  return pieceList[make_piece(c, Pt)];
}

inline const Square* Position::squares(Color c, PieceType pt) const {
  return pieceList[make_piece(c, pt)];
}

template<PieceType Pt> inline Square Position::square(Color c) const {
  assert(pieceCount[make_piece(c, Pt)] == 1);
  return squares<Pt>(c)[0];
}

inline Square Position::ep_square() const {
  return st->epSquare;
}

inline Bitboard Position::gates(Color c) const {
  assert(var != nullptr);
  return st->gatesBB[c];
}

inline bool Position::is_on_semiopen_file(Color c, Square s) const {
  return !(pieces(c, PAWN, SHOGI_PAWN) & file_bb(s));
}

inline bool Position::can_castle(CastlingRights cr) const {
  return st->castlingRights & cr;
}

inline int Position::castling_rights(Color c) const {
  return st->castlingRights & (c == WHITE ? WHITE_CASTLING : BLACK_CASTLING);
}

inline bool Position::castling_impeded(CastlingRights cr) const {
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);

  return pieces() & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRights cr) const {
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);

  return castlingRookSquare[cr];
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s, Color c) const {
  return attacks_from(c, Pt, s);
}

inline Bitboard Position::attacks_from(Color c, PieceType pt, Square s) const {
  PieceType movePt = pt == KING ? king_type() : pt;
  Bitboard b = attacks_bb(c, movePt, s, byTypeBB[ALL_PIECES]);
  // Xiangqi soldier
  if (pt == SOLDIER && !(promoted_soldiers(c) & s))
      b &= file_bb(file_of(s));
  // Janggi cannon restrictions
  if (pt == JANGGI_CANNON)
  {
      b &= ~pieces(pt);
      b &= attacks_bb(c, pt, s, pieces() ^ pieces(pt));
  }
  // Janggi palace moves
  if (diagonal_lines() & s)
  {
      PieceType diagType = movePt == WAZIR ? FERS : movePt == SOLDIER ? PAWN : movePt == ROOK ? BISHOP : NO_PIECE_TYPE;
      if (diagType)
          b |= attacks_bb(c, diagType, s, pieces()) & diagonal_lines();
      else if (movePt == JANGGI_CANNON)
          // TODO: fix for longer diagonals
          b |=   attacks_bb(c, ALFIL, s, pieces())
              & ~attacks_bb(c, ELEPHANT, s, pieces() ^ pieces(pt))
              & ~pieces(pt)
              & diagonal_lines();
  }
  return b & board_bb(c, pt);
}

inline Bitboard Position::moves_from(Color c, PieceType pt, Square s) const {
  PieceType movePt = pt == KING ? king_type() : pt;
  Bitboard b = moves_bb(c, movePt, s, byTypeBB[ALL_PIECES]);
  // Xiangqi soldier
  if (pt == SOLDIER && !(promoted_soldiers(c) & s))
      b &= file_bb(file_of(s));
  // Janggi cannon restrictions
  if (pt == JANGGI_CANNON)
  {
      b &= ~pieces(pt);
      b &= attacks_bb(c, pt, s, pieces() ^ pieces(pt));
  }
  // Janggi palace moves
  if (diagonal_lines() & s)
  {
      PieceType diagType = movePt == WAZIR ? FERS : movePt == SOLDIER ? PAWN : movePt == ROOK ? BISHOP : NO_PIECE_TYPE;
      if (diagType)
          b |= attacks_bb(c, diagType, s, pieces()) & diagonal_lines();
      else if (movePt == JANGGI_CANNON)
          // TODO: fix for longer diagonals
          b |=   attacks_bb(c, ALFIL, s, pieces())
              & ~attacks_bb(c, ELEPHANT, s, pieces() ^ pieces(pt))
              & ~pieces(pt)
              & diagonal_lines();
  }
  return b & board_bb(c, pt);
}

inline Bitboard Position::attackers_to(Square s) const {
  return attackers_to(s, pieces());
}

inline Bitboard Position::attackers_to(Square s, Color c) const {
  return attackers_to(s, byTypeBB[ALL_PIECES], c);
}

inline Bitboard Position::attackers_to(Square s, Bitboard occupied, Color c) const {
  return attackers_to(s, occupied, c, byTypeBB[JANGGI_CANNON]);
}

inline Bitboard Position::checkers() const {
  return st->checkersBB;
}

inline Bitboard Position::blockers_for_king(Color c) const {
  return st->blockersForKing[c];
}

inline Bitboard Position::check_squares(PieceType pt) const {
  return st->checkSquares[pt];
}

inline bool Position::is_discovery_check_on_king(Color c, Move m) const {
  return is_ok(from_sq(m)) && st->blockersForKing[c] & from_sq(m);
}

inline bool Position::pawn_passed(Color c, Square s) const {
  return !(pieces(~c, PAWN) & passed_pawn_span(c, s));
}

inline bool Position::advanced_pawn_push(Move m) const {
  return   type_of(moved_piece(m)) == PAWN
        && relative_rank(sideToMove, to_sq(m), max_rank()) > (max_rank() + 1) / 2;
}

inline int Position::pawns_on_same_color_squares(Color c, Square s) const {
  return popcount(pieces(c, PAWN) & ((DarkSquares & s) ? DarkSquares : ~DarkSquares));
}

inline Key Position::key() const {
  return st->key;
}

inline Key Position::pawn_key() const {
  return st->pawnKey;
}

inline Key Position::material_key() const {
  return st->materialKey;
}

inline Score Position::psq_score() const {
  return psq;
}

inline Value Position::non_pawn_material(Color c) const {
  return st->nonPawnMaterial[c];
}

inline Value Position::non_pawn_material() const {
  return non_pawn_material(WHITE) + non_pawn_material(BLACK);
}

inline int Position::game_ply() const {
  return gamePly;
}

inline int Position::counting_ply(int countStarted) const {
  return countStarted == 0 ? st->countingPly : std::min(st->countingPly, std::max(1 + gamePly - countStarted, 0));
}

inline int Position::rule50_count() const {
  return st->rule50;
}

inline bool Position::opposite_bishops() const {
  return   count<BISHOP>(WHITE) == 1
        && count<BISHOP>(BLACK) == 1
        && opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

inline bool Position::is_promoted(Square s) const {
  return promotedPieces & s;
}

inline bool Position::is_chess960() const {
  return chess960;
}

inline bool Position::capture_or_promotion(Move m) const {
  assert(is_ok(m));
  return type_of(m) == PROMOTION || type_of(m) == ENPASSANT || (type_of(m) != CASTLING && !empty(to_sq(m)));
}

inline bool Position::capture(Move m) const {
  assert(is_ok(m));
  // Castling is encoded as "king captures rook"
  return (!empty(to_sq(m)) && type_of(m) != CASTLING && from_sq(m) != to_sq(m)) || type_of(m) == ENPASSANT;
}

inline Piece Position::captured_piece() const {
  return st->capturedPiece;
}

inline Thread* Position::this_thread() const {
  return thisThread;
}

inline void Position::put_piece(Piece pc, Square s, bool isPromoted, Piece unpromotedPc) {

  board[s] = pc;
  byTypeBB[ALL_PIECES] |= s;
  byTypeBB[type_of(pc)] |= s;
  byColorBB[color_of(pc)] |= s;
  index[s] = pieceCount[pc]++;
  pieceList[pc][index[s]] = s;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
  psq += PSQT::psq[pc][s];
  if (isPromoted)
      promotedPieces |= s;
  unpromotedBoard[s] = unpromotedPc;
}

inline void Position::remove_piece(Square s) {

  // WARNING: This is not a reversible operation. If we remove a piece in
  // do_move() and then replace it in undo_move() we will put it at the end of
  // the list and not in its original place, it means index[] and pieceList[]
  // are not invariant to a do_move() + undo_move() sequence.
  Piece pc = board[s];
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[type_of(pc)] ^= s;
  byColorBB[color_of(pc)] ^= s;
  /* board[s] = NO_PIECE;  Not needed, overwritten by the capturing one */
  Square lastSquare = pieceList[pc][--pieceCount[pc]];
  index[lastSquare] = index[s];
  pieceList[pc][index[lastSquare]] = lastSquare;
  pieceList[pc][pieceCount[pc]] = SQ_NONE;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
  psq -= PSQT::psq[pc][s];
  promotedPieces -= s;
  unpromotedBoard[s] = NO_PIECE;
}

inline void Position::move_piece(Square from, Square to) {

  // index[from] is not updated and becomes stale. This works as long as index[]
  // is accessed just by known occupied squares.
  Piece pc = board[from];
  Bitboard fromTo = square_bb(from) ^ to; // from == to needs to cancel out
  byTypeBB[ALL_PIECES] ^= fromTo;
  byTypeBB[type_of(pc)] ^= fromTo;
  byColorBB[color_of(pc)] ^= fromTo;
  board[from] = NO_PIECE;
  board[to] = pc;
  index[to] = index[from];
  pieceList[pc][index[to]] = to;
  psq += PSQT::psq[pc][to] - PSQT::psq[pc][from];
  if (is_promoted(from))
      promotedPieces ^= fromTo;
  unpromotedBoard[to] = unpromotedBoard[from];
  unpromotedBoard[from] = NO_PIECE;
}

inline void Position::do_move(Move m, StateInfo& newSt) {
  do_move(m, newSt, gives_check(m));
}

inline int Position::count_in_hand(Color c, PieceType pt) const {
  return pieceCountInHand[c][pt];
}

inline int Position::count_with_hand(Color c, PieceType pt) const {
  return pieceCount[make_piece(c, pt)] + pieceCountInHand[c][pt];
}

inline bool Position::bikjang() const {
  return st->bikjang;
}

inline Value Position::material_counting_result() const {
  auto weigth_count = [this](PieceType pt, int v){ return v * (count(WHITE, pt) - count(BLACK, pt)); };
  int materialCount;
  Value result;
  switch (var->materialCounting)
  {
  case JANGGI_MATERIAL:
      materialCount =  weigth_count(ROOK, 13)
                     + weigth_count(JANGGI_CANNON, 7)
                     + weigth_count(HORSE, 5)
                     + weigth_count(JANGGI_ELEPHANT, 3)
                     + weigth_count(WAZIR, 3)
                     + weigth_count(SOLDIER, 2)
                     - 1;
      result = materialCount > 0 ? VALUE_MATE : -VALUE_MATE;
      break;
  case UNWEIGHTED_MATERIAL:
      result =  count(WHITE, ALL_PIECES) > count(BLACK, ALL_PIECES) ?  VALUE_MATE
              : count(WHITE, ALL_PIECES) < count(BLACK, ALL_PIECES) ? -VALUE_MATE
                                                                    :  VALUE_DRAW;
      break;
  case WHITE_DRAW_ODDS:
      result = VALUE_MATE;
      break;
  case BLACK_DRAW_ODDS:
      result = -VALUE_MATE;
      break;
  default:
      assert(false);
      result = VALUE_DRAW;
  }
  return sideToMove == WHITE ? result : -result;
}

inline void Position::add_to_hand(Piece pc) {
  pieceCountInHand[color_of(pc)][type_of(pc)]++;
  pieceCountInHand[color_of(pc)][ALL_PIECES]++;
  psq += PSQT::psq[pc][SQ_NONE];
}

inline void Position::remove_from_hand(Piece pc) {
  pieceCountInHand[color_of(pc)][type_of(pc)]--;
  pieceCountInHand[color_of(pc)][ALL_PIECES]--;
  psq -= PSQT::psq[pc][SQ_NONE];
}

inline void Position::drop_piece(Piece pc_hand, Piece pc_drop, Square s) {
  assert(pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)]);
  put_piece(pc_drop, s, pc_drop != pc_hand, pc_drop != pc_hand ? pc_hand : NO_PIECE);
  remove_from_hand(pc_hand);
}

inline void Position::undrop_piece(Piece pc_hand, Square s) {
  remove_piece(s);
  board[s] = NO_PIECE;
  add_to_hand(pc_hand);
  assert(pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)]);
}

#endif // #ifndef POSITION_H_INCLUDED
