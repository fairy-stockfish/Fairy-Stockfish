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

namespace Stockfish {

namespace Zobrist {

  Key psq[PIECE_NB][SQUARE_NB];
  Key enpassant[FILE_NB];
  Key castling[CASTLING_RIGHT_NB];
  Key side, noPawns;
  Key inHand[PIECE_NB][SQUARE_NB];
  Key checks[COLOR_NB][CHECKS_NB];
  Key wall[SQUARE_NB];
}


/// operator<<(Position) returns an ASCII representation of the position

std::ostream& operator<<(std::ostream& os, const Position& pos) {

  os << "\n ";
  for (File f = FILE_A; f <= pos.max_file(); ++f)
      os << "+---";
  os << "+\n";

  for (Rank r = pos.max_rank(); r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= pos.max_file(); ++f)
          if (pos.state()->wallSquares & make_square(f, r))
              os << " | *";
          else if (pos.unpromoted_piece_on(make_square(f, r)))
              os << " |+" << pos.piece_to_char()[pos.unpromoted_piece_on(make_square(f, r))];
          else
              os << " | " << pos.piece_to_char()[pos.piece_on(make_square(f, r))];

      os << " |" << (1 + r);
      if (r == pos.max_rank() || r == RANK_1)
      {
          Color c = r == RANK_1 ? WHITE : BLACK;
          if (c == pos.side_to_move())
              os << " *";
          else
              os << "  ";
          if (!pos.variant()->freeDrops && (pos.piece_drops() || pos.seirawan_gating()))
          {
              os << " [";
              for (PieceType pt = KING; pt >= PAWN; --pt)
                  os << std::string(pos.count_in_hand(c, pt), pos.piece_to_char()[make_piece(c, pt)]);
              os << "]";
          }
      }
      os << "\n ";
      for (File f = FILE_A; f <= pos.max_file(); ++f)
          os << "+---";
      os << "+\n";
  }

  for (File f = FILE_A; f <= pos.max_file(); ++f)
      os << "   " << char('a' + f);
  os << "\n";
  os << "\nFen: " << pos.fen() << "\nSfen: " << pos.fen(true) << "\nKey: " << std::hex << std::uppercase
     << std::setfill('0') << std::setw(16) << pos.key()
     << std::setfill(' ') << std::dec << "\nCheckers: ";

  for (Bitboard b = pos.checkers(); b; )
      os << UCI::square(pos, pop_lsb(b)) << " ";

  os << "\nChased: ";
  for (Bitboard b = pos.state()->chased; b; )
      os << UCI::square(pos, pop_lsb(b)) << " ";

  if (    int(Tablebases::MaxCardinality) >= popcount(pos.pieces())
      && Options["UCI_Variant"] == "chess"
      && !pos.can_castle(ANY_CASTLING))
  {
      StateInfo st;
      ASSERT_ALIGNED(&st, Eval::NNUE::CacheLineSize);

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


/// Position::init() initializes at startup the various arrays used to compute hash keys

void Position::init() {

  PRNG rng(1070372);

  for (Color c : {WHITE, BLACK})
      for (PieceType pt = PAWN; pt <= KING; ++pt)
          for (Square s = SQ_A1; s <= SQ_MAX; ++s)
              Zobrist::psq[make_piece(c, pt)][s] = rng.rand<Key>();

  for (File f = FILE_A; f <= FILE_MAX; ++f)
      Zobrist::enpassant[f] = rng.rand<Key>();

  for (int cr = NO_CASTLING; cr <= ANY_CASTLING; ++cr)
      Zobrist::castling[cr] = rng.rand<Key>();

  Zobrist::side = rng.rand<Key>();
  Zobrist::noPawns = rng.rand<Key>();

  for (Color c : {WHITE, BLACK})
      for (int n = 0; n < CHECKS_NB; ++n)
          Zobrist::checks[c][n] = rng.rand<Key>();

  for (Color c : {WHITE, BLACK})
      for (PieceType pt = PAWN; pt <= KING; ++pt)
          for (int n = 0; n < SQUARE_NB; ++n)
              Zobrist::inHand[make_piece(c, pt)][n] = rng.rand<Key>();

  for (Square s = SQ_A1; s <= SQ_MAX; ++s)
      Zobrist::wall[s] = rng.rand<Key>();

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
              if ((type_of(pc) != PAWN) && (attacks_bb(c, type_of(pc), s1, 0) & s2))
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
      is the position "behind" the pawn. Following X-FEN standard, this is recorded only
      if there is a pawn in position to make an en passant capture, and if there really
      is a pawn that might have advanced two squares.

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
  st = si;

  var = v;

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
          }
#endif
          sq += (token - '0') * EAST; // Advance the given number of files
      }

      else if (token == '/')
      {
          sq += 2 * SOUTH + (FILE_MAX - max_file()) * EAST;
          if (!is_ok(sq))
              break;
      }

      // Wall square
      else if (token == '*')
      {
          st->wallSquares |= sq;
          byTypeBB[ALL_PIECES] |= sq;
          ++sq;
      }

      else if ((idx = piece_to_char().find(token)) != string::npos || (idx = piece_to_char_synonyms().find(token)) != string::npos)
      {
          if (ss.peek() == '~')
              ss >> token;
          put_piece(Piece(idx), sq, token == '~');
          ++sq;
      }

      // Promoted shogi pieces
      else if (token == '+' && (idx = piece_to_char().find(ss.peek())) != string::npos)
      {
          ss >> token;
          put_piece(make_piece(color_of(Piece(idx)), promoted_piece_type(type_of(Piece(idx)))), sq, true, Piece(idx));
          ++sq;
      }

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
  sideToMove = (token != (sfen ? 'w' : 'b') ? WHITE : BLACK);  // Invert colors for SFEN
  ss >> token;

  // 3-4. Skip parsing castling and en passant flags if not present
  st->epSquare = SQ_NONE;
  st->castlingKingSquare[WHITE] = st->castlingKingSquare[BLACK] = SQ_NONE;
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
          Piece rook = make_piece(c, castling_rook_piece());

          token = char(toupper(token));

          if (token == 'K')
              for (rsq = make_square(FILE_MAX, castling_rank(c)); piece_on(rsq) != rook; --rsq) {}

          else if (token == 'Q')
              for (rsq = make_square(FILE_A, castling_rank(c)); piece_on(rsq) != rook; ++rsq) {}

          else if (token >= 'A' && token <= 'A' + max_file())
              rsq = make_square(File(token - 'A'), castling_rank(c));

          else
              continue;

          // Determine castling "king" position
          if (castling_enabled() && st->castlingKingSquare[c] == SQ_NONE)
          {
              Bitboard castlingKings = pieces(c, castling_king_piece()) & rank_bb(castling_rank(c));
              // Ambiguity resolution for 960 variants with more than one "king"
              // e.g., EAH means that an e-file king can castle with a- and h-file rooks
              st->castlingKingSquare[c] =  isChess960 && piece_on(rsq) == make_piece(c, castling_king_piece()) ? rsq
                                         : castlingKings && (!more_than_one(castlingKings) || isChess960) ? lsb(castlingKings)
                                         : make_square(castling_king_file(), castling_rank(c));
          }

          // Set gates (and skip castling rights)
          if (gating())
          {
              st->gatesBB[c] |= rsq;
              if (token == 'K' || token == 'Q')
                  st->gatesBB[c] |= st->castlingKingSquare[c];
              // Do not set castling rights for gates unless there are no pieces in hand,
              // which means that the file is referring to a chess960 castling right.
              else if (!seirawan_gating() || count_in_hand(c, ALL_PIECES) > 0 || captures_to_hand())
                  continue;
          }

