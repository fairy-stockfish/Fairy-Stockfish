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

#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED

#include <string>

#include "types.h"

namespace Bitbases {

void init();
bool probe(Square wksq, Square wpsq, Square bksq, Color us);

}

namespace Bitboards {

void init();
const std::string pretty(Bitboard b);

}

#ifdef LARGEBOARDS
constexpr Bitboard AllSquares = ((~Bitboard(0)) >> 8);
#else
constexpr Bitboard AllSquares = ~Bitboard(0);
#endif
#ifdef LARGEBOARDS
constexpr Bitboard DarkSquares = (Bitboard(0xAAA555AAA555AAULL) << 64) ^ Bitboard(0xA555AAA555AAA555ULL);
#else
constexpr Bitboard DarkSquares = 0xAA55AA55AA55AA55ULL;
#endif

#ifdef LARGEBOARDS
constexpr Bitboard FileABB = (Bitboard(0x00100100100100ULL) << 64) ^ Bitboard(0x1001001001001001ULL);
#else
constexpr Bitboard FileABB = 0x0101010101010101ULL;
#endif
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;
#ifdef LARGEBOARDS
constexpr Bitboard FileIBB = FileABB << 8;
constexpr Bitboard FileJBB = FileABB << 9;
constexpr Bitboard FileKBB = FileABB << 10;
constexpr Bitboard FileLBB = FileABB << 11;
#endif


#ifdef LARGEBOARDS
constexpr Bitboard Rank1BB = 0xFFF;
#else
constexpr Bitboard Rank1BB = 0xFF;
#endif
constexpr Bitboard Rank2BB = Rank1BB << (FILE_NB * 1);
constexpr Bitboard Rank3BB = Rank1BB << (FILE_NB * 2);
constexpr Bitboard Rank4BB = Rank1BB << (FILE_NB * 3);
constexpr Bitboard Rank5BB = Rank1BB << (FILE_NB * 4);
constexpr Bitboard Rank6BB = Rank1BB << (FILE_NB * 5);
constexpr Bitboard Rank7BB = Rank1BB << (FILE_NB * 6);
constexpr Bitboard Rank8BB = Rank1BB << (FILE_NB * 7);
#ifdef LARGEBOARDS
constexpr Bitboard Rank9BB = Rank1BB << (FILE_NB * 8);
constexpr Bitboard Rank10BB = Rank1BB << (FILE_NB * 9);
#endif

extern int SquareDistance[SQUARE_NB][SQUARE_NB];

