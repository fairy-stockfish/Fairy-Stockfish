/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2022 The Stockfish developers (see AUTHORS file)

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
#include "evaluate.h"
#include "psqt.h"
#include "types.h"
#include "variant.h"
#include "movegen.h"

#include "nnue/nnue_accumulator.h"

namespace Stockfish {

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
  Bitboard epSquares;
  Square castlingKingSquare[COLOR_NB];
  Bitboard wallSquares;
  Bitboard gatesBB[COLOR_NB];
  Bitboard not_moved_pieces[COLOR_NB];

  // Not copied when making a move (will be recomputed anyhow)
  Key        key;
  Bitboard   checkersBB;
  Piece      unpromotedCapturedPiece;
  Piece      unpromotedBycatch[SQUARE_NB];
  Bitboard   promotedBycatch;
  Bitboard   demotedBycatch;
  StateInfo* previous;
  Bitboard   blockersForKing[COLOR_NB];
  Bitboard   pinners[COLOR_NB];
  Bitboard   checkSquares[PIECE_TYPE_NB];
  Piece      capturedPiece;
  Square     captureSquare; // when != to_sq, e.g., en passant
  Piece      promotionPawn;
  Bitboard   nonSlidingRiders;
  Bitboard   flippedPieces;
  Bitboard   pseudoRoyalCandidates;
  Bitboard   pseudoRoyals;
  OptBool    legalCapture;
  bool       capturedpromoted;
  bool       shak;
  bool       bikjang;
  Bitboard   chased;
  bool       pass;
  Move       move;
  int        repetition;

  // Used by NNUE
  Eval::NNUE::Accumulator accumulator;
  DirtyPiece dirtyPiece;
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
  std::string fen(bool sfen = false, bool showPromoted = false, int countStarted = 0, std::string holdings = "-", Bitboard fogArea = 0) const;

  // Variant rule properties
  const Variant* variant() const;
  Rank max_rank() const;
  File max_file() const;
  int ranks() const;
  int files() const;
  bool two_boards() const;
  Bitboard board_bb() const;
  Bitboard board_bb(Color c, PieceType pt) const;
  PieceSet piece_types() const;
  const std::string& piece_to_char() const;
  const std::string& piece_to_char_synonyms() const;
  Bitboard promotion_zone(Color c) const;
  Bitboard promotion_zone(Color c, PieceType pt) const;
  Bitboard promotion_zone(Piece p) const;
  Square promotion_square(Color c, Square s) const;
  PieceType promotion_pawn_type(Color c) const;
  PieceSet promotion_piece_types(Color c) const;
  bool sittuyin_promotion() const;
  int promotion_limit(PieceType pt) const;
  PieceType promoted_piece_type(PieceType pt) const;
  bool piece_promotion_on_capture() const;
  bool mandatory_pawn_promotion() const;
  bool mandatory_piece_promotion() const;
  bool piece_demotion() const;
  bool blast_on_capture() const;
  PieceSet blast_immune_types() const;
  PieceSet mutually_immune_types() const;
  EndgameEval endgame_eval() const;
  Bitboard double_step_region(Color c) const;
  Bitboard double_step_region(Color c, PieceType pt) const;
  Bitboard double_step_region(Piece p) const;
  Bitboard triple_step_region(Color c) const;
  Bitboard triple_step_region(Color c, PieceType pt) const;
  Bitboard triple_step_region(Piece p) const;
  bool castling_enabled() const;
  bool castling_dropped_piece() const;
  File castling_kingside_file() const;
  File castling_queenside_file() const;
  Rank castling_rank(Color c) const;
  File castling_king_file() const;
  PieceType castling_king_piece(Color c) const;
  PieceSet castling_rook_pieces(Color c) const;
  PieceType king_type() const;
  PieceType nnue_king() const;
  Square nnue_king_square(Color c) const;
  bool nnue_use_pockets() const;
  bool nnue_applicable() const;
  bool checking_permitted() const;
  bool drop_checks() const;
  bool must_capture() const;
  bool has_capture() const;
  bool must_drop() const;
  bool piece_drops() const;
  bool drop_loop() const;
  bool captures_to_hand() const;
  bool first_rank_pawn_drops() const;
  bool can_drop(Color c, PieceType pt) const;
  EnclosingRule enclosing_drop() const;
  Bitboard drop_region(Color c) const;
  Bitboard drop_region(Color c, PieceType pt) const;
  bool sittuyin_rook_drop() const;
  bool drop_opposite_colored_bishop() const;
  bool drop_promoted() const;
  PieceType drop_no_doubled() const;
  bool immobility_illegal() const;
  bool gating() const;
  bool walling() const;
  WallingRule walling_rule() const;
  bool seirawan_gating() const;
  bool cambodian_moves() const;
  Bitboard diagonal_lines() const;
  bool pass(Color c) const;
  bool pass_on_stalemate(Color c) const;
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
  PieceSet extinction_piece_types() const;
  bool extinction_single_piece() const;
  int extinction_piece_count() const;
  int extinction_opponent_piece_count() const;
  bool extinction_pseudo_royal() const;
  PieceType flag_piece(Color c) const;
  Bitboard flag_region(Color c) const;
  bool flag_move() const;
  bool flag_reached(Color c) const;
  bool check_counting() const;
  int connect_n() const;
  PieceSet connect_piece_types() const;
  bool connect_horizontal() const;
  bool connect_vertical() const;
  bool connect_diagonal() const;
  const std::vector<Direction>& getConnectDirections() const;
  int connect_nxn() const;
  int collinear_n() const;

