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

constexpr Bitboard QueenSide   = FileABB | FileBBB | FileCBB | FileDBB;
constexpr Bitboard CenterFiles = FileCBB | FileDBB | FileEBB | FileFBB;
constexpr Bitboard KingSide    = FileEBB | FileFBB | FileGBB | FileHBB;
constexpr Bitboard Center      = (FileDBB | FileEBB) & (Rank4BB | Rank5BB);

constexpr Bitboard KingFlank[FILE_NB] = {
  QueenSide ^ FileDBB, QueenSide, QueenSide,
  CenterFiles, CenterFiles,
  KingSide, KingSide, KingSide ^ FileEBB
};

extern uint8_t PopCnt16[1 << 16];
extern uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];
extern Bitboard PseudoAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard PseudoMoves[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard LeaperAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard LeaperMoves[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard BoardSizeBB[FILE_NB][RANK_NB];
extern RiderType AttackRiderTypes[PIECE_TYPE_NB];
extern RiderType MoveRiderTypes[PIECE_TYPE_NB];

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

#ifndef LARGEBOARDS
    if (Is64Bit)
#endif
        return unsigned(((occupied & mask) * magic) >> shift);

    unsigned lo = unsigned(occupied) & unsigned(mask);
    unsigned hi = unsigned(occupied >> 32) & unsigned(mask >> 32);
    return (lo * unsigned(magic) ^ hi * unsigned(magic >> 32)) >> shift;
  }
};

extern Magic RookMagicsH[SQUARE_NB];
extern Magic RookMagicsV[SQUARE_NB];
extern Magic BishopMagics[SQUARE_NB];
extern Magic CannonMagicsH[SQUARE_NB];
extern Magic CannonMagicsV[SQUARE_NB];

inline Bitboard square_bb(Square s) {
  assert(s >= SQ_A1 && s <= SQ_MAX);
  return SquareBB[s];
}

/// Overloads of bitwise operators between a Bitboard and a Square for testing
/// whether a given bit is set in a bitboard, and for setting and clearing bits.

inline Bitboard  operator&( Bitboard  b, Square s) { return b &  square_bb(s); }
inline Bitboard  operator|( Bitboard  b, Square s) { return b |  square_bb(s); }
inline Bitboard  operator^( Bitboard  b, Square s) { return b ^  square_bb(s); }
inline Bitboard& operator|=(Bitboard& b, Square s) { return b |= square_bb(s); }
inline Bitboard& operator^=(Bitboard& b, Square s) { return b ^= square_bb(s); }

inline Bitboard  operator-( Bitboard  b, Square s) { return b & ~square_bb(s); }
inline Bitboard& operator-=(Bitboard& b, Square s) { return b &= ~square_bb(s); }

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
  return Rank1BB << (FILE_NB * r);
}

inline Bitboard rank_bb(Square s) {
  return rank_bb(rank_of(s));
}

inline Bitboard file_bb(File f) {
  return FileABB << f;
}

inline Bitboard file_bb(Square s) {
  return file_bb(file_of(s));
}


/// make_bitboard() returns a bitboard from a list of squares

constexpr Bitboard make_bitboard() { return 0; }

template<typename ...Squares>
constexpr Bitboard make_bitboard(Square s, Squares... squares) {
  return (Bitboard(1) << s) | make_bitboard(squares...);
}


/// shift() moves a bitboard one step along direction D

template<Direction D>
constexpr Bitboard shift(Bitboard b) {
  return  D == NORTH      ?  b                       << NORTH      : D == SOUTH      ?  b             >> NORTH
        : D == NORTH+NORTH?  b                       <<(2 * NORTH) : D == SOUTH+SOUTH?  b             >> (2 * NORTH)
        : D == EAST       ? (b & ~file_bb(FILE_MAX)) << EAST       : D == WEST       ? (b & ~FileABB) >> EAST
        : D == NORTH_EAST ? (b & ~file_bb(FILE_MAX)) << NORTH_EAST : D == NORTH_WEST ? (b & ~FileABB) << NORTH_WEST
        : D == SOUTH_EAST ? (b & ~file_bb(FILE_MAX)) >> NORTH_WEST : D == SOUTH_WEST ? (b & ~FileABB) >> NORTH_EAST
        : Bitboard(0);
}


