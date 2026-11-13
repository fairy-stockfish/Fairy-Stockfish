#!/bin/bash
# verify perft numbers (positions from https://www.chessprogramming.org/Perft_Results)

error()
{
  echo "perft testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

echo "perft testing started"

TESTS_FAILED=0

EXPECT_SCRIPT=$(mktemp)

cat << 'EOF' > $EXPECT_SCRIPT
#!/usr/bin/expect -f
set timeout 120
lassign [lrange $argv 0 5] var pos depth result chess960 logfile
log_file -noappend $logfile
spawn ./stockfish
send "setoption name UCI_Chess960 value $chess960\n"
send "setoption name UCI_Variant value $var\n"
send "position $pos\ngo perft $depth\n"
expect {
  "Nodes searched? $result" {}
  timeout {puts "TIMEOUT: Expected $result nodes"; exit 1}
  eof {puts "EOF: Stockfish crashed"; exit 2}
}
send "quit\n"
expect eof
EOF

chmod +x $EXPECT_SCRIPT

run_test() {
  local var="$1"
  local pos="$2"
  local depth="$3"
  local expected="$4"
  local chess960="${5:-false}"
  local tmp_file=$(mktemp)

  if $EXPECT_SCRIPT "$var" "$pos" "$depth" "$expected" "$chess960" "$tmp_file" > /dev/null 2>&1; then
    rm -f "$tmp_file"
  else
    local exit_code=$?
    echo "FAILED (exit code: $exit_code): $var depth $depth: ${pos:0:60}"
    echo "===== Output for failed test ====="
    cat "$tmp_file"
    echo "=================================="
    rm -f "$tmp_file"
    TESTS_FAILED=1
  fi
}

# chess
if [[ $1 == "" || $1 == "chess" ]]; then
  run_test chess startpos 5 4865609
  run_test chess "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -" 5 193690690
  run_test chess "fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -" 6 11030083
  run_test chess "fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" 5 15833292
  run_test chess "fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" 5 89941194
  run_test chess "fen r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10" 5 164075551
fi

# variants
if [[ $1 == "all" || $1 == "variant" ]]; then
  # small board
  run_test losalamos startpos 5 191846
  run_test losalamos "fen 6/2P3/6/1K1k2/6/6 w - - 0 1" 6 187431
  # fairy
  run_test torpedo startpos 4 209719
  run_test torpedo "fen rnbqkbnr/1ppppppp/8/6P1/p7/8/PPPPPP1P/RNBQKBNR w KQkq - 0 1" 4 232819
  run_test berolina "fen rnbqkbnr/pppp1ppp/8/2p5/5P2/8/PPP1PPPP/RNBQKBNR w KQkq c5d6 2 2" 3 46643
  run_test berolina "fen k7/6P1/8/8/8/2K2p2/4p3/8 w - - 0 1" 3 1983
  run_test berolina "fen rnbqkbnr/pp1p1ppp/8/2pPp3/8/8/PP1PPPPP/RNBQKBNR w KQkq d6c5 0 1" 2 1047
  run_test pawnsideways startpos 3 10022
  run_test pawnback startpos 3 9222
  run_test legan startpos 4 8138
  run_test makruk startpos 4 273026
  run_test cambodian startpos 4 361719
  run_test cambodian "fen r1s1ks1r/3nm3/pppNpppp/3n4/5P2/PPPPPNPP/8/R1SKMS1R b DEe 0 0 5" 2 72
  run_test karouk "fen rn1mksnr/3s4/pppppppp/8/4N3/PPPPPPPP/8/R1SKMSNR b DEde - 3 2" 4 358460
  run_test makpong "fen 3mk3/r3s1R1/1psppnp1/p1pn4/1P2NP2/P1PPP1P1/4NS2/R1SKM3 w - - 0 1" 4 593103
  run_test asean startpos 4 273026
  run_test ai-wok startpos 4 485045
  run_test ai-wok "fen 8/8/8/2sp2k1/7p/3P4/6K1/7r w - - 0 1" 5 30055
  run_test sittuyin startpos 3 580096
  run_test sittuyin "fen 8/8/6R1/s3r3/P5R1/1KP3p1/1F2kr2/8[] b - - 0 72" 4 652686
  run_test sittuyin "fen 2r5/6k1/6p1/3s2P1/3npR2/8/p2N2F1/3K4[] w - - 1 50" 4 373984
  run_test sittuyin "fen 8/6s1/5P2/3n4/pR2K2S/1P6/1k4p1/8[] w - - 1 50" 4 268869
  run_test sittuyin "fen 1k5K/3r2P1/8/8/8/8/8/8[] w - - 0 1" 5 68662
  run_test almost startpos 3 11895
  run_test almost "fen 8/1k5P/8/8/8/3K4/p7/8 b - - 0 1" 2 140
  run_test sortofalmost startpos 3 10815
  run_test sortofalmost "fen 8/1k5P/8/8/8/3K4/p7/8 b - - 0 1" 2 139
  run_test chigorin startpos 3 11408
  run_test chigorin "fen 8/1k5P/8/8/8/3K4/p7/8 b - - 0 1" 2 117
  run_test perfect startpos 3 15082
  run_test perfect "fen c3k2r/pppppppp/8/8/8/8/PPPPPPPP/C3K2R w KQkq - 0 1" 3 17500
  run_test spartan startpos 3 14244
  # duple check & mate
  run_test spartan "fen k6k/hh2Q2h/8/8/8/8/8/4K3 w - - 0 1" 3 6130
  # self duple check with promotions
  run_test spartan "fen 8/8/8/8/6Q1/8/2h3h1/4K1k1 b - - 0 1" 3 3456
  run_test shatar startpos 4 177344
  run_test shatranj startpos 4 68122
  run_test amazon startpos 4 318185
  run_test nightrider startpos 4 419019
  run_test grasshopper startpos 4 635298
  run_test hoppelpoppel startpos 4 202459
  run_test newzealand startpos 4 200310
  # alternative goals
  run_test racingkings startpos 4 296242
  run_test racingkings "fen 6r1/2K5/5k2/8/3R4/8/8/8 w - - 0 1" 4 86041
  run_test racingkings "fen 6R1/2k5/5K2/8/3r4/8/8/8 b - - 0 1" 4 86009
  run_test racingkings "fen 4brn1/2K2k2/8/8/8/8/8/8 w - - 0 1" 6 265932
  run_test kingofthehill "fen rnb2b1r/ppp2ppp/3k4/8/1PKp1pn1/3Pq3/PBP1P2P/RN1Q1B1R w - - 4 12" 3 19003
  run_test 3check "fen 7r/1p4p1/pk3p2/RN6/8/P5Pp/3p1P1P/4R1K1 w - - 1+3 1 39" 3 12407
  run_test 3check "fen 7r/1p4p1/pk3p2/RN6/8/P5Pp/3p1P1P/4R1K1 w - - 1 39 +2+0" 3 12407
  run_test atomic startpos 4 197326
  run_test atomic "fen rn2kb1r/1pp1p2p/p2q1pp1/3P4/2P3b1/4PN2/PP3PPP/R2QKB1R b KQkq - 0 1" 4 1434825
  run_test atomic "fen rn1qkb1r/p5pp/2p5/3p4/N3P3/5P2/PPP4P/R1BQK3 w Qkq - 0 1" 4 714499
  run_test atomic "fen r4b1r/2kb1N2/p2Bpnp1/8/2Pp3p/1P1PPP2/P5PP/R3K2R b KQ - 0 1" 2 148
  run_test atomar startpos 4 197779
  run_test atomar "fen 7r/6Pb/1np5/1p2kp2/P1P1n3/1P2KP1B/5N2/8 w - - 0 1" 3 24644
  run_test nocheckatomic startpos 4 197779
  run_test nocheckatomic "fen 7r/6Pb/1np5/1p2kp2/P1P1n3/1P2KP1B/5N2/8 w - - 0 1" 3 21347
  run_test antichess startpos 4 153299
  run_test giveaway startpos 4 153299
  run_test giveaway "fen 8/1p6/8/8/8/8/P7/8 w - - 0 1" 4 3
  run_test giveaway "fen 8/2p5/8/8/8/8/P7/8 w - - 0 1" 12 2557
  run_test codrus startpos 4 153299
  run_test codrus "fen 5bnr/2pp2pp/P4k2/R3Q1P1/8/2N1K3/1P1PP1PP/2B2BNR w - - 1 15" 4 19
  run_test horde startpos 4 23310
  run_test horde "fen 4k3/pp4q1/3P2p1/8/P3PP2/PPP2r2/PPP5/PPPP4 b - - 0 1" 4 56539
  run_test horde "fen k7/5p2/4p2P/3p2P1/2p2P2/1p2P2P/p2P2P1/2P2P2 w - - 0 1" 4 33781
  run_test horde "fen 4k3/7r/8/P7/2p1n2P/3p2P1/1P3P2/PPP1PPP1 w - - 0 1" 4 128809
  run_test horde "fen rnbqkbnr/6p1/2p1Pp1P/P1PPPP2/Pp4PP/1p2PPPP/1P2PPPP/PP1nPPPP b kq a3 0 18" 4 197287
  run_test coregal startpos 4 195896
  run_test coregal "fen rn2kb1r/ppp1pppp/6q1/8/2PP2b1/5B2/PP3P1P/R1BQK1NR w KQkq - 1 9" 3 20421
  run_test coregal "fen 2Q5/3Pq2k/6p1/4Bp1p/5P1P/8/8/K7 w - - 2 72" 4 55970
  run_test coregal "fen r3kb1r/1pp1pppp/p1q2n2/3P4/6b1/2N2N2/PPP2PPP/R1BQ1RK1 b kq - 0 9" 4 136511
  run_test knightmate startpos 4 139774
  run_test losers startpos 4 152955
  run_test kinglet startpos 4 197742
  run_test threekings startpos 4 199514
  run_test dobutsu startpos 6 71677
  run_test dobutsu "fen g1e/1cL/lC1/E1G[] w - - 4 3" 4 2290

  # pockets
  run_test crazyhouse startpos 4 197281
  run_test crazyhouse "fen 2k5/8/8/8/8/8/8/4K3[QRBNPqrbnp] w - - 0 1" 2 75353
  run_test crazyhouse "fen 2k5/8/8/8/8/8/8/4K3[Qn] w - - 0 1" 3 88634
  run_test crazyhouse "fen 2k5/8/8/8/8/8/8/4K3/Qn w - - 0 1" 3 88634
  run_test crazyhouse "fen r1bqk2r/pppp1ppp/2n1p3/4P3/1b1Pn3/2NB1N2/PPP2PPP/R1BQK2R[] b KQkq - 0 1" 3 58057
  run_test loop startpos 4 197281
  run_test loop "fen 5R2/2p1Nb2/2B4k/6p1/8/P3PP2/1PPqR3/3R1BKn[QBNPPPPrrrnppp] b - - 1 1" 2 31983
  run_test chessgi startpos 4 197281
  run_test chessgi "fen 5Rp1/2p1Nb2/2B4k/6p1/8/P3PP2/1PPqR3/3R1BKn[QBNPPPPrrrnpp] b - - 1 48" 2 32816
  run_test pocketknight startpos 3 88617
  run_test placement startpos 3 50560
  run_test placement "fen rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/QR1BKNN1[BRk] w - - 0 1" 6 17804
  run_test placement "fen 1n1r1q2/pppppppp/8/8/8/8/PPPPPPPP/1N1B1R1N[KQRBkrbbn] b - - 0 4" 5 145152
  run_test placement "fen r3k3/pppppppp/8/8/8/8/PPPPPPPP/R6R[Kr] w q - 0 1" 4 18492
  run_test seirawan startpos 4 782599
  run_test seirawan "fen reb1k2r/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] b KQkqc - 3 7" 4 890467
  run_test shouse startpos 3 546694
  run_test euroshogi startpos 4 380499
  run_test minishogi startpos 5 533203
  run_test kyotoshogi startpos 5 225903
  run_test micro startpos 5 71328
  run_test torishogi startpos 4 103857
  run_test koedem "fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB2BNR[KQ] w kq - 0 1" 1 34
  run_test koedem "fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR[Q] w KQkq - 0 1" 1 54
  # non-chess
  run_test ataxx startpos 4 155888
  run_test ataxx "fen 7/7/7/7/ppppppp/ppppppp/PPPPPPP w 0 1" 5 452980
  run_test breakthrough startpos 4 256036
  run_test breakthrough "fen 1p2pp1p/2p2ppp/2P5/8/8/3P2P1/1p1P2PP/1PP1PP1P w - - 1 26" 4 121264
  run_test clobber startpos 3 80063
  run_test flipello startpos 7 55092
  run_test flipersi startpos 9 38208
  # 960 variants
  run_test atomic "fen 8/8/8/8/8/8/2k5/rR4KR w KQ - 0 1" 4 61401 true
  run_test atomic "fen r3k1rR/5K2/8/8/8/8/8/8 b kq - 0 1" 4 98729 true
  run_test atomic "fen Rr2k1rR/3K4/3p4/8/8/8/7P/8 w kq - 0 1" 4 241478 true
  run_test atomic "fen 1R4kr/4K3/8/8/8/8/8/8 b k - 0 1" 4 17915 true
  run_test extinction "fen rnbqb1kr/pppppppp/8/8/8/8/PPPPPPPP/RNBQB1KR w AHah - 0 1" 4 195286 true
  run_test seirawan "fen qbbrnkrn/pppppppp/8/8/8/8/PPPPPPPP/QBBRNKRN[HEhe] w ABCDEFGHabcdefgh - 0 1" 3 21170 true
fi

# large-board variants
if [[ $1 == "all" ||  $1 == "largeboard" ]]; then
  run_test shogi startpos 4 719731
  run_test shoshogi startpos 4 445372  # configurable pieces
  run_test yarishogi startpos 4 158404  # configurable pieces
  run_test capablanca startpos 4 805128
  run_test embassy startpos 4 809539
  run_test janus startpos 4 772074
  run_test modern startpos 4 433729
  run_test chancellor startpos 4 436656
  run_test courier startpos 4 500337
  run_test grand startpos 3 259514
  run_test grand "fen r8r/1nbqkcabn1/ppp2ppppp/3p6/4pP4/10/10/PPPPP1PPPP/1NBQKCABN1/R8R w - e7 0 3" 2 5768
  run_test opulent startpos 3 133829
  run_test tencubed startpos 3 68230
  run_test centaur startpos 3 24490
  run_test gustav3 startpos 4 331659
  run_test omicron startpos 4 967381
  run_test troitzky startpos 3 8766
  run_test wolf startpos 3 13722
  run_test wolf "fen 8/k5SP/8/8/8/8/8/8/8/7K w - - 0 1" 4 10587
  run_test shako "fen 4kc3c/ernbq1b1re/ppp3p1pp/3p2pp2/4p5/5P4/2PN2P3/PP1PP2PPP/ER1BQKBNR1/5C3C w KQ - 0 9" 3 26325
  run_test shako "fen 4ncr1k1/1cr2P4/pp2p2pp1/P7PN/2Ep1p4/B3P1eN2/2P1n1P3/1B1P1K4/9p/5C2CR w - - 0 1" 3 180467
  run_test shako "fen r5k3/4q2c2/1ebppnp3/1pp3BeEQ/10/2PE2P3/1P3P4/5NP2P/rR3KB3/7C2 w Q - 3 35" 2 4940
  run_test shako "fen 10/rr3k4/ppppp5/10/10/10/10/6PPPP/5K2RR/10 w Kq - 0 1" 2 460
  run_test xiangqi startpos 4 3290240
  run_test xiangqi "fen 1rbaka2R/5r3/6n2/2p1p1p2/4P1bP1/PpC3Bc1/1nPR2P2/2N2AN2/1c2K1p2/2BAC4 w - - 0 1" 4 4485547
  run_test xiangqi "fen 4kcP1N/8n/3rb4/9/9/9/9/3p1A3/4K4/5CB2 w - - 0 1" 4 92741
  run_test manchu startpos 4 798554
  run_test janggi startpos 4 1065277
  run_test janggi "fen 1n1kaabn1/cr2N4/5C1c1/p1pNp3p/9/9/P1PbP1P1P/3r1p3/4A4/R1BA1KB1R b - - 0 1" 4 76763
  run_test janggi "fen 1Pbcka3/3nNn1c1/N2CaC3/1pB6/9/9/5P3/9/4K4/9 w - - 0 23" 4 151202
  run_test jesonmor startpos 3 27960
  run_test jesonmor "fen nn1nnn1nn/9/3n1n3/9/9/9/3N1N3/9/NN1NNN1NN w - - 4 3" 3 37564

  # non-chess
  run_test flipello10 startpos 7 55180
fi

# special variants
if [[ $1 == "all" ]]; then
  run_test duck startpos 1 640
  run_test amazons startpos 1 2176
fi

rm -f $EXPECT_SCRIPT

if [ $TESTS_FAILED -ne 0 ]; then
  echo "Some tests failed"
  exit 1
fi

echo "perft testing OK"
