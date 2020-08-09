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

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

/// When compiling with provided Makefile (e.g. for Linux and OSX), configuration
/// is done automatically. To get started type 'make help'.
///
/// When Makefile is not used (e.g. with Microsoft Visual Studio) some switches
/// need to be set manually:
///
/// -DNDEBUG      | Disable debugging mode. Always use this for release.
///
/// -DNO_PREFETCH | Disable use of prefetch asm-instruction. You may need this to
///               | run on some very old machines.
///
/// -DUSE_POPCNT  | Add runtime support for use of popcnt asm-instruction. Works
///               | only in 64-bit mode and requires hardware with popcnt support.
///
/// -DUSE_PEXT    | Add runtime support for use of pext asm-instruction. Works
///               | only in 64-bit mode and requires hardware with pext support.

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#if defined(_MSC_VER)
// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#pragma comment(linker, "/STACK:8000000") // Use 8 MB stack size for MSVC
#endif

/// Predefined macros hell:
///
/// __GNUC__           Compiler is gcc, Clang or Intel on Linux
/// __INTEL_COMPILER   Compiler is Intel
/// _MSC_VER           Compiler is MSVC or Intel on Windows
/// _WIN32             Building on Windows (any)
/// _WIN64             Building on Windows 64 bit

#if defined(_WIN64) && defined(_MSC_VER) // No Makefile used
#  include <intrin.h> // Microsoft header for _BitScanForward64()
#  define IS_64BIT
#endif

#if defined(USE_POPCNT) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <nmmintrin.h> // Intel and Microsoft header for _mm_popcnt_u64()
#endif

#if !defined(NO_PREFETCH) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <xmmintrin.h> // Intel and Microsoft header for _mm_prefetch()
#endif

#if defined(USE_PEXT)
#  include <immintrin.h> // Header for _pext_u64() intrinsic
#  ifdef LARGEBOARDS
#    define pext(b, m) (_pext_u64(b, m) ^ (_pext_u64(b >> 64, m >> 64) << popcount((m << 64) >> 64)))
#  else
#    define pext(b, m) _pext_u64(b, m)
#  endif
#else
#  define pext(b, m) 0
#endif

#ifdef USE_POPCNT
constexpr bool HasPopCnt = true;
#else
constexpr bool HasPopCnt = false;
#endif

#ifdef USE_PEXT
constexpr bool HasPext = true;
#else
constexpr bool HasPext = false;
#endif

#ifdef IS_64BIT
constexpr bool Is64Bit = true;
#else
constexpr bool Is64Bit = false;
#endif

typedef uint64_t Key;
#ifdef LARGEBOARDS
#if defined(__GNUC__) && defined(IS_64BIT)
typedef unsigned __int128 Bitboard;
#else
struct Bitboard {
    uint64_t b64[2];

    constexpr Bitboard() : b64 {0, 0} {}
    constexpr Bitboard(uint64_t i) : b64 {0, i} {}
    constexpr Bitboard(uint64_t hi, uint64_t lo) : b64 {hi, lo} {};

    constexpr operator bool() const {
        return b64[0] || b64[1];
    }

    constexpr operator long long unsigned () const {
        return b64[1];
    }

    constexpr operator unsigned() const {
        return b64[1];
    }

    constexpr Bitboard operator << (const unsigned int bits) const {
        return Bitboard(  bits >= 64 ? b64[1] << (bits - 64)
                        : bits == 0  ? b64[0]
                        : ((b64[0] << bits) | (b64[1] >> (64 - bits))),
                        bits >= 64 ? 0 : b64[1] << bits);
    }

    constexpr Bitboard operator >> (const unsigned int bits) const {
        return Bitboard(bits >= 64 ? 0 : b64[0] >> bits,
                          bits >= 64 ? b64[0] >> (bits - 64)
                        : bits == 0  ? b64[1]
                        : ((b64[1] >> bits) | (b64[0] << (64 - bits))));
    }

    constexpr Bitboard operator << (const int bits) const {
        return *this << unsigned(bits);
    }

