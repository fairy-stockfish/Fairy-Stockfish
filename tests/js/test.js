before(() => {
  chai = require('chai');
  return new Promise((resolve) => {
    pgnDir = __dirname + '/../pgn/';
    srcDir = __dirname + '/../../src/';
    ffish = require('./ffish.js');
    WHITE = true;
    BLACK = false;
    ffish['onRuntimeInitialized'] = () => {
      resolve();
    }
  });
});

describe('ffish.loadVariantConfig(config)', function () {
  it("it loads a custom variant configuration from a string", () => {
    fs = require('fs');
    let configFilePath = srcDir + 'variants.ini';
     fs.readFile(configFilePath, 'utf8', function (err,data) {
       if (err) {
         return console.log(err);
       }
       ffish.loadVariantConfig(data)
       let board = new ffish.Board("tictactoe");
       chai.expect(board.fen()).to.equal("3/3/3[PPPPPpppp] w - - 0 1");
       board.delete();
     });
  });
});

describe('Board()', function () {
  it("it creates a chess board from the default position", () => {
    const board = new ffish.Board();
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    chai.expect(board.is960()).to.equal(false);
    board.delete();
  });
});

describe('Board(uciVariant) ', function () {
  it("it creates a board object from a given UCI-variant", () => {
    const board = new ffish.Board("chess");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    chai.expect(board.is960()).to.equal(false);
    board.delete();
  });
});

describe('Board(uciVariant) with large board', function () {
  it("it creates a large-board object from a given UCI-variant", () => {
    const board = new ffish.Board("xiangqi");
    chai.expect(board.fen()).to.equal("rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1");
    chai.expect(board.is960()).to.equal(false);
    board.delete();
  });
});

describe('Board(uciVariant, fen) ', function () {
  it("it creates a board object for a given UCI-variant with a given FEN", () => {
    const board = new ffish.Board("crazyhouse", "rnbqkb1r/pp3ppp/5p2/2pp4/8/5N2/PPPP1PPP/RNBQKB1R/Np w KQkq - 0 5");
    chai.expect(board.fen()).to.equal("rnbqkb1r/pp3ppp/5p2/2pp4/8/5N2/PPPP1PPP/RNBQKB1R[Np] w KQkq - 0 5");
    chai.expect(board.is960()).to.equal(false);
    board.delete();
  });
});

describe('Board(uciVariant, fen, is960)', function () {
  it("it creates a board object for a given UCI-variant with a given FEN and is960 identifier", () => {
    const board = new ffish.Board("chess", "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b KQkq - 1 5", true);
    chai.expect(board.fen()).to.equal("rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b GAga - 1 5");
    chai.expect(board.is960()).to.equal(true);
    board.delete();
  });
});

