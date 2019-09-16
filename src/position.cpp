/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2019 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

#include <algorithm>
#include <cassert>
#include <cstddef> // For offsetof()
#include <cstring> // For std::memset, std::memcmp
#include <iomanip>
#include <sstream>

#include "bitboard.h"
#include "misc.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

using std::string;

namespace Zobrist {

  Key psq[PIECE_NB][SQUARE_NB];
  Key enpassant[FILE_NB];
  Key castling[CASTLING_RIGHT_NB];
  Key side, noPawns;
  Key inHand[PIECE_NB][SQUARE_NB];
  Key checks[COLOR_NB][CHECKS_NB];
}

namespace {

// min_attacker() is a helper function used by see_ge() to locate the least
// valuable attacker for the side to move, remove the attacker we just found
// from the bitboards and scan for new X-ray attacks behind it.

template<PieceType Pt>
PieceType min_attacker(const Bitboard* byTypeBB, Square to, Bitboard stmAttackers,
                       Bitboard& occupied, Bitboard& attackers) {

  Bitboard b = stmAttackers & byTypeBB[Pt];
  if (!b)
      return min_attacker<PieceType(Pt + 1)>(byTypeBB, to, stmAttackers, occupied, attackers);

  occupied ^= lsb(b); // Remove the attacker from occupied

  // Add any X-ray attack behind the just removed piece. For instance with
  // rooks in a8 and a7 attacking a1, after removing a7 we add rook in a8.
  // Note that new added attackers can be of any color.
  if (Pt == PAWN || Pt == BISHOP || Pt == QUEEN)
      attackers |= attacks_bb<BISHOP>(to, occupied) & (byTypeBB[BISHOP] | byTypeBB[QUEEN]);

  if (Pt == ROOK || Pt == QUEEN)
      attackers |= attacks_bb<ROOK>(to, occupied) & (byTypeBB[ROOK] | byTypeBB[QUEEN]);

  // X-ray may add already processed pieces because byTypeBB[] is constant: in
  // the rook example, now attackers contains _again_ rook in a7, so remove it.
  attackers &= occupied;
  return Pt;
}

template<>
PieceType min_attacker<KING>(const Bitboard*, Square, Bitboard, Bitboard&, Bitboard&) {
  return KING; // No need to update bitboards: it is the last cycle
}

} // namespace


/// operator<<(Position) returns an ASCII representation of the position

std::ostream& operator<<(std::ostream& os, const Position& pos) {

  os << "\n ";
  for (File f = FILE_A; f <= pos.max_file(); ++f)
      os << "+---";
  os << "+\n";

  for (Rank r = pos.max_rank(); r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= pos.max_file(); ++f)
          os << " | " << pos.piece_to_char()[pos.piece_on(make_square(f, r))];

      os << " |\n ";
      for (File f = FILE_A; f <= pos.max_file(); ++f)
          os << "+---";
      os << "+\n";
  }

  os << "\nFen: " << pos.fen() << "\nKey: " << std::hex << std::uppercase
     << std::setfill('0') << std::setw(16) << pos.key()
     << std::setfill(' ') << std::dec << "\nCheckers: ";

  for (Bitboard b = pos.checkers(); b; )
      os << UCI::square(pos, pop_lsb(&b)) << " ";

  if (    int(Tablebases::MaxCardinality) >= popcount(pos.pieces())
      && Options["UCI_Variant"] == "chess"
      && !pos.can_castle(ANY_CASTLING))
  {
      StateInfo st;
      Position p;
      p.set(pos.variant(), pos.fen(), pos.is_chess960(), &st, pos.this_thread());
      Tablebases::ProbeState s1, s2;
      Tablebases::WDLScore wdl = Tablebases::probe_wdl(p, &s1);
      int dtz = Tablebases::probe_dtz(p, &s2);
      os << "\nTablebases WDL: " << std::setw(4) << wdl << " (" << s1 << ")"
         << "\nTablebases DTZ: " << std::setw(4) << dtz << " (" << s2 << ")";
  }

  return os;
}


// Marcel van Kervinck's cuckoo algorithm for fast detection of "upcoming repetition"
// situations. Description of the algorithm in the following paper:
// https://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf

// First and second hash functions for indexing the cuckoo tables
#ifdef LARGEBOARDS
inline int H1(Key h) { return h & 0x7fff; }
inline int H2(Key h) { return (h >> 16) & 0x7fff; }
#else
inline int H1(Key h) { return h & 0x1fff; }
inline int H2(Key h) { return (h >> 16) & 0x1fff; }
#endif

// Cuckoo tables with Zobrist hashes of valid reversible moves, and the moves themselves
#ifdef LARGEBOARDS
Key cuckoo[65536];
Move cuckooMove[65536];
#else
Key cuckoo[8192];
Move cuckooMove[8192];
#endif


/// Position::init() initializes at startup the various arrays used to compute
/// hash keys.

void Position::init() {

  PRNG rng(1070372);

  for (Color c : {WHITE, BLACK})
      for (PieceType pt = PAWN; pt <= KING; ++pt)
          for (Square s = SQ_A1; s <= SQ_MAX; ++s)
              Zobrist::psq[make_piece(c, pt)][s] = rng.rand<Key>();

  for (File f = FILE_A; f <= FILE_MAX; ++f)
      Zobrist::enpassant[f] = rng.rand<Key>();

  for (int cr = NO_CASTLING; cr <= ANY_CASTLING; ++cr)
  {
      Zobrist::castling[cr] = 0;
      Bitboard b = cr;
      while (b)
      {
          Key k = Zobrist::castling[1ULL << pop_lsb(&b)];
          Zobrist::castling[cr] ^= k ? k : rng.rand<Key>();
      }
  }

  Zobrist::side = rng.rand<Key>();
  Zobrist::noPawns = rng.rand<Key>();

  for (Color c : {WHITE, BLACK})
      for (int n = 0; n < CHECKS_NB; ++n)
          Zobrist::checks[c][n] = rng.rand<Key>();

  for (Color c : {WHITE, BLACK})
      for (PieceType pt = PAWN; pt <= KING; ++pt)
          for (int n = 0; n < SQUARE_NB; ++n)
              Zobrist::inHand[make_piece(c, pt)][n] = rng.rand<Key>();

  // Prepare the cuckoo tables
  std::memset(cuckoo, 0, sizeof(cuckoo));
  std::memset(cuckooMove, 0, sizeof(cuckooMove));
  int count = 0;
  for (Color c : {WHITE, BLACK})
      for (PieceType pt = KNIGHT; pt <= QUEEN || pt == KING; pt != QUEEN ? ++pt : pt = KING)
      {
      Piece pc = make_piece(c, pt);
      for (Square s1 = SQ_A1; s1 <= SQ_MAX; ++s1)
          for (Square s2 = Square(s1 + 1); s2 <= SQ_MAX; ++s2)
              if (PseudoAttacks[WHITE][type_of(pc)][s1] & s2)
              {
                  Move move = make_move(s1, s2);
                  Key key = Zobrist::psq[pc][s1] ^ Zobrist::psq[pc][s2] ^ Zobrist::side;
                  int i = H1(key);
                  while (true)
                  {
                      std::swap(cuckoo[i], key);
                      std::swap(cuckooMove[i], move);
                      if (move == MOVE_NONE) // Arrived at empty slot?
                          break;
                      i = (i == H1(key)) ? H2(key) : H1(key); // Push victim to alternative slot
                  }
                  count++;
             }
      }
#ifdef LARGEBOARDS
  assert(count == 9344);
#else
  assert(count == 3668);
#endif
}


/// Position::set() initializes the position object with the given FEN string.
/// This function is not very robust - make sure that input FENs are correct,
/// this is assumed to be the responsibility of the GUI.

