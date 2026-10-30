/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

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

#include "movepick.h"

#include <cassert>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "bitboard.h"
#include "misc.h"
#include "position.h"

namespace Stockfish {

// Since continuation history grows quadratically with the number of piece types,
// we need to reserve a limited number of slots and map piece types to these slots
// in order to reduce memory consumption to a reasonable level.
int history_slot(Piece pc) {
    return pc == NO_PIECE
           ? 0
           : (type_of(pc) == KING ? PIECE_SLOTS - 1 : type_of(pc) % (PIECE_SLOTS - 1))
               + color_of(pc) * PIECE_SLOTS;
}

namespace {

enum Stages {
    MAIN_TT,
    CAPTURE_INIT,
    GOOD_CAPTURE,
    QUIET_INIT,
    GOOD_QUIET,
    BAD_CAPTURE,
    BAD_QUIET,

    // generate evasion moves
    EVASION_TT,
    EVASION_INIT,
    EVASION,
    PROBCUT_TT,
    PROBCUT_INIT,
    PROBCUT,
    QSEARCH_TT,
    QCAPTURE_INIT,
    QCAPTURE
};

#ifdef USE_AVX512
// Load the Move, and the ExtMove value, into all lanes of 512-bit registers
static void splat_extmove(const ExtMove& m, __m512i& move, __m512i& value) {
    move  = _mm512_set1_epi32(m.raw());
    value = _mm512_set1_epi32(m.value);
}

// Sorts up to 16 moves.
struct MoveSorter {
    static constexpr int MAX_ELEMENTS = 16;
    __m512i              sortedValues, sortedMoves;

    explicit MoveSorter(const ExtMove& first) {
        splat_extmove(first, sortedMoves, sortedValues);

        // Set the uninitialized move values to INT_MIN, so that they sort less than any other move
        sortedValues = _mm512_mask_set1_epi32(sortedValues, ~1, std::numeric_limits<int>::min());
    }

    void insert(const ExtMove& m) {
        __m512i move, value;
        splat_extmove(m, move, value);

        // Mask of all elements except the insertion point
        assert(m.value != std::numeric_limits<int>::min());
        const u16 expand = _kadd_mask16(_mm512_cmplt_epi32_mask(sortedValues, value), -1);

        sortedValues = _mm512_mask_expand_epi32(value, expand, sortedValues);
        sortedMoves  = _mm512_mask_expand_epi32(move, expand, sortedMoves);
    }