    constexpr Bitboard operator >> (const int bits) const {
        return *this >> unsigned(bits);
    }

    constexpr bool operator == (const Bitboard y) const {
        return (b64[0] == y.b64[0]) && (b64[1] == y.b64[1]);
    }

    constexpr bool operator != (const Bitboard y) const {
        return !(*this == y);
    }

    inline Bitboard& operator |=(const Bitboard x) {
        b64[0] |= x.b64[0];
        b64[1] |= x.b64[1];
        return *this;
    }
    inline Bitboard& operator &=(const Bitboard x) {
        b64[0] &= x.b64[0];
        b64[1] &= x.b64[1];
        return *this;
    }
    inline Bitboard& operator ^=(const Bitboard x) {
        b64[0] ^= x.b64[0];
        b64[1] ^= x.b64[1];
        return *this;
    }

    constexpr Bitboard operator ~ () const {
        return Bitboard(~b64[0], ~b64[1]);
    }

    constexpr Bitboard operator | (const Bitboard x) const {
        return Bitboard(b64[0] | x.b64[0], b64[1] | x.b64[1]);
    }

    constexpr Bitboard operator & (const Bitboard x) const {
        return Bitboard(b64[0] & x.b64[0], b64[1] & x.b64[1]);
    }

    constexpr Bitboard operator ^ (const Bitboard x) const {
        return Bitboard(b64[0] ^ x.b64[0], b64[1] ^ x.b64[1]);
    }

    constexpr Bitboard operator - (const Bitboard x) const {
        return Bitboard(b64[0] - x.b64[0] - (b64[1] < x.b64[1]), b64[1] - x.b64[1]);
    }

    constexpr Bitboard operator - (const int x) const {
        return *this - Bitboard(x);
    }

    inline Bitboard operator * (const Bitboard x) const {
        uint64_t a_lo = (uint32_t)b64[1];
        uint64_t a_hi = b64[1] >> 32;
        uint64_t b_lo = (uint32_t)x.b64[1];
        uint64_t b_hi = x.b64[1] >> 32;

        uint64_t t1 = (a_hi * b_lo) + ((a_lo * b_lo) >> 32);
        uint64_t t2 = (a_lo * b_hi) + (t1 & 0xFFFFFFFF);

        return Bitboard(b64[0] * x.b64[1] + b64[1] * x.b64[0] + (a_hi * b_hi) + (t1 >> 32) + (t2 >> 32),
                        (t2 << 32) + (a_lo * b_lo & 0xFFFFFFFF));
   }
};
#endif
constexpr int SQUARE_BITS = 7;
#else
typedef uint64_t Bitboard;
constexpr int SQUARE_BITS = 6;
#endif

constexpr int MAX_MOVES = 1024;
constexpr int MAX_PLY   = 246;

/// A move needs 16 bits to be stored
///
/// bit  0- 5: destination square (from 0 to 63)
/// bit  6-11: origin square (from 0 to 63)
/// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
/// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
/// NOTE: EN-PASSANT bit is set only when a pawn can be captured
///
/// Special cases are MOVE_NONE and MOVE_NULL. We can sneak these in because in
/// any normal move destination square is always different from origin square
/// while MOVE_NONE and MOVE_NULL have the same origin and destination square.

enum Move : int {
  MOVE_NONE,
  MOVE_NULL = 1 + (1 << SQUARE_BITS)
};

enum MoveType : int {
  NORMAL,
  ENPASSANT          = 1 << (2 * SQUARE_BITS),
  CASTLING           = 2 << (2 * SQUARE_BITS),
  PROMOTION          = 3 << (2 * SQUARE_BITS),
  DROP               = 4 << (2 * SQUARE_BITS),
  PIECE_PROMOTION    = 5 << (2 * SQUARE_BITS),
  PIECE_DEMOTION     = 6 << (2 * SQUARE_BITS),
  SPECIAL            = 7 << (2 * SQUARE_BITS),
};

constexpr int MOVE_TYPE_BITS = 4;

enum Color {
  WHITE, BLACK, COLOR_NB = 2
};