Position& Position::set(const Variant* v, const string& fenStr, bool isChess960, StateInfo* si, Thread* th, bool sfen) {
/*
   A FEN string defines a particular position using only the ASCII character set.

   A FEN string contains six fields separated by a space. The fields are:

   1) Piece placement (from white's perspective). Each rank is described, starting
      with rank 8 and ending with rank 1. Within each rank, the contents of each
      square are described from file A through file H. Following the Standard
      Algebraic Notation (SAN), each piece is identified by a single letter taken
      from the standard English names. White pieces are designated using upper-case
      letters ("PNBRQK") whilst Black uses lowercase ("pnbrqk"). Blank squares are
      noted using digits 1 through 8 (the number of blank squares), and "/"
      separates ranks.

   2) Active color. "w" means white moves next, "b" means black.

   3) Castling availability. If neither side can castle, this is "-". Otherwise,
      this has one or more letters: "K" (White can castle kingside), "Q" (White
      can castle queenside), "k" (Black can castle kingside), and/or "q" (Black
      can castle queenside).

   4) En passant target square (in algebraic notation). If there's no en passant
      target square, this is "-". If a pawn has just made a 2-square move, this
      is the position "behind" the pawn. This is recorded only if there is a pawn
      in position to make an en passant capture, and if there really is a pawn
      that might have advanced two squares.

   5) Halfmove clock. This is the number of halfmoves since the last pawn advance
      or capture. This is used to determine if a draw can be claimed under the
      fifty-move rule.

   6) Fullmove number. The number of the full move. It starts at 1, and is
      incremented after Black's move.
*/

  unsigned char col, row, token;
  size_t idx;
  std::istringstream ss(fenStr);

  std::memset(this, 0, sizeof(Position));
  std::memset(si, 0, sizeof(StateInfo));
  std::fill_n(&pieceList[0][0], sizeof(pieceList) / sizeof(Square), SQ_NONE);
  var = v;
  st = si;

  ss >> std::noskipws;

  Square sq = SQ_A1 + max_rank() * NORTH;

  // 1. Piece placement
  while ((ss >> token) && !isspace(token))
  {
      if (isdigit(token))
      {
#ifdef LARGEBOARDS
          if (isdigit(ss.peek()))
          {
              sq += 10 * (token - '0') * EAST;
              ss >> token;
              sq += (token - '0') * EAST;
          }
          else
#endif
          sq += (token - '0') * EAST; // Advance the given number of files
      }

      else if (token == '/')
      {
          sq += 2 * SOUTH + (FILE_MAX - max_file()) * EAST;
          if (!is_ok(sq))
              break;
      }

      else if ((idx = piece_to_char().find(token)) != string::npos)
      {
          put_piece(Piece(idx), sq);
          ++sq;
      }
      // Promoted shogi pieces
      else if (token == '+')
      {
          ss >> token;
          idx = piece_to_char().find(token);
          unpromotedBoard[sq] = Piece(idx);
          promotedPieces |= SquareBB[sq];
          put_piece(make_piece(color_of(Piece(idx)), promoted_piece_type(type_of(Piece(idx)))), sq);
          ++sq;
      }
      // Set flag for promoted pieces
      else if (captures_to_hand() && !drop_loop() && token == '~')
          promotedPieces |= SquareBB[sq - 1];
      // Stop before pieces in hand
      else if (token == '[')
          break;
  }
  // Pieces in hand
  if (!isspace(token))
      while ((ss >> token) && !isspace(token))
      {
          if (token == ']')
              continue;
          else if ((idx = piece_to_char().find(token)) != string::npos)
              add_to_hand(Piece(idx));
      }

  // 2. Active color
  ss >> token;
  sideToMove = (token == 'w' ? WHITE : BLACK);
  // Invert side to move for SFEN
  if (sfen)
      sideToMove = ~sideToMove;
  ss >> token;

  // 3-4. Skip parsing castling and en passant flags if not present
  st->epSquare = SQ_NONE;
  if (!isdigit(ss.peek()) && !sfen)
  {
      // 3. Castling availability. Compatible with 3 standards: Normal FEN standard,
      // Shredder-FEN that uses the letters of the columns on which the rooks began
      // the game instead of KQkq and also X-FEN standard that, in case of Chess960,
      // if an inner rook is associated with the castling right, the castling tag is
      // replaced by the file letter of the involved rook, as for the Shredder-FEN.
      while ((ss >> token) && !isspace(token))
      {
          Square rsq;
          Color c = islower(token) ? BLACK : WHITE;
          Piece rook = make_piece(c, ROOK);

          token = char(toupper(token));

          if (token == 'K')
              for (rsq = make_square(FILE_MAX, relative_rank(c, castling_rank(), max_rank())); piece_on(rsq) != rook; --rsq) {}

          else if (token == 'Q')
              for (rsq = make_square(FILE_A, relative_rank(c, castling_rank(), max_rank())); piece_on(rsq) != rook; ++rsq) {}

          else if (token >= 'A' && token <= 'A' + max_file())
              rsq = make_square(File(token - 'A'), relative_rank(c, castling_rank(), max_rank()));

          else
              continue;

          // Set gates (and skip castling rights)
          if (gating())
          {
              st->gatesBB[c] |= rsq;
              if (token == 'K' || token == 'Q')
                  st->gatesBB[c] |= count<KING>(c) ? square<KING>(c) : make_square(FILE_E, relative_rank(c, castling_rank(), max_rank()));
              // Do not set castling rights for gates unless there are no pieces in hand,
              // which means that the file is referring to a chess960 castling right.
              else if (count_in_hand(c, ALL_PIECES) || captures_to_hand())
                  continue;
          }

          set_castling_right(c, rsq);
      }

      // Set castling rights for 960 gating variants
      if (gating())
          for (Color c : {WHITE, BLACK})
              if ((gates(c) & pieces(KING)) && !castling_rights(c) && (count_in_hand(c, ALL_PIECES) || captures_to_hand()))
              {
                  Bitboard castling_rooks = gates(c) & pieces(ROOK);
                  while (castling_rooks)
                      set_castling_right(c, pop_lsb(&castling_rooks));
              }

      // counting limit
      if (counting_rule() && isdigit(ss.peek()))
          ss >> st->countingLimit;

      // 4. En passant square. Ignore if no pawn capture is possible
      else if (   ((ss >> col) && (col >= 'a' && col <= 'a' + max_file()))
               && ((ss >> row) && (row >= '1' && row <= '1' + max_rank())))
      {
          st->epSquare = make_square(File(col - 'a'), Rank(row - '1'));

          if (   !(attackers_to(st->epSquare) & pieces(sideToMove, PAWN))
              || !(pieces(~sideToMove, PAWN) & (st->epSquare + pawn_push(~sideToMove))))
              st->epSquare = SQ_NONE;
      }
  }

  // Check counter for nCheck
  ss >> std::skipws >> token >> std::noskipws;

  if (check_counting() && ss.peek() == '+')
  {
      st->checksRemaining[WHITE] = CheckCount(std::max(token - '0', 0));
      ss >> token >> token;
      st->checksRemaining[BLACK] = CheckCount(std::max(token - '0', 0));
  }
  else
      ss.putback(token);

  // 5-6. Halfmove clock and fullmove number
  if (sfen)
  {
      // Pieces in hand for SFEN
      while ((ss >> token) && !isspace(token))
      {
          if (token == '-')
              continue;
          else if ((idx = piece_to_char().find(token)) != string::npos)
              add_to_hand(Piece(idx));
      }
      // Move count is in ply for SFEN
      ss >> std::skipws >> gamePly;
      gamePly = std::max(gamePly - 1, 0);
  }
  else
  {
      ss >> std::skipws >> st->rule50 >> gamePly;

      // Convert from fullmove starting from 1 to gamePly starting from 0,
      // handle also common incorrect FEN with fullmove = 0.
      gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);
  }

  // counting rules
  if (st->countingLimit && st->rule50)
  {
      st->countingPly = st->rule50;
      st->rule50 = 0;
  }

  chess960 = isChess960 || v->chess960;
  thisThread = th;
  set_state(st);

  assert(pos_is_ok());

  return *this;
}


/// Position::set_castling_right() is a helper function used to set castling
/// rights given the corresponding color and the rook starting square.

void Position::set_castling_right(Color c, Square rfrom) {

  Square kfrom = count<KING>(c) ? square<KING>(c) : make_square(FILE_E, relative_rank(c, castling_rank(), max_rank()));
  CastlingRights cr = c & (kfrom < rfrom ? KING_SIDE: QUEEN_SIDE);

  st->castlingRights |= cr;
  castlingRightsMask[kfrom] |= cr;
  castlingRightsMask[rfrom] |= cr;
  castlingRookSquare[cr] = rfrom;

  Square kto = make_square(cr & KING_SIDE ? castling_kingside_file() : castling_queenside_file(),
                           relative_rank(c, castling_rank(), max_rank()));
  Square rto = kto + (cr & KING_SIDE ? WEST : EAST);

  castlingPath[cr] =   (between_bb(rfrom, rto) | between_bb(kfrom, kto) | rto | kto)
                    & ~(square_bb(kfrom) | rfrom);
}


/// Position::set_check_info() sets king attacks to detect if a move gives check

void Position::set_check_info(StateInfo* si) const {

  si->blockersForKing[WHITE] = slider_blockers(pieces(BLACK), count<KING>(WHITE) ? square<KING>(WHITE) : SQ_NONE, si->pinners[BLACK], BLACK);
  si->blockersForKing[BLACK] = slider_blockers(pieces(WHITE), count<KING>(BLACK) ? square<KING>(BLACK) : SQ_NONE, si->pinners[WHITE], WHITE);

  Square ksq = count<KING>(~sideToMove) ? square<KING>(~sideToMove) : SQ_NONE;

  // For unused piece types, the check squares are left uninitialized
  for (PieceType pt : piece_types())
      si->checkSquares[pt] = ksq != SQ_NONE ? attacks_from(~sideToMove, pt, ksq) : Bitboard(0);
  si->checkSquares[KING]   = 0;
  si->shak = si->checkersBB & (byTypeBB[KNIGHT] | byTypeBB[ROOK] | byTypeBB[BERS]);
}


