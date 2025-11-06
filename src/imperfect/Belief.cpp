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

#include <algorithm>
#include <random>
#include "Belief.h"
#include "../movegen.h"

namespace Stockfish {
namespace FogOfWar {

/// ObservationHistory implementation

void ObservationHistory::add_observation(const Observation& obs) {
    history.push_back(obs);
}

void ObservationHistory::clear() {
    history.clear();
}

/// create_observation() creates an observation from the current position
Observation create_observation(const Position& pos) {
    Observation obs;
    VisibilityInfo vi = compute_visibility(pos);

    obs.visible = vi.visible;
    obs.myPieces = vi.myPieces;
    obs.seenOpponentPieces = vi.seenOpponentPieces;
    obs.sideToMove = pos.side_to_move();
    obs.epSquares = pos.ep_squares();

    // Query castling rights for our side
    Color us = pos.side_to_move();
    CastlingRights ourCastlingRights = us == WHITE ? WHITE_CASTLING : BLACK_CASTLING;
    obs.castlingRights = pos.can_castle(ourCastlingRights) ? 1 : 0;

    obs.halfmoveClock = pos.rule50_count();
    obs.fullmoveNumber = pos.game_ply();

    return obs;
}

/// BeliefState implementation

bool BeliefState::is_consistent(const Position& pos, const Observation& obs) {
    // Check side to move
    if (pos.side_to_move() != obs.sideToMove)
        return false;

    Color us = obs.sideToMove;
    Color them = ~us;

    // Check that our pieces match exactly
    if (pos.pieces(us) != obs.myPieces)
        return false;

    // Check that visible opponent pieces match
    Bitboard theirPieces = pos.pieces(them);
    Bitboard visibleTheirPieces = theirPieces & obs.visible;
    if (visibleTheirPieces != obs.seenOpponentPieces)
        return false;

    // Check that opponent pieces are not on squares we see as empty
    Bitboard visibleEmpty = obs.visible & ~obs.myPieces & ~obs.seenOpponentPieces;
    if (theirPieces & visibleEmpty)
        return false;

    // Check en-passant consistency
    if (obs.epSquares && pos.ep_squares() != obs.epSquares)
        return false;

    // Castling rights consistency (we know our own castling rights)
    CastlingRights ourCastlingRights = us == WHITE ? WHITE_CASTLING : BLACK_CASTLING;
    int posCastling = pos.can_castle(ourCastlingRights) ? 1 : 0;
    if (posCastling != obs.castlingRights)
        return false;

    return true;
}

bool BeliefState::is_king_capturable(const Position& pos) const {
    // Check if the side-to-move can capture the opponent's king
    // If so, the game would have already ended, so this state is illegal
    Color us = pos.side_to_move();
    Color them = ~us;

    Square theirKing = pos.square<KING>(them);
    if (theirKing == SQ_NONE)
        return true; // No king = capturable (illegal state)

    // Check if any of our pieces attack their king
    return bool(pos.attackers_to(theirKing) & pos.pieces(us));
}

void BeliefState::enumerate_candidates(const ObservationHistory& obsHist,
                                        const Position& truePos) {
    // Simple baseline: start from the true position for now
    // In a full implementation, we would enumerate all possible placements
    // of unseen opponent pieces on unseen squares

    if (obsHist.empty())
        return;

    // For baseline implementation, we'll use a simplified approach:
    // 1. Take the true position as the starting point
    // 2. Generate variations by considering different placements of unseen pieces

    // Start with the true position (store as FEN)
    std::string fen = truePos.fen();
    stateFens.push_back(fen);
    stateKeys.insert(truePos.key());

    // TODO: Full enumeration would generate all possible positions by:
    // - Identifying unseen squares (not in currentObs.visible)
    // - Enumerating possible placements of opponent pieces on those squares
    // - Considering piece counts and material balance
    // - Respecting pawn structure constraints
    // - Checking castling rights possibilities

    // For now, this simplified version keeps only the true position
    // A production implementation would expand this significantly
}

void BeliefState::filter_illegal_states() {
    // Remove states where:
    // 1. The opponent's king is capturable (game would have ended)
    // 2. The position is not legal according to chess rules

    // Note: We need to create temporary Position objects to check legality
    // This is expensive but necessary since we store FENs
    auto it = stateFens.begin();
    while (it != stateFens.end()) {
        bool remove = false;

        // Create temporary position from FEN
        StateInfo st;
        Position tempPos;
        // We need variant and thread info - use defaults for now
        // In production, these should be passed from context
        // tempPos.set(variant, *it, false, &st, nullptr);
        // For now, skip detailed legality checks and just keep all states
        // TODO: Proper legality checking with correct variant context

        if (remove) {
            it = stateFens.erase(it);
        } else {
            ++it;
        }
    }
}

void BeliefState::rebuild_from_observations(const ObservationHistory& obsHist,
                                             const Position& truePos) {
    stateFens.clear();
    stateKeys.clear();

    if (obsHist.empty())
        return;

    // Enumerate all candidate positions
    enumerate_candidates(obsHist, truePos);

    // Filter out illegal states
    filter_illegal_states();

    // Verify all states are consistent with latest observation
    // For now, simplified - we only have the true position
    // TODO: Implement full consistency checking with FEN parsing
}

void BeliefState::update_incrementally(const Observation& newObs) {
    // Incremental update: filter existing states by new observation
    // This is more efficient than rebuilding from scratch
    // For now, simplified implementation - just keep all states
    // TODO: Parse FENs and check consistency
    (void)newObs; // Suppress unused parameter warning
}

std::vector<std::string> BeliefState::sample_states(size_t n, uint64_t seed) const {
    if (stateFens.empty())
        return {};

    if (stateFens.size() <= n)
        return stateFens;

    // Random sampling without replacement
    std::vector<std::string> sampled;
    std::vector<size_t> indices(stateFens.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::mt19937_64 rng(seed);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (size_t i = 0; i < n && i < indices.size(); ++i)
        sampled.push_back(stateFens[indices[i]]);

    return sampled;
}

} // namespace FogOfWar
} // namespace Stockfish