          if (castling_enabled() && piece_on(rsq) == rook)
              set_castling_right(c, rsq);
      }

      // Set castling rights for 960 gating variants
      if (gating() && castling_enabled())
          for (Color c : {WHITE, BLACK})
              if ((gates(c) & pieces(castling_king_piece())) && !castling_rights(c) && (!seirawan_gating() || count_in_hand(c, ALL_PIECES) > 0 || captures_to_hand()))
              {
                  Bitboard castling_rooks = gates(c) & pieces(castling_rook_piece());
                  while (castling_rooks)
                      set_castling_right(c, pop_lsb(castling_rooks));
              }

      // counting limit
      if (counting_rule() && isdigit(ss.peek()))
          ss >> st->countingLimit;

      // 4. En passant square.
      // Ignore if square is invalid or not on side to move relative rank 6.
      else if (   ((ss >> col) && (col >= 'a' && col <= 'a' + max_file()))
               && ((ss >> row) && (row >= '1' && row <= '1' + max_rank())))
      {
          st->epSquare = make_square(File(col - 'a'), Rank(row - '1'));
#ifdef LARGEBOARDS
          // Consider different rank numbering in CECP
          if (max_rank() == RANK_10 && CurrentProtocol == XBOARD)
              st->epSquare += NORTH;
#endif

          // En passant square will be considered only if
          // a) side to move have a pawn threatening epSquare
          // b) there is an enemy pawn in front of epSquare
          // c) there is no piece on epSquare or behind epSquare
          bool enpassant;
          enpassant = pawn_attacks_bb(~sideToMove, st->epSquare) & pieces(sideToMove, PAWN)
                  && (pieces(~sideToMove, PAWN) & (st->epSquare + pawn_push(~sideToMove)))
                  && !(pieces() & (st->epSquare | (st->epSquare + pawn_push(sideToMove))));
          if (!enpassant)
              st->epSquare = SQ_NONE;
      }
  }

  // Check counter for nCheck
  ss >> std::skipws >> token >> std::noskipws;

  if (check_counting())
  {
      if (ss.peek() == '+')
      {
          st->checksRemaining[WHITE] = CheckCount(std::max(token - '0', 0));
          ss >> token >> token;
          st->checksRemaining[BLACK] = CheckCount(std::max(token - '0', 0));
      }
      else
      {
          // If check count is not provided, assume that the next check wins
          st->checksRemaining[WHITE] = CheckCount(1);
          st->checksRemaining[BLACK] = CheckCount(1);
          ss.putback(token);
      }
  }
  else
      ss.putback(token);

  // 5-6. Halfmove clock and fullmove number
  if (sfen)
  {
      // Pieces in hand for SFEN
      int handCount = 1;
      while ((ss >> token) && !isspace(token))
      {
          if (token == '-')
              continue;
          else if (isdigit(token))
          {
              handCount = token - '0';
              while (isdigit(ss.peek()) && ss >> token)
                  handCount = 10 * handCount + (token - '0');
          }
          else if ((idx = piece_to_char().find(token)) != string::npos)
          {
              for (int i = 0; i < handCount; i++)
                  add_to_hand(Piece(idx));
              handCount = 1;
          }
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

  // Lichess-style counter for 3check
  if (check_counting())
  {
      if (ss >> token && token == '+')
      {
          ss >> token;
          st->checksRemaining[WHITE] = CheckCount(std::max(3 - (token - '0'), 0));
          ss >> token >> token;
          st->checksRemaining[BLACK] = CheckCount(std::max(3 - (token - '0'), 0));
      }
  }

  chess960 = isChess960 || v->chess960;
  tsumeMode = Options["TsumeMode"];
  thisThread = th;
  set_state(st);

  assert(pos_is_ok());

  return *this;
}


/// Position::set_castling_right() is a helper function used to set castling
/// rights given the corresponding color and the rook starting square.

void Position::set_castling_right(Color c, Square rfrom) {

  assert(st->castlingKingSquare[c] != SQ_NONE);
  Square kfrom = st->castlingKingSquare[c];
  CastlingRights cr = c & (kfrom < rfrom ? KING_SIDE: QUEEN_SIDE);

  st->castlingRights |= cr;
  castlingRightsMask[kfrom] |= cr;
  castlingRightsMask[rfrom] |= cr;
  castlingRookSquare[cr] = rfrom;

  Square kto = make_square(cr & KING_SIDE ? castling_kingside_file() : castling_queenside_file(), castling_rank(c));
  Square rto = kto + (cr & KING_SIDE ? WEST : EAST);

  castlingPath[cr] =   (between_bb(rfrom, rto) | between_bb(kfrom, kto))
                    & ~(kfrom | rfrom);
}


/// Position::set_check_info() sets king attacks to detect if a move gives check

void Position::set_check_info(StateInfo* si) const {

  si->blockersForKing[WHITE] = slider_blockers(pieces(BLACK), count<KING>(WHITE) ? square<KING>(WHITE) : SQ_NONE, si->pinners[BLACK], BLACK);
  si->blockersForKing[BLACK] = slider_blockers(pieces(WHITE), count<KING>(BLACK) ? square<KING>(BLACK) : SQ_NONE, si->pinners[WHITE], WHITE);

  Square ksq = count<KING>(~sideToMove) ? square<KING>(~sideToMove) : SQ_NONE;

  // For unused piece types, the check squares are left uninitialized
  si->nonSlidingRiders = 0;
  for (PieceType pt : piece_types())
  {
      si->checkSquares[pt] = ksq != SQ_NONE ? attacks_bb(~sideToMove, pt, ksq, pieces()) : Bitboard(0);
      // Collect special piece types that require slower check and evasion detection
      if (AttackRiderTypes[pt] & NON_SLIDING_RIDERS)
          si->nonSlidingRiders |= pieces(pt);
  }
  si->shak = si->checkersBB & (byTypeBB[KNIGHT] | byTypeBB[ROOK] | byTypeBB[BERS]);
  si->bikjang = var->bikjangRule && ksq != SQ_NONE ? bool(attacks_bb(sideToMove, ROOK, ksq, pieces()) & pieces(sideToMove, KING)) : false;
  si->chased = var->chasingRule ? chased() : Bitboard(0);
  si->legalCapture = NO_VALUE;
  if (var->extinctionPseudoRoyal)
  {
      si->pseudoRoyals = 0;
      for (PieceType pt : extinction_piece_types())
      {
          if (count(sideToMove, pt) <= var->extinctionPieceCount + 1)
              si->pseudoRoyals |= pieces(sideToMove, pt);
          if (count(~sideToMove, pt) <= var->extinctionPieceCount + 1)
              si->pseudoRoyals |= pieces(~sideToMove, pt);
      }
  }
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
  si->move = MOVE_NONE;

  set_check_info(si);

  for (Bitboard b = pieces(); b; )
  {
      Square s = pop_lsb(b);
      Piece pc = piece_on(s);
      si->key ^= Zobrist::psq[pc][s];

      if (!pc)
          si->key ^= Zobrist::wall[s];

      else if (type_of(pc) == PAWN)
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

          if (piece_drops() || seirawan_gating())
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

  assert(code[0] == 'K');

  string sides[] = { code.substr(code.find('K', 1)),      // Weak
                     code.substr(0, std::min(code.find('v'), code.find('K', 1))) }; // Strong

  assert(sides[0].length() > 0 && sides[0].length() < 8);
  assert(sides[1].length() > 0 && sides[1].length() < 8);

  std::transform(sides[c].begin(), sides[c].end(), sides[c].begin(), tolower);

  string n = std::to_string(FILE_NB);
  string fenStr =  n + "/" + sides[0] + char(FILE_NB - sides[0].length() + '0') + "/" + n + "/" + n + "/" + n + "/"
                 + n + "/" + sides[1] + char(FILE_NB - sides[1].length() + '0') + "/" + n + " w - - 0 10";

  return set(variants.find("fairy")->second, fenStr, false, si, nullptr);
}


/// Position::fen() returns a FEN representation of the position. In case of
/// Chess960 the Shredder-FEN notation is used. This is mainly a debugging function.

string Position::fen(bool sfen, bool showPromoted, int countStarted, std::string holdings) const {

  int emptyCnt;
  std::ostringstream ss;

  for (Rank r = max_rank(); r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= max_file(); ++f)
      {
          for (emptyCnt = 0; f <= max_file() && !(pieces() & make_square(f, r)); ++f)
              ++emptyCnt;

          if (emptyCnt)
              ss << emptyCnt;

          if (f <= max_file())
          {
              if (empty(make_square(f, r)))
                  // Wall square
                  ss << "*";
              else if (unpromoted_piece_on(make_square(f, r)))
                  // Promoted shogi pieces, e.g., +r for dragon
                  ss << "+" << piece_to_char()[unpromoted_piece_on(make_square(f, r))];
              else
              {
                  ss << piece_to_char()[piece_on(make_square(f, r))];

                  // Set promoted pieces
                  if (((captures_to_hand() && !drop_loop()) || showPromoted) && is_promoted(make_square(f, r)))
                      ss << "~";
              }
          }
      }

      if (r > RANK_1)
          ss << '/';
  }

  // SFEN
  if (sfen)
  {
      ss << (sideToMove == WHITE ? " b " : " w ");
      for (Color c : {WHITE, BLACK})
          for (PieceType pt = KING; pt >= PAWN; --pt)
              if (pieceCountInHand[c][pt] > 0)
              {
                  if (pieceCountInHand[c][pt] > 1)
                      ss << pieceCountInHand[c][pt];
                  ss << piece_to_char()[make_piece(c, pt)];
              }
      if (count_in_hand(ALL_PIECES) == 0)
          ss << '-';
      ss << " " << gamePly + 1;
      return ss.str();
  }

  // pieces in hand
  if (!variant()->freeDrops && (piece_drops() || seirawan_gating()))
  {
      ss << '[';
      if (holdings != "-")
          ss << holdings;
      else
          for (Color c : {WHITE, BLACK})
              for (PieceType pt = KING; pt >= PAWN; --pt)
              {
                  assert(pieceCountInHand[c][pt] >= 0);
                  ss << std::string(pieceCountInHand[c][pt], piece_to_char()[make_piece(c, pt)]);
              }
      ss << ']';
  }

  ss << (sideToMove == WHITE ? " w " : " b ");

  // Disambiguation for chess960 "king" square
  if (chess960 && can_castle(WHITE_CASTLING) && popcount(pieces(WHITE, castling_king_piece()) & rank_bb(castling_rank(WHITE))) > 1)
      ss << char('A' + castling_king_square(WHITE));

  if (can_castle(WHITE_OO))
      ss << (chess960 ? char('A' + file_of(castling_rook_square(WHITE_OO ))) : 'K');

  if (can_castle(WHITE_OOO))
      ss << (chess960 ? char('A' + file_of(castling_rook_square(WHITE_OOO))) : 'Q');

  if (gating() && gates(WHITE) && (!seirawan_gating() || count_in_hand(WHITE, ALL_PIECES) > 0 || captures_to_hand()))
      for (File f = FILE_A; f <= max_file(); ++f)
          if (   (gates(WHITE) & file_bb(f))
              // skip gating flags redundant with castling flags
              && !(!chess960 && can_castle(WHITE_CASTLING) && f == file_of(castling_king_square(WHITE)))
              && !(can_castle(WHITE_OO ) && f == file_of(castling_rook_square(WHITE_OO )))
              && !(can_castle(WHITE_OOO) && f == file_of(castling_rook_square(WHITE_OOO))))
              ss << char('A' + f);

  // Disambiguation for chess960 "king" square
  if (chess960 && can_castle(BLACK_CASTLING) && popcount(pieces(BLACK, castling_king_piece()) & rank_bb(castling_rank(BLACK))) > 1)
      ss << char('a' + castling_king_square(BLACK));

  if (can_castle(BLACK_OO))
      ss << (chess960 ? char('a' + file_of(castling_rook_square(BLACK_OO ))) : 'k');

  if (can_castle(BLACK_OOO))
      ss << (chess960 ? char('a' + file_of(castling_rook_square(BLACK_OOO))) : 'q');

  if (gating() && gates(BLACK) && (!seirawan_gating() || count_in_hand(BLACK, ALL_PIECES) > 0 || captures_to_hand()))
      for (File f = FILE_A; f <= max_file(); ++f)
          if (   (gates(BLACK) & file_bb(f))
              // skip gating flags redundant with castling flags
              && !(!chess960 && can_castle(BLACK_CASTLING) && f == file_of(castling_king_square(BLACK)))
              && !(can_castle(BLACK_OO ) && f == file_of(castling_rook_square(BLACK_OO )))
              && !(can_castle(BLACK_OOO) && f == file_of(castling_rook_square(BLACK_OOO))))
              ss << char('a' + f);

  if (!can_castle(ANY_CASTLING) && !(gating() && (gates(WHITE) | gates(BLACK))))
      ss << '-';

  // Counting limit or ep-square
  if (st->countingLimit)
      ss << " " << counting_limit(countStarted) << " ";
  else
      ss << (ep_square() == SQ_NONE ? " - " : " " + UCI::square(*this, ep_square()) + " ");

  // Check count
  if (check_counting())
      ss << st->checksRemaining[WHITE] << "+" << st->checksRemaining[BLACK] << " ";

  // Counting ply or 50-move rule counter
  if (st->countingLimit)
      ss << counting_ply(countStarted);
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
  Bitboard slidingSnipers = 0;

  if (var->fastAttacks)
  {
      snipers = (  (attacks_bb<  ROOK>(s) & pieces(c, QUEEN, ROOK, CHANCELLOR))
                 | (attacks_bb<BISHOP>(s) & pieces(c, QUEEN, BISHOP, ARCHBISHOP))) & sliders;
      slidingSnipers = snipers;
  }
  else
  {
      for (PieceType pt : piece_types())
      {
          Bitboard b = sliders & (PseudoAttacks[~c][pt][s] ^ LeaperAttacks[~c][pt][s]) & pieces(c, pt);
          if (b)
          {
              // Consider asymmetrical moves (e.g., horse)
              if (AttackRiderTypes[pt] & ASYMMETRICAL_RIDERS)
              {
                  Bitboard asymmetricals = PseudoAttacks[~c][pt][s] & pieces(c, pt);
                  while (asymmetricals)
                  {
                      Square s2 = pop_lsb(asymmetricals);
                      if (!(attacks_from(c, pt, s2) & s))
                          snipers |= s2;
                  }
              }
              else
                  snipers |= b & ~attacks_bb(~c, pt, s, pieces());
              if (AttackRiderTypes[pt] & ~HOPPING_RIDERS)
                  slidingSnipers |= snipers & pieces(pt);
          }
      }
      // Diagonal rook pins in Janggi palace
      if (diagonal_lines() & s)
      {
          Bitboard diags = diagonal_lines() & PseudoAttacks[~c][BISHOP][s] & sliders & pieces(c, ROOK);
          while (diags)
          {
              Square s2 = pop_lsb(diags);
              if (!(attacks_from(c, ROOK, s2) & s))
              {
                  snipers |= s2;
                  slidingSnipers |= s2;
              }
          }
      }
  }
  Bitboard occupancy = pieces() ^ slidingSnipers;

  while (snipers)
  {
    Square sniperSq = pop_lsb(snipers);
    bool isHopper = AttackRiderTypes[type_of(piece_on(sniperSq))] & HOPPING_RIDERS;
    Bitboard b = between_bb(s, sniperSq, type_of(piece_on(sniperSq))) & (isHopper ? (pieces() ^ sniperSq) : occupancy);

    if (b && (!more_than_one(b) || (isHopper && popcount(b) == 2)))
    {
        // Janggi cannons block each other
        if ((pieces(JANGGI_CANNON) & sniperSq) && (pieces(JANGGI_CANNON) & b))
            b &= pieces(JANGGI_CANNON);
        blockers |= b;
        if (b & pieces(color_of(piece_on(s))))
            pinners |= sniperSq;
    }
  }
  return blockers;
}


/// Position::attackers_to() computes a bitboard of all pieces which attack a
/// given square. Slider attacks use the occupied bitboard to indicate occupancy.

Bitboard Position::attackers_to(Square s, Bitboard occupied, Color c, Bitboard janggiCannons) const {

  // Use a faster version for variants with moderate rule variations
  if (var->fastAttacks)
  {
      return  (pawn_attacks_bb(~c, s)          & pieces(c, PAWN))
            | (attacks_bb<KNIGHT>(s)           & pieces(c, KNIGHT, ARCHBISHOP, CHANCELLOR))
            | (attacks_bb<  ROOK>(s, occupied) & pieces(c, ROOK, QUEEN, CHANCELLOR))
            | (attacks_bb<BISHOP>(s, occupied) & pieces(c, BISHOP, QUEEN, ARCHBISHOP))
            | (attacks_bb<KING>(s)             & pieces(c, KING, COMMONER));
  }

  // Use a faster version for selected fairy pieces
  if (var->fastAttacks2)
  {
      return  (pawn_attacks_bb(~c, s)             & pieces(c, PAWN, BREAKTHROUGH_PIECE, GOLD))
            | (attacks_bb<KNIGHT>(s)              & pieces(c, KNIGHT))
            | (attacks_bb<  ROOK>(s, occupied)    & (  pieces(c, ROOK, QUEEN, DRAGON)
                                                     | (pieces(c, LANCE) & PseudoAttacks[~c][LANCE][s])))
            | (attacks_bb<BISHOP>(s, occupied)    & pieces(c, BISHOP, QUEEN, DRAGON_HORSE))
            | (attacks_bb<KING>(s)                & pieces(c, KING, COMMONER))
            | (attacks_bb<FERS>(s)                & pieces(c, FERS, DRAGON, SILVER))
            | (attacks_bb<WAZIR>(s)               & pieces(c, WAZIR, DRAGON_HORSE, GOLD))
            | (LeaperAttacks[~c][SHOGI_KNIGHT][s] & pieces(c, SHOGI_KNIGHT))
            | (LeaperAttacks[~c][SHOGI_PAWN][s]   & pieces(c, SHOGI_PAWN, SILVER));
  }

  Bitboard b = 0;
  for (PieceType pt : piece_types())
      if (board_bb(c, pt) & s)
      {
          PieceType move_pt = pt == KING ? king_type() : pt;
          // Consider asymmetrical moves (e.g., horse)
          if (AttackRiderTypes[move_pt] & ASYMMETRICAL_RIDERS)
          {
              Bitboard asymmetricals = PseudoAttacks[~c][move_pt][s] & pieces(c, pt);
              while (asymmetricals)
              {
                  Square s2 = pop_lsb(asymmetricals);
                  if (attacks_bb(c, move_pt, s2, occupied) & s)
                      b |= s2;
              }
          }
          else if (pt == JANGGI_CANNON)
              b |= attacks_bb(~c, move_pt, s, occupied) & attacks_bb(~c, move_pt, s, occupied & ~janggiCannons) & pieces(c, JANGGI_CANNON);
          else
              b |= attacks_bb(~c, move_pt, s, occupied) & pieces(c, pt);
      }

  // Janggi palace moves
  if (diagonal_lines() & s)
  {
      Bitboard diags = 0;
      if (king_type() == WAZIR)
          diags |= attacks_bb(~c, FERS, s, occupied) & pieces(c, KING);
      diags |= attacks_bb(~c, FERS, s, occupied) & pieces(c, WAZIR);
      diags |= attacks_bb(~c, PAWN, s, occupied) & pieces(c, SOLDIER);
      diags |= rider_attacks_bb<RIDER_BISHOP>(s, occupied) & pieces(c, ROOK);
      diags |=  rider_attacks_bb<RIDER_CANNON_DIAG>(s, occupied)
              & rider_attacks_bb<RIDER_CANNON_DIAG>(s, occupied & ~janggiCannons)
              & pieces(c, JANGGI_CANNON);
      b |= diags & diagonal_lines();
  }

  // Unpromoted soldiers
  if (b & pieces(SOLDIER) && relative_rank(c, s, max_rank()) < var->soldierPromotionRank)
      b ^= b & pieces(SOLDIER) & ~PseudoAttacks[~c][SHOGI_PAWN][s];

  return b;
}


Bitboard Position::attackers_to(Square s, Bitboard occupied) const {
  return attackers_to(s, occupied, WHITE) | attackers_to(s, occupied, BLACK);
}

/// Position::attackers_to_pseudo_royals computes a bitboard of all pieces
/// of a particular color attacking at least one opposing pseudo-royal piece
Bitboard Position::attackers_to_pseudo_royals(Color c) const {
  Bitboard attackers = 0;
  Bitboard pseudoRoyals = st->pseudoRoyals & pieces(~c);
  Bitboard pseudoRoyalsTheirs = st->pseudoRoyals & pieces(c);
  while (pseudoRoyals) {
      Square sr = pop_lsb(pseudoRoyals);
      if (blast_on_capture()
          && pseudoRoyalsTheirs & attacks_bb<KING>(sr))
          // skip if capturing this piece would blast all of the attacker's pseudo-royal pieces
          continue;
      attackers |= attackers_to(sr, c);
  }
  return attackers;
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
  assert(board_bb() & to);

  // Illegal checks
  if ((!checking_permitted() || (sittuyin_promotion() && type_of(m) == PROMOTION) || (!drop_checks() && type_of(m) == DROP)) && gives_check(m))
      return false;

  // Illegal quiet moves
  if (must_capture() && !capture(m) && has_capture())
      return false;

  // Illegal non-drop moves
  if (must_drop() && count_in_hand(us, var->mustDropType) > 0)
  {
      if (type_of(m) == DROP)
      {
          if (var->mustDropType != ALL_PIECES && var->mustDropType != in_hand_piece_type(m))
              return false;
      }
      else if (checkers())
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
          Bitboard remaining = drop_region(us, BISHOP) & ~pieces() & ~square_bb(to);
          // Are enough squares available to drop bishops on opposite colors?
          if (   popcount( DarkSquares & (pieces(us, BISHOP) | remaining)) < count_with_hand(us, BISHOP) / 2
              || popcount(~DarkSquares & (pieces(us, BISHOP) | remaining)) < count_with_hand(us, BISHOP) / 2)
              return false;
      }
      else
          // Drop resulting in same-colored bishops
          if (popcount((DarkSquares & to ? DarkSquares : ~DarkSquares) & pieces(us, BISHOP)) + 1 > (count_with_hand(us, BISHOP) + 1) / 2)
              return false;
  }

  // No legal moves from target square
  if (immobility_illegal() && (type_of(m) == DROP || type_of(m) == NORMAL) && !(PseudoMoves[us][type_of(moved_piece(m))][to] & board_bb()))
      return false;

  // Illegal king passing move
  if (pass_on_stalemate() && is_pass(m) && !checkers())
  {
      for (const auto& move : MoveList<NON_EVASIONS>(*this))
          if (!is_pass(move) && legal(move))
              return false;
  }

  // Check for attacks to pseudo-royal pieces
  if (var->extinctionPseudoRoyal)
  {
      Square kto = to;
      Bitboard occupied = (type_of(m) != DROP ? pieces() ^ from : pieces()) | kto;
      if (type_of(m) == CASTLING)
      {
          // After castling, the rook and king final positions are the same in
          // Chess960 as they would be in standard chess.
          kto = make_square(to > from ? castling_kingside_file() : castling_queenside_file(), castling_rank(us));
          Direction step = kto > from ? EAST : WEST;
          Square rto = kto - step;
          // Pseudo-royal king
          if (st->pseudoRoyals & from)
              for (Square s = from; s != kto; s += step)
                  if (  !(blast_on_capture() && (attacks_bb<KING>(s) & st->pseudoRoyals & pieces(~sideToMove)))
                      && attackers_to(s, pieces() ^ from, ~us))
                      return false;
          occupied = (pieces() ^ from ^ to) | kto | rto;
      }
      if (type_of(m) == EN_PASSANT)
          occupied &= ~square_bb(kto - pawn_push(us));
      if (capture(m) && blast_on_capture())
          occupied &= ~((attacks_bb<KING>(kto) & ((pieces(WHITE) | pieces(BLACK)) ^ pieces(PAWN))) | kto);
      Bitboard pseudoRoyals = st->pseudoRoyals & pieces(sideToMove);
      Bitboard pseudoRoyalsTheirs = st->pseudoRoyals & pieces(~sideToMove);
      if (is_ok(from) && (pseudoRoyals & from))
          pseudoRoyals ^= square_bb(from) ^ kto;
      if (type_of(m) == PROMOTION && extinction_piece_types().find(promotion_type(m)) != extinction_piece_types().end())
          pseudoRoyals |= kto;
      // Self-explosions are illegal
      if (pseudoRoyals & ~occupied)
          return false;
      // Check for legality unless we capture a pseudo-royal piece
      if (!(pseudoRoyalsTheirs & ~occupied))
          while (pseudoRoyals)
          {
              Square sr = pop_lsb(pseudoRoyals);
              // Touching pseudo-royal pieces are immune
              if (  !(blast_on_capture() && (pseudoRoyalsTheirs & attacks_bb<KING>(sr)))
                  && (attackers_to(sr, occupied, ~us) & (occupied & ~square_bb(kto))))
                  return false;
          }
  }

  // Petrifying the king is illegal
  if (var->petrifyOnCapture && capture(m) && type_of(moved_piece(m)) == KING)
      return false;

  // En passant captures are a tricky special case. Because they are rather
  // uncommon, we do it simply by testing whether the king is attacked after
  // the move is made.
  if (type_of(m) == EN_PASSANT && count<KING>(us))
  {
      Square ksq = square<KING>(us);
      Square capsq = to - pawn_push(us);
      Bitboard occupied = (pieces() ^ from ^ capsq) | to;

      assert(to == ep_square());
      assert(moved_piece(m) == make_piece(us, PAWN));
      assert(piece_on(capsq) == make_piece(~us, PAWN));
      assert(piece_on(to) == NO_PIECE);

      return !(attackers_to(ksq, occupied, ~us) & occupied);
  }

  // Castling moves generation does not check if the castling path is clear of
  // enemy attacks, it is delayed at a later time: now!
  if (type_of(m) == CASTLING)
  {
      // After castling, the rook and king final positions are the same in
      // Chess960 as they would be in standard chess.
      to = make_square(to > from ? castling_kingside_file() : castling_queenside_file(), castling_rank(us));
      Direction step = to > from ? WEST : EAST;

      // Will the gate be blocked by king or rook?
      Square rto = to + (to_sq(m) > from_sq(m) ? WEST : EAST);
      if (is_gating(m) && (gating_square(m) == to || gating_square(m) == rto))
          return false;

      // Non-royal pieces can not be impeded from castling
      if (type_of(piece_on(from)) != KING)
          return true;

      for (Square s = to; s != from; s += step)
          if (attackers_to(s, ~us)
              || (var->flyingGeneral && (attacks_bb(~us, ROOK, s, pieces() ^ from) & pieces(~us, KING))))
              return false;

      // In case of Chess960, verify if the Rook blocks some checks
      // For instance an enemy queen in SQ_A1 when castling rook is in SQ_B1.
      return !attackers_to(to, pieces() ^ to_sq(m), ~us);
  }

  Bitboard occupied = (type_of(m) != DROP ? pieces() ^ from : pieces()) | to;

  // Flying general rule and bikjang
  // In case of bikjang passing is always allowed, even when in check
  if (st->bikjang && is_pass(m))
      return true;
  if ((var->flyingGeneral && count<KING>(us)) || st->bikjang)
  {
      Square s = type_of(moved_piece(m)) == KING ? to : square<KING>(us);
      if (attacks_bb(~us, ROOK, s, occupied) & pieces(~us, KING) & ~square_bb(to))
          return false;
  }

  // Makpong rule
  if (var->makpongRule && checkers() && type_of(moved_piece(m)) == KING && (checkers() ^ to))
      return false;

  // If the moving piece is a king, check whether the destination square is
  // attacked by the opponent.
  if (type_of(moved_piece(m)) == KING)
      return !attackers_to(to, occupied, ~us);

  // Return early when without king
  if (!count<KING>(us))
      return true;

  Bitboard janggiCannons = pieces(JANGGI_CANNON);
  if (type_of(moved_piece(m)) == JANGGI_CANNON)
      janggiCannons = (type_of(m) == DROP ? janggiCannons : janggiCannons ^ from) | to;
  else if (janggiCannons & to)
      janggiCannons ^= to;

  // A non-king move is legal if the king is not under attack after the move.
  return !(attackers_to(square<KING>(us), occupied, ~us, janggiCannons) & ~SquareBB[to]);
}