extern Bitboard BoardSizeBB[FILE_NB][RANK_NB];
extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard FileBB[FILE_NB];
extern Bitboard RankBB[RANK_NB];
extern Bitboard AdjacentFilesBB[FILE_NB];
extern Bitboard ForwardRanksBB[COLOR_NB][RANK_NB];
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];
extern Bitboard DistanceRingBB[SQUARE_NB][FILE_NB];
extern Bitboard ForwardFileBB[COLOR_NB][SQUARE_NB];
extern Bitboard PassedPawnMask[COLOR_NB][SQUARE_NB];
extern Bitboard PawnAttackSpan[COLOR_NB][SQUARE_NB];
extern Bitboard PseudoAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard PseudoMoves[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard LeaperAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard LeaperMoves[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];

#ifdef LARGEBOARDS
int popcount(Bitboard b); // required for 128 bit pext
#endif

/// Magic holds all magic bitboards relevant data for a single square
struct Magic {
  Bitboard  mask;
  Bitboard  magic;
  Bitboard* attacks;
  unsigned  shift;

  // Compute the attack's index using the 'magic bitboards' approach
  unsigned index(Bitboard occupied) const {

    if (HasPext)
        return unsigned(pext(occupied, mask));

    if (Is64Bit)
        return unsigned(((occupied & mask) * magic) >> shift);

    unsigned lo = unsigned(occupied) & unsigned(mask);
    unsigned hi = unsigned(occupied >> 32) & unsigned(mask >> 32);
    return (lo * unsigned(magic) ^ hi * unsigned(magic >> 32)) >> shift;
  }
};

extern Magic RookMagics[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];


/// Overloads of bitwise operators between a Bitboard and a Square for testing
/// whether a given bit is set in a bitboard, and for setting and clearing bits.

inline Bitboard operator&(Bitboard b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b & SquareBB[s];
}

inline Bitboard operator|(Bitboard b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b | SquareBB[s];
}

inline Bitboard operator^(Bitboard b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b ^ SquareBB[s];
}

inline Bitboard operator-(Bitboard b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b & ~SquareBB[s];
}

inline Bitboard& operator|=(Bitboard& b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b |= SquareBB[s];
}

inline Bitboard& operator^=(Bitboard& b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b ^= SquareBB[s];
}

inline Bitboard& operator-=(Bitboard& b, Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return b &= ~SquareBB[s];
}

constexpr bool more_than_one(Bitboard b) {
  return b & (b - 1);
}

/// board_size_bb() returns a bitboard representing all the squares
/// on a board with given size.

inline Bitboard board_size_bb(File f, Rank r) {
  return BoardSizeBB[f][r];
}

inline bool opposite_colors(Square s1, Square s2) {
  return bool(DarkSquares & s1) != bool(DarkSquares & s2);
}

/// rank_bb() and file_bb() return a bitboard representing all the squares on
/// the given file or rank.

inline Bitboard rank_bb(Rank r) {
  return RankBB[r];
}

inline Bitboard rank_bb(Square s) {
  return RankBB[rank_of(s)];
}

inline Bitboard file_bb(File f) {
  return FileBB[f];
}

inline Bitboard file_bb(Square s) {
  return FileBB[file_of(s)];
}


/// make_bitboard() returns a bitboard from a list of squares

constexpr Bitboard make_bitboard() { return 0; }

template<typename ...Squares>
constexpr Bitboard make_bitboard(Square s, Squares... squares) {
  return (Bitboard(1) << s) | make_bitboard(squares...);
}


/// shift() moves a bitboard one step along direction D (mainly for pawns)

template<Direction D>
constexpr Bitboard shift(Bitboard b) {
  return  D == NORTH      ?  b                       << NORTH      : D == SOUTH      ?  b             >> NORTH
        : D == EAST       ? (b & ~file_bb(FILE_MAX)) << EAST       : D == WEST       ? (b & ~FileABB) >> EAST
        : D == NORTH_EAST ? (b & ~file_bb(FILE_MAX)) << NORTH_EAST : D == NORTH_WEST ? (b & ~FileABB) << NORTH_WEST
        : D == SOUTH_EAST ? (b & ~file_bb(FILE_MAX)) >> NORTH_WEST : D == SOUTH_WEST ? (b & ~FileABB) >> NORTH_EAST
        : 0;
}


/// shift() moves a bitboard one step along direction D (mainly for pawns)

constexpr Bitboard shift(Direction D, Bitboard b) {
  return  D == NORTH      ?  b                       << NORTH      : D == SOUTH      ?  b             >> NORTH
        : D == EAST       ? (b & ~file_bb(FILE_MAX)) << EAST       : D == WEST       ? (b & ~FileABB) >> EAST
        : D == NORTH_EAST ? (b & ~file_bb(FILE_MAX)) << NORTH_EAST : D == NORTH_WEST ? (b & ~FileABB) << NORTH_WEST
        : D == SOUTH_EAST ? (b & ~file_bb(FILE_MAX)) >> NORTH_WEST : D == SOUTH_WEST ? (b & ~FileABB) >> NORTH_EAST
        : 0;
}


/// pawn_attacks_bb() returns the pawn attacks for the given color from the
/// squares in the given bitboard.

template<Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
  return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                    : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}


/// double_pawn_attacks_bb() returns the pawn attacks for the given color
/// from the squares in the given bitboard.