/// shift() moves a bitboard one step along direction D (mainly for pawns)

constexpr Bitboard shift(Direction D, Bitboard b) {
  return  D == NORTH      ?  b                       << NORTH      : D == SOUTH      ?  b             >> NORTH
        : D == NORTH+NORTH?  b                       <<(2 * NORTH) : D == SOUTH+SOUTH?  b             >> (2 * NORTH)
        : D == EAST       ? (b & ~file_bb(FILE_MAX)) << EAST       : D == WEST       ? (b & ~FileABB) >> EAST
        : D == NORTH_EAST ? (b & ~file_bb(FILE_MAX)) << NORTH_EAST : D == NORTH_WEST ? (b & ~FileABB) << NORTH_WEST
        : D == SOUTH_EAST ? (b & ~file_bb(FILE_MAX)) >> NORTH_WEST : D == SOUTH_WEST ? (b & ~FileABB) >> NORTH_EAST
        : Bitboard(0);
}


/// pawn_attacks_bb() returns the squares attacked by pawns of the given color
/// from the squares in the given bitboard.

template<Color C>
constexpr Bitboard pawn_attacks_bb(Bitboard b) {
  return C == WHITE ? shift<NORTH_WEST>(b) | shift<NORTH_EAST>(b)
                    : shift<SOUTH_WEST>(b) | shift<SOUTH_EAST>(b);
}


/// pawn_double_attacks_bb() returns the squares doubly attacked by pawns of the
/// given color from the squares in the given bitboard.

template<Color C>
constexpr Bitboard pawn_double_attacks_bb(Bitboard b) {
  return C == WHITE ? shift<NORTH_WEST>(b) & shift<NORTH_EAST>(b)
                    : shift<SOUTH_WEST>(b) & shift<SOUTH_EAST>(b);
}


/// adjacent_files_bb() returns a bitboard representing all the squares on the
/// adjacent files of the given one.

inline Bitboard adjacent_files_bb(Square s) {
  return shift<EAST>(file_bb(s)) | shift<WEST>(file_bb(s));
}


/// between_bb() returns squares that are linearly between the given squares
/// If the given squares are not on a same file/rank/diagonal, return 0.

inline Bitboard between_bb(Square s1, Square s2) {
  return LineBB[s1][s2] & ( (AllSquares << (s1 +  (s1 < s2)))
                           ^(AllSquares << (s2 + !(s1 < s2))));
}


/// forward_ranks_bb() returns a bitboard representing the squares on the ranks
/// in front of the given one, from the point of view of the given color. For instance,
/// forward_ranks_bb(BLACK, SQ_D3) will return the 16 squares on ranks 1 and 2.

inline Bitboard forward_ranks_bb(Color c, Square s) {
  return c == WHITE ? (AllSquares ^ Rank1BB) << FILE_NB * (rank_of(s) - RANK_1)
                    : (AllSquares ^ rank_bb(RANK_MAX)) >> FILE_NB * (RANK_MAX - rank_of(s));
}

inline Bitboard forward_ranks_bb(Color c, Rank r) {
  return c == WHITE ? (AllSquares ^ Rank1BB) << FILE_NB * (r - RANK_1)
                    : (AllSquares ^ rank_bb(RANK_MAX)) >> FILE_NB * (RANK_MAX - r);
}


/// promotion_zone_bb() returns a bitboard representing the squares on all the ranks
/// in front of and on the given relative rank, from the point of view of the given color.
/// For instance, promotion_zone_bb(BLACK, RANK_7) will return the 16 squares on ranks 1 and 2.

inline Bitboard promotion_zone_bb(Color c, Rank r, Rank maxRank) {
  return forward_ranks_bb(c, relative_rank(c, r, maxRank)) | rank_bb(relative_rank(c, r, maxRank));
}


/// forward_file_bb() returns a bitboard representing all the squares along the
/// line in front of the given one, from the point of view of the given color.

inline Bitboard forward_file_bb(Color c, Square s) {
  return forward_ranks_bb(c, s) & file_bb(s);
}


