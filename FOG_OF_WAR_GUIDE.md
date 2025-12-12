# Fog-of-War Chess Guide for Fairy-Stockfish

This guide explains how to play and analyze Fog-of-War (FoW) chess variants using Fairy-Stockfish from the command line.

## What is Fog-of-War Chess?

In Fog-of-War chess, players can only see squares that their pieces can attack or move to. The opponent's pieces on unseen squares are hidden, creating an imperfect information game similar to poker or StarCraft.

## Available FoW Variants

Fairy-Stockfish supports three FoW-based variants:

- **fogofwar**: Standard chess with fog-of-war rules
- **darkcrazyhouse**: Crazyhouse with fog-of-war rules. When you have a piece in hand, all empty squares are visible (you can drop anywhere)
- **darkcrazyhouse2**: Crazyhouse with fog-of-war rules. You can only drop pieces on visible squares (standard FoW vision rules apply)

## Basic Usage (Standard FoW Play)

### Using UCI Protocol

1. Start Fairy-Stockfish and initialize UCI:
```
uci
```

2. Select the FoW variant:
```
setoption name UCI_Variant value fogofwar
```

3. Set up the starting position:
```
position startpos
```

4. Make moves and get the fog view:
```
position startpos moves e2e4
go movetime 1000
```

5. The engine will search and return its best move considering the fog-of-war rules.

### Using CECP/XBoard Protocol

1. Set the protocol:
```
xboard
```

2. Select the variant:
```
variant fogofwar
```

3. (Optional) Display the board:
```
d
```

4. Make your first move:
```
e2e4
```

5. Ask the engine to play:
```
go
```

6. Continue playing by making moves. The engine will automatically reply after the first `go` command.

## Advanced: Obscuro-Style Imperfect Information Search

Fairy-Stockfish includes an advanced search algorithm specifically designed for imperfect information games, based on the Obscuro research paper. This uses game-theoretic search techniques including:

- **Belief state management**: Tracks all possible board states consistent with observations
- **Counterfactual Regret Minimization (CFR)**: Computes Nash equilibrium strategies
- **Knowledge-Limited Subgame Solving (KLUSS)**: Solves local subgames efficiently
- **Game-Theoretic CFR expansion**: Explores the game tree using PUCT selection

### Enabling Obscuro Search

To use the advanced imperfect information search:

```
uci
setoption name UCI_Variant value fogofwar
setoption name UCI_FoW value true
setoption name UCI_IISearch value true
position startpos
go movetime 5000
```

### FoW Configuration Options

The following UCI options control the Obscuro search behavior:

#### `UCI_FoW` (default: false)
Enable Fog-of-War search mode. Must be set to `true` to use imperfect information search.
```
setoption name UCI_FoW value true
```

#### `UCI_IISearch` (default: true)
Enable Imperfect Information Search. When true, uses the full Obscuro algorithm. When false, uses simplified search.
```
setoption name UCI_IISearch value true
```

#### `UCI_MinInfosetSize` (default: 256, range: 1-10000)
Minimum number of nodes in an information set before expansion. Larger values create bigger subgames, improving solution quality but increasing computation time.
```
setoption name UCI_MinInfosetSize value 256
```

#### `UCI_ExpansionThreads` (default: 2, range: 1-16)
Number of threads for GT-CFR expansion (PUCT-based game tree exploration). More threads explore more branches in parallel.
```
setoption name UCI_ExpansionThreads value 2
```

#### `UCI_CFRThreads` (default: 1, range: 1-8)
Number of threads for CFR solving. Typically 1 is sufficient since CFR is memory-bound.
```
setoption name UCI_CFRThreads value 1
```

#### `UCI_PurifySupport` (default: 3, range: 1-10)
Maximum support for action purification (max number of actions to consider). Lower values force more deterministic play, higher values allow more mixed strategies.
```
setoption name UCI_PurifySupport value 3
```

