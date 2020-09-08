<h2 align="center">ffish.js</h2>

<p align="center">
<a href="https://img.shields.io/badge/-ffish.js-green"><img src="https://img.shields.io/badge/-ffish.js-green" alt="Package"></a>
<a href="https://npmcharts.com/compare/ffish?minimal=true"><img src="https://img.shields.io/npm/dm/ffish.svg?sanitize=true" alt="Downloads"></a>
<a href="https://www.npmjs.com/package/ffish"><img src="https://img.shields.io/npm/v/ffish.svg?sanitize=true" alt="Version"></a>
</p>

<p align="center">
<a href="https://img.shields.io/badge/-ffish--es6.js-green"><img src="https://img.shields.io/badge/-ffish--es6.js-green" alt="Package-ES6"></a>
<a href="https://npmcharts.com/compare/ffish-es6?minimal=true"><img src="https://img.shields.io/npm/dm/ffish-es6.svg?sanitize=true" alt="Downloads-ES6"></a>
<a href="https://www.npmjs.com/package/ffish-es6"><img src="https://img.shields.io/npm/v/ffish-es6.svg?sanitize=true" alt="Version-ES6"></a>
</p>


The package **ffish.js** is a high performance WebAssembly chess variant library based on [_Fairy-Stockfish_](https://github.com/ianfab/Fairy-Stockfish).

It is available as a [standard module](https://www.npmjs.com/package/ffish) and as an [ES6 module](https://www.npmjs.com/package/ffish-es6).

## Install instructions

### Standard module

```bash
npm install ffish
```

### ES6 module
```bash
npm install ffish-es6
```

## Examples

Load the API in JavaScript:

### Standard module

```javascript
const ffish = require('ffish');
```

### ES6 module

```javascript
import Module from 'ffish';
let ffish = null;

new Module().then(loadedModule => {
    ffish = loadedModule;
    console.log(`initialized ${ffish} ${loadedModule}`);
    }
});
```

### Board object

Create a new variant board from its default starting position.
The even `onRuntimeInitialized` ensures that the wasm file was properly loaded.

```javascript
ffish['onRuntimeInitialized'] = () => {
  let board = new ffish.Board("chess");
}
```

Set a custom fen position:
```javascript
board.setFen("rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
```

Alternatively, you can initialize a board with a custom FEN directly:
```javascript
let board2 = new ffish.Board("crazyhouse", "rnb1kb1r/ppp2ppp/4pn2/8/3P4/2N2Q2/PPP2PPP/R1B1KB1R/QPnp b KQkq - 0 6");
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
    console.log(`${i}: ${legalMoves[i]}, ${legalMoves})
}
```

For examples for every function see [test.js](https://github.com/ianfab/Fairy-Stockfish/blob/master/tests/js/test.js).

Unfortunately, it is impossible for Emscripten to call the destructors on C++ object.
Therefore, you need to call `.delete()` to free the heap memory of an object.
```javascript
board.delete();
```


## Build instuctions

It is built using emscripten/Embind from C++ source code.

* https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html


If you want to disable variants with a board greater than 8x8,
 you can remove the flags `-s TOTAL_MEMORY=67108864 -s
  ALLOW_MEMORY_GROWTH=1 -s WASM_MEM_MAX=2147483648
   -DLARGEBOARDS -DPRECOMPUTED_MAGICS`.

The pre-compiled wasm binary is built with `-DLARGEBOARDS`.

### Compile as standard module

```bash
cd Fairy-Stockfish/src
```
```bash
emcc -O3 --bind -s TOTAL_MEMORY=67108864 -s ALLOW_MEMORY_GROWTH=1 \
 -s WASM_MEM_MAX=2147483648 -DLARGEBOARDS -DPRECOMPUTED_MAGICS \
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

### Compile as ES6/ES2015 module

Some environments such as [vue-js](https://vuejs.org/) may require the library to be exported
  as a ES6/ES2015 module.

```bash
cd Fairy-Stockfish/src
```
```bash
emcc -O3 -DLARGEBOARDS --bind \
-s TOTAL_MEMORY=67108864 -s ALLOW_MEMORY_GROWTH=1 \
-s WASM_MEM_MAX=2147483648 -DLARGEBOARDS -DPRECOMPUTED_MAGICS \
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

Reference: [emscripten/#10114](https://github.com/emscripten-core/emscripten/issues/10114)

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