template<Color C>
constexpr Bitboard double_pawn_attacks_bb(Bitboard b) {
  return C == WHITE ? shift<NORTH_WEST>(b) & shift<NORTH_EAST>(b)
                    : shift<SOUTH_WEST>(b) & shift<SOUTH_EAST>(b);
}


/// adjacent_files_bb() returns a bitboard representing all the squares on the
/// adjacent files of the given one.

inline Bitboard adjacent_files_bb(File f) {
  return AdjacentFilesBB[f];
}


/// between_bb() returns a bitboard representing all the squares between the two
/// given ones. For instance, between_bb(SQ_C4, SQ_F7) returns a bitboard with
/// the bits for square d5 and e6 set. If s1 and s2 are not on the same rank, file
/// or diagonal, 0 is returned.

inline Bitboard between_bb(Square s1, Square s2) {
  return BetweenBB[s1][s2];
}


/// forward_ranks_bb() returns a bitboard representing the squares on all the ranks
/// in front of the given one, from the point of view of the given color. For instance,
/// forward_ranks_bb(BLACK, SQ_D3) will return the 16 squares on ranks 1 and 2.

inline Bitboard forward_ranks_bb(Color c, Rank r) {
  return ForwardRanksBB[c][r];
}

inline Bitboard forward_ranks_bb(Color c, Square s) {
  return ForwardRanksBB[c][rank_of(s)];
}


/// promotion_zone_bb() returns a bitboard representing the squares on all the ranks
/// in front of and on the given relative rank, from the point of view of the given color.
/// For instance, promotion_zone_bb(BLACK, RANK_7) will return the 16 squares on ranks 1 and 2.

inline Bitboard promotion_zone_bb(Color c, Rank r, Rank maxRank) {
  return ForwardRanksBB[c][relative_rank(c, r, maxRank)] | rank_bb(relative_rank(c, r, maxRank));
}


/// forward_file_bb() returns a bitboard representing all the squares along the line
/// in front of the given one, from the point of view of the given color:
///      ForwardFileBB[c][s] = forward_ranks_bb(c, s) & file_bb(s)

inline Bitboard forward_file_bb(Color c, Square s) {
  return ForwardFileBB[c][s];
}


/// pawn_attack_span() returns a bitboard representing all the squares that can be
/// attacked by a pawn of the given color when it moves along its file, starting
/// from the given square:
///      PawnAttackSpan[c][s] = forward_ranks_bb(c, s) & adjacent_files_bb(file_of(s));

inline Bitboard pawn_attack_span(Color c, Square s) {
  return PawnAttackSpan[c][s];
}


/// passed_pawn_mask() returns a bitboard mask which can be used to test if a
/// pawn of the given color and on the given square is a passed pawn:
///      PassedPawnMask[c][s] = pawn_attack_span(c, s) | forward_file_bb(c, s)

inline Bitboard passed_pawn_mask(Color c, Square s) {
  return PassedPawnMask[c][s];
}


/// aligned() returns true if the squares s1, s2 and s3 are aligned either on a
/// straight or on a diagonal line.

inline bool aligned(Square s1, Square s2, Square s3) {
  return LineBB[s1][s2] & s3;
}


/// distance() functions return the distance between x and y, defined as the
/// number of steps for a king in x to reach y. Works with squares, ranks, files.

template<typename T> inline int distance(T x, T y) { return x < y ? y - x : x - y; }
template<> inline int distance<Square>(Square x, Square y) { return SquareDistance[x][y]; }

template<typename T1, typename T2> inline int distance(T2 x, T2 y);
template<> inline int distance<File>(Square x, Square y) { return distance(file_of(x), file_of(y)); }
template<> inline int distance<Rank>(Square x, Square y) { return distance(rank_of(x), rank_of(y)); }


/// attacks_bb() returns a bitboard representing all the squares attacked by a
/// piece of type Pt (bishop or rook) placed on 's'.

template<PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occupied) {

  assert(Pt == BISHOP || Pt == ROOK);
  const Magic& m = Pt == ROOK ? RookMagics[s] : BishopMagics[s];
  return m.attacks[m.index(occupied)];
}