#### `UCI_PUCT_C` (default: 100, range: 1-1000)
Exploration constant for PUCT selection in GT-CFR. Higher values encourage more exploration, lower values focus on exploitation.
```
setoption name UCI_PUCT_C value 100
```

#### `UCI_FoW_TimeMs` (default: 5000, range: 100-600000)
Time budget in milliseconds for FoW search. The planner will use approximately this much time to compute strategies before selecting an action.
```
setoption name UCI_FoW_TimeMs value 5000
```

### Example: Full Configuration

Here's a complete example configuring all FoW options for strong play:

```
uci
setoption name UCI_Variant value fogofwar
setoption name UCI_FoW value true
setoption name UCI_IISearch value true
setoption name UCI_MinInfosetSize value 512
setoption name UCI_ExpansionThreads value 4
setoption name UCI_CFRThreads value 1
setoption name UCI_PurifySupport value 3
setoption name UCI_PUCT_C value 100
setoption name UCI_FoW_TimeMs value 10000
position startpos
go
```

### Tuning for Different Scenarios

**Fast play (weaker but quick)**:
```
setoption name UCI_MinInfosetSize value 128
setoption name UCI_ExpansionThreads value 1
setoption name UCI_FoW_TimeMs value 1000
```

**Strong play (slower but better)**:
```
setoption name UCI_MinInfosetSize value 1024
setoption name UCI_ExpansionThreads value 8
setoption name UCI_FoW_TimeMs value 30000
```

**Blitz/Bullet (very fast)**:
```
setoption name UCI_MinInfosetSize value 64
setoption name UCI_ExpansionThreads value 2
setoption name UCI_FoW_TimeMs value 500
```

## Playing Dark Crazyhouse Variants

### Dark Crazyhouse

Dark Crazyhouse combines Crazyhouse rules (captured pieces can be dropped back on the board) with Fog-of-War. **When you have a piece in hand, all empty squares become visible**, allowing you to drop anywhere:

```
uci
setoption name UCI_Variant value darkcrazyhouse
setoption name UCI_FoW value true
position startpos
go movetime 5000
```

Key features:
- Not knowing what pieces your opponent has in their hand
- When you capture a piece and have it in hand, you can see all empty squares
- Drops allowed on any empty square when you have pieces
- Managing your own captured pieces while dealing with incomplete information

### Dark Crazyhouse 2

Dark Crazyhouse 2 is a more restrictive variant where **you can only drop pieces on squares you can see** (based on standard FoW vision rules):

```
uci
setoption name UCI_Variant value darkcrazyhouse2
setoption name UCI_FoW value true
position startpos
go movetime 5000
```

Key differences from Dark Crazyhouse:
- Drops restricted to visible squares only
- No automatic visibility of empty squares when holding pieces
- More strategic piece placement required
- Surprise drops only possible in areas your pieces can already see

## Analyzing FoW Positions

### Using Standard FEN

To analyze a specific position where you know the full board state, use the `position fen` command:

```
uci
setoption name UCI_Variant value fogofwar
setoption name UCI_FoW value true
position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1
go infinite
```

To stop the search:
```
stop
```

### Using Fog FEN (Partial Observation)

When you only know what you can see (your observation), use `position fog_fen` to specify the partial observation. This is useful for analyzing positions from the perspective of a player who has incomplete information:

```
uci
setoption name UCI_Variant value darkcrazyhouse2
setoption name UCI_FoW value true
setoption name UCI_IISearch value true
setoption name UCI_FoW_TimeMs value 10000
position fog_fen ????????/??????pp/1?????1P/?1??p1?1/8/1P2P3/PB1P1PP1/NQ1NRBKR b KQk - 0 8
go infinite
```

In a fog FEN:
- `?` represents unknown/fogged squares
- Visible pieces are shown normally (e.g., `p`, `P`, `N`, etc.)
- Empty visible squares are shown as part of the rank count (e.g., `8`, `1`)

The engine will:
1. Parse and store the fog FEN
2. Use it to initialize the belief state (set of possible positions consistent with observations)
3. Search over the belief state to find the best move

