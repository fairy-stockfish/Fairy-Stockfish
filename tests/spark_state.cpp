// Verify incremental Spark Chess state against reconstruction from FEN.

#include <cstdlib>
#include <iostream>

#include "bitboard.h"
#include "endgame.h"
#include "movegen.h"
#include "position.h"
#include "psqt.h"
#include "search.h"
#include "thread.h"
#include "uci.h"
#include "piece.h"
#include "variant.h"

using namespace Stockfish;

namespace {

  unsigned tested = 0;
  unsigned swaps = 0;

  void require(bool condition, const char* message, const Position& pos) {

    if (!condition)
    {
        std::cerr << message << ": " << pos.fen() << std::endl;
        std::exit(EXIT_FAILURE);
    }
  }

  void compare(const Position& pos, const Position& reference) {

    require(pos.fen() == reference.fen(), "FEN", pos);
    require(pos.state()->key == reference.state()->key, "Position key", pos);
    require(pos.pawn_key() == reference.pawn_key(), "Pawn key", pos);
    require(pos.state()->materialKey == reference.state()->materialKey, "Material key", pos);
    require(pos.psq_score() == reference.psq_score(), "Piece-square score", pos);
    for (Color c : {WHITE, BLACK})
    {
        require(pos.pieces(c) == reference.pieces(c), "Colour bitboard", pos);
        require(pos.state()->nonPawnMaterial[c] == reference.state()->nonPawnMaterial[c],
                "Non-pawn material", pos);
        require(pos.state()->coordinationSquares[c] == reference.state()->coordinationSquares[c],
                "Node bitboard", pos);
    }
    for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, COMMONER})
        require(pos.pieces(pt) == reference.pieces(pt), "Piece bitboard", pos);
  }

  void walk(Position& pos, int depth) {

    StateInfo originalState;
    Position original;
    original.set(pos.variant(), pos.fen(), false, &originalState, Threads.main());
    for (Move move : MoveList<LEGAL>(pos))
    {
        require(pos.pseudo_legal(move), "Generated move is not pseudo-legal", pos);
        bool swap = type_of(move) == COORDINATION && !pos.empty(to_sq(move));
        Piece first = pos.piece_on(from_sq(move));
        Piece second = pos.piece_on(to_sq(move));
        if (type_of(move) == COORDINATION)
            require(!pos.capture(move) && !pos.capture_or_promotion(move), "Spark capture", pos);

        StateInfo next, rebuiltState;
        pos.do_move(move, next);
        Position rebuilt;
        rebuilt.set(pos.variant(), pos.fen(), false, &rebuiltState, Threads.main());
        compare(pos, rebuilt);
        if (swap)
        {
            const auto& dp = next.dirtyPiece;
            require(dp.dirty_num == 2 && dp.piece[0] == first && dp.piece[1] == second
                    && dp.from[0] == from_sq(move) && dp.to[0] == to_sq(move)
                    && dp.from[1] == to_sq(move) && dp.to[1] == from_sq(move),
                    "NNUE swap dirty pieces", pos);
            ++swaps;
        }
        ++tested;
        if (depth > 1 && pos.count<COMMONER>(WHITE) && pos.count<COMMONER>(BLACK))
            walk(pos, depth - 1);
        pos.undo_move(move);
        compare(pos, original);
    }
  }
}

int main(int argc, char* argv[]) {

  pieceMap.init();
  variants.init();
  CommandLine::init(argc, argv);
  UCI::init(Options);
  Tune::init();
  const Variant* variant = variants.find("sparkchess")->second;
  PSQT::init(variant);
  Bitboards::init();
  Position::init();
  Bitbases::init();
  Endgames::init();
  Threads.set(1);
  Search::clear();
  // Exercise dirty-piece bookkeeping without loading or evaluating a network.
  Eval::useNNUE = true;

  for (const std::string& fen : {
       variant->startFen,
       std::string("4k3/8/8/1P6/8/2N5/8/3QK3 w - - 0 1"),
       std::string("7k/8/8/1P6/3N4/2N5/4R3/4K3 w - - 0 1"),
       std::string("4k3/8/8/8/p7/8/R2P4/R3K3 w - - 0 1"),
       std::string("4k3/P7/8/8/8/8/7p/4K3 w - - 0 1"),
       std::string("4k3/8/8/8/8/5N2/3P4/4K2r w - - 0 1")})
  {
      StateInfo state;
      Position pos;
      pos.set(variant, fen, false, &state, Threads.main());
      walk(pos, 3);
  }

  Threads.set(0);
  variants.clear_all();
  pieceMap.clear_all();
  std::cout << "Passed " << tested << " do/undo checks, including " << swaps << " swaps" << std::endl;
}