  CheckCount checks_remaining(Color c) const;
  MaterialCounting material_counting() const;
  CountingRule counting_rule() const;

  // Variant-specific properties
  int count_in_hand(PieceType pt) const;
  int count_in_hand(Color c, PieceType pt) const;
  int count_with_hand(Color c, PieceType pt) const;
  bool bikjang() const;
  bool allow_virtual_drop(Color c, PieceType pt) const;

  // Position representation
  Bitboard pieces(PieceType pt = ALL_PIECES) const;
  Bitboard pieces(PieceType pt1, PieceType pt2) const;
  Bitboard pieces(Color c) const;
  Bitboard pieces(Color c, PieceType pt) const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2, PieceType pt3) const;
  Bitboard major_pieces(Color c) const;
  Bitboard non_sliding_riders() const;
  Piece piece_on(Square s) const;
  Piece unpromoted_piece_on(Square s) const;
  Bitboard ep_squares() const;
  Square castling_king_square(Color c) const;
  Bitboard gates(Color c) const;
  bool empty(Square s) const;
  int count(Color c, PieceType pt) const;
  template<PieceType Pt> int count(Color c) const;
  template<PieceType Pt> int count() const;
  template<PieceType Pt> Square square(Color c) const;
  Square square(Color c, PieceType pt) const;
  bool is_on_semiopen_file(Color c, Square s) const;

  // Castling
  CastlingRights castling_rights(Color c) const;
  bool can_castle(CastlingRights cr) const;
  bool castling_impeded(CastlingRights cr) const;
  Square castling_rook_square(CastlingRights cr) const;

  // Checking
  Bitboard checkers() const;
  Bitboard blockers_for_king(Color c) const;
  Bitboard check_squares(PieceType pt) const;
  Bitboard pinners(Color c) const;
  Bitboard checked_pseudo_royals(Color c) const;

  // Attacks to/from a given square
  Bitboard attackers_to(Square s) const;
  Bitboard attackers_to(Square s, Color c) const;
  Bitboard attackers_to(Square s, Bitboard occupied) const;
  Bitboard attackers_to(Square s, Bitboard occupied, Color c) const;
  Bitboard attackers_to(Square s, Bitboard occupied, Color c, Bitboard janggiCannons) const;
  Bitboard attacks_from(Color c, PieceType pt, Square s) const;
  Bitboard moves_from(Color c, PieceType pt, Square s) const;
  Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners, Color c) const;

  // Properties of moves
  bool legal(Move m) const;
  bool pseudo_legal(const Move m) const;
  bool virtual_drop(Move m) const;
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  Square capture_square(Square to) const;
  bool gives_check(Move m) const;
  Piece moved_piece(Move m) const;
  Piece captured_piece() const;
  const std::string piece_to_partner() const;

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
  Value blast_see(Move m) const;
  bool see_ge(Move m, Value threshold = VALUE_ZERO) const;

  // Accessing hash keys
  Key key() const;
  Key key_after(Move m) const;
  Key material_key(EndgameEval e = EG_EVAL_CHESS) const;
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
  bool is_draw(int ply) const;
  bool has_game_cycle(int ply) const;
  bool has_repeated() const;
  Bitboard chased() const;
  int count_limit(Color sideToCount) const;
  int board_honor_counting_ply(int countStarted) const;
  bool board_honor_counting_shorter(int countStarted) const;
  int counting_limit(int countStarted) const;
  int counting_ply(int countStarted) const;
  int rule50_count() const;
  Score psq_score() const;
  Value non_pawn_material(Color c) const;
  Value non_pawn_material() const;
  Bitboard not_moved_pieces(Color c) const;
  Bitboard wall_squares() const;
  Bitboard fog_area() const;

  // Position consistency check, for debugging
  bool pos_is_ok() const;
  void flip();

  // Used by NNUE
  StateInfo* state() const;

  void put_piece(Piece pc, Square s, bool isPromoted = false, Piece unpromotedPc = NO_PIECE);
  void remove_piece(Square s);

private:
  // Initialization helpers (used while setting up a position)
  void set_castling_right(Color c, Square rfrom);
  void set_state(StateInfo* si) const;
  void set_check_info(StateInfo* si) const;

  // Other helpers
  void move_piece(Square from, Square to);
  template<bool Do>
  void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

  // Data members
  Piece board[SQUARE_NB];
  Piece unpromotedBoard[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  int pieceCount[PIECE_NB];
  int castlingRightsMask[SQUARE_NB];
  Square castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
  Thread* thisThread;
  StateInfo* st;
  int gamePly;
  Color sideToMove;
  Score psq;

  // variant-specific
  const Variant* var;
  bool tsumeMode;
  bool chess960;
  int pieceCountInHand[COLOR_NB][PIECE_TYPE_NB];
  int virtualPieces;
  Bitboard promotedPieces;
  void add_to_hand(Piece pc);
  void remove_from_hand(Piece pc);
  void drop_piece(Piece pc_hand, Piece pc_drop, Square s);
  void undrop_piece(Piece pc_hand, Square s);
  Bitboard find_drop_region(Direction dir, Square s, Bitboard occupied) const;
};

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

inline int Position::ranks() const {
  assert(var != nullptr);
  return var->maxRank + 1;
}

inline int Position::files() const {
  assert(var != nullptr);
  return var->maxFile + 1;
}

inline bool Position::two_boards() const {
  assert(var != nullptr);
  return var->twoBoards;
}