enum CastlingRights {
  NO_CASTLING,
  WHITE_OO,
  WHITE_OOO = WHITE_OO << 1,
  BLACK_OO  = WHITE_OO << 2,
  BLACK_OOO = WHITE_OO << 3,

  KING_SIDE      = WHITE_OO  | BLACK_OO,
  QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
  WHITE_CASTLING = WHITE_OO  | WHITE_OOO,
  BLACK_CASTLING = BLACK_OO  | BLACK_OOO,
  ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING,

  CASTLING_RIGHT_NB = 16
};

enum CheckCount : int {
  CHECKS_0 = 0, CHECKS_NB = 11
};

enum MaterialCounting {
  NO_MATERIAL_COUNTING, JANGGI_MATERIAL, UNWEIGHTED_MATERIAL, WHITE_DRAW_ODDS, BLACK_DRAW_ODDS
};

enum CountingRule {
  NO_COUNTING, MAKRUK_COUNTING, ASEAN_COUNTING
};

enum EnclosingRule {
  NO_ENCLOSING, REVERSI, ATAXX
};

enum Phase {
  PHASE_ENDGAME,
  PHASE_MIDGAME = 128,
  MG = 0, EG = 1, PHASE_NB = 2
};

enum ScaleFactor {
  SCALE_FACTOR_DRAW    = 0,
  SCALE_FACTOR_NORMAL  = 64,
  SCALE_FACTOR_MAX     = 128,
  SCALE_FACTOR_NONE    = 255
};

enum Bound {
  BOUND_NONE,
  BOUND_UPPER,
  BOUND_LOWER,
  BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Value : int {
  VALUE_ZERO      = 0,
  VALUE_DRAW      = 0,
  VALUE_KNOWN_WIN = 10000,
  VALUE_MATE      = 32000,
  XBOARD_VALUE_MATE = 200000,
  VALUE_INFINITE  = 32001,
  VALUE_NONE      = 32002,

  VALUE_TB_WIN_IN_MAX_PLY  =  VALUE_MATE - 2 * MAX_PLY,
  VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY,
  VALUE_MATE_IN_MAX_PLY  =  VALUE_MATE - MAX_PLY,
  VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY,

  PawnValueMg   = 128,   PawnValueEg   = 213,
  KnightValueMg = 781,   KnightValueEg = 854,
  BishopValueMg = 825,   BishopValueEg = 915,
  RookValueMg   = 1276,  RookValueEg   = 1380,
  QueenValueMg  = 2538,  QueenValueEg  = 2682,
  FersValueMg              = 420,   FersValueEg              = 450,
  AlfilValueMg             = 350,   AlfilValueEg             = 330,
  FersAlfilValueMg         = 700,   FersAlfilValueEg         = 650,
  SilverValueMg            = 630,   SilverValueEg            = 630,
  AiwokValueMg             = 2300,  AiwokValueEg             = 2700,
  BersValueMg              = 2000,  BersValueEg              = 2000,
  ArchbishopValueMg        = 2200,  ArchbishopValueEg        = 2200,
  ChancellorValueMg        = 2300,  ChancellorValueEg        = 2600,
  AmazonValueMg            = 2700,  AmazonValueEg            = 2850,
  KnibisValueMg            = 1100,  KnibisValueEg            = 1200,
  BiskniValueMg            = 750,   BiskniValueEg            = 700,
  KnirooValueMg            = 1050,  KnirooValueEg            = 1250,
  RookniValueMg            = 800,   RookniValueEg            = 950,
  ShogiPawnValueMg         =  90,   ShogiPawnValueEg         = 100,
  LanceValueMg             = 350,   LanceValueEg             = 250,
  ShogiKnightValueMg       = 350,   ShogiKnightValueEg       = 300,
  EuroShogiKnightValueMg   = 400,   EuroShogiKnightValueEg   = 400,
  GoldValueMg              = 640,   GoldValueEg              = 640,
  DragonHorseValueMg       = 1500,  DragonHorseValueEg       = 1500,
  ClobberPieceValueMg      = 300,   ClobberPieceValueEg      = 300,
  BreakthroughPieceValueMg = 300,   BreakthroughPieceValueEg = 300,
  ImmobilePieceValueMg     = 50,    ImmobilePieceValueEg     = 50,
  AtaxxPieceValueMg        = 100,   AtaxxPieceValueEg        = 100,
  CannonPieceValueMg       = 800,   CannonPieceValueEg       = 700,
  JanggiCannonPieceValueMg = 800,   JanggiCannonPieceValueEg = 600,
  SoldierValueMg           = 200,   SoldierValueEg           = 270,
  HorseValueMg             = 520,   HorseValueEg             = 800,
  ElephantValueMg          = 300,   ElephantValueEg          = 300,
  JanggiElephantValueMg    = 340,   JanggiElephantValueEg    = 350,
  BannerValueMg            = 3400,  BannerValueEg            = 3500,
  WazirValueMg             = 400,   WazirValueEg             = 350,
  CommonerValueMg          = 700,   CommonerValueEg          = 900,
  CentaurValueMg           = 1800,  CentaurValueEg           = 1900,
  Tempo = 28,

  MidgameLimit  = 15258, EndgameLimit  = 3915
};

constexpr int PIECE_TYPE_BITS = 6; // PIECE_TYPE_NB = pow(2, PIECE_TYPE_BITS)

enum PieceType {
  NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN,
  FERS, MET = FERS, ALFIL, FERS_ALFIL, SILVER, KHON = SILVER, AIWOK, BERS, DRAGON = BERS,
  ARCHBISHOP, CHANCELLOR, AMAZON, KNIBIS, BISKNI, KNIROO, ROOKNI,
  SHOGI_PAWN, LANCE, SHOGI_KNIGHT, EUROSHOGI_KNIGHT, GOLD, DRAGON_HORSE,
  CLOBBER_PIECE, BREAKTHROUGH_PIECE, IMMOBILE_PIECE, ATAXX_PIECE, CANNON, JANGGI_CANNON,
  SOLDIER, HORSE, ELEPHANT, JANGGI_ELEPHANT, BANNER,
  WAZIR, COMMONER, CENTAUR, KING,
  ALL_PIECES = 0,