describe('board.legalMoves()', function () {
  it("it returns all legal moves in uci notation as a concatenated string", () => {
    const crazyhouseBoard = new ffish.Board("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7");
    const expectedMovesCrazyhouse = 'a2a3 b2b3 d2d3 f2f3 g2g3 h2h3 a2a4 b2b4 d2d4 f2f4 g2g4 h2h4 c3b1 c3d1 c3e2 c3a4 c3b5 c3d5' +
        ' g1e2 g1f3 g1h3 a1b1 P@e2 P@a3 P@b3 P@d3 P@e3 P@f3 P@g3 P@h3 P@a4 P@b4 P@c4 P@d4 P@f4 P@g4 P@h4 P@a5 P@b5' +
        ' P@d5 P@f5 P@g5 P@h5 P@a6 P@b6 P@d6 P@e6 P@f6 P@g6 P@h6 P@e7 Q@b1 Q@d1 Q@f1 Q@e2 Q@a3 Q@b3 Q@d3 Q@e3 Q@f3 ' +
        'Q@g3 Q@h3 Q@a4 Q@b4 Q@c4 Q@d4 Q@f4 Q@g4 Q@h4 Q@a5 Q@b5 Q@d5 Q@f5 Q@g5 Q@h5 Q@a6 Q@b6 Q@d6 Q@e6 Q@f6 Q@g6' +
        ' Q@h6 Q@e7 Q@b8 Q@d8 Q@e8 Q@f8 e1d1 e1f1 e1e2';
    chai.expect(crazyhouseBoard.legalMoves().split(' ').sort().join()).to.equal(expectedMovesCrazyhouse.split(' ').sort().join());
    crazyhouseBoard.delete();
    // test atomic(960) castling rights
    const atomicBoard = new ffish.Board("atomic", "b1rkr1q1/pp3p2/n1p4p/6p1/1P2P1PP/8/PbR2P1Q/BN1KRN1B b Eec - 0 10", true);
    const expectedMovesAtomic = 'c6c5 h6h5 b7b6 f7f6 b7b5 f7f5 g5h4 a6b4 a6c5 a6c7 a6b8 b2a1 b2c1 b2a3 b2c3 b2d4 b2e5 b2f6 b2g7' +
                                ' b2h8 c8c7 c8b8 e8e4 e8e5 e8e6 e8e7 e8f8 g8g6 g8g7 g8h7 g8f8 g8h8 d8c8 d8d7 d8e7';
    chai.expect(atomicBoard.legalMoves().split(' ').sort().join()).to.equal(expectedMovesAtomic.split(' ').sort().join());
    atomicBoard.delete();
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
    chai.expect(board.legalMovesSan().split(' ').sort().join()).to.equal(expectedMoves.split(' ').sort().join());
    board.delete();
  });
});

describe('board.numberLegalMoves()', function () {
  it("it returns all legal moves in uci notation as a concatenated string", () => {
    const board = new ffish.Board("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7");
    chai.expect(board.numberLegalMoves()).to.equal(90);
    board.delete();
  });
});

describe('board.push(uciMove)', function () {
  it("it pushes a move in uci notation to the board", () => {
    let board = new ffish.Board();
    board.push("e2e4");
    board.push("e7e5");
    board.push("g1f3");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    chai.expect(board.push("q2q7")).to.equal(false);
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    board.delete();
  });
});

describe('board.pushSan(sanMove)', function () {
  it("it pushes a move in san notation to the board", () => {
    let board = new ffish.Board();
    board.pushSan("e4");
    board.pushSan("e5");
    board.pushSan("Nf3");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    chai.expect(board.pushSan("Nf3")).to.equal(false);
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    board.delete();
  });
});

describe('board.pushSan(sanMove, notation)', function () {
  it("it pushes a move in san notation to the board", () => {
    let board = new ffish.Board();
    board.pushSan("e4", ffish.Notation.SAN);
    board.pushSan("e5", ffish.Notation.SAN);
    board.pushSan("Nf3", ffish.Notation.SAN);
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    board.delete();
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
    board.delete();
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
    board.delete();
  });
});

describe('board.is960()', function () {
  it("it checks if the board originates from a 960 position", () => {
    let board = new ffish.Board();
    chai.expect(board.is960()).to.equal(false);
    const board2 = new ffish.Board("chess", "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b KQkq - 1 5", true);
    chai.expect(board2.is960()).to.equal(true);
    board.delete();
  });
});

describe('board.fen()', function () {
  it("it returns the current position in fen format", () => {
    let board = new ffish.Board();
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.delete();
  });
});

describe('board.setFen(fen)', function () {
  it("it sets a custom position via fen", () => {
    let board = new ffish.Board();
    board.setFen("r1bqkbnr/ppp2ppp/2np4/1B6/3NP3/8/PPP2PPP/RNBQK2R b KQkq - 0 5");
    chai.expect(board.fen()).to.equal("r1bqkbnr/ppp2ppp/2np4/1B6/3NP3/8/PPP2PPP/RNBQK2R b KQkq - 0 5");
    board.delete();
  });
});

describe('board.sanMove(uciMove)', function () {
  it("it converts an uci move into san", () => {
    const board = new ffish.Board();
    const san = board.sanMove("g1f3");
    chai.expect(san).to.equal("Nf3");
    board.delete();
  });
});