/// Position::set_state() computes the hash keys of the position, and other
/// data that once computed is updated incrementally as moves are made.
/// The function is only used when a new position is set up, and to verify
/// the correctness of the StateInfo data when running in debug mode.

void Position::set_state(StateInfo* si) const {

  si->key = si->materialKey = 0;
  si->pawnKey = Zobrist::noPawns;
  si->nonPawnMaterial[WHITE] = si->nonPawnMaterial[BLACK] = VALUE_ZERO;
  si->checkersBB = count<KING>(sideToMove) ? attackers_to(square<KING>(sideToMove), ~sideToMove) : Bitboard(0);

  set_check_info(si);

  for (Bitboard b = pieces(); b; )
  {
      Square s = pop_lsb(&b);
      Piece pc = piece_on(s);
      si->key ^= Zobrist::psq[pc][s];

      if (type_of(pc) == PAWN)
          si->pawnKey ^= Zobrist::psq[pc][s];

      else if (type_of(pc) != KING)
          si->nonPawnMaterial[color_of(pc)] += PieceValue[MG][pc];
  }

  if (si->epSquare != SQ_NONE)
      si->key ^= Zobrist::enpassant[file_of(si->epSquare)];

  if (sideToMove == BLACK)
      si->key ^= Zobrist::side;

  si->key ^= Zobrist::castling[si->castlingRights];

  for (Color c : {WHITE, BLACK})
      for (PieceType pt = PAWN; pt <= KING; ++pt)
      {
          Piece pc = make_piece(c, pt);

          for (int cnt = 0; cnt < pieceCount[pc]; ++cnt)
              si->materialKey ^= Zobrist::psq[pc][cnt];

          if (piece_drops() || gating())
              si->key ^= Zobrist::inHand[pc][pieceCountInHand[c][pt]];
      }

  if (check_counting())
      for (Color c : {WHITE, BLACK})
          si->key ^= Zobrist::checks[c][si->checksRemaining[c]];
}


/// Position::set() is an overload to initialize the position object with
/// the given endgame code string like "KBPKN". It is mainly a helper to
/// get the material key out of an endgame code.

Position& Position::set(const string& code, Color c, StateInfo* si) {

  assert(code.length() > 0 && code.length() < 8);
  assert(code[0] == 'K');

  string sides[] = { code.substr(code.find('K', 1)),      // Weak
                     code.substr(0, code.find('K', 1)) }; // Strong

  std::transform(sides[c].begin(), sides[c].end(), sides[c].begin(), tolower);

  string n = std::to_string(FILE_NB);
  string fenStr =  n + "/" + sides[0] + char(FILE_NB - sides[0].length() + '0') + "/" + n + "/" + n + "/" + n + "/"
                 + n + "/" + sides[1] + char(FILE_NB - sides[1].length() + '0') + "/" + n + " w - - 0 10";

  return set(variants.find("fairy")->second, fenStr, false, si, nullptr);
}


/// Position::fen() returns a FEN representation of the position. In case of
/// Chess960 the Shredder-FEN notation is used. This is mainly a debugging function.

const string Position::fen() const {

  int emptyCnt;
  std::ostringstream ss;

  for (Rank r = max_rank(); r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= max_file(); ++f)
      {
          for (emptyCnt = 0; f <= max_file() && empty(make_square(f, r)); ++f)
              ++emptyCnt;

          if (emptyCnt)
              ss << emptyCnt;

          if (f <= max_file())
          {
              if (unpromoted_piece_on(make_square(f, r)))
                  // Promoted shogi pieces, e.g., +r for dragon
                  ss << "+" << piece_to_char()[unpromoted_piece_on(make_square(f, r))];
              else
              {
                  ss << piece_to_char()[piece_on(make_square(f, r))];

                  // Set promoted pieces
                  if (captures_to_hand() && is_promoted(make_square(f, r)))
                      ss << "~";
              }
          }
      }

      if (r > RANK_1)
          ss << '/';
  }

  // pieces in hand
  if (piece_drops() || gating())
  {
      ss << '[';
      for (Color c : {WHITE, BLACK})
          for (PieceType pt = KING; pt >= PAWN; --pt)
              ss << std::string(pieceCountInHand[c][pt], piece_to_char()[make_piece(c, pt)]);
      ss << ']';
  }

  ss << (sideToMove == WHITE ? " w " : " b ");

  if (can_castle(WHITE_OO))
      ss << (chess960 ? char('A' + file_of(castling_rook_square(WHITE_OO ))) : 'K');

  if (can_castle(WHITE_OOO))
      ss << (chess960 ? char('A' + file_of(castling_rook_square(WHITE_OOO))) : 'Q');

  if (gating() && gates(WHITE) && (count_in_hand(WHITE, ALL_PIECES) || captures_to_hand()))
      for (File f = FILE_A; f <= max_file(); ++f)
          if (gates(WHITE) & file_bb(f))
              ss << char('A' + f);

  if (can_castle(BLACK_OO))
      ss << (chess960 ? char('a' + file_of(castling_rook_square(BLACK_OO ))) : 'k');

  if (can_castle(BLACK_OOO))
      ss << (chess960 ? char('a' + file_of(castling_rook_square(BLACK_OOO))) : 'q');

  if (gating() && gates(BLACK) && (count_in_hand(BLACK, ALL_PIECES) || captures_to_hand()))
      for (File f = FILE_A; f <= max_file(); ++f)
          if (gates(BLACK) & file_bb(f))
              ss << char('a' + f);

  if (!can_castle(ANY_CASTLING) && !(gating() && (gates(WHITE) | gates(BLACK))))
      ss << '-';

  // Counting limit or ep-square
  if (st->countingLimit)
      ss << " " << st->countingLimit << " ";
  else
      ss << (ep_square() == SQ_NONE ? " - " : " " + UCI::square(*this, ep_square()) + " ");

  // Check count
  if (check_counting())
      ss << st->checksRemaining[WHITE] << "+" << st->checksRemaining[BLACK] << " ";

  // Counting ply or 50-move rule counter
  if (st->countingLimit)
      ss << st->countingPly;
  else
      ss << st->rule50;

  ss << " " << 1 + (gamePly - (sideToMove == BLACK)) / 2;

  return ss.str();
}


/// Position::slider_blockers() returns a bitboard of all the pieces (both colors)
/// that are blocking attacks on the square 's' from 'sliders'. A piece blocks a
/// slider if removing that piece from the board would result in a position where
/// square 's' is attacked. For example, a king-attack blocking piece can be either
/// a pinned or a discovered check piece, according if its color is the opposite
/// or the same of the color of the slider.

Bitboard Position::slider_blockers(Bitboard sliders, Square s, Bitboard& pinners, Color c) const {

  Bitboard blockers = 0;
  pinners = 0;

  if (s == SQ_NONE || !sliders)
      return blockers;

  // Snipers are sliders that attack 's' when a piece and other snipers are removed
  Bitboard snipers = 0;

  for (PieceType pt : piece_types())
  {
      Bitboard b = sliders & (PseudoAttacks[~c][pt][s] ^ LeaperAttacks[~c][pt][s]) & pieces(c, pt);
      if (b)
          snipers |= b & ~attacks_from(~c, pt, s);
  }
  Bitboard occupancy = pieces() ^ snipers;

  while (snipers)
  {
    Square sniperSq = pop_lsb(&snipers);
    Bitboard b = between_bb(s, sniperSq) & occupancy;

    if (b && !more_than_one(b))
    {
        blockers |= b;
        if (b & pieces(color_of(piece_on(s))))
            pinners |= sniperSq;
    }
  }
  return blockers;
}


/// Position::attackers_to() computes a bitboard of all pieces which attack a
/// given square. Slider attacks use the occupied bitboard to indicate occupancy.

Bitboard Position::attackers_to(Square s, Bitboard occupied, Color c) const {

  Bitboard b = 0;
  for (PieceType pt : piece_types())
      b |= attacks_bb(~c, pt, s, occupied) & pieces(c, pt);
  return b;
}


Bitboard Position::attackers_to(Square s, Bitboard occupied) const {
  return attackers_to(s, occupied, WHITE) | attackers_to(s, occupied, BLACK);
}


/// Position::legal() tests whether a pseudo-legal move is legal

