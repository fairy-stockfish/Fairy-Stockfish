/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

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

// Constants used in NNUE evaluation function

#ifndef NNUE_COMMON_H_INCLUDED
#define NNUE_COMMON_H_INCLUDED

#include <cstring>
#include <iostream>

#if defined(USE_AVX2)
#include <immintrin.h>

#elif defined(USE_SSE41)
#include <smmintrin.h>

#elif defined(USE_SSSE3)
#include <tmmintrin.h>

#elif defined(USE_SSE2)
#include <emmintrin.h>

#elif defined(USE_MMX)
#include <mmintrin.h>

#elif defined(USE_NEON)
#include <arm_neon.h>
#endif

namespace Eval::NNUE {

  // Version of the evaluation file
  constexpr std::uint32_t kVersion = 0x7AF32F16u;

  // Constant used in evaluation value calculation
  constexpr int FV_SCALE = 16;
  constexpr int kWeightScaleBits = 6;

  // Size of cache line (in bytes)
  constexpr std::size_t kCacheLineSize = 64;

  // SIMD width (in bytes)
  #if defined(USE_AVX2)
  constexpr std::size_t kSimdWidth = 32;

  #elif defined(USE_SSE2)
  constexpr std::size_t kSimdWidth = 16;

  #elif defined(USE_MMX)
  constexpr std::size_t kSimdWidth = 8;

  #elif defined(USE_NEON)
  constexpr std::size_t kSimdWidth = 16;
  #endif

  constexpr std::size_t kMaxSimdWidth = 32;

  // unique number for each piece type on each square
  enum {
    PS_NONE     =  0,
    PS_W_PAWN   =  1,
    PS_B_PAWN   =  1 * SQUARE_NB_CHESS + 1,
    PS_W_KNIGHT =  2 * SQUARE_NB_CHESS + 1,
    PS_B_KNIGHT =  3 * SQUARE_NB_CHESS + 1,
    PS_W_BISHOP =  4 * SQUARE_NB_CHESS + 1,
    PS_B_BISHOP =  5 * SQUARE_NB_CHESS + 1,
    PS_W_ROOK   =  6 * SQUARE_NB_CHESS + 1,
    PS_B_ROOK   =  7 * SQUARE_NB_CHESS + 1,
    PS_W_QUEEN  =  8 * SQUARE_NB_CHESS + 1,
    PS_B_QUEEN  =  9 * SQUARE_NB_CHESS + 1,
    PS_W_KING   = 10 * SQUARE_NB_CHESS + 1,
    PS_END      = PS_W_KING, // pieces without kings (pawns included)
    PS_B_KING   = 11 * SQUARE_NB_CHESS + 1,
    PS_END2     = 12 * SQUARE_NB_CHESS + 1
  };

  enum {
    SHOGI_HAND_W_PAWN   =   1,
    SHOGI_HAND_B_PAWN   =  20,
    SHOGI_HAND_W_LANCE  =  39,
    SHOGI_HAND_B_LANCE  =  44,
    SHOGI_HAND_W_KNIGHT =  49,
    SHOGI_HAND_B_KNIGHT =  54,
    SHOGI_HAND_W_SILVER =  59,
    SHOGI_HAND_B_SILVER =  64,
    SHOGI_HAND_W_GOLD   =  69,
    SHOGI_HAND_B_GOLD   =  74,
    SHOGI_HAND_W_BISHOP =  79,
    SHOGI_HAND_B_BISHOP =  82,
    SHOGI_HAND_W_ROOK   =  85,
    SHOGI_HAND_B_ROOK   =  88,
    SHOGI_HAND_END      =  90,

    SHOGI_PS_W_PAWN     =  SHOGI_HAND_END,
    SHOGI_PS_B_PAWN     =  1 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_LANCE    =  2 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_LANCE    =  3 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_KNIGHT   =  4 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_KNIGHT   =  5 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_SILVER   =  6 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_SILVER   =  7 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_GOLD     =  8 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_GOLD     =  9 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_BISHOP   = 10 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_BISHOP   = 11 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_HORSE    = 12 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_HORSE    = 13 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_ROOK     = 14 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_ROOK     = 15 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_DRAGON   = 16 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_B_DRAGON   = 17 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_W_KING     = 18 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_END        = SHOGI_PS_W_KING, // pieces without kings (pawns included)
    SHOGI_PS_B_KING     = 19 * SQUARE_NB_SHOGI + SHOGI_HAND_END,
    SHOGI_PS_END2       = 20 * SQUARE_NB_SHOGI + SHOGI_HAND_END
  };