describe('board.sanMove(ffish.Notation)', function () {
  it("it converts an uci move into san using a given notation", () => {
    const board = new ffish.Board();
    chai.expect(board.sanMove("g1f3", ffish.Notation.DEFAULT)).to.equal("Nf3");
    chai.expect(board.sanMove("g1f3", ffish.Notation.SAN)).to.equal("Nf3");
    chai.expect(board.sanMove("g1f3", ffish.Notation.LAN)).to.equal("Ng1-f3");
    chai.expect(board.sanMove("g1f3", ffish.Notation.SHOGI_HOSKING)).to.equal("N36");
    chai.expect(board.sanMove("g1f3", ffish.Notation.SHOGI_HODGES)).to.equal("N-3f");
    chai.expect(board.sanMove("g1f3", ffish.Notation.SHOGI_HODGES_NUMBER)).to.equal("N-36");
    chai.expect(board.sanMove("g1f3", ffish.Notation.JANGGI)).to.equal("N87-66");
    chai.expect(board.sanMove("g1f3", ffish.Notation.XIANGQI_WXF)).to.equal("N2+3");
    board.delete();
  });
});

describe('board.variationSan(uciMoves)', function () {
  it("it converts a list of uci moves into san notation. The board will not changed by this method.", () => {
    let board = new ffish.Board();
    board.push("e2e4")
    const sanMoves = board.variationSan("e7e5 g1f3 b8c6 f1c4");
    chai.expect(sanMoves).to.equal("1...e5 2. Nf3 Nc6 3. Bc4");
    const sanMovesInvalid = board.variationSan("e7e5 g1f3 b8c6 f1c7");
    chai.expect(sanMovesInvalid).to.equal("");
    board.delete();
  });
});

describe('board.variationSan(uciMoves, notation)', function () {
  it("it converts a list of uci moves into san notation given a notation format", () => {
    let board = new ffish.Board();
    board.push("e2e4")
    const sanMoves = board.variationSan("e7e5 g1f3 b8c6 f1c4", ffish.Notation.LAN);
    chai.expect(sanMoves).to.equal("1...e7-e5 2. Ng1-f3 Nb8-c6 3. Bf1-c4");
    board.delete();
  });
});

describe('board.variationSan(uciMoves, notation, moveNumbers)', function () {
  it("it converts a list of uci moves into san notation given a notation format and optionally disabling move numbers", () => {
    let board = new ffish.Board();
    board.push("e2e4")
    const sanMoves = board.variationSan("e7e5 g1f3 b8c6 f1c4", ffish.Notation.SAN, false);
    chai.expect(sanMoves).to.equal("e5 Nf3 Nc6 Bc4");
    board.delete();
  });
});

describe('board.turn()', function () {
  it("it returns the side to move", () => {
    let board = new ffish.Board();
    chai.expect(board.turn()).to.equal(true);
    board.push("e2e4");
    chai.expect(board.turn()).to.equal(false);
    board.delete();
  });
});

describe('board.fullmoveNumber()', function () {
  it("it returns the move number starting with 1 and is increment after each move of the 2nd player", () => {
    let board = new ffish.Board();
    chai.expect(board.fullmoveNumber()).to.equal(1);
    board.push("e2e4");
    chai.expect(board.fullmoveNumber()).to.equal(1);
    board.push("e7e5");
    board.push("g1f3");
    board.push("g8f6");
    board.push("f3e5");
    chai.expect(board.fullmoveNumber()).to.equal(3);
    board.delete();
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
    board.delete();
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
    board.delete();
  });
});

describe('board.isGameOver()', function () {
  it("it checks if the game is over based on the number of legal moves", () => {
    let board = new ffish.Board();
    chai.expect(board.isGameOver()).to.equal(false);
    board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    board.pushSan("Qxf7#");
    chai.expect(board.isGameOver()).to.equal(true);
    board.delete();
  });
});

