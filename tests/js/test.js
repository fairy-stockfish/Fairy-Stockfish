before(() => {
  chai = require('chai');
  return new Promise((resolve) => {
    ffish = require('./ffish.js');
    ffish['onRuntimeInitialized'] = () => {
      resolve();
    }
  });
});

describe('Constructor: no parameter ', function () {
  it("it creates a chess board from the default position", () => {
    const board = new ffish.Board();
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    chai.expect(board.is960()).to.equal(false);
  });
});

describe('Constructor: variant parameter ', function () {
  it("it creates a board object from a given UCI-variant", () => {
    const board = new ffish.Board("chess");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    chai.expect(board.is960()).to.equal(false);
  });
});

describe('Constructor: variant parameter + fen ', function () {
  it("it creates a board object for a given UCI-variant with a given FEN", () => {
    const board = new ffish.Board("crazyhouse", "rnbqkb1r/pp3ppp/5p2/2pp4/8/5N2/PPPP1PPP/RNBQKB1R/Np w KQkq - 0 5");
    chai.expect(board.fen()).to.equal("rnbqkb1r/pp3ppp/5p2/2pp4/8/5N2/PPPP1PPP/RNBQKB1R[Np] w KQkq - 0 5");
    chai.expect(board.is960()).to.equal(false);
  });
});

describe('Constructor: variant parameter + fen + is960', function () {
  it("it creates a board object for a given UCI-variant with a given FEN and is960 identifier", () => {
    const board = new ffish.Board("chess", "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b KQkq - 1 5", true);
    chai.expect(board.fen()).to.equal("rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b GAga - 1 5");
    chai.expect(board.is960()).to.equal(true);
  });
});

describe('board.legalMoves()', function () {
  it("it returns all legal moves in uci notation as a concatenated string", () => {
    const board = new ffish.Board("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7");
    const expectedMoves = 'a2a3 b2b3 d2d3 f2f3 g2g3 h2h3 a2a4 b2b4 d2d4 f2f4 g2g4 h2h4 c3b1 c3d1 c3e2 c3a4 c3b5 c3d5' +
        ' g1e2 g1f3 g1h3 a1b1 P@e2 P@a3 P@b3 P@d3 P@e3 P@f3 P@g3 P@h3 P@a4 P@b4 P@c4 P@d4 P@f4 P@g4 P@h4 P@a5 P@b5' +
        ' P@d5 P@f5 P@g5 P@h5 P@a6 P@b6 P@d6 P@e6 P@f6 P@g6 P@h6 P@e7 Q@b1 Q@d1 Q@f1 Q@e2 Q@a3 Q@b3 Q@d3 Q@e3 Q@f3 ' +
        'Q@g3 Q@h3 Q@a4 Q@b4 Q@c4 Q@d4 Q@f4 Q@g4 Q@h4 Q@a5 Q@b5 Q@d5 Q@f5 Q@g5 Q@h5 Q@a6 Q@b6 Q@d6 Q@e6 Q@f6 Q@g6' +
        ' Q@h6 Q@e7 Q@b8 Q@d8 Q@e8 Q@f8 e1d1 e1f1 e1e2';
    chai.expect(board.legalMoves()).to.equal(expectedMoves);
  });
});

describe('board.legalMovesSan()', function () {
  it("it returns all legal moves in SAN notation as a concatenated string", () => {
    const board = new ffish.Board("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7");
    const expectedMoves = 'a3 b3 d3 f3 g3 h3 a4 b4 d4 f4 g4 h4 Nb1 Nd1 Nce2 Na4 Nb5 Nd5 Nge2 Nf3 Nh3 Rb1 P@e2 P@a3' +
        ' P@b3 P@d3 P@e3 P@f3 P@g3 P@h3 P@a4 P@b4 P@c4 P@d4 P@f4 P@g4 P@h4 P@a5 P@b5 P@d5 P@f5 P@g5 P@h5 P@a6 P@b6' +
        ' P@d6 P@e6+ P@f6 P@g6+ P@h6 P@e7 Q@b1 Q@d1 Q@f1 Q@e2 Q@a3 Q@b3+ Q@d3 Q@e3 Q@f3+ Q@g3 Q@h3 Q@a4 Q@b4 Q@c4+' +
        ' Q@d4 Q@f4+ Q@g4 Q@h4 Q@a5 Q@b5 Q@d5+ Q@f5+ Q@g5 Q@h5+ Q@a6 Q@b6 Q@d6 Q@e6+ Q@f6+ Q@g6+ Q@h6 Q@e7+ Q@b8' +
        ' Q@d8 Q@e8+ Q@f8+ Kd1 Kf1 Ke2';
    chai.expect(board.legalMovesSan()).to.equal(expectedMoves);
  });
});