bool Position::legal(Move m) const {

  assert(is_ok(m));
  assert(type_of(m) != DROP || piece_drops());

  Color us = sideToMove;
  Square from = from_sq(m);
  Square to = to_sq(m);

  assert(color_of(moved_piece(m)) == us);
  assert(!count<KING>(us) || piece_on(square<KING>(us)) == make_piece(us, KING));

  // Illegal moves to squares outside of board
  if (!(board_bb() & to))
      return false;

  // Illegal checks
  if ((!checking_permitted() || (sittuyin_promotion() && type_of(m) == PROMOTION)) && gives_check(m))
      return false;

  // Illegal quiet moves
  if (must_capture() && !capture(m))
  {
      if (checkers())
      {
          for (const auto& mevasion : MoveList<EVASIONS>(*this))
              if (capture(mevasion) && legal(mevasion))
                  return false;
      }
      else
      {
          for (const auto& mcap : MoveList<CAPTURES>(*this))
              if (capture(mcap) && legal(mcap))
                  return false;
      }
  }

  // Illegal non-drop moves
  if (must_drop() && type_of(m) != DROP && count_in_hand(us, ALL_PIECES))
  {
      if (checkers())
      {
          for (const auto& mevasion : MoveList<EVASIONS>(*this))
              if (type_of(mevasion) == DROP && legal(mevasion))
                  return false;
      }
      else
      {
          for (const auto& mquiet : MoveList<QUIETS>(*this))
              if (type_of(mquiet) == DROP && legal(mquiet))
                  return false;
      }
  }

  // Illegal drop move
  if (drop_opposite_colored_bishop() && type_of(m) == DROP)
  {
      if (type_of(moved_piece(m)) != BISHOP)
      {
          Bitboard remaining = drop_region(us) & ~pieces() & ~SquareBB[to] & board_bb();
          // Are enough squares available to drop bishops on opposite colors?
          if (  (!( DarkSquares & pieces(us, BISHOP)) && ( DarkSquares & remaining))
              + (!(~DarkSquares & pieces(us, BISHOP)) && (~DarkSquares & remaining)) < count_in_hand(us, BISHOP))
              return false;
      }
      else
          // Drop resulting in same-colored bishops
          if ((DarkSquares & to ? DarkSquares : ~DarkSquares) & pieces(us, BISHOP))
              return false;
  }

  // No legal moves from target square
  if (immobility_illegal() && (type_of(m) == DROP || type_of(m) == NORMAL) && !(moves_bb(us, type_of(moved_piece(m)), to, 0) & board_bb()))
      return false;

  // En passant captures are a tricky special case. Because they are rather
  // uncommon, we do it simply by testing whether the king is attacked after
  // the move is made.
  if (type_of(m) == ENPASSANT)
  {
      Square ksq = count<KING>(us) ? square<KING>(us) : SQ_NONE;
      Square capsq = to - pawn_push(us);
      Bitboard occupied = (pieces() ^ from ^ capsq) | to;

      assert(to == ep_square());
      assert(moved_piece(m) == make_piece(us, PAWN));
      assert(piece_on(capsq) == make_piece(~us, PAWN));
      assert(piece_on(to) == NO_PIECE);

      return !count<KING>(us) || !(attackers_to(ksq, occupied, ~us) & occupied);
  }

  // Castling moves generation does not check if the castling path is clear of
  // enemy attacks, it is delayed at a later time: now!
  if (type_of(m) == CASTLING)
  {
      // Non-royal pieces can not be impeded from castling
      if (type_of(piece_on(from)) != KING)
          return true;

      // After castling, the rook and king final positions are the same in
      // Chess960 as they would be in standard chess.
      to = make_square(to > from ? castling_kingside_file() : castling_queenside_file(), relative_rank(us, castling_rank(), max_rank()));
      Direction step = to > from ? WEST : EAST;

      for (Square s = to; s != from; s += step)
          if (attackers_to(s, ~us))
              return false;

      // In case of Chess960, verify that when moving the castling rook we do
      // not discover some hidden checker.
      // For instance an enemy queen in SQ_A1 when castling rook is in SQ_B1.
      return   !chess960
            || !(attackers_to(to, pieces() ^ to_sq(m), ~us));
  }

  // If the moving piece is a king, check whether the destination
  // square is attacked by the opponent. Castling moves are checked
  // for legality during move generation.
  if (type_of(moved_piece(m)) == KING)
      return type_of(m) == CASTLING || !attackers_to(to, ~us);

  // A non-king move is legal if the king is not under attack after the move.
  return !count<KING>(us) || !(attackers_to(square<KING>(us), (type_of(m) != DROP ? pieces() ^ from : pieces()) | to, ~us) & ~SquareBB[to]);
}


/// Position::pseudo_legal() takes a random move and tests whether the move is
/// pseudo legal. It is used to validate moves from TT that can be corrupted
/// due to SMP concurrent access or hash position key aliasing.

bool Position::pseudo_legal(const Move m) const {

  Color us = sideToMove;
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = moved_piece(m);

  // Use a fast check for piece drops
  if (type_of(m) == DROP)
      return   piece_drops()
            && count_in_hand(us, in_hand_piece_type(m))
            && (drop_region(us, type_of(pc)) & ~pieces() & to)
            && (   type_of(pc) == in_hand_piece_type(m)
                || (drop_promoted() && type_of(pc) == promoted_piece_type(in_hand_piece_type(m))));

  // Use a slower but simpler function for uncommon cases
  if (type_of(m) != NORMAL || is_gating(m))
      return MoveList<LEGAL>(*this).contains(m);

  // Handle the case where a mandatory piece promotion/demotion is not taken
  if (    mandatory_piece_promotion()
      && (is_promoted(from) ? piece_demotion() : promoted_piece_type(type_of(pc)) != NO_PIECE_TYPE)
      && (promotion_zone_bb(us, promotion_rank(), max_rank()) & (SquareBB[from] | to))
      && (!piece_promotion_on_capture() || capture(m)))
      return false;

  // Is not a promotion, so promotion piece must be empty
  if (promotion_type(m) != NO_PIECE_TYPE)
      return false;

  // If the 'from' square is not occupied by a piece belonging to the side to
  // move, the move is obviously not legal.
  if (pc == NO_PIECE || color_of(pc) != us)
      return false;

  // The destination square cannot be occupied by a friendly piece
  if (pieces(us) & to)
      return false;

  // Handle the special case of a pawn move
  if (type_of(pc) == PAWN)
  {
      // We have already handled promotion moves, so destination
      // cannot be on the 8th/1st rank.
      if (mandatory_pawn_promotion() && (promotion_zone_bb(us, promotion_rank(), max_rank()) & to))
          return false;

      if (   !(attacks_from<PAWN>(us, from) & pieces(~us) & to) // Not a capture
          && !((from + pawn_push(us) == to) && empty(to))       // Not a single push
          && !(   (from + 2 * pawn_push(us) == to)              // Not a double push
               && (rank_of(from) == relative_rank(us, double_step_rank(), max_rank())
                   || (first_rank_double_steps() && rank_of(from) == relative_rank(us, RANK_1, max_rank())))
               && empty(to)
               && empty(to - pawn_push(us))
               && double_step_enabled()))
          return false;
  }
  else if (!((capture(m) ? attacks_from(us, type_of(pc), from) : moves_from(us, type_of(pc), from)) & to))
      return false;

  // Evasions generator already takes care to avoid some kind of illegal moves
  // and legal() relies on this. We therefore have to take care that the same
  // kind of moves are filtered out here.
  if (checkers())
  {
      if (type_of(pc) != KING)
      {
          // Double check? In this case a king move is required
          if (more_than_one(checkers()))
              return false;

          // Our move must be a blocking evasion or a capture of the checking piece
          Square checksq = lsb(checkers());
          if (  !((between_bb(checksq, square<KING>(us)) | checkers()) & to)
              || (LeaperAttacks[~us][type_of(piece_on(checksq))][checksq] & square<KING>(us)))
              return false;
      }
      // In case of king moves under check we have to remove king so as to catch
      // invalid moves like b1a1 when opposite queen is on c1.
      else if (attackers_to(to, pieces() ^ from, ~us))
          return false;
  }

  return true;
}


/// Position::gives_check() tests whether a pseudo-legal move gives a check

bool Position::gives_check(Move m) const {

  assert(is_ok(m));
  assert(color_of(moved_piece(m)) == sideToMove);

  Square from = from_sq(m);
  Square to = to_sq(m);

  // No check possible without king
  if (!count<KING>(~sideToMove))
      return false;

  // Is there a direct check?
  if (type_of(m) != PROMOTION && type_of(m) != PIECE_PROMOTION && type_of(m) != PIECE_DEMOTION && (st->checkSquares[type_of(moved_piece(m))] & to))
      return true;

  // Is there a discovered check?
  if (   type_of(m) != DROP
      && ((st->blockersForKing[~sideToMove] & from) || pieces(sideToMove, CANNON))
      && attackers_to(square<KING>(~sideToMove), (pieces() ^ from) | to, sideToMove))
      return true;

  // Is there a check by gated pieces?
  if (    is_gating(m)
      && attacks_bb(sideToMove, gating_type(m), gating_square(m), (pieces() ^ from) | to) & square<KING>(~sideToMove))
      return true;

  switch (type_of(m))
  {
  case NORMAL:
  case DROP:
      return false;

  case PROMOTION:
      return attacks_bb(sideToMove, promotion_type(m), to, pieces() ^ from) & square<KING>(~sideToMove);

  case PIECE_PROMOTION:
      return attacks_bb(sideToMove, promoted_piece_type(type_of(moved_piece(m))), to, pieces() ^ from) & square<KING>(~sideToMove);

  case PIECE_DEMOTION:
      return attacks_bb(sideToMove, type_of(unpromoted_piece_on(from)), to, pieces() ^ from) & square<KING>(~sideToMove);

  // En passant capture with check? We have already handled the case
  // of direct checks and ordinary discovered check, so the only case we
  // need to handle is the unusual case of a discovered check through
  // the captured pawn.
  case ENPASSANT:
  {
      Square capsq = make_square(file_of(to), rank_of(from));
      Bitboard b = (pieces() ^ from ^ capsq) | to;

      return attackers_to(square<KING>(~sideToMove), b) & pieces(sideToMove) & b;
  }
  case CASTLING:
  {
      Square kfrom = from;
      Square rfrom = to; // Castling is encoded as 'King captures the rook'
      Square kto = make_square(rfrom > kfrom ? castling_kingside_file() : castling_queenside_file(),
                              relative_rank(sideToMove, castling_rank(), max_rank()));
      Square rto = kto + (rfrom > kfrom ? WEST : EAST);

      return   (PseudoAttacks[sideToMove][ROOK][rto] & square<KING>(~sideToMove))
            && (attacks_bb<ROOK>(rto, (pieces() ^ kfrom ^ rfrom) | rto | kto) & square<KING>(~sideToMove));
  }
  default:
      assert(false);
      return false;
  }
}