describe('board.isCheck()', function () {
  it("it checks if a player is in check", () => {
    let board = new ffish.Board();
    chai.expect(board.isCheck()).to.equal(false);
    board.setFen("rnbqkb1r/pppp1Bpp/5n2/4p3/4P3/8/PPPP1PPP/RNBQK1NR b KQkq - 0 3");
    chai.expect(board.isCheck()).to.equal(true);
    board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    board.pushSan("Qxf7#");
    chai.expect(board.isCheck()).to.equal(true);
    board.delete();
  });
});

describe('board.isBikjang()', function () {
  it("it checks if a player is in bikjang (only relevant for janggi)", () => {
    let board = new ffish.Board("janggi");
    chai.expect(board.isBikjang()).to.equal(false);
    board.setFen("rnba1abnr/4k4/1c5c1/p1p3p1p/9/9/P1P3P1P/1C5C1/4K4/RNBA1ABNR w - - 0 1");
    chai.expect(board.isBikjang()).to.equal(true);
    board.delete();
  });
});

describe('board.moveStack()', function () {
  it("it returns the move stack in UCI notation", () => {
    let board = new ffish.Board();
    chai.expect(board.isBikjang()).to.equal(false);
    board.push("e2e4");
    board.push("e7e5");
    board.push("g1f3");
    chai.expect(board.moveStack()).to.equal("e2e4 e7e5 g1f3");
    board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
    board.pushSan("Qxf7#");
    chai.expect(board.moveStack()).to.equal("h5f7");
    board.delete();
  });
});

describe('board.pushMoves(uciMoves)', function () {
  it("it pushes multiple uci moves on the board, passed as a string with ' ' as delimiter", () => {
    let board = new ffish.Board();
    board.pushMoves("e2e4 e7e5 g1f3");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    board.delete();
  });
});

describe('board.pushSanMoves(sanMoves)', function () {
  it("it pushes multiple san moves on the board, passed as a string with ' ' as delimiter", () => {
    let board = new ffish.Board();
    board.pushSanMoves("e4 e5 Nf3");
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    board.delete();
  });
});

