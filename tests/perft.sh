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
   lassign \$argv var pos depth result chess960
   if {\$chess960 eq ""} {set chess960 false}
   spawn ./stockfish
   send "setoption name UCI_Chess960 value \$chess960\\n"
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
if [[ $1 == "all" || $1 == "variant" ]]; then
  # small board
  expect perft.exp losalamos startpos 5 191846 > /dev/null
  expect perft.exp losalamos "fen 6/2P3/6/1K1k2/6/6 w - - 0 1" 6 187431 > /dev/null
  # fairy
  expect perft.exp balancedalternation startpos 5 195252 > /dev/null
  expect perft.exp makruk startpos 4 273026 > /dev/null
  expect perft.exp cambodian startpos 4 361793 > /dev/null
  expect perft.exp cambodian "fen r1s1ks1r/3nm3/pppNpppp/3n4/5P2/PPPPPNPP/8/R1SKMS1R b DEe 0 0 5" 2 72 > /dev/null
  expect perft.exp karouk "fen rn1mksnr/3s4/pppppppp/8/4N3/PPPPPPPP/8/R1SKMSNR b DEde - 3 2" 4 358535 > /dev/null
  expect perft.exp makpong "fen 3mk3/r3s1R1/1psppnp1/p1pn4/1P2NP2/P1PPP1P1/4NS2/R1SKM3 w - - 0 1" 4 593103 > /dev/null
  expect perft.exp asean startpos 4 273026 > /dev/null
  expect perft.exp ai-wok startpos 4 485045 > /dev/null
  expect perft.exp ai-wok "fen 8/8/8/2sp2k1/7p/3P4/6K1/7r w - - 0 1" 5 30055 > /dev/null
  expect perft.exp sittuyin startpos 3 580096 > /dev/null
  expect perft.exp sittuyin "fen 8/8/6R1/s3r3/P5R1/1KP3p1/1F2kr2/8[] b - - 0 72" 4 652686 > /dev/null
  expect perft.exp sittuyin "fen 2r5/6k1/6p1/3s2P1/3npR2/8/p2N2F1/3K4[] w - - 1 50" 4 373984 > /dev/null
  expect perft.exp sittuyin "fen 8/6s1/5P2/3n4/pR2K2S/1P6/1k4p1/8[] w - - 1 50" 4 268869 > /dev/null
  expect perft.exp sittuyin "fen 1k5K/3r2P1/8/8/8/8/8/8[] w - - 0 1" 5 68662 > /dev/null
  expect perft.exp shatar startpos 4 177344 > /dev/null
  expect perft.exp shatranj startpos 4 68122 > /dev/null
  expect perft.exp amazon startpos 4 318185 > /dev/null
  expect perft.exp nightrider startpos 4 419019 > /dev/null
  expect perft.exp grasshopper startpos 4 635298 > /dev/null
  expect perft.exp hoppelpoppel startpos 4 202459 > /dev/null
  expect perft.exp newzealand startpos 4 200310 > /dev/null
  # alternative goals
  expect perft.exp racingkings startpos 4 296242 > /dev/null
  expect perft.exp racingkings "fen 6r1/2K5/5k2/8/3R4/8/8/8 w - - 0 1" 4 86041 > /dev/null
  expect perft.exp racingkings "fen 6R1/2k5/5K2/8/3r4/8/8/8 b - - 0 1" 4 86009 > /dev/null
  expect perft.exp racingkings "fen 4brn1/2K2k2/8/8/8/8/8/8 w - - 0 1" 6 265932 > /dev/null
  expect perft.exp kingofthehill "fen rnb2b1r/ppp2ppp/3k4/8/1PKp1pn1/3Pq3/PBP1P2P/RN1Q1B1R w - - 4 12" 3 19003 > /dev/null
  expect perft.exp 3check "fen 7r/1p4p1/pk3p2/RN6/8/P5Pp/3p1P1P/4R1K1 w - - 1+3 1 39" 3 12407 > /dev/null
  expect perft.exp 3check "fen 7r/1p4p1/pk3p2/RN6/8/P5Pp/3p1P1P/4R1K1 w - - 1 39 +2+0" 3 12407 > /dev/null
  expect perft.exp atomic startpos 4 197326 > /dev/null
  expect perft.exp atomic "fen rn2kb1r/1pp1p2p/p2q1pp1/3P4/2P3b1/4PN2/PP3PPP/R2QKB1R b KQkq - 0 1" 4 1434825 > /dev/null
  expect perft.exp atomic "fen rn1qkb1r/p5pp/2p5/3p4/N3P3/5P2/PPP4P/R1BQK3 w Qkq - 0 1" 4 714499 > /dev/null
  expect perft.exp atomic "fen r4b1r/2kb1N2/p2Bpnp1/8/2Pp3p/1P1PPP2/P5PP/R3K2R b KQ - 0 1" 2 148 > /dev/null
  expect perft.exp antichess startpos 4 153299 > /dev/null
  expect perft.exp giveaway startpos 4 153299 > /dev/null
  expect perft.exp giveaway "fen 8/1p6/8/8/8/8/P7/8 w - - 0 1" 4 3 > /dev/null
  expect perft.exp giveaway "fen 8/2p5/8/8/8/8/P7/8 w - - 0 1" 12 2557 > /dev/null
  expect perft.exp codrus startpos 4 153299 > /dev/null
  expect perft.exp codrus "fen 5bnr/2pp2pp/P4k2/R3Q1P1/8/2N1K3/1P1PP1PP/2B2BNR w - - 1 15" 4 19 > /dev/null
  expect perft.exp horde startpos 4 23310 > /dev/null
  expect perft.exp horde "fen 4k3/pp4q1/3P2p1/8/P3PP2/PPP2r2/PPP5/PPPP4 b - - 0 1" 4 56539 > /dev/null
  expect perft.exp horde "fen k7/5p2/4p2P/3p2P1/2p2P2/1p2P2P/p2P2P1/2P2P2 w - - 0 1" 4 33781 > /dev/null
  expect perft.exp horde "fen 4k3/7r/8/P7/2p1n2P/3p2P1/1P3P2/PPP1PPP1 w - - 0 1" 4 128809 > /dev/null
  expect perft.exp horde "fen rnbqkbnr/6p1/2p1Pp1P/P1PPPP2/Pp4PP/1p2PPPP/1P2PPPP/PP1nPPPP b kq a3 0 18" 4 197287 > /dev/null
  expect perft.exp coregal startpos 4 195896 > /dev/null
  expect perft.exp coregal "fen rn2kb1r/ppp1pppp/6q1/8/2PP2b1/5B2/PP3P1P/R1BQK1NR w KQkq - 1 9" 3 20421 > /dev/null
  expect perft.exp coregal "fen 2Q5/3Pq2k/6p1/4Bp1p/5P1P/8/8/K7 w - - 2 72" 4 55970 > /dev/null
  expect perft.exp coregal "fen r3kb1r/1pp1pppp/p1q2n2/3P4/6b1/2N2N2/PPP2PPP/R1BQ1RK1 b kq - 0 9" 4 136511 > /dev/null
  expect perft.exp knightmate startpos 4 139774 > /dev/null
  expect perft.exp losers startpos 4 152955 > /dev/null
  expect perft.exp kinglet startpos 4 197742 > /dev/null
  expect perft.exp threekings startpos 4 199514 > /dev/null
  # pockets
  expect perft.exp crazyhouse startpos 4 197281 > /dev/null
  expect perft.exp crazyhouse "fen 2k5/8/8/8/8/8/8/4K3[QRBNPqrbnp] w - - 0 1" 2 75353 > /dev/null
  expect perft.exp crazyhouse "fen 2k5/8/8/8/8/8/8/4K3[Qn] w - - 0 1" 3 88634 > /dev/null
  expect perft.exp crazyhouse "fen 2k5/8/8/8/8/8/8/4K3/Qn w - - 0 1" 3 88634 > /dev/null
  expect perft.exp crazyhouse "fen r1bqk2r/pppp1ppp/2n1p3/4P3/1b1Pn3/2NB1N2/PPP2PPP/R1BQK2R[] b KQkq - 0 1" 3 58057 > /dev/null
  expect perft.exp loop startpos 4 197281 > /dev/null
  expect perft.exp loop "fen 5R2/2p1Nb2/2B4k/6p1/8/P3PP2/1PPqR3/3R1BKn[QBNPPPPrrrnppp] b - - 1 1" 2 31983 > /dev/null
  expect perft.exp chessgi startpos 4 197281 > /dev/null
  expect perft.exp chessgi "fen 5Rp1/2p1Nb2/2B4k/6p1/8/P3PP2/1PPqR3/3R1BKn[QBNPPPPrrrnpp] b - - 1 48" 2 32816 > /dev/null
  expect perft.exp pocketknight startpos 3 88617 > /dev/null
  expect perft.exp placement startpos 3 50560 > /dev/null
  expect perft.exp placement "fen rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/QR1BKNN1[BRk] w - - 0 1" 6 17804 > /dev/null
  expect perft.exp placement "fen 1n1r1q2/pppppppp/8/8/8/8/PPPPPPPP/1N1B1R1N[KQRBkrbbn] b - - 0 4" 5 145152 > /dev/null
  expect perft.exp placement "fen r3k3/pppppppp/8/8/8/8/PPPPPPPP/R6R[Kr] w q - 0 1" 4 18492 > /dev/null
  expect perft.exp seirawan startpos 4 782599 > /dev/null
  expect perft.exp seirawan "fen reb1k2r/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] b KQkqc - 3 7" 4 890467 > /dev/null
  expect perft.exp shouse startpos 3 546694 > /dev/null
  expect perft.exp euroshogi startpos 4 380499 > /dev/null
  expect perft.exp minishogi startpos 5 533203 > /dev/null
  expect perft.exp kyotoshogi startpos 5 225903 > /dev/null
  expect perft.exp torishogi startpos 4 103857 > /dev/null
  # non-chess
  expect perft.exp ataxx startpos 4 155888 > /dev/null
  expect perft.exp ataxx "fen 7/7/7/7/ppppppp/ppppppp/PPPPPPP[PPPPPPPPPPPPPPPPPPPPPppppppppppppppppppppp] w 0 1" 5 452980 > /dev/null
  expect perft.exp breakthrough startpos 4 256036 > /dev/null
  expect perft.exp breakthrough "fen 1p2pp1p/2p2ppp/2P5/8/8/3P2P1/1p1P2PP/1PP1PP1P w - - 1 26" 4 121264 > /dev/null
  expect perft.exp clobber startpos 3 80063 > /dev/null
  # 960 variants
  expect perft.exp atomic "fen 8/8/8/8/8/8/2k5/rR4KR w KQ - 0 1" 4 61401 true > /dev/null
  expect perft.exp atomic "fen r3k1rR/5K2/8/8/8/8/8/8 b kq - 0 1" 4 98729 true > /dev/null
  expect perft.exp atomic "fen Rr2k1rR/3K4/3p4/8/8/8/7P/8 w kq - 0 1" 4 241478 true > /dev/null
  expect perft.exp atomic "fen 1R4kr/4K3/8/8/8/8/8/8 b k - 0 1" 4 17915 true > /dev/null
  expect perft.exp extinction "fen rnbqb1kr/pppppppp/8/8/8/8/PPPPPPPP/RNBQB1KR w AHah - 0 1" 4 195286 true > /dev/null
  expect perft.exp seirawan "fen qbbrnkrn/pppppppp/8/8/8/8/PPPPPPPP/QBBRNKRN[HEhe] w ABCDEFGHabcdefgh - 0 1" 3 21170 true > /dev/null