/// Position::pseudo_legal() takes a random move and tests whether the move is
/// pseudo legal. It is used to validate moves from TT that can be corrupted
/// due to SMP concurrent access or hash position key aliasing.

bool Position::pseudo_legal(const Move m) const {

  Color us = sideToMove;
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = moved_piece(m);

  // Illegal moves to squares outside of board or to wall squares
  if (!(board_bb() & to))
      return false;

  // Use a fast check for piece drops
  if (type_of(m) == DROP)
      return   piece_drops()
            && pc != NO_PIECE
            && color_of(pc) == us
            && (can_drop(us, in_hand_piece_type(m)) || (two_boards() && allow_virtual_drop(us, type_of(pc))))
            && (drop_region(us, type_of(pc)) & ~pieces() & to)
            && (   type_of(pc) == in_hand_piece_type(m)
                || (drop_promoted() && type_of(pc) == promoted_piece_type(in_hand_piece_type(m))));

  // Use a slower but simpler function for uncommon cases
  // yet we skip the legality check of MoveList<LEGAL>().
  if (type_of(m) != NORMAL || is_gating(m))
      return checkers() ? MoveList<    EVASIONS>(*this).contains(m)
                        : MoveList<NON_EVASIONS>(*this).contains(m);

  // Illegal wall square placement
  if (wall_gating() && !((board_bb() & ~((pieces() ^ from) | to)) & gating_square(m)))
      return false;
  if (var->arrowGating && !(moves_bb(us, type_of(pc), to, pieces() ^ from) & gating_square(m)))
      return false;
  if (var->pastGating && (from != gating_square(m)))
      return false;
  if (var->staticGating && !(var->staticGatingRegion & gating_square(m)))
      return false;
  
  // Handle the case where a mandatory piece promotion/demotion is not taken
  if (    mandatory_piece_promotion()
      && (is_promoted(from) ? piece_demotion() : promoted_piece_type(type_of(pc)) != NO_PIECE_TYPE)
      && (zone_bb(us, promotion_rank(), max_rank()) & (SquareBB[from] | to))
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
      if (mandatory_pawn_promotion() && rank_of(to) == relative_rank(us, promotion_rank(), max_rank()) && !sittuyin_promotion())
          return false;

      if (   !(pawn_attacks_bb(us, from) & pieces(~us) & to)     // Not a capture
          && !((from + pawn_push(us) == to) && !(pieces() & to)) // Not a single push
          && !(   (from + 2 * pawn_push(us) == to)               // Not a double push
               && (   relative_rank(us, from, max_rank()) <= double_step_rank_max()
                   && relative_rank(us, from, max_rank()) >= double_step_rank_min())
               && !(pieces() & to)
               && !(pieces() & (to - pawn_push(us)))
               && double_step_enabled()))
          return false;
  }
  else if (!((capture(m) ? attacks_from(us, type_of(pc), from) : moves_from(us, type_of(pc), from)) & to))
      return false;

  // Janggi cannon
  if (type_of(pc) == JANGGI_CANNON && (pieces(JANGGI_CANNON) & (between_bb(from, to) | to)))
       return false;

  // Evasions generator already takes care to avoid some kind of illegal moves
  // and legal() relies on this. We therefore have to take care that the same
  // kind of moves are filtered out here.
  if (checkers() && !(checkers() & non_sliding_riders()))
  {
      if (type_of(pc) != KING)
      {
          // Double check? In this case a king move is required
          if (more_than_one(checkers()))
              return false;

          // Our move must be a blocking evasion or a capture of the checking piece
          Square checksq = lsb(checkers());
          if (  !(between_bb(square<KING>(us), lsb(checkers())) & to)
              || ((LeaperAttacks[~us][type_of(piece_on(checksq))][checksq] & square<KING>(us)) && !(checkers() & to)))
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
  if (type_of(m) != PROMOTION && type_of(m) != PIECE_PROMOTION && type_of(m) != PIECE_DEMOTION && type_of(m) != CASTLING
      && !(var->petrifyOnCapture && capture(m) && type_of(moved_piece(m)) != PAWN))
  {
      PieceType pt = type_of(moved_piece(m));
      if (AttackRiderTypes[pt] & (HOPPING_RIDERS | ASYMMETRICAL_RIDERS))
      {
          Bitboard occupied = (type_of(m) != DROP ? pieces() ^ from : pieces()) | to;
          if (attacks_bb(sideToMove, pt, to, occupied) & square<KING>(~sideToMove))
              return true;
      }
      else if (check_squares(pt) & to)
          return true;
  }

  Bitboard janggiCannons = pieces(JANGGI_CANNON);
  if (type_of(moved_piece(m)) == JANGGI_CANNON)
      janggiCannons = (type_of(m) == DROP ? janggiCannons : janggiCannons ^ from) | to;
  else if (janggiCannons & to)
      janggiCannons ^= to;

  // Is there a discovered check?
  if (  ((type_of(m) != DROP && (blockers_for_king(~sideToMove) & from)) || (non_sliding_riders() & pieces(sideToMove)))
      && attackers_to(square<KING>(~sideToMove), (type_of(m) == DROP ? pieces() : pieces() ^ from) | to, sideToMove, janggiCannons))
      return true;

  // Is there a check by gated pieces?
  if (    is_gating(m)
      && attacks_bb(sideToMove, gating_type(m), gating_square(m), (pieces() ^ from) | to) & square<KING>(~sideToMove))
      return true;

  // Petrified piece can't give check
  if (var->petrifyOnCapture && capture(m) && type_of(moved_piece(m)) != PAWN)
      return false;

  // Is there a check by special diagonal moves?
  if (more_than_one(diagonal_lines() & (to | square<KING>(~sideToMove))))
  {
      PieceType pt = type_of(moved_piece(m));
      PieceType diagType = pt == WAZIR ? FERS : pt == SOLDIER ? PAWN : pt == ROOK ? BISHOP : NO_PIECE_TYPE;
      Bitboard occupied = type_of(m) == DROP ? pieces() : pieces() ^ from;
      if (diagType && (attacks_bb(sideToMove, diagType, to, occupied) & square<KING>(~sideToMove)))
          return true;
      else if (pt == JANGGI_CANNON && (  rider_attacks_bb<RIDER_CANNON_DIAG>(to, occupied)
                                       & rider_attacks_bb<RIDER_CANNON_DIAG>(to, occupied & ~janggiCannons)
                                       & square<KING>(~sideToMove)))
          return true;
  }

  switch (type_of(m))
  {
  case NORMAL:
  case DROP:
  case SPECIAL:
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
  case EN_PASSANT:
  {
      Square capsq = make_square(file_of(to), rank_of(from));
      Bitboard b = (pieces() ^ from ^ capsq) | to;

      return attackers_to(square<KING>(~sideToMove), b) & pieces(sideToMove) & b;
  }
  default: //CASTLING
  {
      // Castling is encoded as 'king captures the rook'
      Square kfrom = from;
      Square rfrom = to;
      Square kto = make_square(rfrom > kfrom ? castling_kingside_file() : castling_queenside_file(), castling_rank(sideToMove));
      Square rto = kto + (rfrom > kfrom ? WEST : EAST);

      // Is there a discovered check?
      if (   castling_rank(WHITE) > RANK_1
          && ((blockers_for_king(~sideToMove) & rfrom) || (non_sliding_riders() & pieces(sideToMove)))
          && attackers_to(square<KING>(~sideToMove), (pieces() ^ kfrom ^ rfrom) | rto | kto, sideToMove))
          return true;

      return   (PseudoAttacks[sideToMove][type_of(piece_on(rfrom))][rto] & square<KING>(~sideToMove))
            && (attacks_bb(sideToMove, type_of(piece_on(rfrom)), rto, (pieces() ^ kfrom ^ rfrom) | rto | kto) & square<KING>(~sideToMove));
  }
  }
}

/// Position::do_move() makes a move, and saves all information necessary
/// to a StateInfo object. The move is assumed to be legal. Pseudo-legal
/// moves should be filtered out before this function is called.

void Position::do_move(Move m, StateInfo& newSt, bool givesCheck) {

  assert(is_ok(m));
  assert(&newSt != st);

#ifndef NO_THREADS
  thisThread->nodes.fetch_add(1, std::memory_order_relaxed);
#endif
  Key k = st->key ^ Zobrist::side;

  // Copy some fields of the old state to our new StateInfo object except the
  // ones which are going to be recalculated from scratch anyway and then switch
  // our state pointer to point to the new (ready to be updated) state.
  std::memcpy(static_cast<void*>(&newSt), static_cast<void*>(st), offsetof(StateInfo, key));
  newSt.previous = st;
  st = &newSt;
  st->move = m;

  // Increment ply counters. In particular, rule50 will be reset to zero later on
  // in case of a capture or a pawn move.
  ++gamePly;
  ++st->rule50;
  ++st->pliesFromNull;
  if (st->countingLimit)
      ++st->countingPly;

  // Used by NNUE
  st->accumulator.computed[WHITE] = false;
  st->accumulator.computed[BLACK] = false;
  auto& dp = st->dirtyPiece;
  dp.dirty_num = 1;

  Color us = sideToMove;
  Color them = ~us;
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = moved_piece(m);
  Piece captured = type_of(m) == EN_PASSANT ? make_piece(them, PAWN) : piece_on(to);
  if (to == from)
  {
      assert((type_of(m) == PROMOTION && sittuyin_promotion()) || (is_pass(m) && pass()));
      captured = NO_PIECE;
  }
  st->capturedpromoted = is_promoted(to);
  st->unpromotedCapturedPiece = captured ? unpromoted_piece_on(to) : NO_PIECE;
  st->pass = is_pass(m);

  assert(color_of(pc) == us);
  assert(captured == NO_PIECE || color_of(captured) == (type_of(m) != CASTLING ? them : us));
  assert(type_of(captured) != KING);

  if (check_counting() && givesCheck)
      k ^= Zobrist::checks[us][st->checksRemaining[us]] ^ Zobrist::checks[us][--(st->checksRemaining[us])];

  if (type_of(m) == CASTLING)
  {
      assert(type_of(pc) != NO_PIECE_TYPE);
      assert(captured == make_piece(us, castling_rook_piece()));

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
          if (type_of(m) == EN_PASSANT)
          {
              capsq -= pawn_push(us);

              assert(pc == make_piece(us, PAWN));
              assert(to == st->epSquare);
              assert((var->enPassantRegion & to)
                      && relative_rank(~us, to, max_rank()) <= Rank(double_step_rank_max() + 1)
                      && relative_rank(~us, to, max_rank()) > double_step_rank_min());
              assert(piece_on(to) == NO_PIECE);
              assert(piece_on(capsq) == make_piece(them, PAWN));
          }

          st->pawnKey ^= Zobrist::psq[captured][capsq];
      }
      else
          st->nonPawnMaterial[them] -= PieceValue[MG][captured];

      if (Eval::useNNUE)
      {
          dp.dirty_num = 2;  // 1 piece moved, 1 piece captured
          dp.piece[1] = captured;
          dp.from[1] = capsq;
          dp.to[1] = SQ_NONE;
      }

      // Update board and piece lists
      bool capturedPromoted = is_promoted(capsq);
      Piece unpromotedCaptured = unpromoted_piece_on(capsq);
      remove_piece(capsq);

      if (type_of(m) == EN_PASSANT)
          board[capsq] = NO_PIECE;
      if (captures_to_hand())
      {
          Piece pieceToHand = !capturedPromoted || drop_loop() ? ~captured
                             : unpromotedCaptured ? ~unpromotedCaptured
                                                  : make_piece(~color_of(captured), PAWN);
          add_to_hand(pieceToHand);
          k ^=  Zobrist::inHand[pieceToHand][pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)] - 1]
              ^ Zobrist::inHand[pieceToHand][pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)]];

          if (Eval::useNNUE)
          {
              dp.handPiece[1] = pieceToHand;
              dp.handCount[1] = pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)];
          }
      }
      else if (Eval::useNNUE)
          dp.handPiece[1] = NO_PIECE;

      // Update material hash key and prefetch access to materialTable
      k ^= Zobrist::psq[captured][capsq];
      st->materialKey ^= Zobrist::psq[captured][pieceCount[captured]];