  PIECE_TYPE_NB = 1 << PIECE_TYPE_BITS
};
static_assert(KING < PIECE_TYPE_NB, "KING exceeds PIECE_TYPE_NB.");
static_assert(PIECE_TYPE_BITS <= 6, "PIECE_TYPE uses more than 6 bit");
static_assert(!(PIECE_TYPE_NB & (PIECE_TYPE_NB - 1)), "PIECE_TYPE_NB is not a power of 2");

static_assert(2 * SQUARE_BITS + MOVE_TYPE_BITS + 2 * PIECE_TYPE_BITS <= 32, "Move encoding uses more than 32 bits");

enum Piece {
  NO_PIECE,
  PIECE_NB = 2 * PIECE_TYPE_NB
};

enum RiderType {
  NO_RIDER = 0,
  RIDER_BISHOP = 1 << 0,
  RIDER_ROOK_H = 1 << 1,
  RIDER_ROOK_V = 1 << 2,
  RIDER_CANNON_H = 1 << 3,
  RIDER_CANNON_V = 1 << 4,
  RIDER_HORSE = 1 << 5,
  RIDER_ELEPHANT = 1 << 6,
  RIDER_JANGGI_ELEPHANT = 1 << 7,
  HOPPING_RIDERS = RIDER_CANNON_H | RIDER_CANNON_V,
  ASYMMETRICAL_RIDERS = RIDER_HORSE | RIDER_JANGGI_ELEPHANT,
};

constexpr Value PieceValue[PHASE_NB][PIECE_NB] = {
  { VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg,
    FersValueMg, AlfilValueMg, FersAlfilValueMg, SilverValueMg, AiwokValueMg, BersValueMg,
    ArchbishopValueMg, ChancellorValueMg, AmazonValueMg, KnibisValueMg, BiskniValueMg, KnirooValueMg, RookniValueMg,
    ShogiPawnValueMg, LanceValueMg, ShogiKnightValueMg, EuroShogiKnightValueMg, GoldValueMg, DragonHorseValueMg,
    ClobberPieceValueMg, BreakthroughPieceValueMg, ImmobilePieceValueMg, AtaxxPieceValueMg,
    CannonPieceValueMg, JanggiCannonPieceValueMg, SoldierValueMg, HorseValueMg, ElephantValueMg, JanggiElephantValueMg, BannerValueMg,
    WazirValueMg, CommonerValueMg, CentaurValueMg, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg,
    FersValueMg, AlfilValueMg, FersAlfilValueMg, SilverValueMg, AiwokValueMg, BersValueMg,
    ArchbishopValueMg, ChancellorValueMg, AmazonValueMg, KnibisValueMg, BiskniValueMg, KnirooValueMg, RookniValueMg,
    ShogiPawnValueMg, LanceValueMg, ShogiKnightValueMg, EuroShogiKnightValueMg, GoldValueMg, DragonHorseValueMg,
    ClobberPieceValueMg, BreakthroughPieceValueMg, ImmobilePieceValueMg, AtaxxPieceValueMg,
    CannonPieceValueMg, JanggiCannonPieceValueMg, SoldierValueMg, HorseValueMg, ElephantValueMg, JanggiElephantValueMg, BannerValueMg,
    WazirValueMg, CommonerValueMg, CentaurValueMg, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO },
  { VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg,
    FersValueEg, AlfilValueEg, FersAlfilValueEg, SilverValueEg, AiwokValueEg, BersValueEg,
    ArchbishopValueMg, ChancellorValueEg, AmazonValueEg, KnibisValueMg, BiskniValueMg, KnirooValueEg, RookniValueEg,
    ShogiPawnValueEg, LanceValueEg, ShogiKnightValueEg, EuroShogiKnightValueEg, GoldValueEg, DragonHorseValueEg,
    ClobberPieceValueEg, BreakthroughPieceValueEg, ImmobilePieceValueEg, AtaxxPieceValueEg,
    CannonPieceValueEg, JanggiCannonPieceValueEg, SoldierValueEg, HorseValueEg, ElephantValueEg, JanggiElephantValueEg, BannerValueEg,
    WazirValueEg, CommonerValueEg, CentaurValueEg, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg,
    FersValueEg, AlfilValueEg, FersAlfilValueEg, SilverValueEg, AiwokValueEg, BersValueEg,
    ArchbishopValueMg, ChancellorValueEg, AmazonValueEg, KnibisValueMg, BiskniValueMg, KnirooValueEg, RookniValueEg,
    ShogiPawnValueEg, LanceValueEg, ShogiKnightValueEg, EuroShogiKnightValueEg, GoldValueEg, DragonHorseValueEg,
    ClobberPieceValueEg, BreakthroughPieceValueEg, ImmobilePieceValueEg, AtaxxPieceValueEg,
    CannonPieceValueEg, JanggiCannonPieceValueEg, SoldierValueEg, HorseValueEg, ElephantValueEg, JanggiElephantValueEg, BannerValueEg,
    WazirValueEg, CommonerValueEg, CentaurValueEg, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, VALUE_ZERO, VALUE_ZERO }
};

static_assert(   PieceValue[MG][PIECE_TYPE_NB + 1] == PawnValueMg
              && PieceValue[EG][PIECE_TYPE_NB + 1] == PawnValueEg, "PieceValue array broken");

typedef int Depth;

enum : int {

