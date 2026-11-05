/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Code for calculating NNUE evaluation function

#include "nnue_misc.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#include "../position.h"
#include "../types.h"
#include "network.h"
#include "nnue_accumulator.h"

namespace Stockfish::Eval::NNUE {

namespace {

// Requires the buffer to have capacity for at least 5 values
void format_cp_compact(Value v, char* buffer) {

    buffer[0] = (v < 0 ? '-' : v > 0 ? '+' : ' ');

    int cp = std::abs(100 * v / PawnValueEg);

    if (cp >= 10000)
    {
        buffer[1] = '0' + cp / 10000;
        cp %= 10000;
        buffer[2] = '0' + cp / 1000;
        cp %= 1000;
        buffer[3] = '0' + cp / 100;
        cp %= 100;
        buffer[4] = ' ';
    }
    else if (cp >= 1000)
    {
        buffer[1] = '0' + cp / 1000;
        cp %= 1000;
        buffer[2] = '0' + cp / 100;
        cp %= 100;
        buffer[3] = '.';
        buffer[4] = '0' + cp / 10;
    }
    else
    {
        buffer[1] = '0' + cp / 100;
        cp %= 100;
        buffer[2] = '.';
        buffer[3] = '0' + cp / 10;
        cp %= 10;
        buffer[4] = '0' + cp / 1;
    }
}

// Requires the buffer to have capacity for at least 7 values
void format_cp_aligned_dot(Value v, char* buffer) {
    buffer[0] = (v < 0 ? '-' : v > 0 ? '+' : ' ');

    int cp = std::abs(100 * v / PawnValueEg);

    if (cp >= 10000)
    {
        buffer[1] = '0' + cp / 10000;
        cp %= 10000;
        buffer[2] = '0' + cp / 1000;
        cp %= 1000;
        buffer[3] = '0' + cp / 100;
        cp %= 100;
        buffer[4] = '.';
        buffer[5] = '0' + cp / 10;
        cp %= 10;
        buffer[6] = '0' + cp;
    }
    else if (cp >= 1000)
    {
        buffer[1] = ' ';
        buffer[2] = '0' + cp / 1000;
        cp %= 1000;
        buffer[3] = '0' + cp / 100;
        cp %= 100;
        buffer[4] = '.';
        buffer[5] = '0' + cp / 10;
        cp %= 10;
        buffer[6] = '0' + cp;
    }
    else
    {
        buffer[1] = ' ';
        buffer[2] = ' ';
        buffer[3] = '0' + cp / 100;
        cp %= 100;
        buffer[4] = '.';
        buffer[5] = '0' + cp / 10;
        cp %= 10;
        buffer[6] = '0' + cp / 1;
    }
}


}  // namespace

// trace() returns a string with the value of each piece on a board,
// and a table for (PSQT, Layers) values bucket by bucket.

std::string trace(Position& pos, const Network& network, AccumulatorCaches& caches) {

    std::stringstream ss;

    char board[3 * RANK_NB + 1][8 * FILE_NB + 2];
    std::memset(board, ' ', sizeof(board));
    for (int row = 0; row < 3 * pos.ranks() + 1; ++row)
        board[row][8 * FILE_NB + 1] = '\0';

    // A lambda to output one box of the board
    auto writeSquare = [&board, &pos](File file, Rank rank, Piece pc, Value value) {
        const int x = ((int) file) * 8;
        const int y = (pos.max_rank() - (int) rank) * 3;
        for (int i = 1; i < 8; ++i)
            board[y][x + i] = board[y + 3][x + i] = '-';
        for (int i = 1; i < 3; ++i)
            board[y + i][x] = board[y + i][x + 8] = '|';
        board[y][x] = board[y][x + 8] = board[y + 3][x + 8] = board[y + 3][x] = '+';
        if (pc != NO_PIECE)
            board[y + 1][x + 4] = pos.piece_to_char()[pc];
        if (value != VALUE_NONE)
            format_cp_compact(value, &board[y + 2][x + 2]);
    };

    // We estimate the value of each piece by doing a differential evaluation from
    // the current base eval, simulating the removal of the piece from its square.
    auto accumulatorStack = std::make_unique<AccumulatorStack>();

    // Unscaled evaluation of the network
    auto evaluate = [&]() {
        auto [psqt, positional] = network.evaluate(pos, *accumulatorStack, caches);
        return static_cast<Value>((psqt + positional) / network.output_scale());
    };

    Value base = evaluate();
    base       = pos.side_to_move() == WHITE ? base : -base;

    for (File f = FILE_A; f <= pos.max_file(); ++f)
        for (Rank r = RANK_1; r <= pos.max_rank(); ++r)
        {
            Square sq           = make_square(f, r);
            Piece  pc           = pos.piece_on(sq);
            Piece  unpromotedPc = pos.unpromoted_piece_on(sq);
            bool   isPromoted   = pos.is_promoted(sq);
            Value  v            = VALUE_NONE;

            if (pc != NO_PIECE && type_of(pc) != network.king())
            {
                pos.remove_piece(sq);

                accumulatorStack->reset();
                Value eval = evaluate();
                eval       = pos.side_to_move() == WHITE ? eval : -eval;
                v          = base - eval;

                pos.put_piece(pc, sq, isPromoted, unpromotedPc);
            }

            writeSquare(f, r, pc, v);
        }

    ss << " NNUE derived piece values:\n";
    for (int row = 0; row < 3 * pos.ranks() + 1; ++row)
        ss << board[row] << '\n';
    ss << '\n';

    accumulatorStack->reset();
    auto t = network.trace_evaluate(pos, *accumulatorStack, caches);

    ss << " NNUE network contributions "
       << (pos.side_to_move() == WHITE ? "(White to move)" : "(Black to move)") << std::endl
       << "+------------+------------+------------+------------+\n"
       << "|   Bucket   |  Material  | Positional |   Total    |\n"
       << "|            |   (PSQT)   |  (Layers)  |            |\n"
       << "+------------+------------+------------+------------+\n";

    for (std::size_t bucket = 0; bucket < t.layerStacks; ++bucket)
    {
        char buffer[3][8];
        std::memset(buffer, '\0', sizeof(buffer));

        format_cp_aligned_dot(t.psqt[bucket], buffer[0]);
        format_cp_aligned_dot(t.positional[bucket], buffer[1]);
        format_cp_aligned_dot(t.psqt[bucket] + t.positional[bucket], buffer[2]);

        ss << "|  " << bucket << "        "
           << " |  " << buffer[0] << "  "
           << " |  " << buffer[1] << "  "
           << " |  " << buffer[2] << "  "
           << " |";
        if (bucket == t.correctBucket)
            ss << " <-- this bucket is used";
        ss << '\n';
    }

    ss << "+------------+------------+------------+------------+\n";

    return ss.str();
}

}  // namespace Stockfish::Eval::NNUE