inline Bitboard attacks_bb(Color c, PieceType pt, Square s, Bitboard occupied) {
  return LeaperAttacks[c][pt][s] | (PseudoAttacks[c][pt][s] & (attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied)));
}

inline Bitboard moves_bb(Color c, PieceType pt, Square s, Bitboard occupied) {
  return LeaperMoves[c][pt][s] | (PseudoMoves[c][pt][s] & (attacks_bb<BISHOP>(s, occupied) | attacks_bb<ROOK>(s, occupied)));
}


/// popcount() counts the number of non-zero bits in a bitboard

inline int popcount(Bitboard b) {

#ifndef USE_POPCNT

  extern uint8_t PopCnt16[1 << 16];
#ifdef LARGEBOARDS
  union { Bitboard bb; uint16_t u[8]; } v = { b };
  return  PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]]
        + PopCnt16[v.u[4]] + PopCnt16[v.u[5]] + PopCnt16[v.u[6]] + PopCnt16[v.u[7]];
#else
  union { Bitboard bb; uint16_t u[4]; } v = { b };
  return PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]];
#endif

#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)

  return (int)_mm_popcnt_u64(b);

#else // Assumed gcc or compatible compiler

#ifdef LARGEBOARDS
  return __builtin_popcountll(b >> 64) + __builtin_popcountll(b);
#else
  return __builtin_popcountll(b);
#endif

#endif
}


/// lsb() and msb() return the least/most significant bit in a non-zero bitboard

#if defined(__GNUC__)  // GCC, Clang, ICC

inline Square lsb(Bitboard b) {
  assert(b);
#ifdef LARGEBOARDS
  if (!(b << 64))
      return Square(__builtin_ctzll(b >> 64) + 64);
#endif
  return Square(__builtin_ctzll(b));
}

inline Square msb(Bitboard b) {
  assert(b);
#ifdef LARGEBOARDS
  if (b >> 64)
      return Square(SQUARE_BIT_MASK ^ __builtin_clzll(b >> 64));
  return Square(SQUARE_BIT_MASK ^ (__builtin_clzll(b) + 64));
#else
  return Square(SQUARE_BIT_MASK ^ __builtin_clzll(b));
#endif
}

#elif defined(_MSC_VER)  // MSVC

#ifdef _WIN64  // MSVC, WIN64

inline Square lsb(Bitboard b) {
  assert(b);
  unsigned long idx;
  _BitScanForward64(&idx, b);
  return (Square) idx;
}

inline Square msb(Bitboard b) {
  assert(b);
  unsigned long idx;
  _BitScanReverse64(&idx, b);
  return (Square) idx;
}

#else  // MSVC, WIN32

inline Square lsb(Bitboard b) {
  assert(b);
  unsigned long idx;

  if (b & 0xffffffff) {
      _BitScanForward(&idx, int32_t(b));
      return Square(idx);
  } else {
      _BitScanForward(&idx, int32_t(b >> 32));
      return Square(idx + 32);
  }
}

inline Square msb(Bitboard b) {
  assert(b);
  unsigned long idx;

  if (b >> 32) {
      _BitScanReverse(&idx, int32_t(b >> 32));
      return Square(idx + 32);
  } else {
      _BitScanReverse(&idx, int32_t(b));
      return Square(idx);
  }
}

#endif

#else  // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif


/// pop_lsb() finds and clears the least significant bit in a non-zero bitboard

inline Square pop_lsb(Bitboard* b) {
  const Square s = lsb(*b);
  *b &= *b - 1;
  return s;
}


/// frontmost_sq() and backmost_sq() return the square corresponding to the
/// most/least advanced bit relative to the given color.

inline Square frontmost_sq(Color c, Bitboard b) { return c == WHITE ? msb(b) : lsb(b); }
inline Square  backmost_sq(Color c, Bitboard b) { return c == WHITE ? lsb(b) : msb(b); }

#endif // #ifndef BITBOARD_H_INCLUDED