  DEPTH_QS_CHECKS     =  0,
  DEPTH_QS_NO_CHECKS  = -1,
  DEPTH_QS_RECAPTURES = -5,

  DEPTH_NONE   = -6,
  DEPTH_OFFSET = DEPTH_NONE
};

enum Square : int {
#ifdef LARGEBOARDS
  SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1, SQ_I1, SQ_J1, SQ_K1, SQ_L1,
  SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2, SQ_I2, SQ_J2, SQ_K2, SQ_L2,
  SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3, SQ_I3, SQ_J3, SQ_K3, SQ_L3,
  SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4, SQ_I4, SQ_J4, SQ_K4, SQ_L4,
  SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5, SQ_I5, SQ_J5, SQ_K5, SQ_L5,
  SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6, SQ_I6, SQ_J6, SQ_K6, SQ_L6,
  SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7, SQ_I7, SQ_J7, SQ_K7, SQ_L7,
  SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8, SQ_I8, SQ_J8, SQ_K8, SQ_L8,
  SQ_A9, SQ_B9, SQ_C9, SQ_D9, SQ_E9, SQ_F9, SQ_G9, SQ_H9, SQ_I9, SQ_J9, SQ_K9, SQ_L9,
  SQ_A10, SQ_B10, SQ_C10, SQ_D10, SQ_E10, SQ_F10, SQ_G10, SQ_H10, SQ_I10, SQ_J10, SQ_K10, SQ_L10,
#else
  SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
  SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
  SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
  SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
  SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
  SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
  SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
  SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
#endif
  SQ_NONE,

#ifdef LARGEBOARDS
  SQUARE_NB = 120,
  SQUARE_BIT_MASK = 127,
#else
  SQUARE_NB = 64,
  SQUARE_BIT_MASK = 63,
#endif
  SQ_MAX = SQUARE_NB - 1
};

enum Direction : int {
#ifdef LARGEBOARDS
  NORTH =  12,
#else
  NORTH =  8,
#endif
  EAST  =  1,
  SOUTH = -NORTH,
  WEST  = -EAST,