/// Position::do_move() makes a move, and saves all information necessary
/// to a StateInfo object. The move is assumed to be legal. Pseudo-legal
/// moves should be filtered out before this function is called.

void Position::do_move(Move m, StateInfo& newSt, bool givesCheck) {

  assert(is_ok(m));
  assert(&newSt != st);

  thisThread->nodes.fetch_add(1, std::memory_order_relaxed);
  Key k = st->key ^ Zobrist::side;

  // Copy some fields of the old state to our new StateInfo object except the
  // ones which are going to be recalculated from scratch anyway and then switch
  // our state pointer to point to the new (ready to be updated) state.
  std::memcpy(&newSt, st, offsetof(StateInfo, key));
  newSt.previous = st;
  st = &newSt;

  // Increment ply counters. In particular, rule50 will be reset to zero later on
  // in case of a capture or a pawn move.
  ++gamePly;
  ++st->rule50;
  ++st->pliesFromNull;
  if (st->countingLimit)
      ++st->countingPly;

  Color us = sideToMove;
  Color them = ~us;
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = moved_piece(m);
  Piece captured = type_of(m) == ENPASSANT ? make_piece(them, PAWN) : piece_on(to);
  if (to == from)
  {
      assert(type_of(m) == PROMOTION && sittuyin_promotion());
      captured = NO_PIECE;
  }
  Piece unpromotedCaptured = unpromoted_piece_on(to);

  assert(color_of(pc) == us);
  assert(captured == NO_PIECE || color_of(captured) == (type_of(m) != CASTLING ? them : us));
  assert(type_of(captured) != KING);

  if (check_counting() && givesCheck)
      k ^= Zobrist::checks[us][st->checksRemaining[us]] ^ Zobrist::checks[us][--(st->checksRemaining[us])];

  if (type_of(m) == CASTLING)
  {
      assert(type_of(pc) != NO_PIECE_TYPE);
      assert(captured == make_piece(us, ROOK));

      Square rfrom, rto;
      do_castling<true>(us, from, to, rfrom, rto);

      k ^= Zobrist::psq[captured][rfrom] ^ Zobrist::psq[captured][rto];
      captured = NO_PIECE;
  }

  if (captured)
  {
      Square capsq = to;

      // If the captured piece is a pawn, update pawn hash key, otherwise
      // update non-pawn material.
      if (type_of(captured) == PAWN)
      {
          if (type_of(m) == ENPASSANT)
          {
              capsq -= pawn_push(us);

              assert(pc == make_piece(us, PAWN));
              assert(to == st->epSquare);
              assert(relative_rank(~us, to, max_rank()) == Rank(double_step_rank() + 1));
              assert(piece_on(to) == NO_PIECE);
              assert(piece_on(capsq) == make_piece(them, PAWN));

              board[capsq] = NO_PIECE; // Not done by remove_piece()
          }

          st->pawnKey ^= Zobrist::psq[captured][capsq];
      }
      else
          st->nonPawnMaterial[them] -= PieceValue[MG][captured];

      // Update board and piece lists
      remove_piece(captured, capsq);
      if (captures_to_hand())
      {
          st->capturedpromoted = is_promoted(to);
          Piece pieceToHand =  !is_promoted(to)   ? ~captured
                             : unpromotedCaptured ? ~unpromotedCaptured
                                                  : make_piece(~color_of(captured), PAWN);
          add_to_hand(pieceToHand);
          k ^=  Zobrist::inHand[pieceToHand][pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)] - 1]
              ^ Zobrist::inHand[pieceToHand][pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)]];
          promotedPieces -= to;
      }
      unpromotedBoard[to] = NO_PIECE;

      // Update material hash key and prefetch access to materialTable
      k ^= Zobrist::psq[captured][capsq];
      st->materialKey ^= Zobrist::psq[captured][pieceCount[captured]];
      prefetch(thisThread->materialTable[st->materialKey]);

      // Reset rule 50 counter
      st->rule50 = 0;
  }

  // Update hash key
  if (type_of(m) == DROP)
  {
      Piece pc_hand = make_piece(us, in_hand_piece_type(m));
      k ^=  Zobrist::psq[pc][to]
          ^ Zobrist::inHand[pc_hand][pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)] - 1]
          ^ Zobrist::inHand[pc_hand][pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)]];
  }
  else
      k ^= Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];

  // Reset en passant square
  if (st->epSquare != SQ_NONE)
  {
      k ^= Zobrist::enpassant[file_of(st->epSquare)];
      st->epSquare = SQ_NONE;
  }

  // Update castling rights if needed
  if (type_of(m) != DROP && st->castlingRights && (castlingRightsMask[from] | castlingRightsMask[to]))
  {
      int cr = castlingRightsMask[from] | castlingRightsMask[to];
      k ^= Zobrist::castling[st->castlingRights & cr];
      st->castlingRights &= ~cr;
  }

  // Move the piece. The tricky Chess960 castling is handled earlier
  if (type_of(m) == DROP)
  {
      drop_piece(make_piece(us, in_hand_piece_type(m)), pc, to);
      st->materialKey ^= Zobrist::psq[pc][pieceCount[pc]-1];
      if (type_of(pc) != PAWN)
          st->nonPawnMaterial[us] += PieceValue[MG][pc];
      // Set castling rights for dropped king or rook
      if (castling_dropped_piece() && relative_rank(us, to, max_rank()) == castling_rank())
      {
          if (type_of(pc) == KING && file_of(to) == FILE_E)
          {
              Bitboard castling_rooks =  pieces(us, ROOK)
                                       & rank_bb(relative_rank(us, castling_rank(), max_rank()))
                                       & (file_bb(FILE_A) | file_bb(max_file()));
              while (castling_rooks)
                  set_castling_right(us, pop_lsb(&castling_rooks));
          }
          else if (type_of(pc) == ROOK)
          {
              if (   (file_of(to) == FILE_A || file_of(to) == max_file())
                  && piece_on(make_square(FILE_E, relative_rank(us, castling_rank(), max_rank()))) == make_piece(us, KING))
                  set_castling_right(us, to);
          }
      }
  }
  else if (type_of(m) != CASTLING)
      move_piece(pc, from, to);

  // If the moving piece is a pawn do some special extra work
  if (type_of(pc) == PAWN)
  {
      // Set en-passant square if the moved pawn can be captured
      if (   std::abs(int(to) - int(from)) == 2 * NORTH
          && relative_rank(us, rank_of(from), max_rank()) == double_step_rank()
          && (attacks_from<PAWN>(us, to - pawn_push(us)) & pieces(them, PAWN)))
      {
          st->epSquare = to - pawn_push(us);
          k ^= Zobrist::enpassant[file_of(st->epSquare)];
      }

      else if (type_of(m) == PROMOTION)
      {
          Piece promotion = make_piece(us, promotion_type(m));

          assert(relative_rank(us, to, max_rank()) >= promotion_rank() || sittuyin_promotion());
          assert(type_of(promotion) >= KNIGHT && type_of(promotion) < KING);

          remove_piece(pc, to);
          put_piece(promotion, to);
          if (captures_to_hand() && !drop_loop())
              promotedPieces = promotedPieces | to;

          // Update hash keys
          k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[promotion][to];
          st->pawnKey ^= Zobrist::psq[pc][to];
          st->materialKey ^=  Zobrist::psq[promotion][pieceCount[promotion]-1]
                            ^ Zobrist::psq[pc][pieceCount[pc]];

          // Update material
          st->nonPawnMaterial[us] += PieceValue[MG][promotion];
      }

      // Update pawn hash key and prefetch access to pawnsTable
      if (type_of(m) == DROP)
          st->pawnKey ^= Zobrist::psq[pc][to];
      else
          st->pawnKey ^= Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];

      // Reset rule 50 draw counter
      st->rule50 = 0;
  }
  else if (type_of(m) == PIECE_PROMOTION)
  {
      Piece promotion = make_piece(us, promoted_piece_type(type_of(pc)));

      remove_piece(pc, to);
      put_piece(promotion, to);
      promotedPieces |= to;
      unpromotedBoard[to] = pc;

      // Update hash keys
      k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[promotion][to];
      st->materialKey ^=  Zobrist::psq[promotion][pieceCount[promotion]-1]
                        ^ Zobrist::psq[pc][pieceCount[pc]];

      // Update material
      st->nonPawnMaterial[us] += PieceValue[MG][promotion] - PieceValue[MG][pc];
  }
  else if (type_of(m) == PIECE_DEMOTION)
  {
      Piece demotion = unpromoted_piece_on(from);

      remove_piece(pc, to);
      put_piece(demotion, to);
      promotedPieces ^= from;
      unpromotedBoard[from] = NO_PIECE;

      // Update hash keys
      k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[demotion][to];
      st->materialKey ^=  Zobrist::psq[demotion][pieceCount[demotion]-1]
                        ^ Zobrist::psq[pc][pieceCount[pc]];

      // Update material
      st->nonPawnMaterial[us] += PieceValue[MG][demotion] - PieceValue[MG][pc];
  }

  // Set capture piece
  st->capturedPiece = captured;
  st->unpromotedCapturedPiece = captured ? unpromotedCaptured : NO_PIECE;
  if (captures_to_hand() && !captured)
      st->capturedpromoted = false;

  // Add gating piece
  if (is_gating(m))
  {
      Square gate = gating_square(m);
      Piece gating_piece = make_piece(us, gating_type(m));
      put_piece(gating_piece, gate);
      remove_from_hand(gating_piece);
      st->gatesBB[us] ^= gate;
      k ^= Zobrist::psq[gating_piece][gate];
      st->materialKey ^= Zobrist::psq[gating_piece][pieceCount[gating_piece]];
      st->nonPawnMaterial[us] += PieceValue[MG][gating_piece];
  }

  // Remove gates
  if (gating())
  {
      if (is_ok(from) && (gates(us) & from))
          st->gatesBB[us] ^= from;
      if (type_of(m) == CASTLING && (gates(us) & to_sq(m)))
          st->gatesBB[us] ^= to_sq(m);
      if (gates(them) & to)
          st->gatesBB[them] ^= to;
      if (!count_in_hand(us, ALL_PIECES) && !captures_to_hand())
          st->gatesBB[us] = 0;
  }

  // Update the key with the final value
  st->key = k;
  // Calculate checkers bitboard (if move gives check)
  st->checkersBB = givesCheck ? attackers_to(square<KING>(them), us) & pieces(us) : Bitboard(0);

  // Update information about promoted pieces
  if (type_of(m) != DROP && is_promoted(from))
      promotedPieces = (promotedPieces - from) | to;
  else if (type_of(m) == DROP && in_hand_piece_type(m) != dropped_piece_type(m))
      promotedPieces = promotedPieces | to;

  if (type_of(m) != DROP && unpromoted_piece_on(from))
  {
      unpromotedBoard[to] = unpromotedBoard[from];
      unpromotedBoard[from] = NO_PIECE;
  }
  else if (type_of(m) == DROP && in_hand_piece_type(m) != dropped_piece_type(m))
      unpromotedBoard[to] = make_piece(us, in_hand_piece_type(m));

  sideToMove = ~sideToMove;

  if (   counting_rule()
      && (  ((!st->countingLimit || captured) && count<ALL_PIECES>(sideToMove) == 1)
          || (!st->countingLimit && !count<PAWN>())))
  {
      st->countingLimit = 2 * counting_limit();
      st->countingPly = st->countingLimit && count<ALL_PIECES>(sideToMove) == 1 ? 2 * count<ALL_PIECES>() : 0;
  }

  // Update king attacks used for fast check detection
  set_check_info(st);

  // Calculate the repetition info. It is the ply distance from the previous
  // occurrence of the same position, negative in the 3-fold case, or zero
  // if the position was not repeated.
  st->repetition = 0;
  int end = captures_to_hand() ? st->pliesFromNull : std::min(st->rule50, st->pliesFromNull);
  if (end >= 4)
  {
      StateInfo* stp = st->previous->previous;
      for (int i = 4; i <= end; i += 2)
      {
          stp = stp->previous->previous;
          if (stp->key == st->key)
          {
              st->repetition = stp->repetition ? -i : i;
              break;
          }
      }
  }

  assert(pos_is_ok());
}


