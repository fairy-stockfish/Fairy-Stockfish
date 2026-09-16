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
#include <utility>

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


// Sort moves in descending order up to and including a given limit.
// The order of moves smaller than the limit is left unspecified.
void partial_insertion_sort(ExtMove* begin, ExtMove* end, int limit) {

    for (ExtMove *sortedEnd = begin, *p = begin + 1; p < end; ++p)
        if (p->value >= limit)
        {
            ExtMove tmp = *p, *q;
            *p          = *++sortedEnd;
            for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

}  // namespace


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
    ply(pl) {

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
    threshold(th) {

    assert(!pos.checkers());

    stage = PROBCUT_TT + !(ttm && pos.capture_stage(ttm) && pos.pseudo_legal(ttm));
}

// Assigns a numerical value to each move in a list, used for sorting.
// Captures are ordered by Most Valuable Victim (MVV), preferring captures
// with a good history. Quiets moves are ordered using the history tables.
template<GenType Type>
void MovePicker::score() {

    static_assert(Type == CAPTURES || Type == QUIETS || Type == EVASIONS, "Wrong type");

    // Squares attacked by enemy pieces of lesser value than a given piece type
    [[maybe_unused]] Bitboard threatByLesser[PIECE_TYPE_NB], threatenedPieces = Bitboard(0);
    if constexpr (Type == QUIETS)
    {
        Color    us = pos.side_to_move();
        Bitboard attacksBy[PIECE_TYPE_NB];
        for (PieceSet ps = pos.piece_types(); ps;)
        {
            PieceType pt  = pop_lsb(ps);
            attacksBy[pt] = pos.count(~us, pt) ? pos.attacks_by(~us, pt) : Bitboard(0);
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
        const Square    from          = m.from_sq();
        const Square    to            = m.to_sq();
        const Piece     pc            = pos.moved_piece(m);
        const PieceType pt            = type_of(pc);
        const Piece     capturedPiece = pos.piece_on(to);

        if constexpr (Type == CAPTURES)
            m.value =
              7 * int(PieceValue[MG][pos.piece_on(to_sq(m))])
              + (*gateHistory)[pos.side_to_move()][gating_square(m)]
              + (*captureHistory)[pos.moved_piece(m)][to_sq(m)][type_of(pos.piece_on(to_sq(m)))];

        else if constexpr (Type == QUIETS)
        {
            Piece     pc = pos.moved_piece(m);
            PieceType pt = type_of(pc);
            Square    to = to_sq(m);

            // histories
            m.value = 2 * (*mainHistory)[pos.side_to_move()][from_to(m)];
            m.value += (*gateHistory)[pos.side_to_move()][gating_square(m)];
            m.value += 2 * sharedHistory->pawn_entry(pos)[pc][to];
            m.value += (*continuationHistory[0])[history_slot(pc)][to];
            m.value += (*continuationHistory[1])[history_slot(pc)][to];
            m.value += (*continuationHistory[2])[history_slot(pc)][to];
            m.value += (*continuationHistory[3])[history_slot(pc)][to];
            m.value += (*continuationHistory[5])[history_slot(pc)][to];

            // bonus for checks
            m.value += (bool(pos.check_squares(pt) & to) && pos.see_ge(m, -75)) * 16384;

            // penalty for moving to a square threatened by a lesser piece
            // or bonus for escaping an attack by a lesser piece.
            if (type_of(m) != DROP && pt != KING)
            {
                Square from = from_sq(m);
                int    v    = threatByLesser[pt] & to ? -19 : 20 * bool(threatenedPieces & from);
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