fi

# large-board variants
if [[ $1 == "all" ||  $1 == "largeboard" ]]; then
  expect perft.exp shogi startpos 4 719731 > /dev/null
  expect perft.exp shoshogi startpos 4 445372 > /dev/null  # configurable pieces
  expect perft.exp yarishogi startpos 4 158404 > /dev/null  # configurable pieces
  expect perft.exp capablanca startpos 4 805128 > /dev/null
  expect perft.exp embassy startpos 4 809539 > /dev/null
  expect perft.exp janus startpos 4 772074 > /dev/null
  expect perft.exp modern startpos 4 433729 > /dev/null
  expect perft.exp chancellor startpos 4 436656 > /dev/null
  expect perft.exp courier startpos 4 500337 > /dev/null
  expect perft.exp grand startpos 3 259514 > /dev/null
  expect perft.exp grand "fen r8r/1nbqkcabn1/ppp2ppppp/3p6/4pP4/10/10/PPPPP1PPPP/1NBQKCABN1/R8R w - e7 0 3" 2 5768 > /dev/null
  expect perft.exp opulent startpos 3 133829 > /dev/null
  expect perft.exp tencubed startpos 3 68230 > /dev/null
  expect perft.exp centaur startpos 3 24490 > /dev/null
  expect perft.exp shako "fen 4kc3c/ernbq1b1re/ppp3p1pp/3p2pp2/4p5/5P4/2PN2P3/PP1PP2PPP/ER1BQKBNR1/5C3C w KQ - 0 9" 3 26325 > /dev/null
  expect perft.exp shako "fen 4ncr1k1/1cr2P4/pp2p2pp1/P7PN/2Ep1p4/B3P1eN2/2P1n1P3/1B1P1K4/9p/5C2CR w - - 0 1" 3 180467 > /dev/null
  expect perft.exp shako "fen r5k3/4q2c2/1ebppnp3/1pp3BeEQ/10/2PE2P3/1P3P4/5NP2P/rR3KB3/7C2 w Q - 3 35" 2 4940 > /dev/null
  expect perft.exp xiangqi startpos 4 3290240 > /dev/null
  expect perft.exp xiangqi "fen 1rbaka2R/5r3/6n2/2p1p1p2/4P1bP1/PpC3Bc1/1nPR2P2/2N2AN2/1c2K1p2/2BAC4 w - - 0 1" 4 4485547 > /dev/null
  expect perft.exp xiangqi "fen 4kcP1N/8n/3rb4/9/9/9/9/3p1A3/4K4/5CB2 w - - 0 1" 4 92741 > /dev/null
  expect perft.exp manchu startpos 4 798554 > /dev/null
  expect perft.exp janggi startpos 4 1065277 > /dev/null
  expect perft.exp janggi "fen 1n1kaabn1/cr2N4/5C1c1/p1pNp3p/9/9/P1PbP1P1P/3r1p3/4A4/R1BA1KB1R b - - 0 1" 4 76763 > /dev/null
  expect perft.exp janggi "fen 1Pbcka3/3nNn1c1/N2CaC3/1pB6/9/9/5P3/9/4K4/9 w - - 0 23" 4 151202 > /dev/null
  expect perft.exp jesonmor startpos 3 27960 > /dev/null
fi

# special variants
if [[ $1 == "all" ]]; then
  expect perft.exp amazons startpos 1 2176 > /dev/null
fi

rm perft.exp

echo "perft testing OK"