/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.

void Position::undo_move(Move m) {

  assert(is_ok(m));

  sideToMove = ~sideToMove;

  Color us = sideToMove;
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = piece_on(to);

  assert(type_of(m) == DROP || empty(from) || type_of(m) == CASTLING || is_gating(m) || (type_of(m) == PROMOTION && sittuyin_promotion()));
  assert(type_of(st->capturedPiece) != KING);

  // Remove gated piece
  if (is_gating(m))
  {
      Piece gating_piece = make_piece(us, gating_type(m));
      remove_piece(gating_piece, gating_square(m));
      add_to_hand(gating_piece);
      st->gatesBB[us] |= gating_square(m);
  }

  if (type_of(m) == PROMOTION)
  {
      assert(relative_rank(us, to, max_rank()) >= promotion_rank() || sittuyin_promotion());
      assert(type_of(pc) == promotion_type(m));
      assert(type_of(pc) >= KNIGHT && type_of(pc) < KING);

      remove_piece(pc, to);
      pc = make_piece(us, PAWN);
      put_piece(pc, to);
      if (captures_to_hand() && !drop_loop())
          promotedPieces -= to;
  }
  else if (type_of(m) == PIECE_PROMOTION)
  {
      remove_piece(pc, to);
      pc = unpromoted_piece_on(to);
      put_piece(pc, to);
      unpromotedBoard[to] = NO_PIECE;
      promotedPieces -= to;
  }
  else if (type_of(m) == PIECE_DEMOTION)
  {
      remove_piece(pc, to);
      unpromotedBoard[from] = pc;
      pc = make_piece(us, promoted_piece_type(type_of(pc)));
      put_piece(pc, to);
      promotedPieces |= from;
  }

  if (type_of(m) == CASTLING)
  {
      Square rfrom, rto;
      do_castling<false>(us, from, to, rfrom, rto);
  }
  else
  {
      if (type_of(m) == DROP)
          undrop_piece(make_piece(us, in_hand_piece_type(m)), pc, to); // Remove the dropped piece
      else
          move_piece(pc, to, from); // Put the piece back at the source square
      if (captures_to_hand() && !drop_loop() && is_promoted(to))
      {
          promotedPieces = (promotedPieces - to);
          if (type_of(m) != DROP)
              promotedPieces |= from;
      }
      if (unpromoted_piece_on(to))
      {
          if (type_of(m) != DROP)
              unpromotedBoard[from] = unpromotedBoard[to];
          unpromotedBoard[to] = NO_PIECE;
      }

      if (st->capturedPiece)
      {
          Square capsq = to;

          if (type_of(m) == ENPASSANT)
          {
              capsq -= pawn_push(us);

              assert(type_of(pc) == PAWN);
              assert(to == st->previous->epSquare);
              assert(relative_rank(~us, to, max_rank()) == Rank(double_step_rank() + 1));
              assert(piece_on(capsq) == NO_PIECE);
              assert(st->capturedPiece == make_piece(~us, PAWN));
          }

          put_piece(st->capturedPiece, capsq); // Restore the captured piece
          if (captures_to_hand())
          {
              remove_from_hand(!drop_loop() && st->capturedpromoted ? (st->unpromotedCapturedPiece ? ~st->unpromotedCapturedPiece
                                                                                                   : make_piece(~color_of(st->capturedPiece), PAWN))
                                                                    : ~st->capturedPiece);
              if (!drop_loop() && st->capturedpromoted)
                  promotedPieces |= to;
          }
          if (st->unpromotedCapturedPiece)
              unpromotedBoard[to] = st->unpromotedCapturedPiece;
      }
  }

  // Finally point our state pointer back to the previous state
  st = st->previous;
  --gamePly;

  assert(pos_is_ok());
}


/// Position::do_castling() is a helper used to do/undo a castling move. This
/// is a bit tricky in Chess960 where from/to squares can overlap.
template<bool Do>
void Position::do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto) {

  bool kingSide = to > from;
  rfrom = to; // Castling is encoded as "king captures friendly rook"
  to = make_square(kingSide ? castling_kingside_file() : castling_queenside_file(),
                   relative_rank(us, castling_rank(), max_rank()));
  rto = to + (kingSide ? WEST : EAST);

  // Remove both pieces first since squares could overlap in Chess960
  Piece castling_piece = piece_on(Do ? from : to);
  remove_piece(castling_piece, Do ? from : to);
  remove_piece(make_piece(us, ROOK), Do ? rfrom : rto);
  board[Do ? from : to] = board[Do ? rfrom : rto] = NO_PIECE; // Since remove_piece doesn't do it for us
  put_piece(castling_piece, Do ? to : from);
  put_piece(make_piece(us, ROOK), Do ? rto : rfrom);
}