  constexpr uint32_t shogi_kpp_board_index[COLOR_NB][PIECE_NB] = {
    // convention: W - us, B - them
    // viewed from other side, W and B are reversed
    { PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_BISHOP, SHOGI_PS_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, SHOGI_PS_W_SILVER, PS_NONE, SHOGI_PS_W_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_PAWN, SHOGI_PS_W_LANCE, SHOGI_PS_W_KNIGHT, PS_NONE, SHOGI_PS_W_GOLD,
      SHOGI_PS_W_HORSE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      SHOGI_PS_W_KING, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,

      PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_BISHOP, SHOGI_PS_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, SHOGI_PS_B_SILVER, PS_NONE, SHOGI_PS_B_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_PAWN, SHOGI_PS_B_LANCE, SHOGI_PS_B_KNIGHT, PS_NONE, SHOGI_PS_B_GOLD,
      SHOGI_PS_B_HORSE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      SHOGI_PS_B_KING, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE },

    { PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_BISHOP, SHOGI_PS_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, SHOGI_PS_B_SILVER, PS_NONE, SHOGI_PS_B_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_B_PAWN, SHOGI_PS_B_LANCE, SHOGI_PS_B_KNIGHT, PS_NONE, SHOGI_PS_B_GOLD,
      SHOGI_PS_B_HORSE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      SHOGI_PS_B_KING, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,

      PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_BISHOP, SHOGI_PS_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, SHOGI_PS_W_SILVER, PS_NONE, SHOGI_PS_W_DRAGON, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, SHOGI_PS_W_PAWN, SHOGI_PS_W_LANCE, SHOGI_PS_W_KNIGHT, PS_NONE, SHOGI_PS_W_GOLD,
      SHOGI_PS_W_HORSE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      SHOGI_PS_W_KING, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE }
  };
  static_assert(shogi_kpp_board_index[WHITE][make_piece(WHITE, SHOGI_PAWN)] == SHOGI_PS_W_PAWN);
  static_assert(shogi_kpp_board_index[WHITE][make_piece(WHITE, KING)] == SHOGI_PS_W_KING);
  static_assert(shogi_kpp_board_index[WHITE][make_piece(BLACK, SHOGI_PAWN)] == SHOGI_PS_B_PAWN);
  static_assert(shogi_kpp_board_index[WHITE][make_piece(BLACK, KING)] == SHOGI_PS_B_KING);

  constexpr uint32_t shogi_kpp_hand_index[COLOR_NB][PIECE_TYPE_NB] = {
    // convention: W - us, B - them
    // viewed from other side, W and B are reversed
    { PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_W_BISHOP, SHOGI_HAND_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, SHOGI_HAND_W_SILVER, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_W_PAWN, SHOGI_HAND_W_LANCE, SHOGI_HAND_W_KNIGHT, PS_NONE, SHOGI_HAND_W_GOLD,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE },

    { PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_B_BISHOP, SHOGI_HAND_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, SHOGI_HAND_B_SILVER, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, SHOGI_HAND_B_PAWN, SHOGI_HAND_B_LANCE, SHOGI_HAND_B_KNIGHT, PS_NONE, SHOGI_HAND_B_GOLD,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE }
  };
  static_assert(shogi_kpp_hand_index[WHITE][SHOGI_PAWN] == SHOGI_HAND_W_PAWN);
  static_assert(shogi_kpp_hand_index[WHITE][GOLD] == SHOGI_HAND_W_GOLD);
  static_assert(shogi_kpp_hand_index[BLACK][SHOGI_PAWN] == SHOGI_HAND_B_PAWN);
  static_assert(shogi_kpp_hand_index[BLACK][GOLD] == SHOGI_HAND_B_GOLD);