#ifndef NO_THREADS
      prefetch(thisThread->materialTable[st->materialKey]);
#endif
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
  if (type_of(m) != DROP && !is_pass(m) && st->castlingRights && (castlingRightsMask[from] | castlingRightsMask[to]))
  {
      k ^= Zobrist::castling[st->castlingRights];
      st->castlingRights &= ~(castlingRightsMask[from] | castlingRightsMask[to]);
      k ^= Zobrist::castling[st->castlingRights];
  }

  // Flip enclosed pieces
  st->flippedPieces = 0;
  if (flip_enclosed_pieces() && !is_pass(m))
  {
      // Find end of rows to be flipped
      if (flip_enclosed_pieces() == REVERSI)
      {
          Bitboard b = attacks_bb(us, QUEEN, to, board_bb() & ~pieces(~us)) & ~PseudoAttacks[us][KING][to] & pieces(us);
          while(b)
              st->flippedPieces |= between_bb(pop_lsb(b), to) ^ to;
      }
      else
      {
          assert(flip_enclosed_pieces() == ATAXX);
          st->flippedPieces = PseudoAttacks[us][KING][to] & pieces(~us);
      }

      // Flip pieces
      Bitboard to_flip = st->flippedPieces;
      while(to_flip)
      {
          Square s = pop_lsb(to_flip);
          Piece flipped = piece_on(s);
          Piece resulting = ~flipped;

          // remove opponent's piece
          remove_piece(s);
          k ^= Zobrist::psq[flipped][s];
          st->materialKey ^= Zobrist::psq[flipped][pieceCount[flipped]];
          st->nonPawnMaterial[them] -= PieceValue[MG][flipped];

          // add our piece
          put_piece(resulting, s);
          k ^= Zobrist::psq[resulting][s];
          st->materialKey ^= Zobrist::psq[resulting][pieceCount[resulting]-1];
          st->nonPawnMaterial[us] += PieceValue[MG][resulting];
      }
  }

  // Move the piece. The tricky Chess960 castling is handled earlier
  if (type_of(m) == DROP)
  {
      if (Eval::useNNUE)
      {
          // Add drop piece
          dp.piece[0] = pc;
          dp.handPiece[0] = make_piece(us, in_hand_piece_type(m));
          dp.handCount[0] = pieceCountInHand[us][in_hand_piece_type(m)];
          dp.from[0] = SQ_NONE;
          dp.to[0] = to;
      }

      drop_piece(make_piece(us, in_hand_piece_type(m)), pc, to);
      st->materialKey ^= Zobrist::psq[pc][pieceCount[pc]-1];
      if (type_of(pc) != PAWN)
          st->nonPawnMaterial[us] += PieceValue[MG][pc];
      // Set castling rights for dropped king or rook
      if (castling_dropped_piece() && rank_of(to) == castling_rank(us))
      {
          if (type_of(pc) == castling_king_piece() && file_of(to) == castling_king_file())
          {
              st->castlingKingSquare[us] = to;
              Bitboard castling_rooks =  pieces(us, castling_rook_piece())
                                       & rank_bb(castling_rank(us))
                                       & (file_bb(FILE_A) | file_bb(max_file()));
              while (castling_rooks)
                  set_castling_right(us, pop_lsb(castling_rooks));
          }
          else if (type_of(pc) == castling_rook_piece())
          {
              if (   (file_of(to) == FILE_A || file_of(to) == max_file())
                  && piece_on(make_square(castling_king_file(), castling_rank(us))) == make_piece(us, castling_king_piece()))
              {
                  st->castlingKingSquare[us] = make_square(castling_king_file(), castling_rank(us));
                  set_castling_right(us, to);
              }
          }
      }
  }
  else if (type_of(m) != CASTLING)
  {
      if (Eval::useNNUE)
      {
          dp.piece[0] = pc;
          dp.from[0] = from;
          dp.to[0] = to;
      }

      move_piece(from, to);
  }

  // If the moving piece is a pawn do some special extra work
  if (type_of(pc) == PAWN)
  {
      // Set en passant square if the moved pawn can be captured
      if (   type_of(m) != DROP
          && std::abs(int(to) - int(from)) == 2 * NORTH
          && (var->enPassantRegion & (to - pawn_push(us)))
          && (pawn_attacks_bb(us, to - pawn_push(us)) & pieces(them, PAWN))
          && !(wall_gating() && gating_square(m) == to - pawn_push(us)))
      {
          st->epSquare = to - pawn_push(us);
          k ^= Zobrist::enpassant[file_of(st->epSquare)];
      }

      else if (type_of(m) == PROMOTION || type_of(m) == PIECE_PROMOTION)
      {
          Piece promotion = make_piece(us, type_of(m) == PROMOTION ? promotion_type(m) : promoted_piece_type(PAWN));

          assert(relative_rank(us, to, max_rank()) >= promotion_rank() || sittuyin_promotion());
          assert(type_of(promotion) >= KNIGHT && type_of(promotion) < KING);

          remove_piece(to);
          put_piece(promotion, to, true, type_of(m) == PIECE_PROMOTION ? pc : NO_PIECE);

          if (Eval::useNNUE)
          {
              // Promoting pawn to SQ_NONE, promoted piece from SQ_NONE
              dp.to[0] = SQ_NONE;
              dp.handPiece[0] = NO_PIECE;
              dp.piece[dp.dirty_num] = promotion;
              dp.handPiece[dp.dirty_num] = NO_PIECE;
              dp.from[dp.dirty_num] = SQ_NONE;
              dp.to[dp.dirty_num] = to;
              dp.dirty_num++;
          }

          // Update hash keys
          k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[promotion][to];
          st->pawnKey ^= Zobrist::psq[pc][to];
          st->materialKey ^=  Zobrist::psq[promotion][pieceCount[promotion]-1]
                            ^ Zobrist::psq[pc][pieceCount[pc]];

          // Update material
          st->nonPawnMaterial[us] += PieceValue[MG][promotion];
      }

      // Update pawn hash key
      st->pawnKey ^= (type_of(m) != DROP ? Zobrist::psq[pc][from] : 0) ^ Zobrist::psq[pc][to];

      // Reset rule 50 draw counter
      st->rule50 = 0;
  }
  else if (type_of(m) == PIECE_PROMOTION)
  {
      Piece promotion = make_piece(us, promoted_piece_type(type_of(pc)));

      remove_piece(to);
      put_piece(promotion, to, true, pc);

      if (Eval::useNNUE)
      {
          // Promoting piece to SQ_NONE, promoted piece from SQ_NONE
          dp.to[0] = SQ_NONE;
          dp.handPiece[0] = NO_PIECE;
          dp.piece[dp.dirty_num] = promotion;
          dp.handPiece[dp.dirty_num] = NO_PIECE;
          dp.from[dp.dirty_num] = SQ_NONE;
          dp.to[dp.dirty_num] = to;
          dp.dirty_num++;
      }

      // Update hash keys
      k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[promotion][to];
      st->materialKey ^=  Zobrist::psq[promotion][pieceCount[promotion]-1]
                        ^ Zobrist::psq[pc][pieceCount[pc]];

      // Update material
      st->nonPawnMaterial[us] += PieceValue[MG][promotion] - PieceValue[MG][pc];
  }
  else if (type_of(m) == PIECE_DEMOTION)
  {
      Piece demotion = unpromoted_piece_on(to);

      remove_piece(to);
      put_piece(demotion, to);

      if (Eval::useNNUE)
      {
          // Demoting piece to SQ_NONE, demoted piece from SQ_NONE
          dp.to[0] = SQ_NONE;
          dp.handPiece[0] = NO_PIECE;
          dp.piece[dp.dirty_num] = demotion;
          dp.handPiece[dp.dirty_num] = NO_PIECE;
          dp.from[dp.dirty_num] = SQ_NONE;
          dp.to[dp.dirty_num] = to;
          dp.dirty_num++;
      }

      // Update hash keys
      k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[demotion][to];
      st->materialKey ^=  Zobrist::psq[demotion][pieceCount[demotion]-1]
                        ^ Zobrist::psq[pc][pieceCount[pc]];

      // Update material
      st->nonPawnMaterial[us] += PieceValue[MG][demotion] - PieceValue[MG][pc];
  }

  // Set capture piece
  st->capturedPiece = captured;

  // Add gating piece
  if (is_gating(m))
  {
      Square gate = gating_square(m);
      Piece gating_piece = make_piece(us, gating_type(m));

      if (Eval::useNNUE)
      {
          // Add gating piece
          dp.piece[dp.dirty_num] = gating_piece;
          dp.handPiece[dp.dirty_num] = gating_piece;
          dp.handCount[dp.dirty_num] = pieceCountInHand[us][gating_type(m)];
          dp.from[dp.dirty_num] = SQ_NONE;
          dp.to[dp.dirty_num] = gate;
          dp.dirty_num++;
      }

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
      if (seirawan_gating() && count_in_hand(us, ALL_PIECES) == 0 && !captures_to_hand())
          st->gatesBB[us] = 0;
  }

  // Remove king leaping right when aimed by a rook
  if (cambodian_moves() && type_of(pc) == ROOK && (square<KING>(them) & gates(them) & attacks_bb<ROOK>(to)))
      st->gatesBB[them] ^= square<KING>(them);

  // Remove the blast pieces
  if (captured && (blast_on_capture() || var->petrifyOnCapture))
  {
      std::memset(st->unpromotedBycatch, 0, sizeof(st->unpromotedBycatch));
      st->demotedBycatch = st->promotedBycatch = 0;
      Bitboard blast =  blast_on_capture() ? (attacks_bb<KING>(to) & ((pieces(WHITE) | pieces(BLACK)) ^ pieces(PAWN))) | to
                      : type_of(pc) != PAWN ? square_bb(to) : Bitboard(0);
      while (blast)
      {
          Square bsq = pop_lsb(blast);
          Piece bpc = piece_on(bsq);
          Color bc = color_of(bpc);
          if (type_of(bpc) != PAWN)
              st->nonPawnMaterial[bc] -= PieceValue[MG][bpc];

          if (Eval::useNNUE)
          {
              dp.piece[dp.dirty_num] = bpc;
              dp.handPiece[dp.dirty_num] = NO_PIECE;
              dp.from[dp.dirty_num] = bsq;
              dp.to[dp.dirty_num] = SQ_NONE;
              dp.dirty_num++;
          }

          // Update board and piece lists
          // In order to not have to store the values of both board and unpromotedBoard,
          // demote promoted pieces, but keep promoted pawns as promoted,
          // and store demotion/promotion bitboards to disambiguate the piece state
          bool capturedPromoted = is_promoted(bsq);
          Piece unpromotedCaptured = unpromoted_piece_on(bsq);
          st->unpromotedBycatch[bsq] = unpromotedCaptured ? unpromotedCaptured : bpc;
          if (unpromotedCaptured)
              st->demotedBycatch |= bsq;
          else if (capturedPromoted)
              st->promotedBycatch |= bsq;
          remove_piece(bsq);
          board[bsq] = NO_PIECE;
          if (captures_to_hand())
          {
              Piece pieceToHand = !capturedPromoted || drop_loop() ? ~bpc
                                 : unpromotedCaptured ? ~unpromotedCaptured
                                                      : make_piece(~color_of(bpc), PAWN);
              add_to_hand(pieceToHand);
              k ^=  Zobrist::inHand[pieceToHand][pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)] - 1]
                  ^ Zobrist::inHand[pieceToHand][pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)]];

              if (Eval::useNNUE)
              {
                  dp.handPiece[dp.dirty_num - 1] = pieceToHand;
                  dp.handCount[dp.dirty_num - 1] = pieceCountInHand[color_of(pieceToHand)][type_of(pieceToHand)];
              }
          }

          // Update material hash key
          k ^= Zobrist::psq[bpc][bsq];
          st->materialKey ^= Zobrist::psq[bpc][pieceCount[bpc]];
          if (type_of(bpc) == PAWN)
              st->pawnKey ^= Zobrist::psq[bpc][bsq];

          // Update castling rights if needed
          if (st->castlingRights && castlingRightsMask[bsq])
          {
             k ^= Zobrist::castling[st->castlingRights];
             st->castlingRights &= ~castlingRightsMask[bsq];
             k ^= Zobrist::castling[st->castlingRights];
          }

          // Make a wall square where the piece was
          if (var->petrifyOnCapture)
          {
              st->wallSquares |= bsq;
              byTypeBB[ALL_PIECES] |= bsq;
              k ^= Zobrist::wall[bsq];
          }
      }
  }

  // Add gated wall square
  if (wall_gating())
  {
      // Reset wall squares for duck gating
      if (var->duckGating)
      {
          Bitboard b = st->previous->wallSquares;
          byTypeBB[ALL_PIECES] ^= b;
          while (b)
              k ^= Zobrist::wall[pop_lsb(b)];
          st->wallSquares = 0;
      }
      st->wallSquares |= gating_square(m);
      byTypeBB[ALL_PIECES] |= gating_square(m);
      k ^= Zobrist::wall[gating_square(m)];
  }

  // Update the key with the final value
  st->key = k;
  // Calculate checkers bitboard (if move gives check)
  st->checkersBB = givesCheck ? attackers_to(square<KING>(them), us) & pieces(us) : Bitboard(0);

  sideToMove = ~sideToMove;

  if (counting_rule())
  {
      if (counting_rule() != ASEAN_COUNTING && type_of(captured) == PAWN && count<ALL_PIECES>(~sideToMove) == 1 && !count<PAWN>() && count_limit(~sideToMove))
      {
          st->countingLimit = 2 * count_limit(~sideToMove);
          st->countingPly = 2 * count<ALL_PIECES>() - 1;
      }

      if ((!st->countingLimit || ((captured || type_of(m) == PROMOTION) && count<ALL_PIECES>(sideToMove) == 1)) && count_limit(sideToMove))
      {
          st->countingLimit = 2 * count_limit(sideToMove);
          st->countingPly = counting_rule() == ASEAN_COUNTING || count<ALL_PIECES>(sideToMove) > 1 ? 0 : 2 * count<ALL_PIECES>();
      }
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

  assert(type_of(m) == DROP || empty(from) || type_of(m) == CASTLING || is_gating(m)
         || (type_of(m) == PROMOTION && sittuyin_promotion())
         || (is_pass(m) && pass()));
  assert(type_of(st->capturedPiece) != KING);

  // Reset wall squares
  byTypeBB[ALL_PIECES] ^= st->wallSquares ^ st->previous->wallSquares;

  // Add the blast pieces
  if (st->capturedPiece && (blast_on_capture() || var->petrifyOnCapture))
  {
      Bitboard blast = attacks_bb<KING>(to) | to;
      while (blast)
      {
          Square bsq = pop_lsb(blast);
          Piece unpromotedBpc = st->unpromotedBycatch[bsq];
          Piece bpc = st->demotedBycatch & bsq ? make_piece(color_of(unpromotedBpc), promoted_piece_type(type_of(unpromotedBpc)))
                                               : unpromotedBpc;
          bool isPromoted = (st->promotedBycatch | st->demotedBycatch) & bsq;

          // Update board and piece lists
          if (bpc)
          {
              put_piece(bpc, bsq, isPromoted, st->demotedBycatch & bsq ? unpromotedBpc : NO_PIECE);
              if (captures_to_hand())
                  remove_from_hand(!drop_loop() && (st->promotedBycatch & bsq) ? make_piece(~color_of(unpromotedBpc), PAWN)
                                                                               : ~unpromotedBpc);
          }
      }
      // Reset piece since it exploded itself
      pc = piece_on(to);
  }

  // Remove gated piece
  if (is_gating(m))
  {
      Piece gating_piece = make_piece(us, gating_type(m));
      remove_piece(gating_square(m));
      board[gating_square(m)] = NO_PIECE;
      add_to_hand(gating_piece);
      st->gatesBB[us] |= gating_square(m);
  }

  if (type_of(m) == PROMOTION)
  {
      assert(relative_rank(us, to, max_rank()) >= promotion_rank() || sittuyin_promotion());
      assert(type_of(pc) == promotion_type(m));
      assert(type_of(pc) >= KNIGHT && type_of(pc) < KING);

      remove_piece(to);
      pc = make_piece(us, PAWN);
      put_piece(pc, to);
  }
  else if (type_of(m) == PIECE_PROMOTION)
  {
      Piece unpromotedPiece = unpromoted_piece_on(to);
      remove_piece(to);
      pc = unpromotedPiece;
      put_piece(pc, to);
  }
  else if (type_of(m) == PIECE_DEMOTION)
  {
      remove_piece(to);
      Piece unpromotedPc = pc;
      pc = make_piece(us, promoted_piece_type(type_of(pc)));
      put_piece(pc, to, true, unpromotedPc);
  }

  if (type_of(m) == CASTLING)
  {
      Square rfrom, rto;
      do_castling<false>(us, from, to, rfrom, rto);
  }
  else
  {
      if (type_of(m) == DROP)
          undrop_piece(make_piece(us, in_hand_piece_type(m)), to); // Remove the dropped piece
      else
          move_piece(to, from); // Put the piece back at the source square

      if (st->capturedPiece)
      {
          Square capsq = to;

          if (type_of(m) == EN_PASSANT)
          {
              capsq -= pawn_push(us);

              assert(type_of(pc) == PAWN);
              assert(to == st->previous->epSquare);
              assert(relative_rank(~us, to, max_rank()) <= Rank(double_step_rank_max() + 1));
              assert(piece_on(capsq) == NO_PIECE);
              assert(st->capturedPiece == make_piece(~us, PAWN));
          }

          put_piece(st->capturedPiece, capsq, st->capturedpromoted, st->unpromotedCapturedPiece); // Restore the captured piece
          if (captures_to_hand())
              remove_from_hand(!drop_loop() && st->capturedpromoted ? (st->unpromotedCapturedPiece ? ~st->unpromotedCapturedPiece
                                                                                                   : make_piece(~color_of(st->capturedPiece), PAWN))
                                                                    : ~st->capturedPiece);
      }
  }

  if (flip_enclosed_pieces())
  {
      // Flip pieces
      Bitboard to_flip = st->flippedPieces;
      while(to_flip)
      {
          Square s = pop_lsb(to_flip);
          Piece resulting = ~piece_on(s);
          remove_piece(s);
          put_piece(resulting, s);
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
  to = make_square(kingSide ? castling_kingside_file() : castling_queenside_file(), castling_rank(us));
  rto = to + (kingSide ? WEST : EAST);

  Piece castlingKingPiece = piece_on(Do ? from : to);
  Piece castlingRookPiece = piece_on(Do ? rfrom : rto);

  if (Do && Eval::useNNUE)
  {
      auto& dp = st->dirtyPiece;
      dp.piece[0] = castlingKingPiece;
      dp.from[0] = from;
      dp.to[0] = to;
      dp.piece[1] = castlingRookPiece;
      dp.from[1] = rfrom;
      dp.to[1] = rto;
      dp.dirty_num = 2;
  }

  // Remove both pieces first since squares could overlap in Chess960
  remove_piece(Do ? from : to);
  remove_piece(Do ? rfrom : rto);
  board[Do ? from : to] = board[Do ? rfrom : rto] = NO_PIECE; // Since remove_piece doesn't do it for us
  put_piece(castlingKingPiece, Do ? to : from);
  put_piece(castlingRookPiece, Do ? rto : rfrom);
}


/// Position::do_null_move() is used to do a "null move": it flips
/// the side to move without executing any move on the board.

void Position::do_null_move(StateInfo& newSt) {

  assert(!checkers());
  assert(&newSt != st);

  std::memcpy(&newSt, st, offsetof(StateInfo, accumulator));

  newSt.previous = st;
  st = &newSt;

  st->dirtyPiece.dirty_num = 0;
  st->dirtyPiece.piece[0] = NO_PIECE; // Avoid checks in UpdateAccumulator()
  st->accumulator.computed[WHITE] = false;
  st->accumulator.computed[BLACK] = false;

  if (st->epSquare != SQ_NONE)
  {
      st->key ^= Zobrist::enpassant[file_of(st->epSquare)];
      st->epSquare = SQ_NONE;
  }

  st->key ^= Zobrist::side;
  prefetch(TT.first_entry(key()));

  ++st->rule50;
  st->pliesFromNull = 0;

  sideToMove = ~sideToMove;

  set_check_info(st);

  st->repetition = 0;

  assert(pos_is_ok());
}


/// Position::undo_null_move() must be used to undo a "null move"

void Position::undo_null_move() {

  assert(!checkers());

  st = st->previous;
  sideToMove = ~sideToMove;
}


/// Position::key_after() computes the new hash key after the given move. Needed
/// for speculative prefetch. It doesn't recognize special moves like castling,
/// en passant and promotions.

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


Value Position::blast_see(Move m) const {
  assert(is_ok(m));

  Square from = from_sq(m);
  Square to = to_sq(m);
  Color us = color_of(moved_piece(m));
  Bitboard fromto = type_of(m) == DROP ? square_bb(to) : from | to;
  Bitboard blast = ((attacks_bb<KING>(to) & ~pieces(PAWN)) | fromto) & (pieces(WHITE) | pieces(BLACK));

  Value result = VALUE_ZERO;

  // Add the least valuable attacker for quiet moves
  if (!capture(m))
  {
      Bitboard attackers = attackers_to(to, pieces() ^ fromto, ~us);
      Value minAttacker = VALUE_INFINITE;

      while (attackers)
      {
          Square s = pop_lsb(attackers);
          if (extinction_piece_types().find(type_of(piece_on(s))) == extinction_piece_types().end())
              minAttacker = std::min(minAttacker, blast & s ? VALUE_ZERO : CapturePieceValue[MG][piece_on(s)]);
      }

      if (minAttacker == VALUE_INFINITE)
          return VALUE_ZERO;

      result += minAttacker;
      if (type_of(m) == DROP)
          result -= CapturePieceValue[MG][dropped_piece_type(m)];
  }

  // Sum up blast piece values
  while (blast)
  {
      Piece bpc = piece_on(pop_lsb(blast));
      if (extinction_piece_types().find(type_of(bpc)) != extinction_piece_types().end())
          return color_of(bpc) == us ?  extinction_value()
                        : capture(m) ? -extinction_value()
                                     : VALUE_ZERO;
      result += color_of(bpc) == us ? -CapturePieceValue[MG][bpc] : CapturePieceValue[MG][bpc];
  }

  return capture(m) || must_capture() ? result - 1 : std::min(result, VALUE_ZERO);
}


/// Position::see_ge (Static Exchange Evaluation Greater or Equal) tests if the
/// SEE value of move is greater or equal to the given threshold. We'll use an
/// algorithm similar to alpha-beta pruning with a null window.

bool Position::see_ge(Move m, Value threshold) const {

  assert(is_ok(m));

  // Only deal with normal moves, assume others pass a simple SEE
  if (type_of(m) != NORMAL && type_of(m) != DROP && type_of(m) != PIECE_PROMOTION)
      return VALUE_ZERO >= threshold;

  Square from = from_sq(m), to = to_sq(m);

  // nCheck
  if (check_counting() && color_of(moved_piece(m)) == sideToMove && gives_check(m))
      return true;

  // Atomic explosion SEE
  if (blast_on_capture())
      return blast_see(m) >= threshold;

  // Extinction
  if (   extinction_value() != VALUE_NONE
      && piece_on(to)
      && (   (   extinction_piece_types().find(type_of(piece_on(to))) != extinction_piece_types().end()
              && pieceCount[piece_on(to)] == extinction_piece_count() + 1)
          || (   extinction_piece_types().find(ALL_PIECES) != extinction_piece_types().end()
              && count<ALL_PIECES>(~sideToMove) == extinction_piece_count() + 1)))
      return extinction_value() < VALUE_ZERO;

  // Do not evaluate SEE if value would be unreliable
  if (must_capture() || !checking_permitted() || is_gating(m) || count<CLOBBER_PIECE>() == count<ALL_PIECES>())
      return VALUE_ZERO >= threshold;

  int swap = PieceValue[MG][piece_on(to)] - threshold;
  if (swap < 0)
      return false;

  swap = PieceValue[MG][moved_piece(m)] - swap;
  if (swap <= 0)
      return true;

  Bitboard occupied = (type_of(m) != DROP ? pieces() ^ from : pieces()) ^ to;
  Color stm = color_of(moved_piece(m));
  Bitboard attackers = attackers_to(to, occupied);
  Bitboard stmAttackers, bb;
  int res = 1;

  // Flying general rule
  if (var->flyingGeneral)
  {
      if (attackers & pieces(stm, KING))
          attackers |= attacks_bb(stm, ROOK, to, occupied & ~pieces(ROOK)) & pieces(~stm, KING);
      if (attackers & pieces(~stm, KING))
          attackers |= attacks_bb(~stm, ROOK, to, occupied & ~pieces(ROOK)) & pieces(stm, KING);
  }

  // Janggi cannons can not capture each other
  if (type_of(moved_piece(m)) == JANGGI_CANNON && !(attackers & pieces(~stm) & ~pieces(JANGGI_CANNON)))
      attackers &= ~pieces(~stm, JANGGI_CANNON);

  while (true)
  {
      stm = ~stm;
      attackers &= occupied;

      // If stm has no more attackers then give up: stm loses
      if (!(stmAttackers = attackers & pieces(stm)))
          break;

      // Don't allow pinned pieces to attack as long as there are
      // pinners on their original square.
      if (pinners(~stm) & occupied)
          stmAttackers &= ~blockers_for_king(stm);

      // Ignore distant sliders
      if (var->duckGating)
          stmAttackers &= attacks_bb<KING>(to) | ~(pieces(BISHOP, ROOK) | pieces(QUEEN));

      if (!stmAttackers)
          break;

      res ^= 1;

      // Locate and remove the next least valuable attacker, and add to
      // the bitboard 'attackers' any X-ray attackers behind it.
      if ((bb = stmAttackers & pieces(PAWN)))
      {
          if ((swap = PawnValueMg - swap) < res)
              break;

          occupied ^= least_significant_square_bb(bb);
          attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
      }

      else if ((bb = stmAttackers & pieces(KNIGHT)))
      {
          if ((swap = KnightValueMg - swap) < res)
              break;

          occupied ^= least_significant_square_bb(bb);
      }

      else if ((bb = stmAttackers & pieces(BISHOP)))
      {
          if ((swap = BishopValueMg - swap) < res)
              break;

          occupied ^= least_significant_square_bb(bb);
          attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
      }

      else if ((bb = stmAttackers & pieces(ROOK)))
      {
          if ((swap = RookValueMg - swap) < res)
              break;

          occupied ^= least_significant_square_bb(bb);
          attackers |= attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN);
      }

      else if ((bb = stmAttackers & pieces(QUEEN)))
      {
          if ((swap = QueenValueMg - swap) < res)
              break;

          occupied ^= least_significant_square_bb(bb);
          attackers |=  (attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN))
                      | (attacks_bb<ROOK  >(to, occupied) & pieces(ROOK  , QUEEN));
      }

      // fairy pieces
      // pick next piece without considering value
      else if ((bb = stmAttackers & ~pieces(KING)))
      {
          if ((swap = PieceValue[MG][piece_on(lsb(bb))] - swap) < res)
              break;

          occupied ^= lsb(bb);
      }

      else // KING
           // If we "capture" with the king but opponent still has attackers,
           // reverse the result.
          return (attackers & ~pieces(stm)) ? res ^ 1 : res;
  }

  return bool(res);
}