/// Position::do(undo)_null_move() is used to do(undo) a "null move": It flips
/// the side to move without executing any move on the board.

void Position::do_null_move(StateInfo& newSt) {

  assert(!checkers());
  assert(&newSt != st);

  std::memcpy(&newSt, st, sizeof(StateInfo));
  newSt.previous = st;
  st = &newSt;

  if (st->epSquare != SQ_NONE)
  {
      st->key ^= Zobrist::enpassant[file_of(st->epSquare)];
      st->epSquare = SQ_NONE;
  }

  st->key ^= Zobrist::side;
  prefetch(TT.first_entry(st->key));

  ++st->rule50;
  st->pliesFromNull = 0;

  sideToMove = ~sideToMove;

  set_check_info(st);

  st->repetition = 0;

  assert(pos_is_ok());
}

void Position::undo_null_move() {

  assert(!checkers());

  st = st->previous;
  sideToMove = ~sideToMove;
}


/// Position::key_after() computes the new hash key after the given move. Needed
/// for speculative prefetch. It doesn't recognize special moves like castling,
/// en-passant and promotions.

Key Position::key_after(Move m) const {

  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = moved_piece(m);
  Piece captured = piece_on(to);
  Key k = st->key ^ Zobrist::side;

  if (captured)
  {
      k ^= Zobrist::psq[captured][to];
      if (captures_to_hand())
      {
          Piece removeFromHand = !drop_loop() && is_promoted(to) ? make_piece(~color_of(captured), PAWN) : ~captured;
          k ^= Zobrist::inHand[removeFromHand][pieceCountInHand[color_of(removeFromHand)][type_of(removeFromHand)] + 1]
              ^ Zobrist::inHand[removeFromHand][pieceCountInHand[color_of(removeFromHand)][type_of(removeFromHand)]];
      }
  }
  if (type_of(m) == DROP)
  {
      Piece pc_hand = make_piece(sideToMove, in_hand_piece_type(m));
      return k ^ Zobrist::psq[pc][to] ^ Zobrist::inHand[pc_hand][pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)]]
            ^ Zobrist::inHand[pc_hand][pieceCountInHand[color_of(pc_hand)][type_of(pc_hand)] - 1];
  }

  return k ^ Zobrist::psq[pc][to] ^ Zobrist::psq[pc][from];
}


/// Position::see_ge (Static Exchange Evaluation Greater or Equal) tests if the
/// SEE value of move is greater or equal to the given threshold. We'll use an
/// algorithm similar to alpha-beta pruning with a null window.

bool Position::see_ge(Move m, Value threshold) const {

  assert(is_ok(m));

  // Only deal with normal moves, assume others pass a simple see
  if (type_of(m) != NORMAL && type_of(m) != DROP && type_of(m) != PIECE_PROMOTION)
      return VALUE_ZERO >= threshold;

  Bitboard stmAttackers;
  Square from = from_sq(m), to = to_sq(m);
  PieceType nextVictim = type_of(m) == DROP ? dropped_piece_type(m) : type_of(piece_on(from));
  Color us = type_of(m) == DROP ? sideToMove : color_of(piece_on(from));
  Color stm = ~us; // First consider opponent's move
  Value balance;   // Values of the pieces taken by us minus opponent's ones


  // nCheck
  if (check_counting() && color_of(moved_piece(m)) == sideToMove && gives_check(m))
      return true;

  // Extinction
  if (   extinction_value() != VALUE_NONE
      && piece_on(to)
      && (   (   extinction_piece_types().find(type_of(piece_on(to))) != extinction_piece_types().end()
              && pieceCount[piece_on(to)] == 1)
          || (   extinction_piece_types().find(ALL_PIECES) != extinction_piece_types().end()
              && count<ALL_PIECES>(~sideToMove) == 1)))
      return extinction_value() < VALUE_ZERO;

  // The opponent may be able to recapture so this is the best result
  // we can hope for.
  balance = PieceValue[MG][piece_on(to)] - threshold;

  if (balance < VALUE_ZERO)
      return false;

  // Now assume the worst possible result: that the opponent can
  // capture our piece for free.
  balance -= PieceValue[MG][nextVictim];

  // If it is enough (like in PxQ) then return immediately. Note that
  // in case nextVictim == KING we always return here, this is ok
  // if the given move is legal.
  if (balance >= VALUE_ZERO)
      return true;

  // Find all attackers to the destination square, with the moving piece
  // removed, but possibly an X-ray attacker added behind it.
  Bitboard occupied = type_of(m) == DROP ? pieces() ^ to : pieces() ^ from ^ to;
  Bitboard attackers = attackers_to(to, occupied) & occupied;

  while (true)
  {
      stmAttackers = attackers & pieces(stm);

      // Don't allow pinned pieces to attack (except the king) as long as
      // any pinners are on their original square.
      if (st->pinners[~stm] & occupied)
          stmAttackers &= ~st->blockersForKing[stm];

      // If stm has no more attackers then give up: stm loses
      if (!stmAttackers)
          break;

      // Locate and remove the next least valuable attacker, and add to
      // the bitboard 'attackers' the possibly X-ray attackers behind it.
      nextVictim = min_attacker<PAWN>(byTypeBB, to, stmAttackers, occupied, attackers);

      stm = ~stm; // Switch side to move

      // Negamax the balance with alpha = balance, beta = balance+1 and
      // add nextVictim's value.
      //
      //      (balance, balance+1) -> (-balance-1, -balance)
      //
      assert(balance < VALUE_ZERO);

      balance = -balance - 1 - PieceValue[MG][nextVictim];

      // If balance is still non-negative after giving away nextVictim then we
      // win. The only thing to be careful about it is that we should revert
      // stm if we captured with the king when the opponent still has attackers.
      if (balance >= VALUE_ZERO)
      {
          if (nextVictim == KING && (attackers & pieces(stm)))
              stm = ~stm;
          break;
      }
      assert(nextVictim != KING);
  }
  return us != stm; // We break the above loop when stm loses
}


/// Position::is_optinal_game_end() tests whether the position may end the game by
/// 50-move rule, by repetition, or a variant rule that allows a player to claim a game result.

bool Position::is_optional_game_end(Value& result, int ply) const {

  // n-move rule
  if (n_move_rule() && st->rule50 > (2 * n_move_rule() - 1) && (!checkers() || MoveList<LEGAL>(*this).size()))
  {
      result = VALUE_DRAW;
      return true;
  }

  // n-fold repetition
  if (n_fold_rule())
  {
      int end = captures_to_hand() ? st->pliesFromNull : std::min(st->rule50, st->pliesFromNull);

      if (end >= 4)
      {
          StateInfo* stp = st->previous->previous;
          int cnt = 0;
          bool perpetual = true;

          for (int i = 4; i <= end; i += 2)
          {
              stp = stp->previous->previous;
              perpetual &= bool(stp->checkersBB);

              // Return a draw score if a position repeats once earlier but strictly
              // after the root, or repeats twice before or at the root.
              if (   stp->key == st->key
                  && ++cnt + 1 == (ply > i ? 2 : n_fold_rule()))
              {
                  result = convert_mate_value(  var->perpetualCheckIllegal && perpetual ? VALUE_MATE
                                              : var->nFoldValueAbsolute && sideToMove == BLACK ? -var->nFoldValue
                                              : var->nFoldValue, ply);
                  return true;
              }
          }
      }
  }

  // counting rules
  if (   counting_rule()
      && st->countingLimit
      && st->countingPly >= st->countingLimit
      && (!checkers() || MoveList<LEGAL>(*this).size()))
  {
      result = VALUE_DRAW;
      return true;
  }

  // sittuyin stalemate due to optional promotion (3.9 c.7)
  if (   sittuyin_promotion()
      && count<ALL_PIECES>(sideToMove) == 2
      && count<PAWN>(sideToMove) == 1
      && !checkers())
  {
      bool promotionsOnly = true;
      for (const auto& m : MoveList<LEGAL>(*this))
          if (type_of(m) != PROMOTION)
          {
              promotionsOnly = false;
              break;
          }
      if (promotionsOnly)
      {
          result = VALUE_DRAW;
          return true;
      }
  }

  return false;
}

/// Position::is_immediate_game_end() tests whether the position ends the game
/// immediately by a variant rule, i.e., there are no more legal moves.
/// It does not not detect stalemates.

