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

#ifndef MOVEPICK_H_INCLUDED
#define MOVEPICK_H_INCLUDED

#include <array>
#include <limits>
#include <type_traits>

#include "movegen.h"
#include "position.h"
#include "types.h"

namespace Stockfish {

/// StatsEntry stores the stat table value. It is usually a number but could
/// be a move or even a nested history. We use a class instead of naked value
/// to directly call history update operator<<() on the entry so to use stats
/// tables at caller sites as simple multi-dim arrays.
template<typename T, int D>
class StatsEntry {

  T entry;

public:
  void operator=(const T& v) { entry = v; }
  T* operator&() { return &entry; }
  T* operator->() { return &entry; }
  operator const T&() const { return entry; }

  void operator<<(int bonus) {
    assert(abs(bonus) <= D); // Ensure range is [-D, D]
    static_assert(D <= std::numeric_limits<T>::max(), "D overflows T");

    entry += bonus - entry * abs(bonus) / D;

    assert(abs(entry) <= D);
  }
};

/// Stats is a generic N-dimensional array used to store various statistics.
/// The first template parameter T is the base type of the array, the second
/// template parameter D limits the range of updates in [-D, D] when we update
/// values with the << operator, while the last parameters (Size and Sizes)
/// encode the dimensions of the array.
template <typename T, int D, int Size, int... Sizes>
struct Stats : public std::array<Stats<T, D, Sizes...>, Size>
{
  typedef Stats<T, D, Size, Sizes...> stats;

  void fill(const T& v) {

    // For standard-layout 'this' points to first struct member
    assert(std::is_standard_layout<stats>::value);

    typedef StatsEntry<T, D> entry;
    entry* p = reinterpret_cast<entry*>(this);
    std::fill(p, p + sizeof(*this) / sizeof(entry), v);
  }
};

template <typename T, int D, int Size>
struct Stats<T, D, Size> : public std::array<StatsEntry<T, D>, Size> {};

/// In stats table, D=0 means that the template parameter is not used
enum StatsParams { NOT_USED = 0, PIECE_SLOTS = 8 };
enum StatsType { NoCaptures, Captures };

/// ButterflyHistory records how often quiet moves have been successful or
/// unsuccessful during the current search, and is used for reduction and move
/// ordering decisions. It uses 2 tables (one for each color) indexed by
/// the move's from and to squares, see www.chessprogramming.org/Butterfly_Boards
typedef Stats<int16_t, 13365, COLOR_NB, int(SQUARE_NB + 1) * int(1 << SQUARE_BITS)> ButterflyHistory;

/// At higher depths LowPlyHistory records successful quiet moves near the root
/// and quiet moves which are/were in the PV (ttPv). It is cleared with each new
/// search and filled during iterative deepening.
constexpr int MAX_LPH = 4;
typedef Stats<int16_t, 10692, MAX_LPH, int(SQUARE_NB + 1) * int(1 << SQUARE_BITS)> LowPlyHistory;

/// CounterMoveHistory stores counter moves indexed by [piece][to] of the previous
/// move, see www.chessprogramming.org/Countermove_Heuristic
typedef Stats<Move, NOT_USED, PIECE_NB, SQUARE_NB> CounterMoveHistory;

/// CapturePieceToHistory is addressed by a move's [piece][to][captured piece type]
typedef Stats<int16_t, 10692, PIECE_NB, SQUARE_NB, PIECE_TYPE_NB> CapturePieceToHistory;

/// PieceToHistory is like ButterflyHistory but is addressed by a move's [piece][to]
typedef Stats<int16_t, 29952, 2 * PIECE_SLOTS, SQUARE_NB> PieceToHistory;

/// ContinuationHistory is the combined history of a given pair of moves, usually
/// the current one given a previous one. The nested history table is based on
/// PieceToHistory instead of ButterflyBoards.
typedef Stats<PieceToHistory, NOT_USED, 2 * PIECE_SLOTS, SQUARE_NB> ContinuationHistory;

int history_slot(Piece pc);

/// MovePicker class is used to pick one pseudo-legal move at a time from the
/// current position. The most important method is next_move(), which returns a
/// new pseudo-legal move each time it is called, until there are no moves left,
/// when MOVE_NONE is returned. In order to improve the efficiency of the
/// alpha-beta algorithm, MovePicker attempts to return the moves which are most
/// likely to get a cut-off first.
class MovePicker {

  enum PickType { Next, Best };

public:
  MovePicker(const MovePicker&) = delete;
  MovePicker& operator=(const MovePicker&) = delete;
  MovePicker(const Position&, Move, Value, const CapturePieceToHistory*);
  MovePicker(const Position&, Move, Depth, const ButterflyHistory*,
                                           const CapturePieceToHistory*,
                                           const PieceToHistory**,
                                           Square);
  MovePicker(const Position&, Move, Depth, const ButterflyHistory*,
                                           const LowPlyHistory*,
                                           const CapturePieceToHistory*,
                                           const PieceToHistory**,
                                           Move,
                                           const Move*,
                                           int);
  Move next_move(bool skipQuiets = false);

private:
  template<PickType T, typename Pred> Move select(Pred);
  template<GenType> void score();
  ExtMove* begin() { return cur; }
  ExtMove* end() { return endMoves; }

  const Position& pos;
  const ButterflyHistory* mainHistory;
  const LowPlyHistory* lowPlyHistory;
  const CapturePieceToHistory* captureHistory;
  const PieceToHistory** continuationHistory;
  Move ttMove;
  ExtMove refutations[3], *cur, *endMoves, *endBadCaptures;
  int stage;
  Square recaptureSquare;
  Value threshold;
  Depth depth;
  int ply;
  ExtMove moves[MAX_MOVES];
};

} // namespace Stockfish

#endif // #ifndef MOVEPICK_H_INCLUDED