describe('board.numberLegalMoves()', function () {
  it("it returns all legal moves in uci notation as a concatenated string", () => {
    const board = new ffish.Board("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7");
    chai.expect(board.numberLegalMoves()).to.equal(90);
  });
});

describe('board.push()', function () {
  it("it pushes a move in uci notation to the board", () => {
    let board = new ffish.Board();
    board.push("e2e4");
    board.push("e7e5");
    board.push("g1f3");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
  });
});

describe('board.pushSan()', function () {
  it("it pushes a move in san notation to the board", () => {
    let board = new ffish.Board();
    board.pushSan("e4");
    board.pushSan("e5");
    board.pushSan("Nf3");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
  });
});

describe('board.pop()', function () {
  it("it undos the last move", () => {
    let board = new ffish.Board();
    board.push("e2e4");
    board.push("e7e5");
    board.pop();
    board.push("e7e5");
    board.push("g1f3");
    board.push("b8c6");
    board.push("f1b5");
    board.pop();
    board.pop();

    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
  });
});

describe('board.reset()', function () {
  it("it resets the board to its starting position", () => {
    let board = new ffish.Board();
    board.pushSan("e4");
    board.pushSan("e5");
    board.pushSan("Nf3");
    board.reset();
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  });
});

describe('board.is960()', function () {
  it("it checks if the board originates from a 960 position", () => {
    let board = new ffish.Board();
    chai.expect(board.is960()).to.equal(false);
    const board2 = new ffish.Board("chess", "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b KQkq - 1 5", true);
    chai.expect(board2.is960()).to.equal(true);
  });
});

describe('board.fen()', function () {
  it("it returns the current position in fen format", () => {
    let board = new ffish.Board();
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  });
});

describe('board.setFen()', function () {
  it("it sets a custom position via fen", () => {
    let board = new ffish.Board();
    board.setFen("r1bqkbnr/ppp2ppp/2np4/1B6/3NP3/8/PPP2PPP/RNBQK2R b KQkq - 0 5");
    chai.expect(board.fen()).to.equal("r1bqkbnr/ppp2ppp/2np4/1B6/3NP3/8/PPP2PPP/RNBQK2R b KQkq - 0 5");
  });
});

describe('board.sanMove()', function () {
  it("it converts an uci move into san", () => {
    const board = new ffish.Board();
    const san = board.sanMove("g1f3");
    chai.expect(san).to.equal("Nf3");
  });
});

describe('board.turn()', function () {
  it("it returns the side to move", () => {
    let board = new ffish.Board();
    chai.expect(board.turn()).to.equal(true);
    board.push("e2e4");
    chai.expect(board.turn()).to.equal(false);
  });
});

describe('board.halfmoveClock()', function () {
  it("it returns the halfmoveClock / 50-move-rule-counter", () => {
    let board = new ffish.Board();
    chai.expect(board.halfmoveClock()).to.equal(0);
    board.push("e2e4");
    board.push("e7e5");
    chai.expect(board.halfmoveClock()).to.equal(0);
    board.push("g1f3");
    board.push("g8f6");
    chai.expect(board.halfmoveClock()).to.equal(2);
    board.push("f3e5");
    chai.expect(board.halfmoveClock()).to.equal(0);
  });
});

describe('board.gamePly()', function () {
  it("it returns the current game ply", () => {
    let board = new ffish.Board();
    chai.expect(board.gamePly()).to.equal(0);
    board.push("e2e4");
    chai.expect(board.gamePly()).to.equal(1);
    board.push("e7e5");
    board.push("g1f3");
    board.push("g8f6");
    board.push("f3e5");
    chai.expect(board.gamePly()).to.equal(5);
  });
});

describe('board.isGameOver()', function () {
  it("it checks if the game is over based on the number of legal moves", () => {
    let board = new ffish.Board();
    chai.expect(board.isGameOver()).to.equal(false);
    board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    board.pushSan("Qxf7#");
    chai.expect(board.isGameOver()).to.equal(true);
  });
});