bool Position::is_immediate_game_end(Value& result, int ply) const {

  // bare king rule
  if (    bare_king_value() != VALUE_NONE
      && !bare_king_move()
      && !(count<ALL_PIECES>(sideToMove) - count<KING>(sideToMove)))
  {
      result = bare_king_value(ply);
      return true;
  }
  if (    bare_king_value() != VALUE_NONE
      &&  bare_king_move()
      && !(count<ALL_PIECES>(~sideToMove) - count<KING>(~sideToMove)))
  {
      result = -bare_king_value(ply);
      return true;
  }
  // extinction
  if (extinction_value() != VALUE_NONE)
  {
      for (PieceType pt : extinction_piece_types())
          if (!count(WHITE, pt) || !count(BLACK, pt))
          {
              result = !count(sideToMove, pt) ? extinction_value(ply) : -extinction_value(ply);
              return true;
          }
  }
  // capture the flag
  if (   capture_the_flag_piece()
      && !flag_move()
      && (capture_the_flag(~sideToMove) & pieces(~sideToMove, capture_the_flag_piece())))
  {
      result = mated_in(ply);
      return true;
  }
  if (   capture_the_flag_piece()
      && flag_move()
      && (capture_the_flag(sideToMove) & pieces(sideToMove, capture_the_flag_piece())))
  {
      result =  (capture_the_flag(~sideToMove) & pieces(~sideToMove, capture_the_flag_piece()))
              && sideToMove == WHITE ? VALUE_DRAW : mate_in(ply);
      return true;
  }
  // nCheck
  if (check_counting() && checks_remaining(~sideToMove) == 0)
  {
      result = mated_in(ply);
      return true;
  }
  // Connect-n
  if (connect_n() > 0)
  {
      Bitboard b;
      for (Direction d : {NORTH, NORTH_EAST, EAST, SOUTH_EAST})
      {
          b = pieces(~sideToMove);
          for (int i = 1; i < connect_n() && b; i++)
              b &= shift(d, b);
          if (b)
          {
              result = mated_in(ply);
              return true;
          }
      }
  }

  return false;
}


// Position::has_repeated() tests whether there has been at least one repetition
// of positions since the last capture or pawn move.

bool Position::has_repeated() const {

    StateInfo* stc = st;
    int end = captures_to_hand() ? st->pliesFromNull : std::min(st->rule50, st->pliesFromNull);
    while (end-- >= 4)
    {
        if (stc->repetition)
            return true;

        stc = stc->previous;
    }
    return false;
}


/// Position::has_game_cycle() tests if the position has a move which draws by repetition,
/// or an earlier position has a move that directly reaches the current position.

bool Position::has_game_cycle(int ply) const {

  int j;

  int end = captures_to_hand() ? st->pliesFromNull : std::min(st->rule50, st->pliesFromNull);

  if (end < 3 || var->nFoldValue != VALUE_DRAW)
    return false;

  Key originalKey = st->key;
  StateInfo* stp = st->previous;

  for (int i = 3; i <= end; i += 2)
  {
      stp = stp->previous->previous;

      Key moveKey = originalKey ^ stp->key;
      if (   (j = H1(moveKey), cuckoo[j] == moveKey)
          || (j = H2(moveKey), cuckoo[j] == moveKey))
      {
          Move move = cuckooMove[j];
          Square s1 = from_sq(move);
          Square s2 = to_sq(move);

          if (!(between_bb(s1, s2) & pieces()))
          {
              if (ply > i)
                  return true;

              // For nodes before or at the root, check that the move is a
              // repetition rather than a move to the current position.
              // In the cuckoo table, both moves Rc1c5 and Rc5c1 are stored in
              // the same location, so we have to select which square to check.
              if (color_of(piece_on(empty(s1) ? s2 : s1)) != side_to_move())
                  continue;

              // For repetitions before or at the root, require one more
              if (stp->repetition)
                  return true;
          }
      }
  }
  return false;
}


/// Position::counting_limit() returns the counting limit in full moves.

int Position::counting_limit() const {

  assert(counting_rule());

  // No counting yet
  if (count<PAWN>() && count<ALL_PIECES>(sideToMove) > 1)
      return 0;

  switch (counting_rule())
  {
  case MAKRUK_COUNTING:
      // Board's honor rule
      if (count<ALL_PIECES>(sideToMove) > 1)
          return 64;

      // Pieces' honor rule
      if (count<ROOK>(~sideToMove) > 1)
          return 8;
      if (count<ROOK>(~sideToMove) == 1)
          return 16;
      if (count<KHON>(~sideToMove) > 1)
          return 22;
      if (count<KNIGHT>(~sideToMove) > 1)
          return 32;
      if (count<KHON>(~sideToMove) == 1)
          return 44;

      return 64;

  case ASEAN_COUNTING:
      if (count<ALL_PIECES>(sideToMove) > 1)
          return 0;
      if (count<ROOK>(~sideToMove))
          return 16;
      if (count<KHON>(~sideToMove) && count<MET>(~sideToMove))
          return 44;
      if (count<KNIGHT>(~sideToMove) && count<MET>(~sideToMove))
          return 64;

      return 0;

  default:
      assert(false);
      return 0;
  }

}


/// Position::flip() flips position with the white and black sides reversed. This
/// is only useful for debugging e.g. for finding evaluation symmetry bugs.

void Position::flip() {

  string f, token;
  std::stringstream ss(fen());

  for (Rank r = RANK_MAX; r >= RANK_1; --r) // Piece placement
  {
      std::getline(ss, token, r > RANK_1 ? '/' : ' ');
      f.insert(0, token + (f.empty() ? " " : "/"));
  }

  ss >> token; // Active color
  f += (token == "w" ? "B " : "W "); // Will be lowercased later

  ss >> token; // Castling availability
  f += token + " ";

  std::transform(f.begin(), f.end(), f.begin(),
                 [](char c) { return char(islower(c) ? toupper(c) : tolower(c)); });

  ss >> token; // En passant square
  f += (token == "-" ? token : token.replace(1, 1, token[1] == '3' ? "6" : "3"));

  std::getline(ss, token); // Half and full moves
  f += token;

  set(variant(), f, is_chess960(), st, this_thread());

  assert(pos_is_ok());
}


/// Position::pos_is_ok() performs some consistency checks for the
/// position object and raises an asserts if something wrong is detected.
/// This is meant to be helpful when debugging.

bool Position::pos_is_ok() const {

  constexpr bool Fast = true; // Quick (default) or full check?

  if (   (sideToMove != WHITE && sideToMove != BLACK)
      || (count<KING>(WHITE) && piece_on(square<KING>(WHITE)) != make_piece(WHITE, KING))
      || (count<KING>(BLACK) && piece_on(square<KING>(BLACK)) != make_piece(BLACK, KING))
      || (   ep_square() != SQ_NONE
          && relative_rank(~sideToMove, ep_square(), max_rank()) != Rank(double_step_rank() + 1)))
      assert(0 && "pos_is_ok: Default");

  if (Fast)
      return true;

  if (   pieceCount[make_piece(~sideToMove, KING)]
      && (attackers_to(square<KING>(~sideToMove)) & pieces(sideToMove)))
      assert(0 && "pos_is_ok: Kings");

  if (   pieceCount[make_piece(WHITE, PAWN)] > 64
      || pieceCount[make_piece(BLACK, PAWN)] > 64)
      assert(0 && "pos_is_ok: Pawns");

  if (   (pieces(WHITE) & pieces(BLACK))
      || (pieces(WHITE) | pieces(BLACK)) != pieces()
      || popcount(pieces(WHITE)) > 64
      || popcount(pieces(BLACK)) > 64)
      assert(0 && "pos_is_ok: Bitboards");

  for (PieceType p1 = PAWN; p1 <= KING; ++p1)
      for (PieceType p2 = PAWN; p2 <= KING; ++p2)
          if (p1 != p2 && (pieces(p1) & pieces(p2)))
              assert(0 && "pos_is_ok: Bitboards");

  StateInfo si = *st;
  set_state(&si);
  if (std::memcmp(&si, st, sizeof(StateInfo)))
      assert(0 && "pos_is_ok: State");

  for (Color c : {WHITE, BLACK})
      for (PieceType pt = PAWN; pt <= KING; ++pt)
      {
          Piece pc = make_piece(c, pt);
          if (   pieceCount[pc] != popcount(pieces(c, pt))
              || pieceCount[pc] != std::count(board, board + SQUARE_NB, pc))
              assert(0 && "pos_is_ok: Pieces");

          for (int i = 0; i < pieceCount[pc]; ++i)
              if (board[pieceList[pc][i]] != pc || index[pieceList[pc][i]] != i)
                  assert(0 && "pos_is_ok: Index");
      }

  for (Color c : { WHITE, BLACK })
      for (CastlingRights cr : {c & KING_SIDE, c & QUEEN_SIDE})
      {
          if (!can_castle(cr))
              continue;

          if (   piece_on(castlingRookSquare[cr]) != make_piece(c, ROOK)
              || castlingRightsMask[castlingRookSquare[cr]] != cr
              || (castlingRightsMask[square<KING>(c)] & cr) != cr)
              assert(0 && "pos_is_ok: Castling");
      }

  return true;
}
