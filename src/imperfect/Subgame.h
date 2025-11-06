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

#ifndef SUBGAME_H_INCLUDED
#define SUBGAME_H_INCLUDED

#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include "../types.h"
#include "../position.h"

namespace Stockfish {
namespace FogOfWar {

/// Forward declarations
struct InfosetNode;
struct GameTreeNode;

/// SequenceId uniquely identifies a sequence of moves (infoset)
using SequenceId = uint64_t;

/// NodeId uniquely identifies a node in the game tree
using NodeId = uint64_t;

/// InfosetNode represents an information set where a player cannot distinguish
/// between different states. In FoW chess with perfect opponent model,
/// |I ∩ J| = 1 (player sequence + opponent sequence uniquely identifies node)
struct InfosetNode {
    SequenceId sequenceId;          // Unique ID for this infoset
    Color player;                    // Player to act at this infoset
    std::vector<Move> actions;       // Legal actions from this infoset
    std::vector<float> regrets;      // Regret values for CFR
    std::vector<float> strategy;     // Current strategy (probabilities)
    std::vector<float> cumulativeStrategy; // Cumulative strategy for averaging
    std::vector<int> visitCounts;    // Visit counts for each action
    int totalVisits;                 // Total visits to this infoset
    float value;                     // Current value estimate
    bool expanded;                   // Whether this node has been expanded

    // For PUCT selection
    std::vector<float> qValues;      // Q-values for each action
    std::vector<float> variances;    // Variance estimates

    InfosetNode() : totalVisits(0), value(0.0f), expanded(false) {}
};

/// GameTreeNode represents a specific state in the game tree
struct GameTreeNode {
    NodeId nodeId;
    std::string stateFen;            // FEN string instead of Position object
    SequenceId ourSequence;          // Our move sequence to this node
    SequenceId theirSequence;        // Opponent's move sequence to this node
    bool terminal;                   // Is this a terminal node?
    float terminalValue;             // Value if terminal
    int depth;                       // Depth in tree
    bool inKLUSS;                    // Is this node in the KLUSS region?
    bool expanded;                   // Has this node been expanded?

    // Parent and children
    GameTreeNode* parent;
    std::vector<std::unique_ptr<GameTreeNode>> children;

    GameTreeNode() : nodeId(0), terminal(false), terminalValue(0.0f),
                     depth(0), inKLUSS(false), expanded(false), parent(nullptr) {}
};

/// GadgetType for resolve/maxmargin
enum class GadgetType {
    NONE,
    RESOLVE,
    MAXMARGIN
};

/// Subgame represents the knowledge-limited subgame (KLUSS)
/// Implements 2-KLUSS: keep order-2 neighborhood, unfrozen at distance 1
class Subgame {
public:
    Subgame() : rootNode(nullptr), currentGadget(GadgetType::NONE),
                resolveEntered(false), nodeIdCounter(0) {}

    /// construct() builds the subgame from sampled states (Figure 9)
    /// Takes FEN strings representing sampled positions
    void construct(const std::vector<std::string>& sampledStateFens,
                   int minInfosetSize = 256);

    /// expand_node() expands a leaf node by generating children
    GameTreeNode* expand_node(GameTreeNode* leaf, Position& pos);

    /// get_infoset() returns or creates an infoset for a sequence
    InfosetNode* get_infoset(SequenceId seqId, Color player);

    /// compute_kluss_region() computes the 2-KLUSS (order-2 knowledge region)
    /// Takes FEN strings representing sampled positions
    void compute_kluss_region(const std::vector<std::string>& sampledStateFens);

    /// is_in_kluss() checks if a node is in the KLUSS region
    bool is_in_kluss(const GameTreeNode* node) const;

    /// Gadget management
    void set_gadget_type(GadgetType type) { currentGadget = type; }
    GadgetType get_gadget_type() const { return currentGadget; }
    void mark_resolve_entered() { resolveEntered = true; }
    bool has_resolve_entered() const { return resolveEntered; }

    /// Accessors
    GameTreeNode* root() { return rootNode.get(); }
    const GameTreeNode* root() const { return rootNode.get(); }
    size_t num_infosets() const { return infosets.size(); }
    std::unordered_map<SequenceId, InfosetNode>& get_infosets() { return infosets; }

    /// Statistics
    size_t count_nodes() const;
    int average_depth() const;

private:
    std::unique_ptr<GameTreeNode> rootNode;
    std::unordered_map<SequenceId, InfosetNode> infosets;
    GadgetType currentGadget;
    bool resolveEntered;
    std::atomic<NodeId> nodeIdCounter;

    /// Helper: Generate sequence ID from move sequence
    SequenceId compute_sequence_id(const std::vector<Move>& moves);

    /// Helper: Build tree from sampled states (FEN strings)
    void build_tree_from_samples(const std::vector<std::string>& sampledStateFens);
};

/// compute_sequence_id() generates a unique ID for a move sequence
SequenceId compute_sequence_id_from_moves(const std::vector<Move>& moves);

/// compute_alternative_value() computes v_alt for Resolve gadget (Appendix B.3.1)
float compute_alternative_value(const InfosetNode* infoset,
                                 const std::vector<float>& currentX,
                                 const std::vector<float>& currentY);

/// compute_gift() computes the gift value for Resolve gadget (Appendix B.3.1)
float compute_gift(const InfosetNode* infoset,
                   const std::vector<float>& currentX,
                   const std::vector<float>& currentY);

} // namespace FogOfWar
} // namespace Stockfish

#endif // #ifndef SUBGAME_H_INCLUDED
