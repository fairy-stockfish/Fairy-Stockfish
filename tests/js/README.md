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

It is available as a [standard module](https://www.npmjs.com/package/ffish), as an [ES6 module](https://www.npmjs.com/package/ffish-es6) and aims to have a syntax similar to [python-chess](https://python-chess.readthedocs.io/en/latest/index.html).

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
import Module from 'ffish-es6';
let ffish = null;

new Module().then(loadedModule => {
    ffish = loadedModule;
    console.log(`initialized ${ffish} ${loadedModule}`);
    }
});
```

### Available variants

Show all available variants supported by _Fairy-Stockfish_ and **ffish.js**.

```javascript
ffish.variants()
```
```
3check 3check-crazyhouse 5check ai-wok almost amazon anti-losalamos antichess\
armageddon asean ataxx atomic breakthrough bughouse cambodian capablanca\
capahouse caparandom centaur cfour chancellor chaturanga chess chessgi chigorin\
clobber clobber10 codrus coffeehouse courier crazyhouse dobutsu embassy euroshogi\
extinction fairy fischerandom flipello flipersi gardner gemini giveaway gorogoro\
gothic grand grandhouse hoppelpoppel horde indiangreat janggi janggicasual\
janggihouse janggimodern janggitraditional janus jesonmor judkins karouk kinglet\
kingofthehill knightmate koedem kyotoshogi loop losalamos losers makpong makruk\
makrukhouse manchu micro mini minishogi minixiangqi modern newzealand nocastle\
nocheckatomic normal orda pawnsonly peasant placement pocketknight racingkings\
seirawan semitorpedo shako shatar shatranj shogi shogun shouse sittuyin suicide\
supply threekings tictactoe upsidedown weak xiangqi xiangqihouse
```

### Board object

Create a new variant board from its default starting position.
The event `onRuntimeInitialized` ensures that the wasm file was properly loaded.

```javascript
ffish['onRuntimeInitialized'] = () => {
  let board = new ffish.Board("chess");
}
```

Set a custom fen position including fen validiation:
```javascript
fen = "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3";
if (ffish.validateFen(fen) == 1) {  // ffish.validateFen(fen) can return different error codes, it returns 1 for FEN_OK
    board.setFen(fen);
}
else {
    console.error(`Fen couldn't be parsed.`);
}
```

Alternatively, you can initialize a board with a custom FEN directly:
```javascript
let board2 = new ffish.Board("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
```

### ASCII board

You can show an ASCII representation of the board using the `toString()` method

```javascript
let board = new ffish.Board("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
console.log(board.toString())
```
```
r n b . k b n r
p p p . p p p p
. . . . . . . .
. . . q . . . .
. . . . . . . .
. . . . . . . .
P P P P . P P P
R N B Q K B N R
```

or a more detailed representation using `.toVerboseString()`.

```javascript
let board = new ffish.Board("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
console.log(board.toVerboseString())
```
```
+---+---+---+---+---+---+---+---+
| r | n | b |   | k | b | n | r |8
+---+---+---+---+---+---+---+---+
| p | p | p |   | p | p | p | p |7
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |6
+---+---+---+---+---+---+---+---+
|   |   |   | q |   |   |   |   |5
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |4
+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |3
+---+---+---+---+---+---+---+---+
| P | P | P | P |   | P | P | P |2
+---+---+---+---+---+---+---+---+
| R | N | B | Q | K | B | N | R |1 *
+---+---+---+---+---+---+---+---+
  a   b   c   d   e   f   g   h

Fen: rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3
Sfen: rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR b - 5
Key: AE7D48F19DB356CD
Checkers:
```

## Move generation and application

Add a new move:
```javascript
board.push("g2g4");
```

Generate all legal moves in UCI and SAN notation:
```javascript
let legalMoves = board.legalMoves().split(" ");
let legalMovesSan = board.legalMovesSan().split(" ");

for (var i = 0; i < legalMovesSan.length; i++) {
    console.log(`${i}: ${legalMoves[i]}, ${legalMovesSan[i]}`)
}
```

## Memory management

Unfortunately, it is impossible for Emscripten to call the destructor on C++ objects.
Therefore, you need to call `.delete()` to free the heap memory of an object.
```javascript
board.delete();
```

## PGN parsing

Read a string from a file and parse it as a single PGN game.

```javascript
fs = require('fs');
let pgnFilePath = "data/pgn/kasparov-deep-blue-1997.pgn"

fs.readFile(pgnFilePath, 'utf8', function (err,data) {
  if (err) {
    return console.log(err);
  }
  game = ffish.readGamePGN(data);
  console.log(game.headerKeys());
  console.log(game.headers("White"));
  const mainlineMoves = game.mainlineMoves().split(" ");

  let board = new ffish.Board(game.headers("Variant").toLowerCase());
  for (let idx = 0; idx < mainlineMoves.length; ++idx) {
      board.push(mainlineMoves[idx]);
  }
  // or use board.pushMoves(game.mainlineMoves()); to push all moves at once

  let finalFen = board.fen();
  board.delete();
  game.delete();
}
```

## Custom variants

Fairy-Stockfish also allows defining custom variants by loading a configuration file.

See e.g. the configuration for **connect4**, **tictactoe** or **janggihouse** in [variants.ini](https://github.com/ianfab/Fairy-Stockfish/blob/master/src/variants.ini).
```javascript
fs = require('fs');
let configFilePath = './variants.ini';
 fs.readFile(configFilePath, 'utf8', function (err,data) {
   if (err) {
     return console.log(err);
   }
   ffish.loadVariantConfig(data)
   let board = new ffish.Board("tictactoe");
   board.delete();
 });
```

## Remaining features

For an example of each available function see [test.js](https://github.com/ianfab/Fairy-Stockfish/blob/master/tests/js/test.js).

## Build instuctions

It is built using emscripten/Embind from C++ source code.

* https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html


If you want to disable variants with a board greater than 8x8,
 you can add the flag `largeboards=no`.

The pre-compiled wasm binary is built with `largeboards=yes`.

It is recommended to set `debug=yes` before running tests.


### Compile as standard module

```bash
cd src
make -f Makefile_js build
```

### Compile as ES6/ES2015 module

Some environments such as [vue-js](https://vuejs.org/) may require the library to be exported
  as a ES6/ES2015 module.

```bash
cd src
make -f Makefile_js build es6=yes
```

Make sure that the wasm file is in the `public` directory.

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
