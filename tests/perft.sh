#!/bin/bash
# verify perft numbers (positions from www.chessprogramming.org/Perft_Results)

error()
{
  echo "perft testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

echo "perft testing started"

cat << EOF > perft.exp
   set timeout 60
   lassign \$argv var pos depth result
   spawn ./stockfish
   send "setoption name UCI_Variant value \$var\\n"
   send "position \$pos\\ngo perft \$depth\\n"
   expect "Nodes searched? \$result" {} timeout {exit 1}
   send "quit\\n"
   expect eof
EOF

# chess
if [[ $1 == "" || $1 == "chess" ]]; then
  expect perft.exp chess startpos 5 4865609 > /dev/null
  expect perft.exp chess "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -" 5 193690690 > /dev/null
  expect perft.exp chess "fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -" 6 11030083 > /dev/null
  expect perft.exp chess "fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" 5 15833292 > /dev/null
  expect perft.exp chess "fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" 5 89941194 > /dev/null
  expect perft.exp chess "fen r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10" 5 164075551 > /dev/null
fi

# variants
if [[ $1 == "" || $1 == "variant" ]]; then
  expect perft.exp crazyhouse startpos 5 4888832 > /dev/null
  expect perft.exp losalamos startpos 6 2549164 > /dev/null
  expect perft.exp pocketknight startpos 4 3071267 > /dev/null
  expect perft.exp amazon startpos 5 9319911 > /dev/null
  expect perft.exp makruk startpos 5 6223994 > /dev/null
  expect perft.exp cambodian startpos 5 8601434 > /dev/null
  expect perft.exp shatranj startpos 5 1164248 > /dev/null
  expect perft.exp hoppelpoppel startpos 5 5056643 > /dev/null
  expect perft.exp newzealand startpos 5 4987426 > /dev/null
  expect perft.exp loop startpos 5 4888832 > /dev/null
  expect perft.exp chessgi startpos 5 4889167 > /dev/null
  expect perft.exp racingkings startpos 5 9472927 > /dev/null
  expect perft.exp knightmate startpos 5 3249033 > /dev/null
  expect perft.exp losers startpos 5 2723795 > /dev/null
  expect perft.exp antichess startpos 5 2732672 > /dev/null
  expect perft.exp giveaway startpos 5 2732672 > /dev/null
  expect perft.exp asean startpos 5 6223994 > /dev/null
  expect perft.exp ai-wok startpos 5 13275068 > /dev/null
  expect perft.exp euroshogi startpos 5 9451149 > /dev/null
  expect perft.exp minishogi startpos 5 533203 > /dev/null
  expect perft.exp kyotoshogi startpos 5 225903 > /dev/null
  expect perft.exp horde startpos 6 5396554 > /dev/null
  expect perft.exp placement startpos 4 1597696 > /dev/null
  expect perft.exp sittuyin startpos 3 580096 > /dev/null
  expect perft.exp sittuyin "fen 8/8/6R1/s3r3/P5R1/1KP3p1/1F2kr2/8[] b - - 0 72" 4 652686 > /dev/null
  expect perft.exp sittuyin "fen 2r5/6k1/6p1/3s2P1/3npR2/8/p2N2F1/3K4[] w - - 1 50" 4 373984 > /dev/null
  expect perft.exp sittuyin "fen 8/6s1/5P2/3n4/pR2K2S/1P6/1k4p1/8[] w - - 1 50" 4 268869 > /dev/null
  expect perft.exp seirawan startpos 5 27639803 > /dev/null
  expect perft.exp seirawan "fen reb1k2r/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] b KQkqc - 3 7" 5 31463761 > /dev/null
  expect perft.exp ataxx startpos 4 155888 > /dev/null
  expect perft.exp ataxx "fen 7/7/7/7/ppppppp/ppppppp/PPPPPPP[PPPPPPPPPPPPPPPPPPPPPppppppppppppppppppppp] w 0 1" 5 452980 > /dev/null
fi

# large-board variants
if [[ $1 == "largeboard" ]]; then
  expect perft.exp shogi startpos 4 719731 > /dev/null
  expect perft.exp capablanca startpos 4 805128 > /dev/null
  expect perft.exp embassy startpos 4 809539 > /dev/null
  expect perft.exp janus startpos 4 772074 > /dev/null
  expect perft.exp modern startpos 4 433729 > /dev/null
  expect perft.exp chancellor startpos 4 436656 > /dev/null
  expect perft.exp courier startpos 4 500337 > /dev/null
  expect perft.exp grand startpos 3 259514 > /dev/null
  expect perft.exp xiangqi startpos 4 3290240 > /dev/null
  expect perft.exp xiangqi "fen 1rbaka2R/5r3/6n2/2p1p1p2/4P1bP1/PpC3Bc1/1nPR2P2/2N2AN2/1c2K1p2/2BAC4 w - - 0 1" 4 4485547 > /dev/null
  expect perft.exp janggi startpos 4 1065277 > /dev/null
  expect perft.exp janggi "fen 1n1kaabn1/cr2N4/5C1c1/p1pNp3p/9/9/P1PbP1P1P/3r1p3/4A4/R1BA1KB1R b - - 0 1" 4 76763 > /dev/null
  expect perft.exp janggi "fen 1Pbcka3/3nNn1c1/N2CaC3/1pB6/9/9/5P3/9/4K4/9 w - - 0 23" 4 151202 > /dev/null
  expect perft.exp amazons startpos 1 2176 > /dev/null
fi

rm perft.exp

echo "perft testing OK"
