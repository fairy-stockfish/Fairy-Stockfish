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

// Definition of input features HalfKP of the NNUE evaluation functions of USI shogi engines

#include "half_kp_shogi.h"

#include "../../position.h"

#ifdef LARGEBOARDS

namespace Stockfish::Eval::NNUE::Features {

namespace {

const HalfKPShogi::Layout ShogiLayout;

// Offsets of the pieces in hand and on the board of the side of the
// perspective, in the order of the enumeration of YaneuraOu
constexpr IndexType NoIndex = ~IndexType(0);

struct PieceIndex {
    IndexType hand;   // friend, the enemy follows after the maximum count in hand
    IndexType count;  // maximum count in hand
    IndexType board;  // friend, the enemy follows after 81 squares
};

constexpr PieceIndex piece_index(PieceType pt) {
    switch (pt)
    {
    case SHOGI_PAWN :
        return {1, 19, 90 + 0 * 162};
    case LANCE :
        return {39, 5, 90 + 1 * 162};
    case SHOGI_KNIGHT :
        return {49, 5, 90 + 2 * 162};
    case SILVER :
        return {59, 5, 90 + 3 * 162};
    case GOLD :
        return {69, 5, 90 + 4 * 162};
    case BISHOP :
        return {79, 3, 90 + 5 * 162};
    case DRAGON_HORSE :
        return {NoIndex, 0, 90 + 6 * 162};
    case ROOK :
        return {85, 3, 90 + 7 * 162};
    case DRAGON :
        return {NoIndex, 0, 90 + 8 * 162};
    default :
        return {NoIndex, 0, NoIndex};
    }
}

constexpr PieceSet ShogiPieces = piece_set(SHOGI_PAWN) | LANCE | SHOGI_KNIGHT | SILVER | GOLD
                               | BISHOP | DRAGON_HORSE | ROOK | DRAGON | KING;

constexpr PieceType HandPieces[] = {SHOGI_PAWN, LANCE, SHOGI_KNIGHT, SILVER, GOLD, BISHOP, ROOK};

}  // namespace

// Index of a square from the point of view of the perspective. The squares of YaneuraOu
// are enumerated by file from the right and by rank from the top as seen by the first player,
// and are rotated by 180 degrees for the second player.
inline IndexType HalfKPShogi::orient(Color perspective, Square s) {
    IndexType sq = IndexType(FILE_I - file_of(s)) * 9 + IndexType(RANK_9 - rank_of(s));
    return perspective == WHITE ? sq : Squares - 1 - sq;
}

// Index of a feature for a given king position and another piece on some square
inline IndexType HalfKPShogi::make_index(Color perspective, Square s, Piece pc, IndexType ksq) {
    return FeEnd * ksq + piece_index(type_of(pc)).board
         + (color_of(pc) == perspective ? 0 : Squares) + orient(perspective, s);
}

// Index of a feature for a given king position and another piece in hand
inline IndexType
HalfKPShogi::make_index(Color perspective, int handIndex, Piece pc, IndexType ksq) {
    const PieceIndex index = piece_index(type_of(pc));
    return FeEnd * ksq + index.hand + (color_of(pc) == perspective ? 0 : index.count)
         + IndexType(handIndex);
}

// Returns the layout if the variant is shogi and the number of dimensions matches
const HalfKPShogi::Layout* HalfKPShogi::find_layout(const Variant* v, std::size_t dimensions) {
    return dimensions == Squares * FeEnd && v->maxFile == FILE_I && v->maxRank == RANK_9
            && v->pieceTypes == ShogiPieces && v->pieceDrops && v->capturesToHand
            && v->nnueLayouts[NNUE_LAYOUT_KING].king == KING
           ? &ShogiLayout
           : nullptr;
}

// The square of the king
Square HalfKPShogi::king_square(const Position& pos, const Layout&, Color c) {
    return pos.square(c, KING);
}

// Returns whether the position has the kings required by the features
bool HalfKPShogi::applicable(const Position& pos, const Layout&) {
    return pos.count(WHITE, KING) == 1 && pos.count(BLACK, KING) == 1;
}

// Get a list of indices for active features
void HalfKPShogi::append_active_indices(const Position& pos,
                                        const Layout&   layout,
                                        Color           perspective,
                                        IndexList&      active) {
    // Compute the active features as the difference to an empty board
    PieceState state{};
    IndexList  removed;
    append_changed_indices(pos, layout, perspective, state, removed, active);
}

// append_changed_indices() : get a list of indices for recently changed features
void HalfKPShogi::append_changed_indices(const Layout&,
                                         Square            ksq,
                                         const DirtyPiece& dp,
                                         Color             perspective,
                                         IndexList&        removed,
                                         IndexList&        added,
                                         const Position&) {
    const IndexType oriented_ksq = orient(perspective, ksq);
    for (int i = 0; i < dp.dirty_num; ++i)
    {
        Piece pc = dp.piece[i];

        // Kings are not part of the features
        if (type_of(pc) == KING)
            continue;

        if (dp.from[i] != SQ_NONE)
            removed.push_back(make_index(perspective, dp.from[i], pc, oriented_ksq));
        else if (dp.handPiece[i] != NO_PIECE)
            removed.push_back(
              make_index(perspective, dp.handCount[i] - 1, dp.handPiece[i], oriented_ksq));
        if (dp.to[i] != SQ_NONE)
            added.push_back(make_index(perspective, dp.to[i], pc, oriented_ksq));
        else if (dp.handPiece[i] != NO_PIECE)
            added.push_back(
              make_index(perspective, dp.handCount[i] - 1, dp.handPiece[i], oriented_ksq));
    }
}

// Get the lists of indices that differ between a piece state
// and the position, and update the piece state to the position
void HalfKPShogi::append_changed_indices(const Position& pos,
                                         const Layout&   layout,
                                         Color           perspective,
                                         PieceState&     state,
                                         IndexList&      removed,
                                         IndexList&      added) {
    const IndexType oriented_ksq = orient(perspective, king_square(pos, layout, perspective));
    const Bitboard  occupied     = (pos.pieces(WHITE) | pos.pieces(BLACK)) & ~pos.pieces(KING);

    for (Bitboard bb = state.pieceBB | occupied; bb;)
    {
        Square s      = pop_lsb(bb);
        Piece  before = state.pieces[s];
        Piece  after  = (occupied & s) ? pos.piece_on(s) : NO_PIECE;
        if (before == after)
            continue;

        if (before != NO_PIECE)
            removed.push_back(make_index(perspective, s, before, oriented_ksq));
        if (after != NO_PIECE)
            added.push_back(make_index(perspective, s, after, oriented_ksq));
        state.pieces[s] = after;
    }
    state.pieceBB = occupied;

    // Indices for pieces in hand
    int pieces = popcount(occupied);
    for (Color c : {WHITE, BLACK})
        for (PieceType pt : HandPieces)
        {
            int before = state.handCount[c][pt];
            int after  = std::max(pos.count_in_hand(c, pt), 0);
            pieces += after;

            for (int i = after; i < before; i++)
                removed.push_back(make_index(perspective, i, make_piece(c, pt), oriented_ksq));
            for (int i = before; i < after; i++)
                added.push_back(make_index(perspective, i, make_piece(c, pt), oriented_ksq));
            state.handCount[c][pt] = std::int16_t(after);
        }

    // YaneuraOu always has 38 active features and uses the index zero
    // for every piece missing in handicap games. The count of these
    // features is stored at the unused piece type of the piece state.
    int before = state.handCount[WHITE][NO_PIECE_TYPE];
    int after  = std::max(MaxPieces - pieces, 0);

    for (int i = after; i < before; i++)
        removed.push_back(FeEnd * oriented_ksq);
    for (int i = before; i < after; i++)
        added.push_back(FeEnd * oriented_ksq);
    state.handCount[WHITE][NO_PIECE_TYPE] = std::int16_t(after);
}

bool HalfKPShogi::requires_refresh(const Layout&,
                                   const DirtyPiece& dp,
                                   Color             perspective,
                                   const Position&) {
    return dp.piece[0] == make_piece(perspective, KING);
}

}  // namespace Stockfish::Eval::NNUE::Features

#endif  // LARGEBOARDS
