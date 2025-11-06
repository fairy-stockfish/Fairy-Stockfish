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

#ifndef VISIBILITY_H_INCLUDED
#define VISIBILITY_H_INCLUDED

#include "../types.h"
#include "../bitboard.h"

namespace Stockfish {

class Position;

namespace FogOfWar {

/// Visibility module computes which squares are visible to a player
/// under Fog-of-War chess rules (Appendix A of the Obscuro paper).
///
/// Rules (from paper Appendix A):
/// 1. Players can see all squares their pieces can legally move to
/// 2. Blocked pawns do NOT reveal their blocker
/// 3. En-passant target square is visible when capturable
/// 4. Players always know their own pieces and legal moves
/// 5. Capture-the-king ends the game immediately
///

struct VisibilityInfo {
    Bitboard visible;           // All squares visible to the player
    Bitboard myPieces;          // Player's own pieces (always known)
    Bitboard seenOpponentPieces; // Opponent pieces that are visible
};

/// compute_visibility() returns all squares visible to the side-to-move
/// in the given position under FoW rules.
VisibilityInfo compute_visibility(const Position& pos);

/// is_visible() checks if a specific square is visible to the side-to-move
bool is_visible(const Position& pos, Square s, const VisibilityInfo& vi);

/// compute_pawn_vision() computes visibility from pawns (excluding blocked squares)
Bitboard compute_pawn_vision(const Position& pos, Color us);

/// compute_piece_vision() computes visibility from non-pawn pieces
Bitboard compute_piece_vision(const Position& pos, Color us);

} // namespace FogOfWar

} // namespace Stockfish

#endif // #ifndef VISIBILITY_H_INCLUDED
