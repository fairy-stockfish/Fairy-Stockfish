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

#ifndef MOVEGEN_H_INCLUDED
#define MOVEGEN_H_INCLUDED

#include <algorithm>

#include "types.h"

namespace Stockfish {

class Position;
struct Variant;

enum GenType {
  CAPTURES,
  QUIETS,
  QUIET_CHECKS,
  EVASIONS,
  NON_EVASIONS,
  LEGAL
};

struct ExtMove {
  Move move;
  int value;

  operator Move() const { return move; }
  void operator=(Move m) { move = m; }

  // Inhibit unwanted implicit conversions to Move
  // with an ambiguity that yields to a compile error.
  operator float() const = delete;
};

inline bool operator<(const ExtMove& f, const ExtMove& s) {
  return f.value < s.value;
}

template<GenType>
ExtMove* generate(const Position& pos, ExtMove* moveList);

/// acquire_move_buffer() and release_move_buffer() hand out move buffers from a
/// per-thread pool. Buffers have to be released in reverse order. They have room
/// for MoveBufferCapacity moves, which update_move_buffer_capacity() sets to an
/// upper bound of the number of pseudo-legal moves in the initialized variants.
/// It has to be called whenever the piece tables are initialized for a variant.
extern size_t MoveBufferCapacity;

ExtMove* acquire_move_buffer();
void release_move_buffer(ExtMove* buffer);
void update_move_buffer_capacity(const Variant* v);
[[noreturn]] void move_buffer_overflow(size_t moves);

/// check_move_buffer() aborts if generated moves exceeded the capacity of the
/// buffer, i.e., the upper bound of the moves in the variant was too small.
/// The buffer then already overflowed, but this stops the program before it
/// continues with corrupted memory.
inline void check_move_buffer(const ExtMove* begin, const ExtMove* end) {
  if (size_t(end - begin) > MoveBufferCapacity)
      move_buffer_overflow(end - begin);
}

/// The MoveList struct is a simple wrapper around generate(). It sometimes comes
/// in handy to use this class instead of the low level generate() function.
template<GenType T>
struct MoveList {

  explicit MoveList(const Position& pos) : moveList(acquire_move_buffer()), last(generate<T>(pos, moveList)) {
      check_move_buffer(moveList, last);
  }
  ~MoveList() { release_move_buffer(moveList); }

  // The buffer has to be released exactly once, in reverse order of acquisition
  MoveList(const MoveList&) = delete;
  MoveList& operator=(const MoveList&) = delete;

  const ExtMove* begin() const { return moveList; }
  const ExtMove* end() const { return last; }
  size_t size() const { return last - moveList; }
  bool contains(Move move) const {
    return std::find(begin(), end(), move) != end();
  }

private:
  ExtMove* const moveList;
  ExtMove* last;
};

} // namespace Stockfish

#endif // #ifndef MOVEGEN_H_INCLUDED