  NORTH_EAST = NORTH + EAST,
  SOUTH_EAST = SOUTH + EAST,
  SOUTH_WEST = SOUTH + WEST,
  NORTH_WEST = NORTH + WEST
};

enum File : int {
#ifdef LARGEBOARDS
  FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_I, FILE_J, FILE_K, FILE_L,
#else
  FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
#endif
  FILE_NB,
  FILE_MAX = FILE_NB - 1
};

enum Rank : int {
#ifdef LARGEBOARDS
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9, RANK_10,
#else
  RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
#endif
  RANK_NB,
  RANK_MAX = RANK_NB - 1
};


/// Score enum stores a middlegame and an endgame value in a single integer (enum).
/// The least significant 16 bits are used to store the middlegame value and the
/// upper 16 bits are used to store the endgame value. We have to take care to
/// avoid left-shifting a signed int to avoid undefined behavior.
enum Score : int { SCORE_ZERO };

constexpr Score make_score(int mg, int eg) {
  return Score((int)((unsigned int)eg << 16) + mg);
}

/// Extracting the signed lower and upper 16 bits is not so trivial because
/// according to the standard a simple cast to short is implementation defined
/// and so is a right shift of a signed integer.
inline Value eg_value(Score s) {
  union { uint16_t u; int16_t s; } eg = { uint16_t(unsigned(s + 0x8000) >> 16) };
  return Value(eg.s);
}

inline Value mg_value(Score s) {
  union { uint16_t u; int16_t s; } mg = { uint16_t(unsigned(s)) };
  return Value(mg.s);
}

#define ENABLE_BIT_OPERATORS_ON(T)                                    \
inline T operator~ (T d) { return (T)~(int)d; }                       \
inline T operator| (T d1, T d2) { return (T)((int)d1 | (int)d2); }        \
inline T operator& (T d1, T d2) { return (T)((int)d1 & (int)d2); }        \
inline T operator^ (T d1, T d2) { return (T)((int)d1 ^ (int)d2); }        \
inline T& operator|= (T& d1, T d2) { return (T&)((int&)d1 |= (int)d2); }  \
inline T& operator&= (T& d1, T d2) { return (T&)((int&)d1 &= (int)d2); }  \
inline T& operator^= (T& d1, T d2) { return (T&)((int&)d1 ^= (int)d2); }

#define ENABLE_BASE_OPERATORS_ON(T)                                \
constexpr T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
constexpr T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
constexpr T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }         \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }

#define ENABLE_INCR_OPERATORS_ON(T)                                \
inline T& operator++(T& d) { return d = T(int(d) + 1); }           \
inline T& operator--(T& d) { return d = T(int(d) - 1); }

#define ENABLE_FULL_OPERATORS_ON(T)                                \
ENABLE_BASE_OPERATORS_ON(T)                                        \
constexpr T operator*(int i, T d) { return T(i * int(d)); }        \
constexpr T operator*(T d, int i) { return T(int(d) * i); }        \
constexpr T operator/(T d, int i) { return T(int(d) / i); }        \
constexpr int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }    \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)
ENABLE_FULL_OPERATORS_ON(Direction)

ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Piece)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)
ENABLE_INCR_OPERATORS_ON(CheckCount)

ENABLE_BASE_OPERATORS_ON(Score)

ENABLE_BIT_OPERATORS_ON(RiderType)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_INCR_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON
#undef ENABLE_BIT_OPERATORS_ON

/// Additional operators to add integers to a Value
constexpr Value operator+(Value v, int i) { return Value(int(v) + i); }
constexpr Value operator-(Value v, int i) { return Value(int(v) - i); }
inline Value& operator+=(Value& v, int i) { return v = v + i; }
inline Value& operator-=(Value& v, int i) { return v = v - i; }

/// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square& operator+=(Square& s, Direction d) { return s = s + d; }
inline Square& operator-=(Square& s, Direction d) { return s = s - d; }

/// Only declared but not defined. We don't want to multiply two scores due to
/// a very high risk of overflow. So user should explicitly convert to integer.
Score operator*(Score, Score) = delete;

/// Division of a Score must be handled separately for each term
inline Score operator/(Score s, int i) {
  return make_score(mg_value(s) / i, eg_value(s) / i);
}

/// Multiplication of a Score by an integer. We check for overflow in debug mode.
inline Score operator*(Score s, int i) {

  Score result = Score(int(s) * i);

  assert(eg_value(result) == (i * eg_value(s)));
  assert(mg_value(result) == (i * mg_value(s)));
  assert((i == 0) || (result / i) == s);

  return result;
}

/// Multiplication of a Score by a boolean
inline Score operator*(Score s, bool b) {
  return b ? s : SCORE_ZERO;
}

constexpr Color operator~(Color c) {
  return Color(c ^ BLACK); // Toggle color
}

constexpr Square flip_rank(Square s, Rank maxRank = RANK_8) {
  return Square(s + NORTH * (maxRank - 2 * (s / NORTH)));
}

constexpr Square flip_file(Square s, File maxFile = FILE_H) {
  return Square(s + maxFile - 2 * (s % NORTH));
}

constexpr Piece operator~(Piece pc) {
  return Piece(pc ^ PIECE_TYPE_NB); // Swap color of piece BLACK KNIGHT -> WHITE KNIGHT
}

