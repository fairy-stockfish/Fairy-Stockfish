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
   set timeout 5
   spawn ./stockfish
   send "uci\\n"
   expect "default chess"
   expect "uciok"
   send "usi\\n"
   expect "default shogi"
   expect "usiok"
   send "ucci\\n"
   expect "option UCI_Variant"
   expect "default xiangqi"
   expect "ucciok"
   send "quit\\n"
   expect eof
EOF

cat << EOF > xboard.exp
   set timeout 5
   spawn ./stockfish
   send "xboard\\n"
   send "protover 2\\n"
   expect "feature done=1"
   send "ping\\n"
   expect "pong"
   send "ping\\n"
   expect "pong"
   send "quit\\n"
   expect eof
EOF

for exp in uci.exp xboard.exp
do
  expect $exp > /dev/null
  rm $exp
done

echo "protocol testing OK"