/// Position::is_optinal_game_end() tests whether the position may end the game by
/// 50-move rule, by repetition, or a variant rule that allows a player to claim a game result.

bool Position::is_optional_game_end(Value& result, int ply, int countStarted) const {

  // n-move rule
  if (n_move_rule() && st->rule50 > (2 * n_move_rule() - 1) && (!checkers() || MoveList<LEGAL>(*this).size()))
  {
      int offset = 0;
      if (var->chasingRule == AXF_CHASING && st->pliesFromNull >= 20)
      {
          int end = std::min(st->rule50, st->pliesFromNull);
          StateInfo* stp = st;
          int checkThem = bool(stp->checkersBB);
          int checkUs = bool(stp->previous->checkersBB);
          for (int i = 2; i < end; i += 2)
          {
              stp = stp->previous->previous;
              checkThem += bool(stp->checkersBB);
              checkUs += bool(stp->previous->checkersBB);
          }
          offset = 2 * std::max(std::max(checkThem, checkUs) - 10, 0) + 20 * (CurrentProtocol == UCCI || CurrentProtocol == UCI_CYCLONE);
      }
      if (st->rule50 - offset > (2 * n_move_rule() - 1))
      {
          result = var->materialCounting ? convert_mate_value(material_counting_result(), ply) : VALUE_DRAW;
          return true;
      }
  }

  // n-fold repetition
  if (n_fold_rule())
  {
      int end = captures_to_hand() ? st->pliesFromNull : std::min(st->rule50, st->pliesFromNull);

      if (end >= 4)
      {
          StateInfo* stp = st->previous->previous;
          int cnt = 0;
          bool perpetualThem = st->checkersBB && stp->checkersBB;
          bool perpetualUs = st->previous->checkersBB && stp->previous->checkersBB;
          Bitboard chaseThem = undo_move_board(st->chased, st->previous->move) & stp->chased;
          Bitboard chaseUs = undo_move_board(st->previous->chased, stp->move) & stp->previous->chased;
          int moveRepetition = var->moveRepetitionIllegal
                               && type_of(st->move) == NORMAL
                               && !st->previous->checkersBB && !stp->previous->checkersBB
                               && (board_bb(~side_to_move(), type_of(piece_on(to_sq(st->move)))) & board_bb(side_to_move(), KING))
                               ? (stp->move == reverse_move(st->move) ? 2 : is_pass(stp->move) ? 1 : 0) : 0;

          for (int i = 4; i <= end; i += 2)
          {
              // Janggi repetition rule
              if (moveRepetition > 0)
              {
                  if (i + 1 <= end && stp->previous->previous->previous->checkersBB)
                      moveRepetition = 0;
                  else if (moveRepetition < 4)
                  {
                      if (stp->previous->previous->move == reverse_move((moveRepetition == 1 ? st : stp)->move))
                          moveRepetition++;
                      else
                          moveRepetition = 0;
                  }
                  else
                  {
                      assert(moveRepetition == 4);
                      if (!stp->previous->previous->capturedPiece && from_sq(stp->move) == to_sq(stp->previous->previous->move))
                      {
                          result = VALUE_MATE;
                          return true;
                      }
                      else
                          moveRepetition = 0;
                  }
              }
              // Chased pieces are empty when there is no previous move
              if (i != st->pliesFromNull)
                  chaseThem = undo_move_board(chaseThem, stp->previous->move) & stp->previous->previous->chased;
              stp = stp->previous->previous;
              perpetualThem &= bool(stp->checkersBB);

              // Return a draw score if a position repeats once earlier but strictly
              // after the root, or repeats twice before or at the root.
              if (   stp->key == st->key
                  && ++cnt + 1 == (ply > i && !var->moveRepetitionIllegal ? 2 : n_fold_rule()))
              {
                  result = convert_mate_value(  var->perpetualCheckIllegal && (perpetualThem || perpetualUs) ? (!perpetualUs ? VALUE_MATE : !perpetualThem ? -VALUE_MATE : VALUE_DRAW)
                                              : var->chasingRule && (chaseThem || chaseUs) ? (!chaseUs ? VALUE_MATE : !chaseThem ? -VALUE_MATE : VALUE_DRAW)
                                              : var->nFoldValueAbsolute && sideToMove == BLACK ? -var->nFoldValue
                                              : var->nFoldValue, ply);
                  if (result == VALUE_DRAW && var->materialCounting)
                      result = convert_mate_value(material_counting_result(), ply);
                  return true;
              }

              if (i + 1 <= end)
              {
                  perpetualUs &= bool(stp->previous->checkersBB);
                  chaseUs = undo_move_board(chaseUs, stp->move) & stp->previous->chased;
              }
          }
      }
  }

  // counting rules
  if (   counting_rule()
      && st->countingLimit
      && counting_ply(countStarted) > counting_limit(countStarted)
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

  // Extinction
  // Extinction does not apply for pseudo-royal pieces, because they can not be captured
  if (extinction_value() != VALUE_NONE && (!var->extinctionPseudoRoyal || blast_on_capture()))
  {
      for (Color c : { ~sideToMove, sideToMove })
          for (PieceType pt : extinction_piece_types())
              if (   count_with_hand( c, pt) <= var->extinctionPieceCount
                  && count_with_hand(~c, pt) >= var->extinctionOpponentPieceCount + (extinction_claim() && c == sideToMove))
              {
                  result = c == sideToMove ? extinction_value(ply) : -extinction_value(ply);
                  return true;
              }
  }
  // capture the flag
  if (   capture_the_flag_piece()
      && flag_move()
      && (capture_the_flag(sideToMove) & pieces(sideToMove, capture_the_flag_piece())))
  {
      result =  (capture_the_flag(~sideToMove) & pieces(~sideToMove, capture_the_flag_piece()))
              && sideToMove == WHITE ? VALUE_DRAW : mate_in(ply);
      return true;
  }
  if (   capture_the_flag_piece()
      && (!flag_move() || capture_the_flag_piece() == KING)
      && (capture_the_flag(~sideToMove) & pieces(~sideToMove, capture_the_flag_piece())))
  {
      bool gameEnd = true;
      // Check whether king can move to CTF zone
      if (   flag_move() && sideToMove == BLACK && !checkers() && count<KING>(sideToMove)
          && (capture_the_flag(sideToMove) & attacks_from(sideToMove, KING, square<KING>(sideToMove))))
      {
          assert(capture_the_flag_piece() == KING);
          gameEnd = true;
          for (const auto& m : MoveList<NON_EVASIONS>(*this))
              if (type_of(moved_piece(m)) == KING && (capture_the_flag(sideToMove) & to_sq(m)) && legal(m))
              {
                  gameEnd = false;
                  break;
              }
      }
      if (gameEnd)
      {
          result = mated_in(ply);
          return true;
      }
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
  // Check for bikjang rule (Janggi) and double passing
  if (st->pliesFromNull > 0 && ((st->bikjang && st->previous->bikjang) || (st->pass && st->previous->pass)))
  {
      result = var->materialCounting ? convert_mate_value(material_counting_result(), ply) : VALUE_DRAW;
      return true;
  }
  // Tsume mode: Assume that side with king wins when not in check
  if (tsumeMode && !count<KING>(~sideToMove) && count<KING>(sideToMove) && !checkers())
  {
      result = mate_in(ply);
      return true;
  }
  // Failing to checkmate with virtual pieces is a loss
  if (two_boards() && !checkers())
  {
      int virtualCount = 0;
      for (PieceType pt : piece_types())
          virtualCount += std::max(-count_in_hand(~sideToMove, pt), 0);

      if (virtualCount > 0)
      {
          result = mate_in(ply);
          return true;
      }
  }

  return false;
}

// Position::chased() tests whether the last move was a chase.

Bitboard Position::chased() const {
  Bitboard b = 0;
  if (st->move == MOVE_NONE)
      return b;

  Bitboard pins = blockers_for_king(sideToMove);
  if (var->flyingGeneral)
  {
      Bitboard kingFilePieces = file_bb(file_of(square<KING>(~sideToMove))) & pieces(sideToMove);
      if ((kingFilePieces & pieces(sideToMove, KING)) && !more_than_one(kingFilePieces & ~pieces(KING)))
          pins |= kingFilePieces & ~pieces(KING);
  }
  auto addChased = [&](Square attackerSq, PieceType attackerType, Bitboard attacks) {
      if (attacks & ~b)
      {
          // Exclude attacks on unpromoted soldiers and checks
          attacks &= ~(pieces(sideToMove, KING, SOLDIER) ^ promoted_soldiers(sideToMove));
          // Attacks against stronger pieces
          if (attackerType == HORSE || attackerType == CANNON)
              b |= attacks & pieces(sideToMove, ROOK);
          if (attackerType == ELEPHANT || attackerType == FERS)
              b |= attacks & pieces(sideToMove, ROOK, CANNON, HORSE);
          // Exclude mutual/symmetric attacks
          // Exceptions:
          // - asymmetric pieces ("impaired horse")
          // - pins
          if (attackerType == HORSE && (PseudoAttacks[WHITE][FERS][attackerSq] & pieces()))
          {
              Bitboard horses = attacks & pieces(sideToMove, attackerType);
              while (horses)
              {
                  Square s = pop_lsb(horses);
                  if (attacks_bb(sideToMove, attackerType, s, pieces()) & attackerSq)
                      attacks ^= s;
              }
          }
          else
              attacks &= ~pieces(sideToMove, attackerType) | pins;
          // Attacks against potentially unprotected pieces
          while (attacks)
          {
              Square s = pop_lsb(attacks);
              Bitboard roots = attackers_to(s, pieces() ^ attackerSq, sideToMove) & ~pins;
              if (!roots || (var->flyingGeneral && roots == pieces(sideToMove, KING) && (attacks_bb(sideToMove, ROOK, square<KING>(~sideToMove), pieces() ^ attackerSq) & s)))
                  b |= s;
          }
      }
  };

  // Direct attacks
  Square from = from_sq(st->move);
  Square to = to_sq(st->move);
  PieceType movedPiece = type_of(piece_on(to));
  if (movedPiece != KING && movedPiece != SOLDIER)
  {
      Bitboard directAttacks = attacks_from(~sideToMove, movedPiece, to) & pieces(sideToMove);
      // Only new attacks count. This avoids expensive comparison of previous and new attacks.
      if (movedPiece == ROOK || movedPiece == CANNON)
          directAttacks &= ~line_bb(from, to);
      addChased(to, movedPiece, directAttacks);
  }

  // Discovered attacks
  Bitboard discoveryCandidates =  (PseudoAttacks[WHITE][WAZIR][from] & pieces(~sideToMove, HORSE))
                                | (PseudoAttacks[WHITE][FERS][from] & pieces(~sideToMove, ELEPHANT))
                                | (PseudoAttacks[WHITE][ROOK][from] & pieces(~sideToMove, CANNON, ROOK))
                                | (PseudoAttacks[WHITE][ROOK][to] & pieces(~sideToMove, CANNON));
  while (discoveryCandidates)
  {
      Square s = pop_lsb(discoveryCandidates);
      PieceType discoveryPiece = type_of(piece_on(s));
      Bitboard discoveries =   pieces(sideToMove)
                            &  attacks_bb(~sideToMove, discoveryPiece, s, pieces())
                            & ~attacks_bb(~sideToMove, discoveryPiece, s, (captured_piece() ? pieces() : pieces() ^ to) ^ from);
      addChased(s, discoveryPiece, discoveries);
  }

  // Changes in real roots and discovered checks
  if (st->pliesFromNull > 0)
  {
      // Fake roots
      Bitboard newPins = st->blockersForKing[sideToMove] & ~st->previous->blockersForKing[sideToMove] & pieces(sideToMove);
      while (newPins)
      {
          Square s = pop_lsb(newPins);
          PieceType pinnedPiece = type_of(piece_on(s));
          Bitboard fakeRooted =  pieces(sideToMove)
                               & ~(pieces(sideToMove, KING, SOLDIER) ^ promoted_soldiers(sideToMove))
                               & attacks_bb(sideToMove, pinnedPiece, s, pieces());
          while (fakeRooted)
          {
              Square s2 = pop_lsb(fakeRooted);
              if (attackers_to(s2, ~sideToMove) & ~blockers_for_king(~sideToMove))
                  b |= s2;
          }
      }
      // Discovered checks
      Bitboard newDiscoverers = st->blockersForKing[sideToMove] & ~st->previous->blockersForKing[sideToMove] & pieces(~sideToMove);
      while (newDiscoverers)
      {
          Square s = pop_lsb(newDiscoverers);
          PieceType discoveryPiece = type_of(piece_on(s));
          Bitboard discoveryAttacks = attacks_from(~sideToMove, discoveryPiece, s) & pieces(sideToMove);
          // Include all captures except where the king can pseudo-legally recapture
          b |= discoveryAttacks & ~attacks_from(sideToMove, KING, square<KING>(sideToMove));
          // Include captures where king can not legally recapture
          discoveryAttacks &= attacks_from(sideToMove, KING, square<KING>(sideToMove));
          while (discoveryAttacks)
          {
              Square s2 = pop_lsb(discoveryAttacks);
              if (attackers_to(s2, pieces() ^ s ^ square<KING>(sideToMove), ~sideToMove) & ~square_bb(s))
                  b |= s2;
          }
      }
  }

  return b;
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

  if (end < 3 || var->nFoldValue != VALUE_DRAW || var->perpetualCheckIllegal || var->materialCounting || var->moveRepetitionIllegal || var->duckGating)
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

          if (!((between_bb(s1, s2) ^ s2) & pieces()))
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


/// Position::count_limit() returns the counting limit in full moves.

int Position::count_limit(Color sideToCount) const {

  assert(counting_rule());

  switch (counting_rule())
  {
  case MAKRUK_COUNTING:
      // No counting for side to move
      if (count<PAWN>() || count<ALL_PIECES>(~sideToCount) == 1)
          return 0;
      // Board's honor rule
      if (count<ALL_PIECES>(sideToCount) > 1)
          return 64;
      // Pieces' honor rule
      if (count<ROOK>(~sideToCount) > 1)
          return 8;
      if (count<ROOK>(~sideToCount) == 1)
          return 16;
      if (count<KHON>(~sideToCount) > 1)
          return 22;
      if (count<KNIGHT>(~sideToCount) > 1)
          return 32;
      if (count<KHON>(~sideToCount) == 1)
          return 44;

      return 64;

  case CAMBODIAN_COUNTING:
      // No counting for side to move
      if (count<ALL_PIECES>(sideToCount) > 3 || count<ALL_PIECES>(~sideToCount) == 1)
          return 0;
      // Board's honor rule
      if (count<ALL_PIECES>(sideToCount) > 1)
          return 63;
      // Pieces' honor rule
      if (count<PAWN>())
          return 0;
      if (count<ROOK>(~sideToCount) > 1)
          return 7;
      if (count<ROOK>(~sideToCount) == 1)
          return 15;
      if (count<KHON>(~sideToCount) > 1)
          return 21;
      if (count<KNIGHT>(~sideToCount) > 1)
          return 31;
      if (count<KHON>(~sideToCount) == 1)
          return 43;

      return 63;

  case ASEAN_COUNTING:
      if (count<PAWN>() || count<ALL_PIECES>(sideToCount) > 1)
          return 0;
      if (count<ROOK>(~sideToCount))
          return 16;
      if (count<KHON>(~sideToCount))
          return 44;
      if (count<KNIGHT>(~sideToCount))
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

  for (Rank r = max_rank(); r >= RANK_1; --r) // Piece placement
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
          && relative_rank(~sideToMove, ep_square(), max_rank()) > Rank(double_step_rank_max() + 1)))
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
  ASSERT_ALIGNED(&si, Eval::NNUE::CacheLineSize);

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
      }

  for (Color c : { WHITE, BLACK })
      for (CastlingRights cr : {c & KING_SIDE, c & QUEEN_SIDE})
      {
          if (!can_castle(cr))
              continue;

          if (   piece_on(castlingRookSquare[cr]) != make_piece(c, castling_rook_piece())
              || castlingRightsMask[castlingRookSquare[cr]] != cr
              || (count<KING>(c) && (castlingRightsMask[square<KING>(c)] & cr) != cr))
              assert(0 && "pos_is_ok: Castling");
      }

  return true;
}

} // namespace Stockfish