describe('board.pushSanMoves(sanMoves, notation)', function () {
  it("it pushes multiple san moves on the board, passed as a string with ' ' as delimiter", () => {
    let board = new ffish.Board();
    board.pushSanMoves("e4 e5 Nf3", ffish.Notation.SAN);
    chai.expect(board.fen()).to.equal("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
    board.delete();
  });
});

describe('board.pocket(turn)', function () {
  it("it returns the pocket for the given player as a string with no delimeter. All pieces are returned in lower case.", () => {
    let board = new ffish.Board("crazyhouse", "rnb1kbnr/ppp1pppp/8/8/8/5q2/PPPP1PPP/RNBQKB1R/Pnp w KQkq - 0 4");
    chai.expect(board.pocket(WHITE)).to.equal("p");
    chai.expect(board.pocket(BLACK)).to.equal("np");
    board.delete();
    let board2 = new ffish.Board("crazyhouse", "rnb1kbnr/ppp1pppp/8/8/8/5q2/PPPP1PPP/RNBQKB1R[Pnp] w KQkq - 0 4");
    chai.expect(board2.pocket(WHITE)).to.equal("p");
    chai.expect(board2.pocket(BLACK)).to.equal("np");
    board2.delete();
  });
});

describe('board.toString()', function () {
  it("it returns a compact string representation of the board.", () => {
    const board = new ffish.Board("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
    chai.expect(board.toString()).to.equal("r n b . k b n r\n" +
                                           "p p p . p p p p\n" +
                                           ". . . . . . . .\n" +
                                           ". . . q . . . .\n" +
                                           ". . . . . . . .\n" +
                                           ". . . . . . . .\n" +
                                           "P P P P . P P P\n" +
                                           "R N B Q K B N R");
    board.delete();
    const board2 = new ffish.Board("xiangqi");
    chai.expect(board2.toString()).to.equal("r n b a k a b n r\n" +
                                            ". . . . . . . . .\n" +
                                            ". c . . . . . c .\n" +
                                            "p . p . p . p . p\n" +
                                            ". . . . . . . . .\n" +
                                            ". . . . . . . . .\n" +
                                            "P . P . P . P . P\n" +
                                            ". C . . . . . C .\n" +
                                            ". . . . . . . . .\n" +
                                            "R N B A K A B N R");
    board2.delete();
  });
});

describe('board.toVerboseString()', function () {
  it("it returns a verbose string representation of the board.", () => {
    const board = new ffish.Board("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3");
    chai.expect(board.toVerboseString()).to.equal("\n +---+---+---+---+---+---+---+---+\n" +
                                                  " | r | n | b |   | k | b | n | r |8  \n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " | p | p | p |   | p | p | p | p |7\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " |   |   |   |   |   |   |   |   |6\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " |   |   |   | q |   |   |   |   |5\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " |   |   |   |   |   |   |   |   |4\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " |   |   |   |   |   |   |   |   |3\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " | P | P | P | P |   | P | P | P |2\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  " | R | N | B | Q | K | B | N | R |1 *\n" +
                                                  " +---+---+---+---+---+---+---+---+\n" +
                                                  "   a   b   c   d   e   f   g   h\n\n" +
                                                  "Fen: rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3\n" +
                                                  "Sfen: rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR b - 5\n" +
                                                  "Key: AE7D48F19DB356CD\nCheckers: ")
    board.delete();
    const board2 = new ffish.Board("xiangqi");
    chai.expect(board2.toVerboseString()).to.equal("\n +---+---+---+---+---+---+---+---+---+\n" +
                                                   " | r | n | b | a | k | a | b | n | r |10  \n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " |   |   |   |   |   |   |   |   |   |9\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " |   | c |   |   |   |   |   | c |   |8\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " | p |   | p |   | p |   | p |   | p |7\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " |   |   |   |   |   |   |   |   |   |6\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " |   |   |   |   |   |   |   |   |   |5\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " | P |   | P |   | P |   | P |   | P |4\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " |   | C |   |   |   |   |   | C |   |3\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " |   |   |   |   |   |   |   |   |   |2\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   " | R | N | B | A | K | A | B | N | R |1 *\n" +
                                                   " +---+---+---+---+---+---+---+---+---+\n" +
                                                   "   a   b   c   d   e   f   g   h   i\n\n" +
                                                   "Fen: rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1\n" +
                                                   "Sfen: rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR b - 1\n" +
                                                   "Key: 1FBADA178B89E4C3\nCheckers: ");
    board2.delete();
  });
});

describe('board.variant()', function () {
  it("it returns the uci-variant of the board.", () => {
    const board = new ffish.Board("chess");
    chai.expect(board.variant()).to.equal("chess");
    board.delete();
    const board2 = new ffish.Board("");
    chai.expect(board2.variant()).to.equal("chess");
    board2.delete();
    const board3 = new ffish.Board("standard");
    chai.expect(board3.variant()).to.equal("chess");
    board3.delete();
    const board4 = new ffish.Board("Standard");
    chai.expect(board4.variant()).to.equal("chess");
    board4.delete();
    const board5 = new ffish.Board("atomic");
    chai.expect(board5.variant()).to.equal("atomic");
    board5.delete();
  });
});

describe('ffish.info()', function () {
  it("it returns the version of the Fairy-Stockfish binary", () => {
    chai.expect(ffish.info()).to.be.a('string');
  });
});

describe('ffish.setOption(name, value)', function () {
  it("it sets a string uci option value pair", () => {
    ffish.setOption("VariantPath", "variants.ini");
    chai.expect(true).to.equal(true);
  });
});

describe('ffish.setOptionInt(name, value)', function () {
  it("it sets a int uci option value pair", () => {
    ffish.setOptionInt("Threads", 4);
    chai.expect(true).to.equal(true);
  });
});

describe('ffish.setOptionBool(name, value)', function () {
  it("it sets a boolean uci option value pair", () => {
    ffish.setOptionBool("Ponder", true);
    chai.expect(true).to.equal(true);
  });
});

describe('ffish.startingFen(uciVariant)', function () {
    it("it returns the starting fen for the given uci-variant.", () => {
      chai.expect(ffish.startingFen("chess")).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    });
});

describe('ffish.validateFen(fen)', function () {
    it("it validates a given chess fen and returns +1 if fen is valid. Otherwise an error code will be returned.", () => {
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")).to.equal(1);
      chai.expect(ffish.validateFen("6k1/R7/2p4p/2P2p1P/PPb2Bp1/2P1K1P1/5r2/8 b - - 4 39")).to.equal(1);
    });
});

describe('ffish.validateFen(fen, uciVariant)', function () {
    it("it validates a given fen and returns +1 if fen is valid. Otherwise an error code will be returned.", () => {
      // check if starting fens are valid for all variants
      const variants = ffish.variants().split(" ")
      for (let idx = 0; idx < variants.length; ++idx) {
        const variant = variants[idx];
        const startFen = ffish.startingFen(variant);
        chai.expect(ffish.validateFen(startFen, variant)).to.equal(1);
        // check if the FEN is still valid if board.fen() is returned
        const board = new ffish.Board(variant);
        chai.expect(ffish.validateFen(board.fen(), variant)).to.equal(1);
        board.delete();
      }
      // alternative pocket piece formulation
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/RB w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(1);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(1);

      // error id checks
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[]wKQkq-3+301", "3check-crazyhouse")).to.equal(-12);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-11);
      chai.expect(ffish.validateFen("rnbqkbnr/ppppXppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-10);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppKpp/8/8/8/8/PPPPPPPP/RNBQ1BNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-9);
      chai.expect(ffish.validateFen("rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-8);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-8);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[77] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-7);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] o KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-6);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w K6kq - 3+3 0 1", "3check-crazyhouse")).to.equal(-5);
      chai.expect(ffish.validateFen("rnbq1bnr/pppkpppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-5);
      chai.expect(ffish.validateFen("rnbqkbn1/pppppppr/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-5);
      chai.expect(ffish.validateFen("rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/RB w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-5);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq ss 3+3 0 1", "3check-crazyhouse")).to.equal(-4);
      chai.expect(ffish.validateFen("rnbqkknr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse")).to.equal(-3);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 x 1", "3check-crazyhouse")).to.equal(-2);
      chai.expect(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 -13", "3check-crazyhouse")).to.equal(-1);
      chai.expect(ffish.validateFen("", "chess")).to.equal(0);
    });
});

describe('ffish.readGamePGN(pgn)', function () {
  it("it reads a pgn string and returns a game object", () => {
     fs = require('fs');
     let pgnFiles = ['deep_blue_kasparov_1997.pgn', 'lichess_pgn_2018.12.21_JannLee_vs_CrazyAra.j9eQS4TF.pgn', 'c60_ruy_lopez.pgn', 'pychess-variants_zJxHRVm1.pgn', 'Syrov - Dgebuadze.pgn']

     let expectedFens = ["1r6/5kp1/RqQb1p1p/1p1PpP2/1Pp1B3/2P4P/6P1/5K2 b - - 14 45",
                         "3r2kr/2pb1Q2/4ppp1/3pN2p/1P1P4/3PbP2/P1P3PP/6NK[PPqrrbbnn] b - - 1 37",
                         "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
                         "r1bQkb1r/ppp1pppp/2P5/2n2q2/8/2N2N2/PPP2PPP/R1BEKB1R[Hh] b KQACEFHkqacefh - 0 8",
                         "5rk1/4p3/2p3rR/2p1P3/2Pp1B2/1P1P2P1/2N1n3/6K1 w - - 1 44"]

     for (let idx = 0; idx < pgnFiles.length; ++idx) {
     let pgnFilePath = pgnDir + pgnFiles[idx];

     fs.readFile(pgnFilePath, 'utf8', function (err,data) {
       if (err) {
         return console.log(err);
       }
       let game = ffish.readGamePGN(data);

       const variant = game.headers("Variant").toLowerCase();
       let board = new ffish.Board(variant);
       const mainlineMoves = game.mainlineMoves().split(" ");
       for (let idx2 = 0; idx2 < mainlineMoves.length; ++idx2) {
           board.push(mainlineMoves[idx2]);
           chai.expect(ffish.validateFen(board.fen(), variant)).to.equal(1);
       }
       chai.expect(board.fen()).to.equal(expectedFens[idx]);
       board.delete();
       game.delete();
     });
         }
  });
});

describe('game.headerKeys()', function () {
  it("it returns all available header keys of the loaded game", () => {
     fs = require('fs');
     let pgnFile = 'lichess_pgn_2018.12.21_JannLee_vs_CrazyAra.j9eQS4TF.pgn'
     let pgnFilePath = pgnDir + pgnFile;

     fs.readFile(pgnFilePath, 'utf8', function (err,data) {
       if (err) {
         return console.log(err);
       }
       let game = ffish.readGamePGN(data);
       chai.expect(game.headerKeys()).to.equal('Annotator Termination Variant ECO WhiteTitle BlackRatingDiff UTCTime Result WhiteElo Black UTCDate TimeControl BlackElo Event WhiteRatingDiff BlackTitle White Date Opening Site');
       game.delete();
     });
  });
});


describe('game.headers(key)', function () {
  it("it returns the value for a given header key of a loaded game", () => {
     fs = require('fs');
     let pgnFile = 'pychess-variants_zJxHRVm1.pgn';
     let pgnFilePath = pgnDir + pgnFile;

     fs.readFile(pgnFilePath, 'utf8', function (err,data) {
       if (err) {
         return console.log(err);
       }
       let game = ffish.readGamePGN(data);
       chai.expect(game.headers("White")).to.equal("catask");
       chai.expect(game.headers("Black")).to.equal("Fairy-Stockfish");
       chai.expect(game.headers("Variant")).to.equal("Seirawan");
       chai.expect(game.headers("FEN")).to.equal("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1");

       game.delete();
     });
  });
});

describe('game.mainlineMoves()', function () {
  it("it returns the mainline of the loaded game in UCI notation", () => {
     fs = require('fs');
     let pgnFile = 'lichess_pgn_2018.12.21_JannLee_vs_CrazyAra.j9eQS4TF.pgn';
     let pgnFilePath = pgnDir + pgnFile;

     fs.readFile(pgnFilePath, 'utf8', function (err,data) {
       if (err) {
         return console.log(err);
       }
       let game = ffish.readGamePGN(data);
       chai.expect(game.mainlineMoves()).to.equal('e2e4 b8c6 b1c3 g8f6 d2d4 d7d5 e4e5 f6e4 f1b5 a7a6 b5c6 b7c6 g1e2 c8f5 e1g1 e7e6 f2f3 e4c3 b2c3 h7h5 N@e3 N@h4 N@a5 B@d7 e3f5 h4f5 B@b7 N@e3 a5c6 e3d1 c6d8 a8d8 f1d1 Q@b5 b7a6 b5a6 P@d3 N@e3 c1e3 f5e3 P@d6 e3d1 a1d1 B@e3 g1h1 f8d6 e5d6 a6d6 B@b4 d6b4 c3b4 P@f2 Q@f1 R@g1 f1g1 f2g1q d1g1 P@f2 N@g6 f2g1q e2g1 Q@e7 Q@d6 f7g6 d6e7 e8e7 R@f7 e7f7 N@e5 f7g8 N@f6 g7f6 Q@f7');
       game.delete();
     });
  });
});

describe('ffish.variants()', function () {
  it("it returns all currently available variants", () => {
    chai.expect(ffish.variants().includes("chess")).to.equal(true);
    chai.expect(ffish.variants().includes("crazyhouse")).to.equal(true);
    chai.expect(ffish.variants().includes("janggi")).to.equal(true);
  });
});
