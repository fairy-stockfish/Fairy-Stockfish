#!/bin/bash
# verify protocol implementations

error()
{
  echo "protocol testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

echo "protocol testing started"

cat << EOF > uci.exp
   spawn ./stockfish
   send "uci\\n"
   expect "default chess"
   expect "uciok"
   send "quit\\n"
   expect eof
EOF

cat << EOF > ucci.exp
   spawn ./stockfish
   send "ucci\\n"
   expect "option UCI_Variant"
   expect "default xiangqi"
   expect "ucciok"
   send "quit\\n"
   expect eof
EOF

cat << EOF > usi.exp
   spawn ./stockfish
   send "usi\\n"
   expect "default shogi"
   expect "usiok"
   send "quit\\n"
   expect eof
EOF

cat << EOF > ucicyclone.exp
   spawn ./stockfish
   send "uci\\n"
   expect "uciok"
   send "startpos\\n"
   send "d\\n"
   expect "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1"
   send "quit\\n"
   expect eof
EOF

cat << EOF > ucicyclone2.exp
   spawn ./stockfish ucicyclone
   send "uci\\n"
   expect "uciok"
   send "position startpos\\n"
   send "d\\n"
   expect "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1"
   send "quit\\n"
   expect eof
EOF

cat << EOF > xboard.exp
   spawn ./stockfish load variants.ini
   send "xboard\\n"
   send "protover 2\\n"
   expect "feature done=1"
   send "ping\\n"
   expect "pong"
   send "ping\\n"
   expect "pong"
   send "variant 3check-crazyhouse\\n"
   expect "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR\\\\\[] w KQkq - 3+3 0 1"
   send "quit\\n"
   expect eof
EOF

for exp in uci.exp ucci.exp usi.exp ucicyclone.exp ucicyclone2.exp xboard.exp
do
  echo "Testing $exp"
  timeout 5 expect $exp > /dev/null
  rm $exp
done

echo "protocol testing OK"