  constexpr uint32_t kpp_board_index[COLOR_NB][PIECE_NB] = {
    // convention: W - us, B - them
    // viewed from other side, W and B are reversed
    { PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN,
      PS_W_QUEEN, PS_W_BISHOP, PS_W_BISHOP, PS_W_BISHOP, PS_W_QUEEN,
      PS_W_QUEEN, PS_NONE, PS_NONE, PS_W_QUEEN, PS_W_KNIGHT,
      PS_W_BISHOP, PS_W_KNIGHT, PS_W_ROOK, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_W_BISHOP,
      PS_NONE, PS_W_PAWN, PS_W_KNIGHT, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_W_KING,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_B_PAWN,
      PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN, PS_B_QUEEN,
      PS_B_BISHOP, PS_B_BISHOP, PS_B_BISHOP, PS_B_QUEEN, PS_B_QUEEN,
      PS_NONE, PS_NONE, PS_B_QUEEN, PS_B_KNIGHT, PS_B_BISHOP,
      PS_B_KNIGHT, PS_B_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_B_BISHOP, PS_NONE,
      PS_B_PAWN, PS_B_KNIGHT, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_B_KING, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE },
    { PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_B_BISHOP, PS_B_ROOK, PS_B_QUEEN,
      PS_B_QUEEN, PS_B_BISHOP, PS_B_BISHOP, PS_B_BISHOP, PS_B_QUEEN,
      PS_B_QUEEN, PS_NONE, PS_NONE, PS_B_QUEEN, PS_B_KNIGHT,
      PS_B_BISHOP, PS_B_KNIGHT, PS_B_ROOK, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_B_BISHOP,
      PS_NONE, PS_B_PAWN, PS_B_KNIGHT, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_B_KING,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_W_PAWN,
      PS_W_KNIGHT, PS_W_BISHOP, PS_W_ROOK, PS_W_QUEEN, PS_W_QUEEN,
      PS_W_BISHOP, PS_W_BISHOP, PS_W_BISHOP, PS_W_QUEEN, PS_W_QUEEN,
      PS_NONE, PS_NONE, PS_W_QUEEN, PS_W_KNIGHT, PS_W_BISHOP,
      PS_W_KNIGHT, PS_W_ROOK, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_W_BISHOP, PS_NONE,
      PS_W_PAWN, PS_W_KNIGHT, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_W_KING, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE, PS_NONE, PS_NONE, PS_NONE,
      PS_NONE, PS_NONE }
  };
  // Check that the fragile array definition is correct
  static_assert(kpp_board_index[WHITE][make_piece(WHITE, PAWN)] == PS_W_PAWN);
  static_assert(kpp_board_index[WHITE][make_piece(WHITE, KING)] == PS_W_KING);
  static_assert(kpp_board_index[WHITE][make_piece(BLACK, PAWN)] == PS_B_PAWN);
  static_assert(kpp_board_index[WHITE][make_piece(BLACK, KING)] == PS_B_KING);


  // Type of input feature after conversion
  using TransformedFeatureType = std::uint8_t;
  using IndexType = std::uint32_t;

  // Round n up to be a multiple of base
  template <typename IntType>
  constexpr IntType CeilToMultiple(IntType n, IntType base) {
      return (n + base - 1) / base * base;
  }

  // read_little_endian() is our utility to read an integer (signed or unsigned, any size)
  // from a stream in little-endian order. We swap the byte order after the read if
  // necessary to return a result with the byte ordering of the compiling machine.
  template <typename IntType>
  inline IntType read_little_endian(std::istream& stream) {

      IntType result;
      std::uint8_t u[sizeof(IntType)];
      typename std::make_unsigned<IntType>::type v = 0;

      stream.read(reinterpret_cast<char*>(u), sizeof(IntType));
      for (std::size_t i = 0; i < sizeof(IntType); ++i)
          v = (v << 8) | u[sizeof(IntType) - i - 1];

      std::memcpy(&result, &v, sizeof(IntType));
      return result;
  }

}  // namespace Eval::NNUE

#endif // #ifndef NNUE_COMMON_H_INCLUDED