/// pawn_attack_span() returns a bitboard representing all the squares that can
/// be attacked by a pawn of the given color when it moves along its file,
/// starting from the given square.

inline Bitboard pawn_attack_span(Color c, Square s) {
  return forward_ranks_bb(c, s) & adjacent_files_bb(s);
}


/// passed_pawn_span() returns a bitboard which can be used to test if a pawn of
/// the given color and on the given square is a passed pawn.

inline Bitboard passed_pawn_span(Color c, Square s) {
  return forward_ranks_bb(c, s) & (adjacent_files_bb(s) | file_bb(s));
}


/// aligned() returns true if the squares s1, s2 and s3 are aligned either on a
/// straight or on a diagonal line.

inline bool aligned(Square s1, Square s2, Square s3) {
  return LineBB[s1][s2] & s3;
}


/// distance() functions return the distance between x and y, defined as the
/// number of steps for a king in x to reach y.

template<typename T1 = Square> inline int distance(Square x, Square y);
template<> inline int distance<File>(Square x, Square y) { return std::abs(file_of(x) - file_of(y)); }
template<> inline int distance<Rank>(Square x, Square y) { return std::abs(rank_of(x) - rank_of(y)); }
template<> inline int distance<Square>(Square x, Square y) { return SquareDistance[x][y]; }

template<class T> constexpr const T& clamp(const T& v, const T& lo, const T&  hi) {
  return v < lo ? lo : v > hi ? hi : v;
}


template<RiderType R>
inline Bitboard rider_attacks_bb(Square s, Bitboard occupied) {

  assert(R == RIDER_BISHOP || R == RIDER_ROOK_H || R == RIDER_ROOK_V || R == RIDER_CANNON_H || R == RIDER_CANNON_V);
  const Magic& m =  R == RIDER_ROOK_H ? RookMagicsH[s]
                  : R == RIDER_ROOK_V ? RookMagicsV[s]
                  : R == RIDER_CANNON_H ? CannonMagicsH[s]
                  : R == RIDER_CANNON_V ? CannonMagicsV[s]
                  : BishopMagics[s];
  return m.attacks[m.index(occupied)];
}

/// attacks_bb() returns a bitboard representing all the squares attacked by a
/// piece of type Pt (bishop or rook) placed on 's'.

template<PieceType Pt>
inline Bitboard attacks_bb(Square s, Bitboard occupied) {

  assert(Pt == BISHOP || Pt == ROOK);
  return Pt == BISHOP ? rider_attacks_bb<RIDER_BISHOP>(s, occupied)
                      : rider_attacks_bb<RIDER_ROOK_H>(s, occupied) | rider_attacks_bb<RIDER_ROOK_V>(s, occupied);
}

inline Bitboard attacks_bb(Color c, PieceType pt, Square s, Bitboard occupied) {
  Bitboard b = LeaperAttacks[c][pt][s];
  if (AttackRiderTypes[pt] & RIDER_BISHOP)
      b |= rider_attacks_bb<RIDER_BISHOP>(s, occupied);
  if (AttackRiderTypes[pt] & RIDER_ROOK_H)
      b |= rider_attacks_bb<RIDER_ROOK_H>(s, occupied);
  if (AttackRiderTypes[pt] & RIDER_ROOK_V)
      b |= rider_attacks_bb<RIDER_ROOK_V>(s, occupied);
  if (AttackRiderTypes[pt] & RIDER_ROOK_H)
      b |= rider_attacks_bb<RIDER_ROOK_H>(s, occupied);
  if (AttackRiderTypes[pt] & RIDER_CANNON_V)
      b |= rider_attacks_bb<RIDER_CANNON_V>(s, occupied);
  return b & PseudoAttacks[c][pt][s];
}

