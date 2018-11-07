#!/bin/bash
# verify perft numbers (positions from https://chessprogramming.wikispaces.com/Perft+Results)

error()
{
  echo "perft testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

echo "perft testing started"

cat << EOF > perft.exp
   set timeout 30
   lassign \$argv var pos depth result
   spawn ./stockfish
   send "setoption name UCI_Variant value \$var\\n"
   send "position \$pos\\ngo perft \$depth\\n"
   expect "Nodes searched? \$result" {} timeout {exit 1}
   send "quit\\n"
   expect eof
EOF

# chess
expect perft.exp chess startpos 5 4865609 > /dev/null
expect perft.exp chess "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -" 5 193690690 > /dev/null
expect perft.exp chess "fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -" 6 11030083 > /dev/null
expect perft.exp chess "fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" 5 15833292 > /dev/null
expect perft.exp chess "fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" 5 89941194 > /dev/null
expect perft.exp chess "fen r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10" 5 164075551 > /dev/null

# variants
expect perft.exp crazyhouse startpos 5 4888832 > /dev/null
expect perft.exp losalamos startpos 6 2549164 > /dev/null
expect perft.exp pocketknight startpos 4 3071267 > /dev/null
expect perft.exp amazon startpos 5 9319911 > /dev/null
expect perft.exp makruk startpos 5 6223994 > /dev/null
expect perft.exp shatranj startpos 5 1164248 > /dev/null
expect perft.exp loop startpos 5 4888832 > /dev/null
expect perft.exp chessgi startpos 5 4889167 > /dev/null
expect perft.exp racingkings startpos 5 9472927 > /dev/null
expect perft.exp losers startpos 5 2723795 > /dev/null
expect perft.exp antichess startpos 5 2732672 > /dev/null
expect perft.exp giveaway startpos 5 2732672 > /dev/null
expect perft.exp asean startpos 5 6223994 > /dev/null
expect perft.exp ai-wok startpos 5 13275068 > /dev/null
expect perft.exp euroshogi startpos 5 9451149 > /dev/null
expect perft.exp minishogi startpos 5 533203 > /dev/null
expect perft.exp horde startpos 6 5396554 > /dev/null
expect perft.exp placement startpos 4 1597696 > /dev/null
expect perft.exp sittuyin startpos 3 580096 > /dev/null
expect perft.exp sittuyin "fen 8/8/6R1/s3r3/P5R1/1KP3p1/1F2kr2/8[-] b - - 0 72" 4 657824 > /dev/null
expect perft.exp sittuyin "fen 2r5/6k1/6p1/3s2P1/3npR2/8/p2N2F1/3K4 w - - 1 50" 4 394031 > /dev/null

rm perft.exp

echo "perft testing OK"