constexpr CastlingRights operator&(Color c, CastlingRights cr) {
  return CastlingRights((c == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr);
}

constexpr Value mate_in(int ply) {
  return VALUE_MATE - ply;
}

constexpr Value mated_in(int ply) {
  return -VALUE_MATE + ply;
}

constexpr Value convert_mate_value(Value v, int ply) {
  return  v ==  VALUE_MATE ? mate_in(ply)
        : v == -VALUE_MATE ? mated_in(ply)
        : v;
}

constexpr Square make_square(File f, Rank r) {
  return Square(r * FILE_NB + f);
}

constexpr Piece make_piece(Color c, PieceType pt) {
  return Piece((c << PIECE_TYPE_BITS) + pt);
}

constexpr PieceType type_of(Piece pc) {
  return PieceType(pc & (PIECE_TYPE_NB - 1));
}

inline Color color_of(Piece pc) {
  assert(pc != NO_PIECE);
  return Color(pc >> PIECE_TYPE_BITS);
}

constexpr bool is_ok(Square s) {
  return s >= SQ_A1 && s <= SQ_MAX;
}

constexpr File file_of(Square s) {
  return File(s % FILE_NB);
}

constexpr Rank rank_of(Square s) {
  return Rank(s / FILE_NB);
}

constexpr Rank relative_rank(Color c, Rank r, Rank maxRank = RANK_8) {
  return Rank(c == WHITE ? r : maxRank - r);
}

constexpr Rank relative_rank(Color c, Square s, Rank maxRank = RANK_8) {
  return relative_rank(c, rank_of(s), maxRank);
}

constexpr Square relative_square(Color c, Square s, Rank maxRank = RANK_8) {
  return make_square(file_of(s), relative_rank(c, s, maxRank));
}

constexpr Direction pawn_push(Color c) {
  return c == WHITE ? NORTH : SOUTH;
}

constexpr MoveType type_of(Move m) {
  return MoveType(m & (15 << (2 * SQUARE_BITS)));
}

constexpr Square to_sq(Move m) {
  return Square(m & SQUARE_BIT_MASK);
}

constexpr Square from_sq(Move m) {
  return type_of(m) == DROP ? SQ_NONE : Square((m >> SQUARE_BITS) & SQUARE_BIT_MASK);
}

inline int from_to(Move m) {
 return to_sq(m) + (from_sq(m) << SQUARE_BITS);
}

inline PieceType promotion_type(Move m) {
  return type_of(m) == PROMOTION ? PieceType((m >> (2 * SQUARE_BITS + MOVE_TYPE_BITS)) & (PIECE_TYPE_NB - 1)) : NO_PIECE_TYPE;
}

inline PieceType gating_type(Move m) {
  return PieceType((m >> (2 * SQUARE_BITS + MOVE_TYPE_BITS)) & (PIECE_TYPE_NB - 1));
}

inline Square gating_square(Move m) {
  return Square((m >> (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) & SQUARE_BIT_MASK);
}

inline bool is_gating(Move m) {
  return gating_type(m) && (type_of(m) == NORMAL || type_of(m) == CASTLING);
}

inline bool is_pass(Move m) {
  return type_of(m) == SPECIAL && from_sq(m) == to_sq(m);
}

constexpr Move make_move(Square from, Square to) {
  return Move((from << SQUARE_BITS) + to);
}

template<MoveType T>
inline Move make(Square from, Square to, PieceType pt = NO_PIECE_TYPE) {
  return Move((pt << (2 * SQUARE_BITS + MOVE_TYPE_BITS)) + T + (from << SQUARE_BITS) + to);
}

constexpr Move make_drop(Square to, PieceType pt_in_hand, PieceType pt_dropped) {
  return Move((pt_in_hand << (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) + (pt_dropped << (2 * SQUARE_BITS + MOVE_TYPE_BITS)) + DROP + to);
}

constexpr Move reverse_move(Move m) {
  return make_move(to_sq(m), from_sq(m));
}

template<MoveType T>
constexpr Move make_gating(Square from, Square to, PieceType pt, Square gate) {
  return Move((gate << (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) + (pt << (2 * SQUARE_BITS + MOVE_TYPE_BITS)) + T + (from << SQUARE_BITS) + to);
}

constexpr PieceType dropped_piece_type(Move m) {
  return PieceType((m >> (2 * SQUARE_BITS + MOVE_TYPE_BITS)) & (PIECE_TYPE_NB - 1));
}

constexpr PieceType in_hand_piece_type(Move m) {
  return PieceType((m >> (2 * SQUARE_BITS + MOVE_TYPE_BITS + PIECE_TYPE_BITS)) & (PIECE_TYPE_NB - 1));
}

inline bool is_ok(Move m) {
  return from_sq(m) != to_sq(m) || type_of(m) == PROMOTION || type_of(m) == SPECIAL; // Catch MOVE_NULL and MOVE_NONE
}

inline int dist(Direction d) {
  return std::abs(d % NORTH) < NORTH / 2 ? std::max(std::abs(d / NORTH), std::abs(d % NORTH))
      : std::max(std::abs(d / NORTH) + 1, NORTH - std::abs(d % NORTH));
}

#endif // #ifndef TYPES_H_INCLUDED

#include "tune.h" // Global visibility to tuning setup
