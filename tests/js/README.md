# ffish.js

**ffish.js** is a high performance JavaScript library which supports all chess variants of _FairyStockfish_.

It is built using emscripten/Embind from C++ source code.

* https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html

## Build instuctions

```bash
cd ../../src
```
```bash
emcc -O3 --bind ffishjs.cpp \
benchmark.cpp \
bitbase.cpp \
bitboard.cpp \
endgame.cpp \
evaluate.cpp \
material.cpp \
misc.cpp \
movegen.cpp \
movepick.cpp \
parser.cpp \
partner.cpp \
pawns.cpp \
piece.cpp \
position.cpp \
psqt.cpp \
search.cpp \
thread.cpp \
timeman.cpp \
tt.cpp \
uci.cpp \
syzygy/tbprobe.cpp \
ucioption.cpp \
variant.cpp \
xboard.cpp \
-o ../tests/js/ffish.js
```

## Examples

Load the API in JavaScript:

```javascript
const ffish = require('./ffish.js');
```

Create a new variant board from its default starting position:

```javascript
// create new board with starting position
let board = new ffish.Board("chess");
```

Set a custom fen position:
```javascript
board.setFen("rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
```

Initialize a board with a custom FEN:
```javascript
board = new ffish.Board("crazyhouse", "rnb1kb1r/ppp2ppp/4pn2/8/3P4/2N2Q2/PPP2PPP/R1B1KB1R/QPnp b KQkq - 0 6");
// create a new board object for a given fen
let board2 = new ffish.Board("crazyhouse", );
```

Add a new move:
```javascript
board.push("g2g4");
```

Generate all legal moves in UCI and SAN notation:
```javascript
let legalMoves = board.legalMoves().split(" ");
let legalMovesSan = board.legalMovesSan().split(" ");

for (var i = 0; i < legalMovesSan.length; i++) {
    console.log(`${i}: ${legalMoves[i]}, ${legalMoves
```

For examples for every function see [test.js](./test.js).

## Instructions to run the tests
```bash
npm install
npm test
```

## Instructions to run the example server
```bash
npm install
```
```bash
node index.js
```