**Note**: The fog_fen feature is currently a basic implementation. The engine stores the fog FEN and reports it, but full integration with belief state enumeration requires additional development.

## Viewing the Fog-of-War Board State

The engine internally tracks what each player can see. When making moves via UCI, the engine automatically:
1. Updates the true game state
2. Computes visibility for the side to move
3. Builds a belief state (set of consistent positions)
4. Searches over the belief state to find the best move

Note: The standard UCI protocol does not have a built-in command to display the fog view from the command line. To get the fog view, you would need to use a GUI that supports FoW or query via the pyffish Python bindings:

```python
import pyffish as sf

fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
fog_fen = sf.get_fog_fen(fen, "fogofwar")
print(fog_fen)  # Shows what black sees (e.g., "********/********/...")
```

## NNUE Evaluation

All three FoW variants use NNUE (neural network) evaluation:

- **fogofwar**: Uses standard chess NNUE
- **darkcrazyhouse**: Uses crazyhouse NNUE
- **darkcrazyhouse2**: Uses crazyhouse NNUE

The Obscuro search algorithm (CFR, KLUSS, GT-CFR) is for **search** (deciding which positions to evaluate), while NNUE is for **evaluation** (scoring individual positions). They work together seamlessly:
- The search explores the game tree and belief states
- NNUE evaluates leaf positions to guide the search
- The fact that pieces use "commoner" instead of "king" doesn't affect NNUE

**Note**: NNUE networks are trained on complete-information games. In FoW positions, NNUE evaluates each possible board state in the belief set as if it were a standard chess/crazyhouse position. The search algorithm then aggregates these evaluations to make decisions under uncertainty.

## FoW Rules Summary

From the Obscuro paper (Appendix A), the key fog-of-war rules are:

1. **Vision**: You see all squares your pieces can attack or move to
2. **Pawn vision**: Pawns reveal their diagonal attack squares (even if empty)
3. **Blocked pieces**: You see the blocking piece but not squares beyond it
4. **En passant**: The en passant square is revealed if you have a pawn that can capture it
5. **Castling**: You can only castle if you can see that the king/rook haven't moved
6. **Check**: You may not know you're in check if the attacking piece is in fog

## Performance Tips

1. **Time management**: Set `UCI_FoW_TimeMs` appropriately for your time control
2. **Thread allocation**: More expansion threads help with complex positions
3. **Information set size**: Larger values are better for endgames, smaller for opening/middlegame
4. **Purification support**: Keep at 3 for balanced play, reduce to 1-2 for more tactical positions

## Troubleshooting

**Engine is too slow**: Reduce `UCI_MinInfosetSize`, `UCI_ExpansionThreads`, or `UCI_FoW_TimeMs`

**Engine plays poorly**: Increase `UCI_MinInfosetSize` and `UCI_FoW_TimeMs`, add more `UCI_ExpansionThreads`

**Engine doesn't respond**: Make sure you set `position startpos` or `position fen ...` after selecting the variant

**FoW search not working**: Verify both `UCI_FoW value true` and `UCI_IISearch value true` are set

## References

- Obscuro paper: "Optimal play in imperfect-information games using counterfactual regret minimization"
- FoW chess rules: See paper Appendix A
- UCI protocol: http://wbec-ridderkerk.nl/html/UCIProtocol.html
- Fairy-Stockfish: https://github.com/fairy-stockfish/Fairy-Stockfish

## Implementation Status

The current implementation includes:
- ✅ Core Obscuro algorithm (CFR, KLUSS, GT-CFR)
- ✅ FoW visibility computation (Appendix A rules)
- ✅ Belief state management
- ✅ UCI integration and options
- ✅ Multi-threaded search
- ⚠️ Belief enumeration (simplified - currently stores true position only)
- ⚠️ Action purification (placeholder implementation)
- 🔲 Full KLUSS order-2 neighborhood computation
- 🔲 Instrumentation (Appendix B.4 metrics)

For development status and technical details, see `OBSCURO_FOW_IMPLEMENTATION.md`.
