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

#include "Subgame.h"
#include "../movegen.h"
#include <algorithm>
#include <numeric>

namespace Stockfish {
namespace FogOfWar {

/// compute_sequence_id_from_moves() generates a hash for a move sequence
SequenceId compute_sequence_id_from_moves(const std::vector<Move>& moves) {
    SequenceId hash = 0xcbf29ce484222325ULL; // FNV-1a offset basis
    constexpr SequenceId prime = 0x100000001b3ULL;

    for (Move m : moves) {
        hash ^= static_cast<SequenceId>(m);
        hash *= prime;
    }

    return hash;
}

SequenceId Subgame::compute_sequence_id(const std::vector<Move>& moves) {
    return compute_sequence_id_from_moves(moves);
}

InfosetNode* Subgame::get_infoset(SequenceId seqId, Color player) {
    auto it = infosets.find(seqId);
    if (it != infosets.end())
        return &it->second;

    // Create new infoset
    InfosetNode& iset = infosets[seqId];
    iset.sequenceId = seqId;
    iset.player = player;
    return &iset;
}

void Subgame::construct(const std::vector<std::string>& sampledStateFens,
                        int minInfosetSize) {
    // Clear existing tree
    rootNode = std::make_unique<GameTreeNode>();
    infosets.clear();
    nodeIdCounter = 0;
    resolveEntered = false;

    // Build tree from sampled states
    build_tree_from_samples(sampledStateFens);

    // Compute KLUSS region (2-KLUSS: order-2 neighborhood, unfrozen at distance 1)
    compute_kluss_region(sampledStateFens);
}

void Subgame::build_tree_from_samples(const std::vector<std::string>& sampledStateFens) {
    if (sampledStateFens.empty())
        return;

    // Initialize root node with first sampled state
    rootNode->nodeId = nodeIdCounter++;
    rootNode->stateFen = sampledStateFens.front();
    // Store FEN instead of Position
    // TODO: Parse FEN when needed for move generation
    rootNode->ourSequence = 0;
    rootNode->theirSequence = 0;
    rootNode->depth = 0;
    rootNode->inKLUSS = true;

    // For now, create a simple root infoset without parsing FENs
    // TODO: Parse FENs to determine correct player and actions
    InfosetNode* rootInfoset = get_infoset(0, WHITE); // Default to WHITE

    // Initialize with empty actions (will be filled during expansion)
    rootInfoset->regrets.clear();
    rootInfoset->strategy.clear();
    rootInfoset->cumulativeStrategy.clear();
    rootInfoset->visitCounts.clear();
    rootInfoset->qValues.clear();
    rootInfoset->variances.clear();
}

void Subgame::compute_kluss_region(const std::vector<std::string>& sampledStateFens) {
    // For 2-KLUSS: nodes are in the knowledge region if they are reachable
    // within 2 moves from any sampled state
    // Simplified implementation: mark root and immediate children as in KLUSS
    (void)sampledStateFens; // Suppress unused parameter warning

    if (!rootNode)
        return;

    // Root is always in KLUSS
    rootNode->inKLUSS = true;

    // Children of root are also in KLUSS (order-1 neighborhood)
    for (auto& child : rootNode->children)
        child->inKLUSS = true;
}

bool Subgame::is_in_kluss(const GameTreeNode* node) const {
    return node && node->inKLUSS;
}

GameTreeNode* Subgame::expand_node(GameTreeNode* leaf, Position& pos) {
    if (!leaf || leaf->expanded)
        return nullptr;

    // Generate children for this leaf
    StateInfo st;
    std::vector<Move> legalMoves;

    for (const auto& m : MoveList<LEGAL>(pos))
        legalMoves.push_back(m);

    if (legalMoves.empty()) {
        // Terminal node (checkmate or stalemate)
        leaf->terminal = true;
        leaf->terminalValue = pos.checkers() ? -1.0f : 0.0f; // Checkmate or stalemate
        return leaf;
    }

    // Create child nodes
    for (Move m : legalMoves) {
        auto child = std::make_unique<GameTreeNode>();
        child->nodeId = nodeIdCounter++;
        child->parent = leaf;
        child->depth = leaf->depth + 1;

        // Update sequences
        child->ourSequence = leaf->ourSequence; // Will be updated with move
        child->theirSequence = leaf->theirSequence;

        // Make move to get child state FEN
        pos.do_move(m, st);
        child->stateFen = pos.fen();
        pos.undo_move(m);

        leaf->children.push_back(std::move(child));
    }

    leaf->expanded = true;
    return leaf;
}

size_t Subgame::count_nodes() const {
    if (!rootNode)
        return 0;

    size_t count = 1;
    std::vector<const GameTreeNode*> stack = {rootNode.get()};

    while (!stack.empty()) {
        const GameTreeNode* node = stack.back();
        stack.pop_back();

        for (const auto& child : node->children) {
            count++;
            stack.push_back(child.get());
        }
    }

    return count;
}

int Subgame::average_depth() const {
    if (!rootNode)
        return 0;

    int totalDepth = 0;
    int nodeCount = 0;
    std::vector<const GameTreeNode*> stack = {rootNode.get()};

    while (!stack.empty()) {
        const GameTreeNode* node = stack.back();
        stack.pop_back();

        totalDepth += node->depth;
        nodeCount++;

        for (const auto& child : node->children)
            stack.push_back(child.get());
    }

    return nodeCount > 0 ? totalDepth / nodeCount : 0;
}

/// compute_alternative_value() for Resolve gadget (Appendix B.3.1)
/// Uses current (x,y) instead of best-response values for stability
float compute_alternative_value(const InfosetNode* infoset,
                                 const std::vector<float>& currentX,
                                 const std::vector<float>& currentY) {
    // Simplified implementation: return current value estimate
    // Full implementation would compute min(evaluate(s), v*) for new states
    return infoset ? infoset->value : 0.0f;
}

/// compute_gift() for Resolve gadget (Appendix B.3.1)
float compute_gift(const InfosetNode* infoset,
                   const std::vector<float>& currentX,
                   const std::vector<float>& currentY) {
    // Gift is the value opponent forfeits by playing into the subgame
    // Simplified: return difference between alternative value and current value
    float altValue = compute_alternative_value(infoset, currentX, currentY);
    float currentValue = infoset ? infoset->value : 0.0f;
    return altValue - currentValue;
}

} // namespace FogOfWar
} // namespace Stockfish
