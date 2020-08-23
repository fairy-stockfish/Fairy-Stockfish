# ffish.js

A high performance WebAssembly chess variant library based on _Fairy-Stockfish_.

It is built using emscripten/Embind from C++ source code.

* https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html

## Install instructions

```bash
npm install ffish
```

## Build instuctions

```bash
cd Fairy-Stockfish/src
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
const ffish = require('ffish');
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

## Compile as ES6/ES2015 module

Some environments such as [vue-js](https://vuejs.org/) may require the library to be exported
  as a ES6/ES2015 module.

```bash
cd Fairy-Stockfish/src
```
```bash
emcc -O3 --bind \
-s ENVIRONMENT='web,worker' -s EXPORT_ES6=1 -s MODULARIZE=1 -s USE_ES6_IMPORT_META=0 \
ffishjs.cpp \
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

Later the module can be imported as follows:

```javascript
import Module from './ffish.js';
let ffish = null;

new Module().then(loadedModule => {
    ffish = loadedModule;
    console.log(`initialized ${ffish} ${loadedModule}`);
    }
});

```
References: [emscripten/#10114](https://github.com/emscripten-core/emscripten/issues/10114)
