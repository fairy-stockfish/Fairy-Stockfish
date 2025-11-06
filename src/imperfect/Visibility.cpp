/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2024 Fabian Fichter

  Fairy-Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Fairy-Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Visibility.h"
#include "../position.h"
#include "../movegen.h"

namespace Stockfish {
namespace FogOfWar {

/// compute_pawn_vision() computes all squares visible from pawns
/// Special rule: Blocked pawns do NOT reveal their blocker
Bitboard compute_pawn_vision(const Position& pos, Color us) {
    Bitboard visible = 0;
    Bitboard ourPawns = pos.pieces(us, PAWN);

    // Pawn captures (diagonal attacks) are always visible
    if (us == WHITE) {
        Bitboard leftAttacks = shift<NORTH_WEST>(ourPawns) & pos.board_bb();
        Bitboard rightAttacks = shift<NORTH_EAST>(ourPawns) & pos.board_bb();
        visible |= leftAttacks | rightAttacks;
    } else {
        Bitboard leftAttacks = shift<SOUTH_EAST>(ourPawns) & pos.board_bb();
        Bitboard rightAttacks = shift<SOUTH_WEST>(ourPawns) & pos.board_bb();
        visible |= leftAttacks | rightAttacks;
    }

    // En-passant squares are visible (paper: en-passant pawn is visible when capturable)
    Bitboard epSquares = pos.ep_squares();
    if (epSquares)
    {
        // Make EP squares visible if any of our pawns can capture
        Bitboard epSquaresCopy = epSquares;
        while (epSquaresCopy)
        {
            Square epSq = pop_lsb(epSquaresCopy);
            Bitboard epCaptorers = ourPawns & pawn_attacks_bb(~us, epSq);
            if (epCaptorers)
                visible |= epSq;
        }
    }

    // Forward moves: single and double pawn pushes
    // IMPORTANT: We see the destination squares, but NOT the blocking piece
    // (paper: "blocked pawn's blocker is NOT revealed")
    Bitboard emptySquares = ~pos.pieces();
    Bitboard singlePush;
    Bitboard doublePush;

    if (us == WHITE) {
        singlePush = shift<NORTH>(ourPawns) & emptySquares & pos.board_bb();
        visible |= singlePush;
        doublePush = shift<NORTH>(singlePush) & emptySquares & Rank3BB & pos.board_bb();
        visible |= doublePush;
    } else {
        singlePush = shift<SOUTH>(ourPawns) & emptySquares & pos.board_bb();
        visible |= singlePush;
        doublePush = shift<SOUTH>(singlePush) & emptySquares & Rank6BB & pos.board_bb();
        visible |= doublePush;
    }

    return visible;
}

/// compute_piece_vision() computes all squares visible from non-pawn pieces
Bitboard compute_piece_vision(const Position& pos, Color us) {
    Bitboard visible = 0;

    // For each piece type (except pawns), compute legal move destinations
    for (PieceType pt = KNIGHT; pt <= KING; ++pt)
    {
        if (!(pos.variant()->pieceTypes & pt))
            continue;

        Bitboard pieces = pos.pieces(us, pt);
        while (pieces)
        {
            Square from = pop_lsb(pieces);
            // Get all attacks from this piece (pseudo-legal destinations)
            Bitboard attacks = pos.attacks_from(us, pt, from) & pos.board_bb();
            visible |= attacks;
        }
    }

    return visible;
}

/// compute_visibility() returns complete visibility information for side-to-move
VisibilityInfo compute_visibility(const Position& pos) {
    VisibilityInfo vi;
    Color us = pos.side_to_move();
    Color them = ~us;

    // We always know our own pieces
    vi.myPieces = pos.pieces(us);

    // Compute all squares we can move to (legal destinations)
    vi.visible = vi.myPieces; // We can always see our own pieces

    // Add pawn vision (excluding blocked squares)
    vi.visible |= compute_pawn_vision(pos, us);

    // Add vision from all other pieces
    vi.visible |= compute_piece_vision(pos, us);

    // Opponent pieces that are on visible squares
    vi.seenOpponentPieces = vi.visible & pos.pieces(them);

    return vi;
}

/// is_visible() checks if a specific square is visible
bool is_visible(const Position&, Square s, const VisibilityInfo& vi) {
    return vi.visible & s;
}

} // namespace FogOfWar
} // namespace Stockfish