    void write_sorted(ExtMove* moves, isize count) const {
        static_assert(sizeof(ExtMove) == 8);
        assert(count <= MAX_ELEMENTS);

        // Because values and moves are stored separately, we need to reassemble the ExtMoves
        auto write = [&](int offset, const __m512i indices) {
            const __m512i extMoves = _mm512_permutex2var_epi32(sortedMoves, indices, sortedValues);
            const isize   storeCount = count - offset;

            if (storeCount > 0)
                _mm512_mask_storeu_epi64(moves + offset, (1 << storeCount) - 1, extMoves);
        };

        write(0, _mm512_setr_epi32(0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23));
        write(8, _mm512_setr_epi32(8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31));
    }
};
#endif

// Sort moves in descending order up to and including a given limit.
// The order of moves smaller than the limit is left unspecified.
void partial_insertion_sort(ExtMove* begin, ExtMove* end, int limit) {
    ExtMove *sortedEnd = begin, *p = begin + 1;

#ifdef USE_AVX512
    if (begin == end)
        return;

    MoveSorter sorter(*begin);
    for (; p < end; ++p)
    {
        if (p->value >= limit)
        {
            if (sortedEnd - begin + 1 >= MoveSorter::MAX_ELEMENTS)  // sorter full
                break;

            sorter.insert(*p);
            *p = *++sortedEnd;
        }
    }
    sorter.write_sorted(begin, sortedEnd - begin + 1);
    // Use scalar implementation for any remaining elements
#endif

    for (; p < end; ++p)
        if (p->value >= limit)
        {
            ExtMove tmp = *p, *q;
            *p          = *++sortedEnd;
            for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

// Per-thread LIFO pool of move buffers. MovePicker instances have strictly
// nested lifetimes within a search thread, so buffers can be handed out and
// returned stack-wise. This keeps the large MAX_MOVES array off the machine
// stack, whose 8MB limit deep searches can otherwise exceed (see issue #957).
thread_local std::vector<std::unique_ptr<ExtMove[]>> moveBuffers;
thread_local size_t                                  usedMoveBuffers = 0;

ExtMove* acquire_move_buffer() {
    if (usedMoveBuffers == moveBuffers.size())
        moveBuffers.emplace_back(new ExtMove[MAX_MOVES]);
    return moveBuffers[usedMoveBuffers++].get();
}

void release_move_buffer() {
    assert(usedMoveBuffers > 0);
    --usedMoveBuffers;
}

}  // namespace


MovePicker::~MovePicker() { release_move_buffer(); }

// Constructors of the MovePicker class. As arguments, we pass information
// to decide which class of moves to emit, to help sorting the (presumably)
// good moves first, and how important move ordering is at the current node.

// MovePicker constructor for the main search and for the quiescence search
MovePicker::MovePicker(const Position&              p,
                       Move                         ttm,
                       Depth                        d,
                       const ButterflyHistory*      mh,
                       const GateHistory*           dh,
                       const LowPlyHistory*         lph,
                       const CapturePieceToHistory* cph,
                       const PieceToHistory**       ch,
                       const SharedHistories*       sh,
                       int                          pl) :
    pos(p),
    mainHistory(mh),
    gateHistory(dh),
    lowPlyHistory(lph),
    captureHistory(cph),
    continuationHistory(ch),
    sharedHistory(sh),
    ttMove(ttm),
    depth(d),
    ply(pl),
    moves(acquire_move_buffer()) {

    if (pos.checkers())
        stage = EVASION_TT + !(ttm && pos.pseudo_legal(ttm));

    else
        stage = (depth > 0 ? MAIN_TT : QSEARCH_TT) + !(ttm && pos.pseudo_legal(ttm));
}

/// MovePicker constructor for ProbCut: we generate captures with SEE greater
/// than or equal to the given threshold.
MovePicker::MovePicker(
  const Position& p, Move ttm, Value th, const GateHistory* dh, const CapturePieceToHistory* cph) :
    pos(p),
    gateHistory(dh),
    captureHistory(cph),
    ttMove(ttm),
    threshold(th),
    moves(acquire_move_buffer()) {

    assert(!pos.checkers());

    stage = PROBCUT_TT + !(ttm && pos.capture_stage(ttm) && pos.pseudo_legal(ttm));
}

// Assigns a numerical value to each move in a list, used for sorting.
// Captures are ordered by Most Valuable Victim (MVV), preferring captures
// with a good history. Quiet moves are ordered using the history tables.
template<GenType Type>
void MovePicker::score() {

    static_assert(Type == CAPTURES || Type == QUIETS || Type == EVASIONS, "Wrong type");

    // Squares attacked by enemy pieces of lesser value than a given piece type
    [[maybe_unused]] Bitboard threatByLesser[PIECE_TYPE_NB], threatenedPieces = Bitboard(0);
    [[maybe_unused]] Bitboard attackedByThem = Bitboard(0);
    if constexpr (Type == QUIETS)
    {
        Color    us = pos.side_to_move();
        Bitboard attacksBy[PIECE_TYPE_NB];
        for (PieceSet ps = pos.piece_types(); ps;)
        {
            PieceType pt  = pop_lsb(ps);
            attacksBy[pt] = pos.count(~us, pt) ? pos.attacks_by(~us, pt) : Bitboard(0);
            attackedByThem |= attacksBy[pt];
        }
        for (PieceSet ps = pos.piece_types(); ps;)
        {
            PieceType pt       = pop_lsb(ps);
            threatByLesser[pt] = Bitboard(0);
            for (PieceSet ps2 = pos.piece_types(); ps2;)
            {
                PieceType pt2 = pop_lsb(ps2);
                // The king is threatened by any piece
                if (pt == KING || PieceValue[MG][pt2] < PieceValue[MG][pt])
                    threatByLesser[pt] |= attacksBy[pt2];
            }
            // Pieces threatened by pieces of lesser material value
            threatenedPieces |= pos.pieces(us, pt) & threatByLesser[pt];
        }
    }

    for (auto& m : *this)
    {
        const Square    to = m.to_sq();
        const Piece     pc = pos.moved_piece(m);
        const PieceType pt = type_of(pc);

        if constexpr (Type == CAPTURES)
            m.value =
              7 * int(PieceValue[MG][pos.piece_on(to_sq(m))]) * !pos.must_capture()
              + (*gateHistory)[pos.side_to_move()][gating_square(m)]
              + (*captureHistory)[pos.moved_piece(m)][to_sq(m)][type_of(pos.piece_on(to_sq(m)))];

        else if constexpr (Type == QUIETS)
        {
            // histories
            m.value = 2 * (*mainHistory)[pos.side_to_move()][from_to(m)];
            // The gate history is only filled by walling variants
            m.value += 4 * (*gateHistory)[pos.side_to_move()][gating_square(m)];
            m.value += 2 * sharedHistory->pawn_entry(pos)[history_slot(pc)][to];
            m.value += (*continuationHistory[0])[history_slot(pc)][to];
            m.value += (*continuationHistory[1])[history_slot(pc)][to];
            m.value += (*continuationHistory[2])[history_slot(pc)][to];
            m.value += (*continuationHistory[3])[history_slot(pc)][to];
            m.value += (*continuationHistory[5])[history_slot(pc)][to];

            // bonus for checks
            m.value += ((pos.check_squares(pt) & to) && pos.see_ge(m, -75)) * 16384;

            // penalty for moving to a square threatened by a lesser piece
            // or bonus for escaping an attack by a lesser piece.
            // With mandatory captures, moving to an attacked square forces a capture
            if (pos.must_capture())
                m.value += 16384 * bool(attackedByThem & to);

            else if (type_of(m) != DROP && pt != KING)
            {
                Square from = from_sq(m);
                int    v    = 20 * (bool(threatenedPieces & from) - bool(threatByLesser[pt] & to));
                m.value += int(PieceValue[MG][pt]) * v;
            }

            if (ply < LOW_PLY_HISTORY_SIZE)
                m.value += 8 * (*lowPlyHistory)[ply][from_to(m)] / (1 + ply);
        }

        else  // Type == EVASIONS
        {
            if (pos.capture_stage(m))
                m.value = PieceValue[MG][pos.piece_on(to_sq(m))] + (1 << 28);
            else
                m.value = (*mainHistory)[pos.side_to_move()][from_to(m)]
                        + (*continuationHistory[0])[history_slot(pos.moved_piece(m))][to_sq(m)];
        }
    }
}

// Returns the next move satisfying a predicate function.
// This never returns the TT move, as it was emitted before.
template<typename Pred>
Move MovePicker::select(Pred filter) {

    for (; cur < endCur; ++cur)
        if (*cur != ttMove && filter())
            return *cur++;

    return Move::none();
}

// This is the most important method of the MovePicker class. We emit one
// new pseudo-legal move on every call until there are no more moves left,
// picking the move with the highest score from a list of generated moves.
Move MovePicker::next_move() {

    constexpr int goodQuietThreshold = -14000;
top:
    switch (stage)
    {

    case MAIN_TT :
    case EVASION_TT :
    case QSEARCH_TT :
    case PROBCUT_TT :
        ++stage;
        assert(pos.legal(ttMove) == MoveList<LEGAL>(pos).contains(ttMove)
               || pos.virtual_drop(ttMove));
        return ttMove;

    case CAPTURE_INIT :
    case PROBCUT_INIT :
    case QCAPTURE_INIT :
        cur = endBadCaptures = moves;
        endCur = endCaptures = generate<CAPTURES>(pos, cur);

        score<CAPTURES>();
        partial_insertion_sort(cur, endCur, std::numeric_limits<int>::min());
        ++stage;
        goto top;

    case GOOD_CAPTURE :
        if (select([&]() {
                if (pos.see_ge(*cur, -cur->value / 18
                                       - 500 * (pos.captures_to_hand() && pos.gives_check(*cur))))
                    return true;
                std::swap(*endBadCaptures++, *cur);
                return false;
            }))
            return *(cur - 1);

        ++stage;
        [[fallthrough]];

    case QUIET_INIT :
        if (!skipQuiets)
        {
            // Skip quiet moves in case of mandatory captures
            if (pos.must_capture() && pos.has_capture())
                endCur = endGenerated = endCaptures;
            else
            {
                endCur = endGenerated = generate<QUIETS>(pos, cur);

                score<QUIETS>();
                partial_insertion_sort(cur, endCur, -3560 * depth);
            }
        }

        ++stage;
        [[fallthrough]];

    case GOOD_QUIET :
        if (!skipQuiets && select([&]() { return cur->value > goodQuietThreshold; }))
            return *(cur - 1);

        // Prepare the pointers to loop over the bad captures
        cur    = moves;
        endCur = endBadCaptures;

        ++stage;
        [[fallthrough]];

    case BAD_CAPTURE :
        if (select([]() { return true; }))
            return *(cur - 1);

        // Prepare the pointers to loop over quiets again
        cur    = endCaptures;
        endCur = endGenerated;

        ++stage;
        [[fallthrough]];

    case BAD_QUIET :
        if (!skipQuiets)
            return select([&]() { return cur->value <= goodQuietThreshold; });

        return Move::none();

    case EVASION_INIT :
        cur    = moves;
        endCur = endGenerated = generate<EVASIONS>(pos, cur);

        score<EVASIONS>();
        partial_insertion_sort(cur, endCur, std::numeric_limits<int>::min());
        ++stage;
        [[fallthrough]];

    case EVASION :
    case QCAPTURE :
        return select([]() { return true; });

    case PROBCUT :
        return select([&]() { return pos.see_ge(*cur, threshold); });
    }

    assert(false);
    return Move::none();  // Silence warning
}

void MovePicker::skip_quiet_moves() { skipQuiets = true; }

// this function must be called after all quiet moves and captures have been generated
}  // namespace Stockfish