inline Bitboard moves_bb(Color c, PieceType pt, Square s, Bitboard occupied) {
  Bitboard b = LeaperMoves[c][pt][s];
  if (MoveRiderTypes[pt] & RIDER_BISHOP)
      b |= rider_attacks_bb<RIDER_BISHOP>(s, occupied);
  if (MoveRiderTypes[pt] & RIDER_ROOK_H)
      b |= rider_attacks_bb<RIDER_ROOK_H>(s, occupied);
  if (MoveRiderTypes[pt] & RIDER_ROOK_V)
      b |= rider_attacks_bb<RIDER_ROOK_V>(s, occupied);
  if (MoveRiderTypes[pt] & RIDER_CANNON_H)
      b |= rider_attacks_bb<RIDER_CANNON_H>(s, occupied);
  if (MoveRiderTypes[pt] & RIDER_CANNON_V)
      b |= rider_attacks_bb<RIDER_CANNON_V>(s, occupied);
  return b & PseudoMoves[c][pt][s];
}


/// popcount() counts the number of non-zero bits in a bitboard

inline int popcount(Bitboard b) {

#ifndef USE_POPCNT

#ifdef LARGEBOARDS
  union { Bitboard bb; uint16_t u[8]; } v = { b };
  return  PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]]
        + PopCnt16[v.u[4]] + PopCnt16[v.u[5]] + PopCnt16[v.u[6]] + PopCnt16[v.u[7]];
#else
  union { Bitboard bb; uint16_t u[4]; } v = { b };
  return PopCnt16[v.u[0]] + PopCnt16[v.u[1]] + PopCnt16[v.u[2]] + PopCnt16[v.u[3]];
#endif

#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)

#ifdef LARGEBOARDS
  return (int)_mm_popcnt_u64(uint64_t(b >> 64)) + (int)_mm_popcnt_u64(uint64_t(b));
#else
  return (int)_mm_popcnt_u64(b);
#endif

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
#ifdef LARGEBOARDS
  if (uint64_t(b))
  {
      _BitScanForward64(&idx, uint64_t(b));
      return Square(idx);
  }
  else
  {
      _BitScanForward64(&idx, uint64_t(b >> 64));
      return Square(idx + 64);
  }
#else
  _BitScanForward64(&idx, b);
  return (Square) idx;
#endif
}

inline Square msb(Bitboard b) {
  assert(b);
  unsigned long idx;
#ifdef LARGEBOARDS
  if (b >> 64)
  {
      _BitScanReverse64(&idx, uint64_t(b >> 64));
      return Square(idx + 64);
  }
  else
  {
      _BitScanReverse64(&idx, uint64_t(b));
      return Square(idx);
  }
#else
  _BitScanReverse64(&idx, b);
  return (Square) idx;
#endif
}

#else  // MSVC, WIN32

inline Square lsb(Bitboard b) {
  assert(b);
  unsigned long idx;

#ifdef LARGEBOARDS
  if (b << 96) {
      _BitScanForward(&idx, uint32_t(b));
      return Square(idx);
  } else if (b << 64) {
      _BitScanForward(&idx, uint32_t(b >> 32));
      return Square(idx + 32);
  } else if (b << 32) {
      _BitScanForward(&idx, uint32_t(b >> 64));
      return Square(idx + 64);
  } else {
      _BitScanForward(&idx, uint32_t(b >> 96));
      return Square(idx + 96);
  }
#else
  if (b & 0xffffffff) {
      _BitScanForward(&idx, uint32_t(b));
      return Square(idx);
  } else {
      _BitScanForward(&idx, uint32_t(b >> 32));
      return Square(idx + 32);
  }
#endif
}

inline Square msb(Bitboard b) {
  assert(b);
  unsigned long idx;

#ifdef LARGEBOARDS
  if (b >> 96) {
      _BitScanReverse(&idx, uint32_t(b >> 96));
      return Square(idx + 96);
  } else if (b >> 64) {
      _BitScanReverse(&idx, uint32_t(b >> 64));
      return Square(idx + 64);
  } else
#endif
  if (b >> 32) {
      _BitScanReverse(&idx, uint32_t(b >> 32));
      return Square(idx + 32);
  } else {
      _BitScanReverse(&idx, uint32_t(b));
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


/// frontmost_sq() returns the most advanced square for the given color
inline Square frontmost_sq(Color c, Bitboard b) {
  return c == WHITE ? msb(b) : lsb(b);
}

#endif // #ifndef BITBOARD_H_INCLUDED