inline Bitboard Position::board_bb() const {
  assert(var != nullptr);
  return board_size_bb(var->maxFile, var->maxRank) & ~st->wallSquares;
}

inline Bitboard Position::board_bb(Color c, PieceType pt) const {
  assert(var != nullptr);
  return var->mobilityRegion[c][pt] ? var->mobilityRegion[c][pt] & board_bb() : board_bb();
}

inline PieceSet Position::piece_types() const {
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

inline Bitboard Position::promotion_zone(Color c) const {
  assert(var != nullptr);
  return var->promotionRegion[c];
}

inline Bitboard Position::promotion_zone(Color c, PieceType pt) const {
    assert(var != nullptr);
    assert(pt != NO_PIECE_TYPE);
    Bitboard b = var->promotionRegion[c];

    // Piece specific promotion region
    // Only filter moves based on var->promotionRegion[] which is a restriction that applies to all pieces
    // Set whitePromotionRegion/blackPromotionRegion to AllSquares to remove the restriction
    if (var->pieceSpecificPromotionRegion)
    {
        if (c == WHITE)
        {
            b &= var->whitePiecePromotionRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
        }
        else if (c == BLACK)
        {
            b &= var->blackPiecePromotionRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
        }
    }
    return b;
}

inline Bitboard Position::promotion_zone(Piece p) const {
    assert(var != nullptr);
    assert(p != NO_PIECE);
    return promotion_zone(color_of(p), type_of(p));
}

inline Square Position::promotion_square(Color c, Square s) const {
  assert(var != nullptr);
  Piece p = piece_on(s);
  Bitboard b = ((p == NO_PIECE) ? Bitboard(0) : promotion_zone(p)) & forward_file_bb(c, s) & board_bb();
  return !b ? SQ_NONE : c == WHITE ? lsb(b) : msb(b);
}

inline PieceType Position::promotion_pawn_type(Color c) const {
  assert(var != nullptr);
  return var->promotionPawnType[c];
}

inline PieceSet Position::promotion_piece_types(Color c) const {
  assert(var != nullptr);
  return var->promotionPieceTypes[c];
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

inline bool Position::blast_on_capture() const {
  assert(var != nullptr);
  return var->blastOnCapture;
}

inline PieceSet Position::blast_immune_types() const {
  assert(var != nullptr);
  return var->blastImmuneTypes;
}

inline PieceSet Position::mutually_immune_types() const {
  assert(var != nullptr);
  return var->mutuallyImmuneTypes;
}

inline EndgameEval Position::endgame_eval() const {
  assert(var != nullptr);
  return !count_in_hand(ALL_PIECES) && (var->endgameEval != EG_EVAL_CHESS || count<KING>() == 2) ? var->endgameEval : NO_EG_EVAL;
}

inline Bitboard Position::double_step_region(Color c) const {
  assert(var != nullptr);
  return var->doubleStepRegion[c];
}

inline Bitboard Position::double_step_region(Color c, PieceType pt) const {
    assert(var != nullptr);
    assert(pt != NO_PIECE_TYPE);
    Bitboard b = var->doubleStepRegion[c];

    // Piece specific multi-step region (currently double & triple steps)
    // Only filter moves based on var->doubleStepRegion[] which is a restriction that applies to all pieces
    // Set whiteDoubleStepRegion/blackDoubleStepRegion to AllSquares to remove the restriction
    if (var->pieceSpecificDoubleStepRegion)
    {
        if (c == WHITE)
        {
            b &= var->whitePieceDoubleStepRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
        }
        else if (c == BLACK)
        {
            b &= var->blackPieceDoubleStepRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
        }
    }

    return b;
}

inline Bitboard Position::double_step_region(Piece p) const {
    assert(var != nullptr);
    assert(p != NO_PIECE);
    return double_step_region(color_of(p), type_of(p));
}

inline Bitboard Position::triple_step_region(Color c) const {
  assert(var != nullptr);
  return var->tripleStepRegion[c];
}

inline Bitboard Position::triple_step_region(Color c, PieceType pt) const {
    assert(var != nullptr);
    assert(pt != NO_PIECE_TYPE);
    Bitboard b = var->tripleStepRegion[c];

    // Piece specific multi-step region (currently double & triple steps)
    // Only filter moves based on var->tripleStepRegion[] which is a restriction that applies to all pieces
    // Set whiteTripleStepRegion/blackTripleStepRegion to AllSquares to remove the restriction
    if (var->pieceSpecificTripleStepRegion)
    {
        if (c == WHITE)
        {
            b &= var->whitePieceTripleStepRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
        }
        else if (c == BLACK)
        {
            b &= var->blackPieceTripleStepRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
        }
    }

    return b;
}

inline Bitboard Position::triple_step_region(Piece p) const {
    assert(var != nullptr);
    assert(p != NO_PIECE);
    return triple_step_region(color_of(p), type_of(p));
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

inline File Position::castling_king_file() const {
  assert(var != nullptr);
  return var->castlingKingFile;
}

inline PieceType Position::castling_king_piece(Color c) const {
  assert(var != nullptr);
  return var->castlingKingPiece[c];
}

inline PieceSet Position::castling_rook_pieces(Color c) const {
  assert(var != nullptr);
  return var->castlingRookPieces[c];
}

inline PieceType Position::king_type() const {
  assert(var != nullptr);
  return var->kingType;
}

inline PieceType Position::nnue_king() const {
  assert(var != nullptr);
  return var->nnueKing;
}

inline Square Position::nnue_king_square(Color c) const {
  return nnue_king() ? square(c, nnue_king()) : SQ_NONE;
}

inline bool Position::nnue_use_pockets() const {
  assert(var != nullptr);
  return var->nnueUsePockets;
}

inline bool Position::nnue_applicable() const {
  // Do not use NNUE during setup phases (placement, sittuyin)
  return (!count_in_hand(ALL_PIECES) || nnue_use_pockets() || !must_drop()) && !virtualPieces;
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

inline bool Position::has_capture() const {
  // Check for cached value
  if (st->legalCapture != NO_VALUE)
      return st->legalCapture == VALUE_TRUE;
  if (checkers())
  {
      for (const auto& mevasion : MoveList<EVASIONS>(*this))
          if (capture(mevasion) && legal(mevasion))
          {
              st->legalCapture = VALUE_TRUE;
              return true;
          }
  }
  else
  {
      for (const auto& mcap : MoveList<CAPTURES>(*this))
          if (capture(mcap) && legal(mcap))
          {
              st->legalCapture = VALUE_TRUE;
              return true;
          }
  }
  st->legalCapture = VALUE_FALSE;
  return false;
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

  // Piece specific drop region
  // Only filter moves based on drop_region() which is a restriction that applies to all pieces
  // Set whiteDropRegion/blackDropRegion to AllSquares to remove the restriction
  if (var->pieceSpecificDropRegion)
  {
      if (c == WHITE)
      {
          b &= var->whitePieceDropRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
      }
      else if (c == BLACK)
      {
          b &= var->blackPieceDropRegion.boardOfPiece(toupper(piece_to_char()[(c << PIECE_TYPE_BITS) | pt]));
      }
  }

  // Pawns on back ranks
  if (pt == PAWN)
  {
      if (!var->promotionZonePawnDrops)
          b &= ~promotion_zone(c, pt);
      if (!first_rank_pawn_drops())
          b &= ~rank_bb(relative_rank(c, RANK_1, max_rank()));
  }
  // Doubled shogi pawns
  if (pt == drop_no_doubled())
      for (File f = FILE_A; f <= max_file(); ++f)
          if (popcount(file_bb(f) & pieces(c, pt)) >= var->dropNoDoubledCount)
              b &= ~file_bb(f);
  // Sittuyin rook drops
  if (pt == ROOK && sittuyin_rook_drop())
      b &= rank_bb(relative_rank(c, RANK_1, max_rank()));

  if (enclosing_drop())
  {
      // Reversi start
      if (var->enclosingDropStart & ~pieces())
          b &= var->enclosingDropStart;
      else
      {
          // Filter out squares where the drop does not enclose at least one opponent's piece
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
                  Square s = pop_lsb(b2);
                  if (!(attacks_bb(c, QUEEN, s, board_bb() & ~pieces(~c)) & ~PseudoAttacks[c][KING][s] & pieces(c)))
                      b ^= s;
              }
          }
          else if (enclosing_drop() == SNORT)
          {
              Bitboard theirs = pieces(~c);
              b &=   ~(shift<NORTH     >(theirs) | shift<SOUTH     >(theirs)
                  | shift<EAST      >(theirs) | shift<WEST      >(theirs));
          }
          else if (enclosing_drop() == ANYSIDE)
          {
              Bitboard occupied = pieces();
              b = 0ULL;
              Bitboard candidates = (shift<WEST>(occupied) | file_bb(max_file())) & ~occupied;

              for (Rank r = RANK_1; r <= max_rank(); ++r) {
                  if (!(occupied & make_square(FILE_A, r))) {
                      b |= lsb(candidates & rank_bb(r));
                  }
              }
              candidates = (shift<SOUTH>(occupied) | rank_bb(max_rank())) & ~occupied;
              for (File f = FILE_A; f <= max_file(); ++f) {
                  if (!(occupied & make_square(f, RANK_1))) {
                      b |= lsb(candidates & file_bb(f));
                  }
              }
              candidates = (shift<NORTH>(occupied) | rank_bb(RANK_1)) & ~occupied;
              for (File f = FILE_A; f <= max_file(); ++f) {
                  if (!(occupied & make_square(f, max_rank()))) {
                      b |= lsb(candidates & file_bb(f));
                  }
              }
              candidates = (shift<EAST>(occupied) | file_bb(FILE_A)) & ~occupied;
              for (Rank r = RANK_1; r <= max_rank(); ++r) {
                  if (!(occupied & make_square(max_file(), r))) {
                      b |= lsb(candidates & rank_bb(r));
                  }
              }
          }
          else if (enclosing_drop() == TOP)
          {
              b &= shift<NORTH>(pieces()) | Rank1BB;
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

inline PieceType Position::drop_no_doubled() const {
  assert(var != nullptr);
  return var->dropNoDoubled;
}

inline bool Position::immobility_illegal() const {
  assert(var != nullptr);
  return var->immobilityIllegal;
}

inline bool Position::gating() const {
  assert(var != nullptr);
  return var->gating;
}

inline bool Position::walling() const {
  assert(var != nullptr);
  return var->wallingRule != NO_WALLING;
}

inline WallingRule Position::walling_rule() const {
  assert(var != nullptr);
  return var->wallingRule;
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

inline bool Position::pass(Color c) const {
  assert(var != nullptr);
  return var->pass[c] || var->passOnStalemate[c];
}

inline bool Position::pass_on_stalemate(Color c) const {
  assert(var != nullptr);
  return var->passOnStalemate[c];
}

inline Bitboard Position::promoted_soldiers(Color c) const {
  assert(var != nullptr);
  return pieces(c, SOLDIER) & zone_bb(c, var->soldierPromotionRank, max_rank());
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
  // Check for checkmate of pseudo-royal pieces
  if (var->extinctionPseudoRoyal)
  {
      Bitboard pseudoRoyals = st->pseudoRoyals & pieces(sideToMove);
      Bitboard pseudoRoyalsTheirs = st->pseudoRoyals & pieces(~sideToMove);
      while (pseudoRoyals)
      {
          Square sr = pop_lsb(pseudoRoyals);
          if (  !(blast_on_capture() && (pseudoRoyalsTheirs & attacks_bb<KING>(sr)))
              && attackers_to(sr, ~sideToMove))
              return convert_mate_value(var->checkmateValue, ply);
      }
      // Look for duple check
      if (var->dupleCheck)
      {
          Bitboard pseudoRoyalCandidates = st->pseudoRoyalCandidates & pieces(sideToMove);
          bool allCheck = bool(pseudoRoyalCandidates);
          while (allCheck && pseudoRoyalCandidates)
          {
              Square sr = pop_lsb(pseudoRoyalCandidates);
              // Touching pseudo-royal pieces are immune
              if (!(  !(blast_on_capture() && (pseudoRoyalsTheirs & attacks_bb<KING>(sr)))
                    && attackers_to(sr, ~sideToMove)))
                  allCheck = false;
          }
          if (allCheck)
              return convert_mate_value(var->checkmateValue, ply);
      }
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
  // Checkmate using virtual pieces
  if (two_boards() && var->checkmateValue < VALUE_ZERO)
  {
      Value virtualMaterial = VALUE_ZERO;
      for (PieceSet ps = piece_types(); ps;)
      {
          PieceType pt = pop_lsb(ps);
          virtualMaterial += std::max(-count_in_hand(~sideToMove, pt), 0) * PieceValue[MG][pt];
      }

      if (virtualMaterial > 0)
          return -VALUE_VIRTUAL_MATE + virtualMaterial / 20 + ply;
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

inline PieceSet Position::extinction_piece_types() const {
  assert(var != nullptr);
  return var->extinctionPieceTypes;
}

inline bool Position::extinction_single_piece() const {
  assert(var != nullptr);
  return   var->extinctionValue == -VALUE_MATE
        && (var->extinctionPieceTypes & ~piece_set(ALL_PIECES));
}

inline int Position::extinction_piece_count() const {
  assert(var != nullptr);
  return var->extinctionPieceCount;
}

inline int Position::extinction_opponent_piece_count() const {
  assert(var != nullptr);
  return var->extinctionOpponentPieceCount;
}

inline bool Position::extinction_pseudo_royal() const {
  assert(var != nullptr);
  return var->extinctionPseudoRoyal;
}

inline PieceType Position::flag_piece(Color c) const {
  assert(var != nullptr);
  return var->flagPiece[c];
}

inline Bitboard Position::flag_region(Color c) const {
  assert(var != nullptr);
  return var->flagRegion[c];
}

inline bool Position::flag_move() const {
  assert(var != nullptr);
  return var->flagMove;
}

inline bool Position::flag_reached(Color c) const {
  assert(var != nullptr);
  bool simpleResult = 
        (flag_region(c) & pieces(c, flag_piece(c)))
        && (   popcount(flag_region(c) & pieces(c, flag_piece(c))) >= var->flagPieceCount
            || (var->flagPieceBlockedWin && !(flag_region(c) & ~pieces())));
      
  if (simpleResult&&var->flagPieceSafe)
  {
      Bitboard piecesInFlagZone = flag_region(c) & pieces(c, flag_piece(c));
      int potentialPieces = (popcount(piecesInFlagZone));
      /*
      There isn't a variant that uses it, but in the hypothetical game where the rules say I need 3
      pieces in the flag zone and they need to be safe: If I have 3 pieces there, but one is under
      threat, I don't think I can declare victory. If I have 4 there, but one is under threat, I
      think that's victory.
      */      
      while (piecesInFlagZone)
      {
          Square sr = pop_lsb(piecesInFlagZone);
          Bitboard flagAttackers = attackers_to(sr, ~c);

          if ((potentialPieces < var->flagPieceCount) || (potentialPieces >= var->flagPieceCount + 1)) break;
          while (flagAttackers)
          {
              Square currentAttack = pop_lsb(flagAttackers);
              if (legal(make_move(currentAttack, sr)))
              {
                  potentialPieces--;
                  break;
              }
          }
      }
      return potentialPieces >= var->flagPieceCount;
  }
  return simpleResult;
}

inline bool Position::check_counting() const {
  assert(var != nullptr);
  return var->checkCounting;
}

inline int Position::connect_n() const {
  assert(var != nullptr);
  return var->connectN;
}

inline PieceSet Position::connect_piece_types() const {
  assert(var != nullptr);
  return var->connectPieceTypesTrimmed;
}

inline bool Position::connect_horizontal() const {
  assert(var != nullptr);
  return var->connectHorizontal;
}
inline bool Position::connect_vertical() const {
  assert(var != nullptr);
  return var->connectVertical;
}
inline bool Position::connect_diagonal() const {
  assert(var != nullptr);
  return var->connectDiagonal;
}

inline const std::vector<Direction>& Position::getConnectDirections() const {
    assert(var != nullptr);
    return var->connect_directions;
}

inline int Position::connect_nxn() const {
  assert(var != nullptr);
  return var->connectNxN;
}

inline int Position::collinear_n() const {
  assert(var != nullptr);
  return var->collinearN;
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

inline bool Position::is_draw(int ply) const {
  Value result;
  return is_optional_game_end(result, ply);
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

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2, PieceType pt3) const {
  return pieces(c) & (pieces(pt1) | pieces(pt2) | pieces(pt3));
}

inline Bitboard Position::major_pieces(Color c) const {
  return pieces(c) & (pieces(QUEEN) | pieces(AIWOK) | pieces(ARCHBISHOP) | pieces(CHANCELLOR) | pieces(AMAZON));
}

inline Bitboard Position::non_sliding_riders() const {
  return st->nonSlidingRiders;
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

template<PieceType Pt> inline Square Position::square(Color c) const {
  assert(count<Pt>(c) == 1);
  return lsb(pieces(c, Pt));
}

inline Square Position::square(Color c, PieceType pt) const {
  assert(count(c, pt) == 1);
  return lsb(pieces(c, pt));
}

inline Bitboard Position::ep_squares() const {
  return st->epSquares;
}

inline Square Position::castling_king_square(Color c) const {
  return st->castlingKingSquare[c];
}

inline Bitboard Position::gates(Color c) const {
  assert(var != nullptr);
  return st->gatesBB[c];
}

inline bool Position::is_on_semiopen_file(Color c, Square s) const {
  return !((pieces(c, PAWN) | pieces(c, SHOGI_PAWN, SOLDIER)) & file_bb(s));
}

inline bool Position::can_castle(CastlingRights cr) const {
  return st->castlingRights & cr;
}

inline CastlingRights Position::castling_rights(Color c) const {
  return c & CastlingRights(st->castlingRights);
}

inline bool Position::castling_impeded(CastlingRights cr) const {
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);

  return pieces() & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRights cr) const {
  assert(cr == WHITE_OO || cr == WHITE_OOO || cr == BLACK_OO || cr == BLACK_OOO);

  return castlingRookSquare[cr];
}

inline Bitboard Position::attacks_from(Color c, PieceType pt, Square s) const {
  if (var->fastAttacks || var->fastAttacks2)
      return attacks_bb(c, pt, s, byTypeBB[ALL_PIECES]) & board_bb();

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
          b |=  rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces())
              & rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces() ^ pieces(pt))
              & ~pieces(pt)
              & diagonal_lines();
  }
  return b & board_bb(c, pt);
}

inline Bitboard Position::moves_from(Color c, PieceType pt, Square s) const {

    Bitboard extraDestinations = 0x00;

    // Piece specific double/triple step region
    // It adds new moves to the pieces, enabling the piece to move 2 or 3 squares ahead
    // Since double step in introduced from chess variants where pawns cannot capture forward, capturing moves are not included here.
    // Double/Triple step cannot attack other pieces, so attacks_from(Color c, PieceType pt, Square s) is not changed
    // Due to some unknown issues, shift<Direction D>(Bitboard b) cannot be used here
    if (var->pieceSpecificTripleStepRegion)
    {
        Bitboard tripleStepRegion = this->triple_step_region(c, pt);
        Bitboard occupied = this->pieces();  //Bitboard where the bits whose corresponding squares having a piece on it are 1
        Bitboard piecePosition = square_bb(s);  //Bitboard where only the bit which refers to the square that the piece starts the move (original square) is 1
        Bitboard extraMultipleStepMoveDestinations = 0x00;  //Bitboard where extra legal multi-step destination square bits are 1
        if (tripleStepRegion & piecePosition & this->not_moved_pieces(c))  //If the original square is in tripleStepRegion and the piece is not moved
        {
            Bitboard oneSquareAhead = (c == WHITE) ? piecePosition << NORTH : piecePosition >> NORTH;
            if (!(oneSquareAhead & occupied))  //If the square which is 1 square ahead of original square is NOT blocked
            {
                extraMultipleStepMoveDestinations |= oneSquareAhead;  //Add the square which is 1 square ahead of original square to destination squares for triple step
                Bitboard twoSquareAhead = (c == WHITE) ? piecePosition << NORTH << NORTH : piecePosition >> NORTH >> NORTH;
                if (!(twoSquareAhead & occupied))  //If the square which is 2 squares ahead of original square is NOT blocked
                {
                    extraMultipleStepMoveDestinations |= twoSquareAhead;  //Add the square which is 2 squares ahead of original square to destination squares for triple step
                    Bitboard threeSquareAhead = (c == WHITE) ? piecePosition << NORTH << NORTH << NORTH : piecePosition >> NORTH >> NORTH >> NORTH;
                    if (!(threeSquareAhead & occupied))  //If the square which is 3 squares ahead of original square is NOT blocked
                    {
                        extraMultipleStepMoveDestinations |= threeSquareAhead;  //Add the square which is 3 squares ahead of original square to destination squares for triple step
                    }
                }
            }
            extraDestinations |= extraMultipleStepMoveDestinations; //Add destination squares to base board
        }
    }
    if (var->pieceSpecificDoubleStepRegion)
    {
        Bitboard doubleStepRegion = this->double_step_region(c, pt);
        Bitboard occupied = this->pieces();  //Bitboard where the bits whose corresponding squares having a piece on it are 1
        Bitboard piecePosition = square_bb(s);  //Bitboard where only the bit which refers to the square that the piece starts the move (original square) is 1
        Bitboard extraMultipleStepMoveDestinations = 0x00;  //Bitboard where extra legal multi-step destination square bits are 1
        if (doubleStepRegion & piecePosition & this->not_moved_pieces(c))  //If the original square is in doubleStepRegion and the piece is not moved
        {
            Bitboard oneSquareAhead = (c == WHITE) ? piecePosition << NORTH : piecePosition >> NORTH;
            if (!(oneSquareAhead & occupied))  //If the square which is 1 square ahead of original square is NOT blocked
            {
                extraMultipleStepMoveDestinations |= oneSquareAhead;  //Add the square which is 1 square ahead of original square to destination squares for triple step
                Bitboard twoSquareAhead = (c == WHITE) ? piecePosition << NORTH << NORTH : piecePosition >> NORTH >> NORTH;
                if (!(twoSquareAhead & occupied))  //If the square which is 2 squares ahead of original square is NOT blocked
                {
                    extraMultipleStepMoveDestinations |= twoSquareAhead;  //Add the square which is 2 squares ahead of original square to destination squares for triple step
                }
            }
            extraDestinations |= extraMultipleStepMoveDestinations; //Add destination squares to base board
        }
    }

  if (var->fastAttacks || var->fastAttacks2)
      return (moves_bb(c, pt, s, byTypeBB[ALL_PIECES]) | extraDestinations) & board_bb();

  PieceType movePt = pt == KING ? king_type() : pt;
  Bitboard b = (moves_bb(c, movePt, s, byTypeBB[ALL_PIECES]) | extraDestinations);
  // Add initial moves
  if (double_step_region(c, pt) & s)
      b |= moves_bb<true>(c, movePt, s, byTypeBB[ALL_PIECES]);
  
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
          b |=  rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces())
              & rider_attacks_bb<RIDER_CANNON_DIAG>(s, pieces() ^ pieces(pt))
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

inline Bitboard Position::pinners(Color c) const {
  return st->pinners[c];
}

inline Bitboard Position::check_squares(PieceType pt) const {
  return st->checkSquares[pt];
}

inline bool Position::pawn_passed(Color c, Square s) const {
  return !(pieces(~c, PAWN) & passed_pawn_span(c, s));
}

inline int Position::pawns_on_same_color_squares(Color c, Square s) const {
  return popcount(pieces(c, PAWN) & ((DarkSquares & s) ? DarkSquares : ~DarkSquares));
}

inline Key Position::key() const {
  return st->rule50 < 14 ? st->key
                         : st->key ^ make_key((st->rule50 - 14) / 8);
}

inline Key Position::pawn_key() const {
  return st->pawnKey;
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

inline int Position::board_honor_counting_ply(int countStarted) const {
  return countStarted == 0 ?
      st->countingPly :
      countStarted < 0 ? 0 : std::max(1 + gamePly - countStarted, 0);
}

inline bool Position::board_honor_counting_shorter(int countStarted) const {
  return counting_rule() == CAMBODIAN_COUNTING && 126 - board_honor_counting_ply(countStarted) < st->countingLimit - st->countingPly;
}

inline int Position::counting_limit(int countStarted) const {
  return board_honor_counting_shorter(countStarted) ? 126 : st->countingLimit;
}

inline int Position::counting_ply(int countStarted) const {
  return !count<PAWN>() && (count<ALL_PIECES>(WHITE) <= 1 || count<ALL_PIECES>(BLACK) <= 1) && !board_honor_counting_shorter(countStarted) ?
      st->countingPly :
      board_honor_counting_ply(countStarted);
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
  return type_of(m) == PROMOTION || type_of(m) == EN_PASSANT || (type_of(m) != CASTLING && !empty(to_sq(m)));
}

inline bool Position::capture(Move m) const {
  assert(is_ok(m));
  // Castling is encoded as "king captures rook"
  return (!empty(to_sq(m)) && type_of(m) != CASTLING && from_sq(m) != to_sq(m)) || type_of(m) == EN_PASSANT;
}

inline Square Position::capture_square(Square to) const {
  assert(is_ok(to));
  // The capture square of en passant is either the marked ep piece or the closest piece behind the target square
  Bitboard b = ep_squares() & pieces() ? ep_squares() & pieces() : pieces(~sideToMove) & forward_file_bb(~sideToMove, to);
  return sideToMove == WHITE ? msb(b) : lsb(b);
}

inline bool Position::virtual_drop(Move m) const {
  assert(is_ok(m));
  return type_of(m) == DROP && !can_drop(side_to_move(), in_hand_piece_type(m));
}

inline Piece Position::captured_piece() const {
  return st->capturedPiece;
}

inline Bitboard Position::fog_area() const {
  Bitboard b = board_bb();
  // Our own pieces are visible
  Bitboard visible = pieces(sideToMove);
  // Squares where we can move to are visible as well
  for (const auto& m : MoveList<LEGAL>(*this))
  {
    Square to = to_sq(m);
    visible |= to;
  }
  // Everything else is invisible
  return ~visible & b;
}

inline const std::string Position::piece_to_partner() const {
  if (!st->capturedPiece) return std::string();
  Color color = color_of(st->capturedPiece);
  Piece piece = st->capturedpromoted ?
      (st->unpromotedCapturedPiece ? st->unpromotedCapturedPiece : make_piece(color, promotion_pawn_type(color))) :
      st->capturedPiece;
  return std::string(1, piece_to_char()[piece]);
}

inline Thread* Position::this_thread() const {
  return thisThread;
}

inline void Position::put_piece(Piece pc, Square s, bool isPromoted, Piece unpromotedPc) {

  board[s] = pc;
  byTypeBB[ALL_PIECES] |= byTypeBB[type_of(pc)] |= s;
  byColorBB[color_of(pc)] |= s;
  pieceCount[pc]++;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
  psq += PSQT::psq[pc][s];
  if (isPromoted)
      promotedPieces |= s;
  unpromotedBoard[s] = unpromotedPc;

  //Adding new pieces will make it as a not-moved-piece
  this->st->not_moved_pieces[color_of(pc)] |= square_bb(s);
}

inline void Position::remove_piece(Square s) {

  Piece pc = board[s];
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[type_of(pc)] ^= s;
  byColorBB[color_of(pc)] ^= s;
  board[s] = NO_PIECE;
  pieceCount[pc]--;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
  psq -= PSQT::psq[pc][s];
  promotedPieces -= s;
  unpromotedBoard[s] = NO_PIECE;

  //not-moved-piece bitboard must ensure that there is a piece
  this->st->not_moved_pieces[WHITE] &= (~square_bb(s));
  this->st->not_moved_pieces[BLACK] &= (~square_bb(s));
}

inline void Position::move_piece(Square from, Square to) {

  Piece pc = board[from];
  Bitboard fromTo = square_bb(from) ^ to; // from == to needs to cancel out
  byTypeBB[ALL_PIECES] ^= fromTo;
  byTypeBB[type_of(pc)] ^= fromTo;
  byColorBB[color_of(pc)] ^= fromTo;
  board[from] = NO_PIECE;
  board[to] = pc;
  psq += PSQT::psq[pc][to] - PSQT::psq[pc][from];
  if (is_promoted(from))
      promotedPieces ^= fromTo;
  unpromotedBoard[to] = unpromotedBoard[from];
  unpromotedBoard[from] = NO_PIECE;

  //Once moved, no matter whether the piece is on original square or on destination square (including captures) or the color of the piece, it is no longer not-moved-piece
  this->st->not_moved_pieces[WHITE] &= (~(square_bb(from) | square_bb(to)));
  this->st->not_moved_pieces[BLACK] &= (~(square_bb(from) | square_bb(to)));
}

inline void Position::do_move(Move m, StateInfo& newSt) {
  do_move(m, newSt, gives_check(m));
}

inline StateInfo* Position::state() const {

  return st;
}

// Variant-specific

inline int Position::count_in_hand(PieceType pt) const {
  return pieceCountInHand[WHITE][pt] + pieceCountInHand[BLACK][pt];
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

inline bool Position::allow_virtual_drop(Color c, PieceType pt) const {
  assert(two_boards());
  // Do we allow a virtual drop?
  return pt != KING && (   count_in_hand(c, PAWN) >= -(pt == PAWN)
                        && count_in_hand(c, KNIGHT) >= -(pt == PAWN)
                        && count_in_hand(c, BISHOP) >= -(pt == PAWN)
                        && count_in_hand(c, ROOK) >= 0
                        && count_in_hand(c, QUEEN) >= 0);
}

inline Value Position::material_counting_result() const {
  auto weight_count = [this](PieceType pt, int v){ return v * (count(WHITE, pt) - count(BLACK, pt)); };
  int materialCount;
  Value result;
  switch (var->materialCounting)
  {
  case JANGGI_MATERIAL:
      materialCount =  weight_count(ROOK, 13)
                     + weight_count(JANGGI_CANNON, 7)
                     + weight_count(HORSE, 5)
                     + weight_count(JANGGI_ELEPHANT, 3)
                     + weight_count(WAZIR, 3)
                     + weight_count(SOLDIER, 2)
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
  if (variant()->freeDrops) return;
  pieceCountInHand[color_of(pc)][type_of(pc)]++;
  pieceCountInHand[color_of(pc)][ALL_PIECES]++;
  psq += PSQT::psq[pc][SQ_NONE];
}

inline void Position::remove_from_hand(Piece pc) {
  if (variant()->freeDrops) return;
  pieceCountInHand[color_of(pc)][type_of(pc)]--;
  pieceCountInHand[color_of(pc)][ALL_PIECES]--;
  psq -= PSQT::psq[pc][SQ_NONE];
}

inline void Position::drop_piece(Piece pc_hand, Piece pc_drop, Square s) {
  assert(can_drop(color_of(pc_hand), type_of(pc_hand)) || var->twoBoards);
  put_piece(pc_drop, s, pc_drop != pc_hand, pc_drop != pc_hand ? pc_hand : NO_PIECE);
  remove_from_hand(pc_hand);
  virtualPieces += (pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)] < 0);
}

inline void Position::undrop_piece(Piece pc_hand, Square s) {
  virtualPieces -= (pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)] < 0);
  remove_piece(s);
  board[s] = NO_PIECE;
  add_to_hand(pc_hand);
  assert(can_drop(color_of(pc_hand), type_of(pc_hand)) || var->twoBoards);
}

inline bool Position::can_drop(Color c, PieceType pt) const {
  return variant()->freeDrops || count_in_hand(c, pt) > 0;
}

//Returns the pieces that are not moved (including newly added pieces during the game, i.e. DROPS) of a side
inline Bitboard Position::not_moved_pieces(Color c) const {
    return this->st->not_moved_pieces[c];
}

//Returns the places of wall squares
inline Bitboard Position::wall_squares() const {
    return this->st->wallSquares;
}

} // namespace Stockfish

#endif // #ifndef POSITION_H_INCLUDED
