const express = require('express')
const ffish = require('./ffish.js');
const { PerformanceObserver, performance } = require('perf_hooks');
const { Chess } = require('chess.js')
const { Crazyhouse } = require('crazyhouse.js')

const app = express();

app.get('/', (req, res) => {

let board = new ffish.Board("chess"); //, "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3", false);
let legalMoves = board.legalMoves();

let it = 1000;

console.log("Standard Chess")
console.log("==================")

var t0 = performance.now()
for (let i = 0; i < it; ++i) {
   legalMoves = board.legalMoves().split(" ");
}
var t1 = performance.now()
console.log(`Call to board.legalMoves()+legalMoves.split(" ") took ${(t1 - t0).toFixed(2)}  milliseconds.`)

var t0 = performance.now()
for (let i = 0; i < it; ++i) {
  legalMoves = board.legalMovesSan().split(" ")
}
var t1 = performance.now()
console.log(`board.legalMovesSan().split(" ").length: ${legalMoves.length}`)
console.log(`Call to board.legalMovesSan()+legalMoves.split(" ") took ${(t1 - t0).toFixed(2)}  milliseconds.`)


// pass in a FEN string to load a particular position
const chess = new Chess(
    "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3"
)
var t0 = performance.now()
for (let i = 0; i < it; ++i) {
  legalMoves = chess.moves()
}
var t1 = performance.now()
console.log(`chess.moves().length: ${legalMoves.length}`)
console.log(`Call to chess.moves() took ${(t1 - t0).toFixed(2)}  milliseconds.`)

console.log("Crazyhouse")
console.log("===========")

let crazyhouseFen = "rnb1kb1r/ppp2ppp/4pn2/8/3P4/2N2Q2/PPP2PPP/R1B1KB1R/QPnp b KQkq - 0 6";
board = new ffish.Board("crazyhouse", crazyhouseFen);

var t0 = performance.now()
for (let i = 0; i < it; ++i) {
  legalMoves = board.legalMovesSan().split(" ")
}
var t1 = performance.now()
console.log(`board.legalMoves().split(" ").length: ${legalMoves.length}`)
console.log(`Call to board.legalMoves() took ${(t1 - t0).toFixed(2)}  milliseconds.`)

cz_moves = ["e4", "d5", "exd5", "Qxd5", "Nf3", "Nf6", "Nc3", "e6", "d4", "Qxf3", "Qxf3"]
// pass in a FEN string to load a particular position
const crazyhouse = new Crazyhouse()

for (let idx = 0; idx < cz_moves.length; ++idx) {
  crazyhouse.move(cz_moves[idx])
}

var t0 = performance.now()
for (let i = 0; i < it; ++i) {
  legalMoves = crazyhouse.moves()
}
var t1 = performance.now()
console.log(`crazyhouse.moves().length: ${legalMoves.length}`)
console.log(`Call to crazyhouse.moves() took ${(t1 - t0).toFixed(2)}  milliseconds.`)


let legalMovesSan = board.legalMovesSan().split(" ");

for (var i = 0; i < legalMovesSan.length; i++) {
    console.log(`${i}: ${legalMoves[i]}, ${legalMovesSan[i]}`);
}
console.log(board.fen());

  res.send(String("Test server of ffish.js"));
});

app.listen(8000, () => {
  console.log('Test server of ffish.js listening on port 8000.')
  console.log('http://127.0.0.1:8000/')
});
