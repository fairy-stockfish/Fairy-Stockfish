# Spark Chess

Spark Chess is a chess variant by Aleksandr Ivanovich Solomachev (GenMate).
The engine variant identifier is `sparkchess`.

## Rules

The game uses an 8x8 board and the usual chess pieces. Ordinary piece movement,
captures and pawn promotion follow chess, with these exceptions:

- There is no castling, initial two-square pawn move or en passant.
- Kings are capturable. Capturing the opposing king wins immediately.
- If the side to move has no legal move, the game is drawn.
- Check is not compulsory to answer. A king may move into attack; a pinned
  piece may move. There is no checkmate or stalemate loss.
- A king currently attacked by an opposing piece cannot make a Relay Swap.
  Its ordinary moves are not restricted by that attack.
- There is no fifty-move rule. The second occurrence of the same position,
  with the same side to move, is a draw.

The ordinary chess array is the engine's default starting position, not a
required opening arrangement. Alternative starting positions must have exactly
one king of each colour. Material need not match the ordinary chess array.
Adjacent kings, attacked kings and material exceeding the ordinary starting
array are permitted. Placement is not required to be reachable from that array.
An unpromoted white pawn cannot stand on rank 8, nor a black pawn on rank 1.
White pawns on rank 1 and black pawns on rank 8 are permitted.
A starting position with no legal moves is permitted and is immediately drawn.
Two bare kings automatically draw the game if they are not adjacent. If they
are adjacent, the game continues and the side to move may capture the other king.

On an 8x8 board, king and one bishop or knight against a lone king also
automatically draw if the kings are not adjacent, the minor piece does not
attack the lone king, and the lone king is outside the following zones:

| Minor piece | Zone where this automatic draw does not apply |
| --- | --- |
| Bishop | All edge squares, plus b2, g2, b7, g7 |
| Knight | a1-c1, f1-h1, a8-c8, f8-h8; a2, a3, a6, a7, h2, h3, h6, h7; b2, g2, b7, g7 |

This is an immediate game-ending rule, not merely a drawn search evaluation.
Inside these zones play continues; a win is not implied.

FEN copied from chess or Chess960 may contain either ordinary castling flags
(`KQkq`) or Shredder-FEN rook-file letters (for example `HAha` or `GBgb`).
Spark Chess ignores these rights and exports `-` in the castling field.
The placement, side to move and move counters are preserved; importing such
flags never enables castling and does not require enabling Chess960 mode.

### Nodes

A node of a side is a square influenced by at least two distinct friendly
non-pawn pieces. Kings and knights influence their ordinary attack squares.
Rooks, bishops and queens influence squares along their movement lines, with
one exception to ordinary blocking:

- Orthogonal rays pass through friendly rooks and queens.
- Diagonal rays pass through friendly bishops and queens.
- All other pieces, including all opposing pieces, block a ray after their
  own square has been included.

A piece counts only once for a given square. Occupied squares may be nodes,
but only empty nodes can be used for Simple Shift. Pawns do not contribute
to nodes. These influence rules do not change ordinary captures or attacks
used to determine whether a king is attacked.

### Simple Shift

A pawn may move to any empty friendly node. Distance, intervening pieces and
direction do not matter. A pawn may move backwards, reach its first rank or
share a file with another friendly pawn.

A one-square forward pawn push, when available, is represented as an ordinary
move rather than a second, duplicate Simple Shift.

### Direct Swap

A knight may exchange squares with a friendly pawn on one of its ordinary
attack squares. No node or additional knight is required.

### Relay Swap

A king, queen, rook or bishop may exchange squares with a friendly pawn if
one and the same friendly knight attacks both their squares. That knight
does not move. Several qualifying knights do not create different moves.
A knight cannot itself make a Relay Swap.

### Common restrictions

Spark moves do not capture or promote. A pawn cannot arrive on its promotion
rank by Simple Shift or either form of Swap. Thus a white piece on rank 8,
or a black piece on rank 1, cannot exchange squares with its own pawn.
The restriction concerns the pawn's destination, not the colour of the
non-pawn piece's destination square.

## Notation and protocols

Ordinary moves use the engine's existing algebraic notation. A Spark move
includes the origin square and the separator `>`:

| Mechanism | Example | Meaning |
|---|---|---|
| Simple Shift | `d2>a4` | Pawn from d2 to a4 |
| Direct Swap | `Nb3>d2` | Knight b3 and pawn d2 exchange squares |
| Relay Swap | `Qd1>b5` | Queen d1 and pawn b5 exchange squares |

In SAN, `+` indicates that the move leaves the opposing king attacked. It
does not require an answer. A king capture has no checkmate suffix `#`;
the game result records the win. This extension applies only to Spark Chess.

UCI remains unchanged: select the variant with
`setoption name UCI_Variant value sparkchess`. Moves use coordinate notation,
for example `d2a4` or `b3d2`, without `>` or `+`. Ordinary promotions retain
their promotion suffix, for example `a7a8q`.

The UCI field `score mate N` is retained for protocol compatibility. In this
variant it represents a forced win or loss, not checkmate. A Spark Chess GUI
may label it `W12` / `-W12` or `Win in 12` / `Loss in 12`. The sign must be
interpreted relative to the displayed evaluation's point of view. The number
uses the existing UCI move-distance convention, not a raw count of plies.
A centipawn advantage alone must not be labelled a proven win.

For PGN exports use the display name `Spark Chess` and include `SetUp` and
`FEN` when the game starts from a supplied position. The JavaScript PGN reader maps this
display name to the engine identifier `sparkchess` when importing. Other
applications should use the same mapping.

The FEN halfmove clock resets on captures and promotions; ordinary pawn moves
and Spark moves increment it. There is no move-count draw adjudication.
Position import requires both kings; a terminal position after king capture
should be obtained by playing the final move from a valid position.
Applications should call `validate_fen` (Python) or `validateFen` (JavaScript)
before constructing a board from user input. These validate king counts and
pawn placement as well as FEN syntax; board construction is not a substitute
for validation.

## Implementation and testing

Move generation and node calculation use the engine's Bitboard type, including
in LARGEBOARDS builds. Swaps update both pieces, position and pawn hashes,
piece-square scores and NNUE dirty-piece records. The existing COMMONER piece
and extinction mechanism implement king movement and the win condition.

This branch does not add endgame tablebase files, loaders or UCI options.
The pawnless endings described above are adjudicated by the engine's immediate
game-end check, without consulting external tables.

Build the standard 64-bit-board engine using the upstream build system:

```sh
cd src
make -j2 ARCH=x86-64 build
make -f Makefile -f ../tests/spark_state.mk ARCH=x86-64 spark-state
../tests/perft.sh chess
../tests/perft.sh variant
../tests/protocol.sh
```

For the Python bindings and focused rules/SAN tests, from the repository root:

```sh
python3 setup.py build_ext --inplace
python3 test.py
python3 -m unittest discover -s tests -p test_spark_chess.py
```

The Python build also exercises LARGEBOARDS. The existing JavaScript build
uses the same move generator and SAN implementation; no separate rules engine
is required in an application.
