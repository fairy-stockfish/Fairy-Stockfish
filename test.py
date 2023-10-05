# -*- coding: utf-8 -*-

import faulthandler
import unittest
import pyffish as sf

faulthandler.enable()

CHESS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
CHESS960 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w HAha - 0 1"
CAPA = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq - 0 1"
CAPAHOUSE = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR[] w KQkq - 0 1"
SITTUYIN = "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1"
MAKRUK = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - - 0 1"
CAMBODIAN = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w DEde - 0 1"
SHOGI = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1"
SHOGI_SFEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
SEIRAWAN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[EHeh] w KQBCDFGkqbcdfg - 0 1"
GRAND = "r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R w - - 0 1"
GRANDHOUSE = "r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R[] w - - 0 1"
XIANGQI = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1"
SHOGUN = "rnb+fkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB+FKBNR[] w KQkq - 0 1"
JANGGI = "rnba1abnr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RNBA1ABNR w - - 0 1"


ini_text = """
# Hybrid variant of Grand-chess and crazyhouse, using Grand-chess as a template
[grandhouse:grand]
startFen = r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R[] w - - 0 1
pieceDrops = true
capturesToHand = true

# Shogun chess
[shogun:crazyhouse]
startFen = rnb+fkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB+FKBNR[] w KQkq - 0 1
commoner = c
centaur = g
archbishop = a
chancellor = m
fers = f
promotionRegionWhite = *6 *7 *8
promotionRegionBlack = *3 *2 *1
promotionLimit = g:1 a:1 m:1 q:1
promotionPieceTypes = -
promotedPieceType = p:c n:g b:a r:m f:q
mandatoryPawnPromotion = false
firstRankPawnDrops = true
promotionZonePawnDrops = true
dropRegionWhite = *1 *2 *3 *4 *5
dropRegionBlack = *4 *5 *6 *7 *8
immobilityIllegal = true

# Asymmetric variant with one army using pieces that move like knights but attack like other pieces (kniroo and knibis)
[orda:chess]
startFen = lhaykahl/8/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1
centaur = h
knibis = a
kniroo = l
silver = y
promotionPieceTypes = qh
flagPiece = k
flagRegionWhite = *8
flagRegionBlack = *1

[diana:losalamos]
pieceToCharTable = PNBRQ................Kpnbrq................k
bishop = b
promotionPieceTypes = rbn
castling = true
castlingKingsideFile = e
castlingQueensideFile = b
startFen = rbnkbr/pppppp/6/6/PPPPPP/RBNKBR w KQkq - 0 1

[passchess:chess]
pass = true

[royalduck:duck]
extinctionValue = none
pseudoRoyalTypes = k

[makhouse:makruk]
startFen = rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR[] w - - 0 1
pieceDrops = true
capturesToHand = true
firstRankPawnDrops = true
promotionZonePawnDrops = true
immobilityIllegal = true

[wazirking:chess]
fers = q
king = k:W
startFen = 7k/5Kq1/8/8/8/8/8/8 w - - 0 1
stalemateValue = loss
nFoldValue = loss

[betzatest]
maxRank = 7
maxFile = 7
customPiece1 = a:lhN
customPiece2 = b:rhN
customPiece3 = c:hlN
customPiece4 = d:hrN
startFen = 7/7/7/3A3/7/7/7 w - - 0 1

[cannonshogi:shogi]
dropNoDoubled = -
shogiPawnDropMateIllegal = false
soldier = p
cannon = u
customPiece1 = a:pR
customPiece2 = c:mBcpB
customPiece3 = i:pB
customPiece4 = w:mRpRmFpB2
customPiece5 = f:mBpBmWpR2
promotedPieceType = u:w a:w c:f i:f
startFen = lnsgkgsnl/1rci1uab1/p1p1p1p1p/9/9/9/P1P1P1P1P/1BAU1ICR1/LNSGKGSNL[-] w 0 1

[fogofwar:chess]
king = -
commoner = k
castlingKingPiece = k
extinctionValue = loss
extinctionPieceTypes = k
"""

sf.load_variant_config(ini_text)


variant_positions = {
    "chess": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1": (False, False),  # startpos
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -": (False, False),  # startpos
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR": (False, False),  # startpos
        "rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3": (False, False),
        "k7/8/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs K
        "k7/b7/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs KB
        "k7/n7/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs KN
        "k7/p7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KP
        "k7/r7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KR
        "k7/q7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KQ
        "k7/nn6/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vsNN K
        "k7/bb6/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KBB opp color
        "k7/b1b5/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs KBB same color
        "kb6/8/8/8/8/8/8/K1B6 w - - 0 1": (True, True),  # KB vs KB same color
        "kb6/8/8/8/8/8/8/KB7 w - - 0 1": (False, False),  # KB vs KB opp color
        "8/8/8/8/8/6KN/8/6nk w - - 0 1": (False, False),  # KN vs KN
    },
    "atomic": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1": (False, False),  # startpos
        "8/8/8/8/3K4/3k4/8/8 b - - 0 1": (True, True),  # K vs K
        "k7/p7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KP
        "k7/q7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KQ
    },
    "crazyhouse": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq - 0 1": (False, False),  # lichess style startpos
    },
    "3check": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+3 0 1": (False, False),  # startpos
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 +0+2": (False, False),  # lichess style check count
        "k7/n7/8/8/8/8/8/K7 w - - 1+2 0 1": (True, False),  # K vs KN
        "k7/b7/8/8/8/8/8/K7 w - - 3+1 0 1": (True, False),  # K vs KB
    },
    "horde": {
        "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq - 0 1": (False, False),  # startpos
    },
    "racingkings": {
        "8/8/8/8/8/8/krbnNBRK/qrbnNBRQ w - - 0 1": (False, False),  # startpos
        "8/8/8/8/8/8/K6k/8 w - - 0 1": (False, False),  # KvK
    },
    "placement": {
        "8/pppppppp/8/8/8/8/PPPPPPPP/8[KQRRBBNNkqrrbbnn] w - - 0 1": (False, False),  # startpos
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 0 1": (False, False),  # chess startpos
        "k7/8/8/8/8/8/8/K7[] w - - 0 1": (True, True),  # K vs K
    },
    "newzealand": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1": (False, False),  # startpos
    },
    "amazon": {
        "rnbakbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBAKBNR w KQkq - 0 1": (False, False),  # startpos
        "8/8/8/8/A7/6k1/8/1K6 w - - 0 1": (False, True),  # KA vs K
    },
    "seirawan": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1": (False, False),  # startpos
        "k7/8/8/8/8/8/8/K7[] w - - 0 1": (True, True),  # K vs K
        "k7/8/8/8/8/8/8/KH6[] w - - 0 1": (False, True),  # KH vs K
        "k7/8/8/8/8/8/8/4K3[E] w E - 0 1": (False, True),  # KE vs K
    },
    "cambodian": {
        "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w DEde 0 0 1": (False, False),  # startpos
        "1ns1ksn1/r6r/pppmpppp/3p4/8/PPPPPPPP/RK2N2R/1NS1MS2 w Ee - 6 5": (False, False),
    },
    "sittuyin": {
        "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1": (False, False),  # startpos
        "k7/8/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs K, skip pocket
        "k6P/8/8/8/8/8/8/K7[] w - - 0 1": (True, True),  # KP vs K
        "k6P/8/8/8/8/8/8/K6p[] w - - 0 1": (False, False),  # KP vs KP
        "k7/8/8/8/8/8/8/KFF5[] w - - 0 1": (False, True),  # KFF vs K
        "k7/8/8/8/8/8/8/KS6[] w - - 0 1": (False, True),  # KS vs K
    },
    "makpong": {
        "8/8/8/4k2K/5m~2/4m~3/8/8 w - 128 8 58": (True, False),  # KFF vs K
        "k7/n7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KN
        "k7/8/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs K
    },
    "xiangqi": {
        "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1": (False, False),  # startpos
        "5k3/4a4/3CN4/9/1PP5p/9/8P/4C4/4A4/2B1K4 w - - 0 46": (False, False),  # issue #53
        "4k4/9/9/9/9/9/9/9/9/4K4 w - - 0 1": (True, True),  # K vs K
        "4k4/9/9/4p4/9/9/9/9/9/4KR3 w - - 0 1": (False, False),  # KR vs KP
        "4k4/9/9/9/9/9/9/9/9/3KN4 w - - 0 1": (False, True),  # KN vs K
        "4k4/9/4b4/9/9/9/9/4B4/9/4K4 w - - 0 1": (True, True),  # KB vs KB
        "4k4/9/9/9/9/9/9/9/4A4/4KC3 w - - 0 1": (False, True),  # KCA vs K
    },
    "janggi": {
        JANGGI: (False, False),  # startpos
        "rhea1aehr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RHEA1AEHR w - - 0 1": (False, False),  # startpos
        "5k3/4a4/3CN4/9/1PP5p/9/8P/4C4/4A4/2B1K4 w - - 0 46": (False, False),  # issue #53
        "4k4/9/9/9/9/4B4/4B4/9/9/4K4 w - - 0 1": (False, True),  # KEE vs K
        "4k4/9/9/9/9/9/9/9/4A4/4KC3 w - - 0 1": (False, True),  # KCA vs K
    },
    "shako": {
        "k9/10/10/10/10/10/10/10/10/KC8 w - - 0 1": (True, True),  # KC vs K
        "k9/10/10/10/10/10/10/10/10/KCC7 w - - 0 1": (False, True),  # KCC vs K
        "k9/10/10/10/10/10/10/10/10/KEC7 w - - 0 1": (False, True),  # KEC vs K
        "k9/10/10/10/10/10/10/10/10/KNE7 w - - 0 1": (False, True),  # KNE vs K
        "kb8/10/10/10/10/10/10/10/10/KE8 w - - 0 1": (False, False),  # KE vs KB opp color
        "kb8/10/10/10/10/10/10/10/10/K1E7 w - - 0 1": (True, True),  # KE vs KB same color
    },
    "orda": {
        "k7/8/8/8/8/8/8/K7 w - - 0 1": (False, False),  # K vs K
    },
    "tencubed": {
        "2cwamwc2/1rnbqkbnr1/pppppppppp/10/10/10/10/PPPPPPPPPP/1RNBQKBNR1/2CWAMWC2 w - - 0 1":  (False, False),  # startpos
        "10/5k4/10/10/10/10/10/10/5KC3/10 w - - 0 1":  (False, True),  # KC vs K
        "10/5k4/10/10/10/10/10/10/5K4/10 w - - 0 1":  (True, True),  # K vs K
    },
    "wazirking": {
        "7k/6K1/8/8/8/8/8/8 b - - 0 1": (False, False),  # K vs K
    },
}

invalid_variant_positions = {
    "chess": (
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 a",  # invalid full move
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - b 1",  # invalid half move
        "rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq -6 0 3",  # invalid en passant
        "rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq .6 0 3",  # invalid en passant
        "rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d- 0 3",  # invalid en passant
        "rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq  0 3",  # invalid/missing en passant
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w 123 - 0 1",  # invalid castling
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR g KQkq - 0 1",  # invalid side to move
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNH w KQkq - 0 1",  # invalid piece type
        "rnbqkbnr/pppppppp/7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # invalid file count
        "rnbqkbnr/pppppppp/9/7/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # invalid file count
        "rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # invalid rank count
        "rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # missing castling rook
        "1nbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # missing castling rook
        "rnbqkbnr/pppppppp/8/8/8/4K3/PPPPPPPP/RNBQ1BNR w KQkq - 0 1",  # king not on castling rank
        "rnbqkbnr/pppppppp/8/8/8/RNBQKBNR/PPPPPPPP/8 w KQkq - 0 1",  # not on castling rank
        "8/pppppppp/rnbqkbnr/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # not on castling rank
    ),
    "atomic": (
        "rnbqkbnr/pppppppp/8/8/8/RNBQKBNR/PPPPPPPP/8 w KQkq - 0 1",  # wrong castling rank
    ),
    "3check": (
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+a 0 1",  # invalid check count
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 +a+2",  # invalid lichess check count
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 +1+4",  # invalid lichess check count
    ),
    "horde": (
        "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPK w kq - 0 1",  # wrong king count
        "rnbq1bnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq - 0 1",  # wrong king count
    ),
    "sittuyin": (
        "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[FRRSSNNkfrrssnn] w - - 0 1",  # wrong king count
    ),
    "shako": {
        "c8c/ernbqkbnre/pppppppppp/10/10/10/10/PPPPPPPPPP/C8C/ERNBQKBNRE w KQkq - 0 1",  # not on castling rank
    },
    "seirawan": {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQK1NR[HEhe] w KQBCDFGkqbcdfg - 0 1",  # white gating flag
        "rnbqkb1r/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1",  # black gating flag
    }
}


class TestPyffish(unittest.TestCase):
    def test_version(self):
        result = sf.version()
        self.assertEqual(len(result), 3)

    def test_info(self):
        result = sf.info()
        self.assertTrue(result.startswith("Fairy-Stockfish"))

    def test_variants_loaded(self):
        variants = sf.variants()
        self.assertTrue("shogun" in variants)

    def test_set_option(self):
        result = sf.set_option("UCI_Variant", "capablanca")
        self.assertIsNone(result)

    def test_two_boards(self):
        self.assertFalse(sf.two_boards("chess"))
        self.assertTrue(sf.two_boards("bughouse"))

    def test_captures_to_hand(self):
        self.assertFalse(sf.captures_to_hand("seirawan"))
        self.assertTrue(sf.captures_to_hand("shouse"))

    def test_start_fen(self):
        result = sf.start_fen("capablanca")
        self.assertEqual(result, CAPA)

        result = sf.start_fen("capahouse")
        self.assertEqual(result, CAPAHOUSE)

        result = sf.start_fen("xiangqi")
        self.assertEqual(result, XIANGQI)

        result = sf.start_fen("grandhouse")
        self.assertEqual(result, GRANDHOUSE)

        result = sf.start_fen("shogun")
        self.assertEqual(result, SHOGUN)

    def test_legal_moves(self):
        fen = "10/10/10/10/10/k9/10/K9 w - - 0 1"
        result = sf.legal_moves("capablanca", fen, [])
        self.assertEqual(result, ["a1b1"])

        result = sf.legal_moves("grand", GRAND, ["a3a5"])
        self.assertIn("a10b10", result)

        result = sf.legal_moves("xiangqi", XIANGQI, ["h3h10"])
        self.assertIn("i10h10", result)

        result = sf.legal_moves("xiangqi", XIANGQI, ["h3h10"])
        self.assertIn("i10h10", result)

        result = sf.legal_moves("shogun", SHOGUN, ["c2c4", "b8c6", "b2b4", "b7b5", "c4b5", "c6b8"])
        self.assertIn("b5b6+", result)

        # Seirawan gating but no castling
        fen = "rnbq3r/pp2bkpp/8/2p1p2K/2p1P3/8/PPPP1PPP/RNB4R[EHeh] b ABCHabcdh - 0 10"
        result = sf.legal_moves("seirawan", fen, [])
        self.assertIn("c8g4h", result)

        # In Cannon Shogi the FGC and FSC can also move one square diagonally and, besides,
        # move or capture two squares diagonally, by leaping an adjacent piece. 
        fen = "lnsg1gsnl/1rc1kuab1/p1+A1p1p1p/3P5/6i2/6P2/P1P1P3P/1B1U1ICR1/LNSGKGSNL[] w - - 1 3"
        result = sf.legal_moves("cannonshogi", fen, [])
        # mF
        self.assertIn("c7b6", result)
        self.assertIn("c7d8", result)
        self.assertNotIn("c7d6", result)
        self.assertNotIn("c7b8", result)
        # pB2
        self.assertIn("c7a9", result)
        self.assertIn("c7e5", result)
        self.assertNotIn("c7a5", result)
        self.assertNotIn("c7e9", result)
        # verify distance limited to 2
        self.assertNotIn("c7f4", result)
        self.assertNotIn("c7g3", result)

        # Cambodian queen cannot capture with its leap
        # Cambodian king cannot leap to escape check
        result = sf.legal_moves("cambodian", CAMBODIAN, ["b1d2", "g8e7", "d2e4", "d6d5", "e4d6"])
        self.assertNotIn("d8d6", result)
        self.assertNotIn("e8g7", result)
        self.assertNotIn("e8c7", result)

        # In Janggi stalemate position pass move (in place king move) is possible
        fen = "4k4/c7R/9/3R1R3/9/9/9/9/9/3K5 b - - 0 1"
        result = sf.legal_moves("janggi", fen, [])
        self.assertEqual(result, ["e10e10"])

        # pawn promotion of dropped pawns beyond promotion rank
        result = sf.legal_moves("makhouse", "rnsmksnr/8/1ppP1ppp/p3p3/8/PPP1PPPP/8/RNSKMSNR[p] w - - 0 4", [])
        self.assertIn("d6d7m", result)
        self.assertNotIn("d6d7", result)

        # Test configurable piece perft
        legals = ['a3a4', 'b3b4', 'c3c4', 'd3d4', 'e3e4', 'f3f4', 'g3g4', 'e1e2', 'f1f2', 'b1a2', 'b1b2', 'b1c2', 'c1b2', 'c1c2', 'c1d2', 'a1a2', 'g1g2', 'd1c2', 'd1d2', 'd1e2']
        result = sf.legal_moves("yarishogi", sf.start_fen("yarishogi"), [])
        self.assertCountEqual(legals, result)

        # Test betza parsing
        result = sf.legal_moves("betzatest", "7/7/7/3A3/7/7/7 w - - 0 1", [])
        self.assertEqual(['d4c2', 'd4b3', 'd4b5', 'd4c6'], result)
        result = sf.legal_moves("betzatest", "7/7/7/3B3/7/7/7 w - - 0 1", [])
        self.assertEqual(['d4e2', 'd4f3', 'd4f5', 'd4e6'], result)
        result = sf.legal_moves("betzatest", "7/7/7/3C3/7/7/7 w - - 0 1", [])
        self.assertEqual(['d4e2', 'd4b3', 'd4f5', 'd4c6'], result)
        result = sf.legal_moves("betzatest", "7/7/7/3D3/7/7/7 w - - 0 1", [])
        self.assertEqual(['d4c2', 'd4f3', 'd4b5', 'd4e6'], result)


    def test_short_castling(self):
        legals = ['f5f4', 'a7a6', 'b7b6', 'c7c6', 'd7d6', 'e7e6', 'i7i6', 'j7j6', 'a7a5', 'b7b5', 'c7c5', 'e7e5', 'i7i5', 'j7j5', 'b8a6', 'b8c6', 'h6g4', 'h6i4', 'h6j5', 'h6f7', 'h6g8', 'h6i8', 'd5a2', 'd5b3', 'd5f3', 'd5c4', 'd5e4', 'd5c6', 'd5e6', 'd5f7', 'd5g8', 'j8g8', 'j8h8', 'j8i8', 'e8f7', 'c8b6', 'c8d6', 'g6g2', 'g6g3', 'g6f4', 'g6g4', 'g6h4', 'g6e5', 'g6g5', 'g6i5', 'g6a6', 'g6b6', 'g6c6', 'g6d6', 'g6e6', 'g6f6', 'g6h8', 'f8f7', 'f8g8', 'f8i8']
        moves = ['b2b4', 'f7f5', 'c2c3', 'g8d5', 'a2a4', 'h8g6', 'f2f3', 'i8h6', 'h2h3']
        result = sf.legal_moves("capablanca", CAPA, moves)
        self.assertCountEqual(legals, result)
        self.assertIn("f8i8", result)

        moves = ['a2a4', 'f7f5', 'b2b3', 'g8d5', 'b1a3', 'i8h6', 'c1a2', 'h8g6', 'c2c4']
        result = sf.legal_moves("capablanca", CAPA, moves)
        self.assertIn("f8i8", result)

        moves = ['f2f4', 'g7g6', 'g1d4', 'j7j6', 'h1g3', 'b8a6', 'i1h3', 'h7h6']
        result = sf.legal_moves("capablanca", CAPA, moves)
        self.assertIn("f1i1", result)

        # Check that chess960 castling notation is used for otherwise ambiguous castling move
        # d1e1 is a normal king move, so castling has to be d1f1
        result = sf.legal_moves("diana", "rbnk1r/pppbpp/3p2/5P/PPPPPB/RBNK1R w KQkq - 2 3", [])
        self.assertIn("d1f1", result)

        # Check that in variants where castling rooks are not in the corner
        # the castling rook is nevertheless assigned correctly
        result = sf.legal_moves("shako", "c8c/ernbqkbnre/pppppppppp/10/10/10/10/PPPPPPPPPP/5K2RR/10 w Kkq - 0 1", [])
        self.assertIn("f2h2", result)
        result = sf.legal_moves("shako", "c8c/ernbqkbnre/pppppppppp/10/10/10/10/PPPPPPPPPP/RR3K4/10 w Qkq - 0 1", [])
        self.assertIn("f2d2", result)

    def test_get_fen(self):
        result = sf.get_fen("chess", CHESS, [])
        self.assertEqual(result, CHESS)

        # incomplete FENs
        result = sf.get_fen("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", [])
        self.assertEqual(result, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1")
        result = sf.get_fen("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", [])
        self.assertEqual(result, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
        result = sf.get_fen("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w 1 2", [])
        self.assertEqual(result, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 1 2")

        # invalid castling rights
        result = sf.get_fen("chess", "8/rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", [])
        self.assertEqual(result, "8/rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1")
        result = sf.get_fen("chess", "r7/1nbqkbnr/pppppppp/8/8/P6P/RPPPPPPR/1NBQKBN1 w KQkq - 0 1", [])
        self.assertEqual(result, "r7/1nbqkbnr/pppppppp/8/8/P6P/RPPPPPPR/1NBQKBN1 w - - 0 1")

        # alternative piece symbols
        result = sf.get_fen("janggi", "rhea1aehr/4k4/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/4K4/RHEA1AEHR w - - 0 1", [])
        self.assertEqual(result, JANGGI)

        result = sf.get_fen("capablanca", CAPA, [])
        self.assertEqual(result, CAPA)

        result = sf.get_fen("xiangqi", XIANGQI, [])
        self.assertEqual(result, XIANGQI)

        result = sf.get_fen("seirawan", SEIRAWAN, [])
        self.assertEqual(result, SEIRAWAN)

        # test idempotence for S-Chess960 gating flags
        fen1 = sf.get_fen("seirawan", SEIRAWAN, [], True)
        fen2 = sf.get_fen("seirawan", fen1, [], True)
        self.assertEqual(fen1, fen2)

        fen = "rnab1kbcnr/ppppPppppp/10/4q5/10/10/PPPPP1PPPP/RNABQKBCNR[p] b KQkq - 0 3"
        result = sf.get_fen("capahouse", CAPA, ["f2f4", "e7e5", "f4e5", "e8e5", "P@e7"])
        self.assertEqual(result, fen)

        fen0 = "reb1k2r/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] b KQkqac - 2 6"
        fen1 = "reb2rk1/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] w KQac - 3 7"
        result = sf.get_fen("seirawan", fen0, ["e8g8"])
        self.assertEqual(result, fen1)

        # handle invalid castling/gating flags
        fen0 = "rnbq3r/pp2bkpp/8/2p1p2K/2p1P3/8/PPPP1PPP/RNB4R[EHeh] b QBCEHabcdk - 0 10"
        fen1 = "rnbq3r/pp2bkpp/8/2p1p2K/2p1P3/8/PPPP1PPP/RNB4R[EHeh] b ABCHabcdh - 0 10"
        result = sf.get_fen("seirawan", fen0, [])
        self.assertEqual(result, fen1)

        result = sf.get_fen("chess", CHESS, [], True, False, False)
        self.assertEqual(result, CHESS960)

        # test O-O-O
        fen = "rbkqnrbn/pppppppp/8/8/8/8/PPPPPPPP/RBKQNRBN w AFaf - 0 1"
        moves = ["d2d4", "f7f5", "e1f3", "h8g6", "h1g3", "c7c6", "c2c3", "e7e6", "b1d3", "d7d5", "d1c2", "b8d6", "e2e3", "d8d7", "c1a1"]
        result = sf.get_fen("chess", fen, moves, True, False, False)
        self.assertEqual(result, "r1k1nrb1/pp1q2pp/2pbp1n1/3p1p2/3P4/2PBPNN1/PPQ2PPP/2KR1RB1 b fa - 2 8")

        # passing should not affect castling rights
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        result = sf.get_fen("passchess", fen, ["e1e1", "e8e8"])
        self.assertEqual(result, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 2 2")

        # only irreversible moves should reset 50 move rule counter
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        result = sf.get_fen("pawnsideways", fen, ["e2e4", "g8f6", "e4d4"])
        self.assertEqual(result, "rnbqkb1r/pppppppp/5n2/8/3P4/8/PPPP1PPP/RNBQKBNR b KQkq - 2 2")
        result = sf.get_fen("pawnback", fen, ["e2e4", "e7e6"])
        self.assertEqual(result, "rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 2 2")
        result = sf.get_fen("pocketknight", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[Nn] w KQkq - 0 1", ["N@e4"])
        self.assertEqual(result, "rnbqkbnr/pppppppp/8/8/4N3/8/PPPPPPPP/RNBQKBNR[n] b KQkq - 0 1")

        # duck chess en passant
        fen = "r1b1k3/pp3pb1/4p3/2p2p2/2PpP2q/1P1P1P2/P1K1*3/RN1Q2N1 b q e3 0 17"
        result = sf.get_fen("duck", fen, [])
        self.assertEqual(result, fen)

        # SFEN
        result = sf.get_fen("shogi", SHOGI, [], False, True)
        self.assertEqual(result, SHOGI_SFEN)

        # makruk FEN
        fen = "rnsmksnr/8/1pM~1pppp/p7/8/PPPP1PPP/8/RNSKMSNR b - - 0 3"
        result = sf.get_fen("makruk", MAKRUK, ["e3e4", "d6d5", "e4d5", "a6a5", "d5c6m"], False, False, True)
        self.assertEqual(result, fen)
        result = sf.get_fen("makruk", fen, [], False, False, True)
        self.assertEqual(result, fen)

        # makruk piece honor counting
        fen = "8/3k4/8/2K1S1P1/8/8/8/8 w - - 0 1"
        moves = ["g5g6m"]
        result = sf.get_fen("makruk", fen, moves, False, False, True)
        self.assertEqual(result, "8/3k4/6M~1/2K1S3/8/8/8/8 b - 88 8 1")

        fen = "8/2K3k1/5m2/4S1S1/8/8/8/8 w - 128 97 1"
        moves = ["e5f6"]
        result = sf.get_fen("makruk", fen, moves, False, False, True)
        self.assertEqual(result, "8/2K3k1/5S2/6S1/8/8/8/8 b - 44 8 1")

        # ignore count_started for piece honor counting
        fen = "8/3k4/8/2K1S1P1/8/8/8/8 w - - 0 1"
        moves = ["g5g6m"]
        result = sf.get_fen("makruk", fen, moves, False, False, True, -1)
        self.assertEqual(result, "8/3k4/6M~1/2K1S3/8/8/8/8 b - 88 8 1")

        fen = "8/2K3k1/5m2/4S1S1/8/8/8/8 w - 128 1 30"
        moves = ["e5f6"]
        result = sf.get_fen("makruk", fen, moves, False, False, True, 58)
        self.assertEqual(result, "8/2K3k1/5S2/6S1/8/8/8/8 b - 44 8 30")

        # makruk board honor counting
        fen = "3k4/2m5/8/4MP2/3KS3/8/8/8 w - - 0 1"
        moves = ["f5f6m"]
        result = sf.get_fen("makruk", fen, moves, False, False, True)
        self.assertEqual(result, "3k4/2m5/5M~2/4M3/3KS3/8/8/8 b - 128 0 1")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 128 0 33"
        moves = ["d4d5"]
        result = sf.get_fen("makruk", fen, moves, False, False, True)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 128 1 33")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 128 36 1"
        moves = ["d4d5"]
        result = sf.get_fen("makruk", fen, moves, False, False, True)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 128 37 1")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 128 0 33"
        moves = ["d4d5"]
        result = sf.get_fen("makruk", fen, moves, False, False, True, -1)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 128 0 33")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 128 7 33"
        moves = ["d4d5"]
        result = sf.get_fen("makruk", fen, moves, False, False, True, 58)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 128 8 33")

        # ouk piece honor counting
        fen = "8/3k4/8/2K1S1P1/8/8/8/8 w - - 0 1"
        moves = ["g5g6m"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "8/3k4/6M~1/2K1S3/8/8/8/8 b - 86 8 1")

        fen = "8/2K3k1/5m2/4S1S1/8/8/8/8 w - 128 97 1"
        moves = ["e5f6"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "8/2K3k1/5S2/6S1/8/8/8/8 b - 42 8 1")

        # adjust to board honor counting if it's faster
        fen = "8/3k4/8/2K1S1P1/8/8/8/8 w - - 0 1"
        moves = ["g5g6m"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True, -1)
        self.assertEqual(result, "8/3k4/6M~1/2K1S3/8/8/8/8 b - 86 8 1")

        fen = "8/2K3k1/5m2/4S1S1/8/8/8/8 w - 126 101 80"
        moves = ["e5f6"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True, 58)
        self.assertEqual(result, "8/2K3k1/5S2/6S1/8/8/8/8 b - 126 102 80")

        # pawn promotion triggers piece honor counting
        fen = "8/8/4k3/5P2/8/2RMK3/8/8 w - 126 41 50"
        moves = ["f5f6m"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True, 58)
        self.assertEqual(result, "8/8/4kM~2/8/8/2RMK3/8/8 b - 30 10 50")

        # king capturing the last unpromoted pawn triggers piece honor counting
        fen = "8/8/4k3/5P2/8/2RMK3/8/8 b - 126 42 50"
        moves = ["e6f5"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True, 58)
        self.assertEqual(result, "8/8/8/5k2/8/2RMK3/8/8 w - 30 7 51")

        # ouk board honor counting
        fen = "3k4/2m5/8/4MP2/3KS3/8/8/8 w - - 0 1"
        moves = ["f5f6m"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "3k4/2m5/5M~2/4M3/3KS3/8/8/8 b - 126 0 1")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 126 0 33"
        moves = ["d4d5"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 126 1 33")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 126 36 1"
        moves = ["d4d5"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 126 37 1")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 126 0 33"
        moves = ["d4d5"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True, -1)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 126 0 33")

        fen = "3k4/2m5/5M~2/4M3/3KS3/8/8/8 w - 126 7 33"
        moves = ["d4d5"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True, 58)
        self.assertEqual(result, "3k4/2m5/5M~2/3KM3/4S3/8/8/8 b - 126 8 33")

        # asean counting
        fen = "4k3/3r4/2K5/8/3R4/8/8/8 w - - 0 1"
        moves = ["d4d7"]
        result = sf.get_fen("asean", fen, moves, False, False, False)
        self.assertEqual(result, "4k3/3R4/2K5/8/8/8/8/8 b - 32 0 1")

        fen = "4k3/3r4/2K5/8/3R4/1P6/8/8 w - - 0 1"
        moves = ["d4d7"]
        result = sf.get_fen("asean", fen, moves, False, False, False)
        self.assertEqual(result, "4k3/3R4/2K5/8/8/1P6/8/8 b - - 0 1")

        fen = "8/2P1k3/2K5/8/8/8/8/8 w - - 0 1"
        moves = ["c7c8b"]
        result = sf.get_fen("asean", fen, moves, False, False, False)
        self.assertEqual(result, "2B5/4k3/2K5/8/8/8/8/8 b - 88 0 1")

        fen = "8/8/4K3/3Q4/1k1N4/5b2/8/8 w - - 0 1"
        moves = ["d4f3"]
        result = sf.get_fen("asean", fen, moves, False, False, False)
        self.assertEqual(result, "8/8/4K3/3Q4/1k6/5N2/8/8 b - 128 0 1")

        fen = "3Q4/4P3/4K3/3Q4/1k6/8/8/8 w - - 0 1"
        moves = ["e7e8q"]
        result = sf.get_fen("asean", fen, moves, False, False, False)
        self.assertEqual(result, "3QQ3/8/4K3/3Q4/1k6/8/8/8 b - - 0 1")

        # Cambodian king loses its leap ability when it is "aimed" by a rook
        fen = "rnsmk1nr/4s3/pppppppp/8/8/PPPPPPPP/R7/1NSKMSNR w DEde - 2 2"
        moves = ["a2e2"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "rnsmk1nr/4s3/pppppppp/8/8/PPPPPPPP/4R3/1NSKMSNR b DEd - 3 2")

        fen = "1nsmksnr/r7/pppppppp/8/8/PPPPPPPP/2SN4/R2KMSNR b DEde - 3 2"
        moves = ["a7d7"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "1nsmksnr/3r4/pppppppp/8/8/PPPPPPPP/2SN4/R2KMSNR w Ede - 4 3")

        fen = "rnsmksnr/8/1ppppppp/8/8/1PPPPPPP/8/RNSKMSNR w DEde - 0 1"
        moves = ["a1a8"]
        result = sf.get_fen("cambodian", fen, moves, False, False, True)
        self.assertEqual(result, "Rnsmksnr/8/1ppppppp/8/8/1PPPPPPP/8/1NSKMSNR b DEd - 0 1")

    def test_get_san(self):
        fen = "4k3/8/3R4/8/1R3R2/8/3R4/4K3 w - - 0 1"
        result = sf.get_san("chess", fen, "b4d4")
        self.assertEqual(result, "Rbd4")

        result = sf.get_san("chess", fen, "f4d4")
        self.assertEqual(result, "Rfd4")

        result = sf.get_san("chess", fen, "d2d4")
        self.assertEqual(result, "R2d4")

        result = sf.get_san("chess", fen, "d6d4")
        self.assertEqual(result, "R6d4")

        fen = "4k3/8/3R4/3P4/1RP1PR2/8/3R4/4K3 w - - 0 1"
        result = sf.get_san("chess", fen, "d2d4")
        self.assertEqual(result, "Rd4")

        fen = "1r2k3/P1P5/8/8/8/8/8/4K3 w - - 0 1"
        result = sf.get_san("chess", fen, "c7b8q")
        self.assertEqual(result, "cxb8=Q+")

        fen = "1r2k3/P1P5/8/8/8/8/8/4K3 w - - 0 1"
        result = sf.get_san("chess", fen, "c7b8q", False, sf.NOTATION_LAN)
        self.assertEqual(result, "c7xb8=Q+")

        result = sf.get_san("capablanca", CAPA, "e2e4")
        self.assertEqual(result, "e4")

        result = sf.get_san("capablanca", CAPA, "e2e4", False, sf.NOTATION_LAN)
        self.assertEqual(result, "e2-e4")

        result = sf.get_san("capablanca", CAPA, "h1i3")
        self.assertEqual(result, "Ci3")

        result = sf.get_san("sittuyin", SITTUYIN, "R@a1")
        self.assertEqual(result, "R@a1")

        fen = "3rr3/1kn3n1/1ss1p1pp/1pPpP3/6PP/p3KN2/2SSFN2/3R3R[] b - - 0 14"
        result = sf.get_san("sittuyin", fen, "c6c5")
        self.assertEqual(result, "Scxc5")

        fen = "7R/1r6/3k1np1/3s2N1/3s3P/4n3/6p1/2R3K1[] w - - 2 55"
        result = sf.get_san("sittuyin", fen, "h4h4f")
        self.assertEqual(result, "h4=F")

        fen = "k7/2K3P1/8/4P3/8/8/8/1R6[] w - - 0 1"
        result = sf.get_san("sittuyin", fen, "e5f6f")
        self.assertEqual(result, "e5f6=F")

        result = sf.get_san("shogi", SHOGI, "i3i4")
        self.assertEqual(result, "P-16")

        result = sf.get_san("shogi", SHOGI, "i3i4", False, sf.NOTATION_SHOGI_HOSKING)
        self.assertEqual(result, "P16")

        result = sf.get_san("shogi", SHOGI, "f1e2", False, sf.NOTATION_SHOGI_HOSKING)
        self.assertEqual(result, "G49-58")
        result = sf.get_san("shogi", SHOGI, "f1e2", False, sf.NOTATION_SHOGI_HODGES)
        self.assertEqual(result, "G4i-5h")
        result = sf.get_san("shogi", SHOGI, "f1e2", False, sf.NOTATION_SHOGI_HODGES_NUMBER)
        self.assertEqual(result, "G49-58")

        # Disambiguation of promotion moves
        fen = "p1ksS/n1n2/4P/5/+L1K1+L[] b - - 3 9"
        result = sf.get_san("kyotoshogi", fen, "c4b2+", False, sf.NOTATION_SHOGI_HODGES_NUMBER)
        self.assertEqual(result, "N32-44+")
        result = sf.get_san("kyotoshogi", fen, "a4b2+", False, sf.NOTATION_SHOGI_HODGES_NUMBER)
        self.assertEqual(result, "N52-44+")

        # Demotion
        fen = "p+nks+l/5/5/L4/1SK+NP[-] b 0 1"
        result = sf.get_san("kyotoshogi", fen, "e5e4-", False, sf.NOTATION_SAN)
        self.assertEqual(result, "Ge4=L")

        fen = "lnsgkgsnl/1r5b1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL w -"
        result = sf.get_san("shogi", fen, "b2h8", False, sf.NOTATION_SHOGI_HODGES)
        self.assertEqual(result, "Bx2b=")
        result = sf.get_san("shogi", fen, "b2h8+", False, sf.NOTATION_SHOGI_HODGES)
        self.assertEqual(result, "Bx2b+")

        fen = "lnsgkg1nl/1r5s1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/7R1/LNSGKGSNL[Bb] w "
        result = sf.get_san("shogi", fen, "B@g7", False, sf.NOTATION_SHOGI_HODGES)
        self.assertEqual(result, "B*3c")
        result = sf.get_san("shogi", fen, "B@g7", False, sf.NOTATION_SHOGI_HODGES_NUMBER)
        self.assertEqual(result, "B*33")

        fen = "lnsgkg1nl/1r4s+B1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/7R1/LNSGKGSNL[B] w "
        result = sf.get_san("shogi", fen, "h8g7", False, sf.NOTATION_SHOGI_HODGES)
        self.assertEqual(result, "+B-3c")

        fen = "lnk2gsnl/7b1/p1p+SGp1pp/6p2/1pP6/4P4/PP3PPPP/1S2G2R1/L2GK1bNL[PRppns] w "
        result = sf.get_san("shogi", fen, "d7d8", False, sf.NOTATION_SHOGI_HODGES)
        self.assertEqual(result, "+S-6b")

        result = sf.get_san("xiangqi", XIANGQI, "h1g3")
        self.assertEqual(result, "Hg3")

        result = sf.get_san("xiangqi", XIANGQI, "h1g3", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "H2+3")

        result = sf.get_san("xiangqi", XIANGQI, "c1e3")
        self.assertEqual(result, "Ece3")

        result = sf.get_san("xiangqi", XIANGQI, "c1e3", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "E7+5")

        result = sf.get_san("xiangqi", XIANGQI, "h3h10")
        self.assertEqual(result, "Cxh10")

        result = sf.get_san("xiangqi", XIANGQI, "h3h10", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "C2+7")

        result = sf.get_san("xiangqi", XIANGQI, "h3h5")
        self.assertEqual(result, "Ch5")

        # WXF notation does not denote check or checkmate
        fen = "4k4/4a3R/9/9/9/9/9/9/4K4/9 w - - 0 1"
        result = sf.get_san("xiangqi", fen, "i9e9", False)
        self.assertEqual(result, "Rxe9+")
        result = sf.get_san("xiangqi", fen, "i9e9", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "R1=5")
        result = sf.get_san("xiangqi", fen, "i9i10", False)
        self.assertEqual(result, "Ri10#")
        result = sf.get_san("xiangqi", fen, "i9i10", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "R1+1")

        # skip disambiguation for elephants and advisors, but not for pieces that require it
        fen = "rnbakabnr/9/1c5c1/p1p1p1p1p/4P4/1NB6/P1P1P3P/1C1A3C1/9/RNBAK4 w - - 0 1"
        result = sf.get_san("xiangqi", fen, "c5e3", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "E7-5")
        result = sf.get_san("xiangqi", fen, "d1e2", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "A6+5")
        result = sf.get_san("xiangqi", fen, "b5c7", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "H++7")
        result = sf.get_san("xiangqi", fen, "e6e7", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "P++1")
        result = sf.get_san("xiangqi", fen, "e4e5", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "P-+1")

        # Tandem pawns
        fen = "rnbakabnr/9/1c5c1/p1p1P1p1p/4P4/9/P3P3P/1C5C1/9/RNBAKABNR w - - 0 1"
        result = sf.get_san("xiangqi", fen, "e7d7", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "15=6")
        result = sf.get_san("xiangqi", fen, "e6d6", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "25=6")
        result = sf.get_san("xiangqi", fen, "e4e5", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "35+1")

        # use tandem pawn notation for pair of tandem pawns
        fen = "5k3/9/3P5/3P1P1P1/5P3/9/9/9/9/4K4 w - - 0 1"
        result = sf.get_san("xiangqi", fen, "d7e7", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "26=5")
        result = sf.get_san("xiangqi", fen, "f6e6", False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, "24=5")

        fen = "1rb1ka2r/4a4/2ncb1nc1/p1p1p1p1p/9/2P6/P3PNP1P/2N1C2C1/9/R1BAKAB1R w - - 1 7"
        result = sf.get_san("xiangqi", fen, "c3e2")
        self.assertEqual(result, "Hce2")

        result = sf.get_san("xiangqi", fen, "c3d5")
        self.assertEqual(result, "Hd5")

        result = sf.get_san("janggi", JANGGI, "b1c3", False, sf.NOTATION_JANGGI)
        self.assertEqual(result, "H02-83")

        fen = "1b1aa2b1/5k3/3ncn3/1pp1pp3/5r2p/9/P1PPB1PPB/2N1CCN1c/9/R2AKAR2 w - - 19 17"
        result = sf.get_san("janggi", fen, "d1e2", False, sf.NOTATION_SAN)
        self.assertEqual(result, "Ade2")

        fen = "1Pbcka3/3nNn1c1/N2CaC3/1pB6/9/9/5P3/9/4K4/9 w - - 0 23"
        result = sf.get_san("janggi", fen, "f8f10", False, sf.NOTATION_SAN)
        self.assertEqual(result, "Cfxf10")

        result = sf.get_san("makruk", MAKRUK, "e3e4")
        self.assertEqual(result, "e4")
        result = sf.get_san("makruk", MAKRUK, "e3e4", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "จ๔")
        result = sf.get_san("makruk", MAKRUK, "e3e4", False, sf.NOTATION_THAI_LAN)
        self.assertEqual(result, "บ จ๓-จ๔")

        fen = "r1smksnr/3n4/pppp1ppp/4p3/4PP2/PPPP2PP/8/RNSKMSNR w - - 0 1"
        result = sf.get_san("makruk", fen, "f4e5")
        self.assertEqual(result, "fxe5")
        result = sf.get_san("makruk", fen, "f4e5", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "ฉxจ๕")
        result = sf.get_san("makruk", fen, "f4e5", False, sf.NOTATION_THAI_LAN)
        self.assertEqual(result, "บ ฉ๔xจ๕")

        fen = "rnsm1s1r/4n1k1/1ppppppp/p7/2PPP3/PP3PPP/4N2R/RNSKMS2 b - - 1 5"
        result = sf.get_san("makruk", fen, "f8f7")
        self.assertEqual(result, "Sf7")
        result = sf.get_san("makruk", fen, "f8f7", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "ค-ฉ๗")
        result = sf.get_san("makruk", fen, "f8f7", False, sf.NOTATION_THAI_LAN)
        self.assertEqual(result, "ค ฉ๘-ฉ๗")

        fen = "4k3/8/8/4S3/8/2S5/8/4K3 w - - 0 1"
        result = sf.get_san("makruk", fen, "e5d4")
        self.assertEqual(result, "Sed4")
        result = sf.get_san("makruk", fen, "c3d4")
        self.assertEqual(result, "Scd4")
        result = sf.get_san("makruk", fen, "e5d4", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "คจ-ง๔")
        result = sf.get_san("makruk", fen, "c3d4", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "คค-ง๔")
        result = sf.get_san("makruk", fen, "e5d4", False, sf.NOTATION_THAI_LAN)
        self.assertEqual(result, "ค จ๕-ง๔")
        result = sf.get_san("makruk", fen, "c3d4", False, sf.NOTATION_THAI_LAN)
        self.assertEqual(result, "ค ค๓-ง๔")

        # Distinction between the regular met and the promoted pawn
        fen = "4k3/8/4M3/4S3/8/2S5/8/4K3 w - - 0 1"
        result = sf.get_san("makruk", fen, "e6d5", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "ม็-ง๕")
        fen = "4k3/8/4M~3/4S3/8/2S5/8/4K3 w - - 0 1"
        result = sf.get_san("makruk", fen, "e6d5", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "ง-ง๕")

        fen = "4k3/8/8/3S4/8/3S4/8/4K3 w - - 0 1"
        result = sf.get_san("makruk", fen, "d3d4")
        self.assertEqual(result, "Sd4")
        result = sf.get_san("makruk", fen, "d3d4", False, sf.NOTATION_THAI_SAN)
        self.assertEqual(result, "ค-ง๔")
        result = sf.get_san("makruk", fen, "d3d4", False, sf.NOTATION_THAI_LAN)
        self.assertEqual(result, "ค ง๓-ง๔")


        UCI_moves = ["e2e4", "e7e5", "g1f3", "b8c6h", "f1c4", "f8c5e"]
        SAN_moves = ["e4", "e5", "Nf3", "Nc6/H", "Bc4", "Bc5/E"]

        fen = SEIRAWAN
        for i, move in enumerate(UCI_moves):
            result = sf.get_san("seirawan", fen, move)
            self.assertEqual(result, SAN_moves[i])
            fen = sf.get_fen("seirawan", SEIRAWAN, UCI_moves[:i + 1])

        result = sf.get_san("seirawan", fen, "e1g1")
        self.assertEqual(result, "O-O")

        result = sf.get_san("seirawan", fen, "e1g1h")
        self.assertEqual(result, "O-O/He1")
        result = sf.get_san("seirawan", fen, "e1g1e")
        self.assertEqual(result, "O-O/Ee1")

        result = sf.get_san("seirawan", fen, "h1e1h")
        self.assertEqual(result, "O-O/Hh1")
        result = sf.get_san("seirawan", fen, "h1e1e")
        self.assertEqual(result, "O-O/Eh1")

        # Disambiguation only when necessary
        fen = "rnbqkb1r/ppp1pppp/5n2/3p4/3P4/5N2/PPP1PPPP/RNBQKB1R[EHeh] w KQABCDEFHkqabcdefh - 2 3"
        result = sf.get_san("seirawan", fen, "b1d2e")
        self.assertEqual(result, "Nd2/E")
        result = sf.get_san("seirawan", fen, "b1d2")
        self.assertEqual(result, "Nbd2")

    def test_get_san_moves(self):
        UCI_moves = ["e2e4", "e7e5", "g1f3", "b8c6h", "f1c4", "f8c5e"]
        SAN_moves = ["e4", "e5", "Nf3", "Nc6/H", "Bc4", "Bc5/E"]
        result = sf.get_san_moves("seirawan", SEIRAWAN, UCI_moves)
        self.assertEqual(result, SAN_moves)

        UCI_moves = ["c3c4", "g7g6", "b2h8"]
        SAN_moves = ["P-76", "P-34", "Bx22="]
        result = sf.get_san_moves("shogi", SHOGI, UCI_moves)
        self.assertEqual(result, SAN_moves)

        UCI_moves = ["h3e3", "h10g8", "h1g3", "c10e8", "a1a3", "i10h10"]
        SAN_moves = ["C2=5", "H8+7", "H2+3", "E3+5", "R9+2", "R9=8"]
        result = sf.get_san_moves("xiangqi", XIANGQI, UCI_moves, False, sf.NOTATION_XIANGQI_WXF)
        self.assertEqual(result, SAN_moves)

        UCI_moves = ["e2e4", "d7d5", "f1a6+", "d8d6"]
        SAN_moves = ["e4", "d5", "Ba6=A", "Qd6"]
        result = sf.get_san_moves("shogun", SHOGUN, UCI_moves)
        self.assertEqual(result, SAN_moves)

    def test_gives_check(self):
        result = sf.gives_check("capablanca", CAPA, [])
        self.assertFalse(result)

        result = sf.gives_check("capablanca", CAPA, ["e2e4"])
        self.assertFalse(result)

        moves = ["g2g3", "d7d5", "a2a3", "c8h3"]
        result = sf.gives_check("capablanca", CAPA, moves)
        self.assertTrue(result)

        # Test giving check to pseudo royal piece
        result = sf.gives_check("atomic", CHESS, [])
        self.assertFalse(result)

        result = sf.gives_check("atomic", CHESS, ["e2e4"])
        self.assertFalse(result)

        result = sf.gives_check("atomic", CHESS, ["e2e4", "d7d5", "f1b5"])
        self.assertTrue(result)

        result = sf.gives_check("atomic", "rnbqkbnr/ppp2ppp/8/8/8/8/PPP2PPP/RNBQKBNR w KQkq - 0 4", ["d1d7"])
        self.assertTrue(result)

        result = sf.gives_check("atomic", "8/8/kK6/8/8/8/Q7/8 b - - 0 1", [])
        self.assertFalse(result)

        # pseudo-royal duple check
        result = sf.gives_check("spartan", "lgkcckw1/hhhhhhhh/1N3lN1/8/8/8/PPPPPPPP/R1BQKB1R b KQ - 11 6", [])
        self.assertTrue(result)
        result = sf.gives_check("spartan", "lgkcckwl/hhhhhhhh/6N1/8/8/8/PPPPPPPP/RNBQKB1R b KQ - 5 3", [])
        self.assertFalse(result)
        result = sf.gives_check("spartan", "lgkcckwl/hhhhhhhh/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1", [])
        self.assertFalse(result)

        # Shako castling discovered check
        result = sf.gives_check("shako", "10/5r4/2p3pBk1/1p6Pr/p3p5/9e/1PP2P4/P2P2PP2/ER3K2R1/8C1 w K - 7 38", ["f2h2"])
        self.assertTrue(result)

        # Janggi palace discovered check
        result = sf.gives_check("janggi", "4ka3/4a4/9/4R4/2B6/9/9/5K3/4p4/3r5 b - - 0 113", ["e2f2"])
        self.assertTrue(result)

    def test_is_capture(self):
        result = sf.is_capture("chess", CHESS, [], "e2e4")
        self.assertFalse(result)

        result = sf.is_capture("chess", CHESS, ["e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5"], "e1g1")
        self.assertFalse(result)

        result = sf.is_capture("chess", CHESS, ["e2e4", "g8f6", "e4e5", "d7d5"], "e5f6")
        self.assertTrue(result)

        # en passant
        result = sf.is_capture("chess", CHESS, ["e2e4", "g8f6", "e4e5", "d7d5"], "e5d6")
        self.assertTrue(result)

        # 960 castling
        result = sf.is_capture("chess", "bqrbkrnn/pppppppp/8/8/8/8/PPPPPPPP/BQRBKRNN w CFcf - 0 1", ["g1f3", "h8g6"], "e1f1", True)
        self.assertFalse(result)

        # Sittuyin in-place promotion
        result = sf.is_capture("sittuyin", "8/2k5/8/4P3/4P1N1/5K2/8/8[] w - - 0 1", [], "e5e5f")
        self.assertFalse(result)

    def test_piece_to_partner(self):
        # take the rook and promote to queen
        result = sf.piece_to_partner("bughouse", "r2qkbnr/1Ppppppp/2n5/8/8/8/1PPPPPPP/RNBQKBNR[] w KQkq - 0 1", ["b7a8q"])
        self.assertEqual(result, "r")

        # take back the queen (promoted pawn)
        result = sf.piece_to_partner("bughouse", "r2qkbnr/1Ppppppp/2n5/8/8/8/1PPPPPPP/RNBQKBNR[] w KQkq - 0 1", ["b7a8q", "d8a8"])
        self.assertEqual(result, "P")

        # just a simple move (no take)
        result = sf.piece_to_partner("bughouse", "r2qkbnr/1Ppppppp/2n5/8/8/8/1PPPPPPP/RNBQKBNR[] w KQkq - 0 1", ["b7a8q", "d8b8"])
        self.assertEqual(result, "")

        # silver takes the pawn and promotes to gold
        result = sf.piece_to_partner("shogi", "lnsgkgsnl/1r5b1/ppppppppp/S8/9/9/PPPPPPPPP/1B5R1/LNSGKG1NL[] w 0 1", ["a6a7+"])
        self.assertEqual(result, "p")

        # take back the gold (promoted silver)
        result = sf.piece_to_partner("shogi", "lnsgkgsnl/1r5b1/ppppppppp/S8/9/9/PPPPPPPPP/1B5R1/LNSGKG1NL[] w 0 1", ["a6a7+", "a9a7"])
        self.assertEqual(result, "S")

    def test_game_result(self):
        result = sf.game_result("chess", CHESS, ["f2f3", "e7e5", "g2g4", "d8h4"])
        self.assertEqual(result, -sf.VALUE_MATE)

        # shogi pawn drop mate
        result = sf.game_result("shogi", "lnsg3nk/1r2b1gs1/ppppppp1p/7N1/7p1/9/PPPPPPPP1/1B5R1/LNSGKGS1L[P] w 0 1", ["P@i8"])
        self.assertEqual(result, sf.VALUE_MATE)

        # losers checkmate
        result = sf.game_result("losers", CHESS, ["f2f3", "e7e5", "g2g4", "d8h4"])
        self.assertEqual(result, sf.VALUE_MATE)

        # suicide stalemate
        result = sf.game_result("suicide", "8/8/8/7p/7P/8/8/8 w - - 0 1", [])
        self.assertEqual(result, sf.VALUE_DRAW)
        result = sf.game_result("suicide", "8/8/8/7p/7P/7P/8/8 w - - 0 1", [])
        self.assertEqual(result, -sf.VALUE_MATE)
        result = sf.game_result("suicide", "8/8/8/7p/7P/8/8/n7 w - - 0 1", [])
        self.assertEqual(result, sf.VALUE_MATE)

        # atomic check- and stalemate
        # checkmate
        result = sf.game_result("atomic", "BQ6/Rk6/8/8/8/8/8/4K3 b - - 0 1", [])
        self.assertEqual(result, -sf.VALUE_MATE)
        # stalemate
        result = sf.game_result("atomic", "KQ6/Rk6/2B5/8/8/8/8/8 b - - 0 1", [])
        self.assertEqual(result, sf.VALUE_DRAW)

        # royalduck checkmate and stalemate
        result = sf.game_result("royalduck", "r1bqkbnr/pp1*p1p1/n2p1pQp/1Bp5/8/2N1PN2/PPPP1PPP/R1B1K2R b KQkq - 1 6", [])
        self.assertEqual(result, -sf.VALUE_MATE)
        result = sf.game_result("royalduck", "rnbqk1nr/pppp1ppp/4p3/8/7P/5Pb1/PPPPP*P1/RNBQKBNR w KQkq - 1 4", [])
        self.assertEqual(result, sf.VALUE_MATE)

    def _check_immediate_game_end(self, variant, fen, moves, game_end, game_result=None):
        with self.subTest(variant=variant, fen=fen, game_end=game_end, game_result=game_result):
            result = sf.is_immediate_game_end(variant, fen, moves)
            self.assertEqual(result[0], game_end)
            if game_result is not None:
                self.assertEqual(result[1], game_result)

    def test_is_immediate_game_end(self):
        self._check_immediate_game_end("capablanca", CAPA, [], False)

        # bikjang (facing kings)
        moves = "e2e3 e9f9 h3d3 e7f7 i1i3 h10i8 i3h3 c10e7 h3h8 i10i9 h8b8 i9g9 d3f3 f9e9 f3f10 e7c10 f10c10 b10c8 c10g10 g9f9 b8c8 a10b10 b3f3 f9h9 a1a2 h9f9 a2d2 b10b9 d2d10 e9d10 c8c10 d10d9 f3f9 i8g9 f9b9 a7a6 g10g7 f7f6 e4e5 c7d7 g1e4 i7i6 e4b6 d9d8 c10c8 d8d9 b9g9 d7d6 b6e8 i6h6 e5e6 f6e6 c1e4 a6b6 e4b6 d6d5 c4c5 d9d10 e3d3 h6i6 c5c6 d5c5"
        self._check_immediate_game_end("janggi", JANGGI, moves.split(), False)

        moves = "e2e3 e9f9 h3d3 e7f7 i1i3 h10i8 i3h3 c10e7 h3h8 i10i9 h8b8 i9g9 d3f3 f9e9 f3f10 e7c10 f10c10 b10c8 c10g10 g9f9 b8c8 a10b10 b3f3 f9h9 a1a2 h9f9 a2d2 b10b9 d2d10 e9d10 c8c10 d10d9 f3f9 i8g9 f9b9 a7a6 g10g7 f7f6 e4e5 c7d7 g1e4 i7i6 e4b6 d9d8 c10c8 d8d9 b9g9 d7d6 b6e8 i6h6 e5e6 f6e6 c1e4 a6b6 e4b6 d6d5 c4c5 d9d10 e3d3 h6i6 c5c6 d5c5 d3d3"
        self._check_immediate_game_end("janggi", JANGGI, moves.split(), True, -sf.VALUE_MATE)

        # full board adjudication
        self._check_immediate_game_end("flipello", "pppppppp/pppppppp/pppPpppp/pPpPpppp/pppppppp/pPpPPPPP/ppPpPPpp/pppppppp[PPpp] b - - 63 32", [], True, sf.VALUE_MATE)
        self._check_immediate_game_end("ataxx", "PPPpppp/pppPPPp/pPPPPPP/PPPPPPp/ppPPPpp/pPPPPpP/pPPPPPP b - - 99 50", [], True, -sf.VALUE_MATE)
        self._check_immediate_game_end("ataxx", "PPPpppp/pppPPPp/pPP*PPP/PP*P*Pp/ppP*Ppp/pPPPPpP/pPPPPPP b - - 99 50", [], True, -sf.VALUE_MATE)

    def _check_optional_game_end(self, variant, fen, moves, game_end, game_result=None):
        with self.subTest(variant=variant, fen=fen, game_end=game_end, game_result=game_result):
            result = sf.is_optional_game_end(variant, fen, moves)
            self.assertEqual(result[0], game_end)
            if game_result is not None:
                self.assertEqual(result[1], game_result)

    def test_is_optional_game_end(self):
        self._check_optional_game_end("capablanca", CAPA, [], False)

        # sittuyin stalemate due to optional promotion
        self._check_optional_game_end("sittuyin", "1k4PK/3r4/8/8/8/8/8/8[] w - - 0 1", [], True, sf.VALUE_DRAW)

        # Xiangqi chasing rules
        # Also see http://www.asianxiangqi.org/English/AXF_rules_Eng.pdf
        # Direct chase by cannon
        self._check_optional_game_end("xiangqi", "2bakabnr/9/r1n1c4/2p1p1p1p/PP7/9/4P1P1P/2C3NC1/9/1NBAKAB1R w - - 0 1", ["c3a3", "a8b8", "a3b3", "b8a8", "b3a3", "a8b8", "a3b3", "b8a8", "b3a3"], True, sf.VALUE_MATE)
        # Chase with chasing side to move
        self._check_optional_game_end("xiangqi", "2bakabnr/9/r1n1c4/2p1p1p1p/PP7/9/4P1P1P/2C3NC1/9/1NBAKAB1R w - - 0 1", ["c3a3", "a8b8", "a3b3", "b8a8", "b3a3", "a8b8", "a3b3", "b8a8", "b3a3", "a8b8", "a3b3", "b8a8"], True, -sf.VALUE_MATE)
        # Discovered chase by cannon (including pawn capture)
        self._check_optional_game_end("xiangqi", "2bakabr1/9/9/r1p1p1p2/p7R/P8/9/9/9/CC1AKA3 w - - 0 1", ["a5a6", "a7b7", "a6b6", "b7a7", "b6a6", "a7b7", "a6b6", "b7a7", "b6a6"], True, sf.VALUE_MATE)
        # Chase by soldier (draw)
        self._check_optional_game_end("xiangqi", "2bakabr1/9/9/r1p1p1p2/p7R/P8/9/9/9/1C1AKA3 w - - 0 1", ["a5a6", "a7b7", "a6b6", "b7a7", "b6a6", "a7b7", "a6b6", "b7a7", "b6a6"], True, sf.VALUE_DRAW)
        # Discovered and anti-discovered chase by cannon
        self._check_optional_game_end("xiangqi", "5k3/9/9/5C3/5c3/5C3/9/9/5p3/4K4 w - - 0 1", ["f5d5", "f6d6", "d5f5", "d6f6", "f5d5", "f6d6", "d5f5", "d6f6"], True, -sf.VALUE_MATE)
        # Mutual chase (draw)
        self._check_optional_game_end("xiangqi", "4k4/7n1/9/4pR3/9/9/4P4/9/9/4K4 w - - 0 1", ["f7h7"] + 2 * ["h9f8", "h7h8", "f8g6", "h8g8", "g6i7", "g8g7", "i7h9", "g7h7"], True, sf.VALUE_DRAW)
        # Perpetual check vs. intermittent checks
        self._check_optional_game_end("xiangqi", "9/3kc4/3a5/3P5/9/4p4/9/4K4/9/3C5 w - - 0 1", 2 * ['d7e7', 'e5d5', 'e7d7', 'd5e5'], True, sf.VALUE_MATE)
        # Perpetual check by soldier
        self._check_optional_game_end("xiangqi", "3k5/9/9/9/9/5p3/9/5p3/5K3/5C3 w - - 0 1", 2 * ['f2e2', 'f3e3', 'e2f2', 'e3f3'], True, sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "3k5/4P4/4b4/3C5/4c4/9/9/9/9/5K3 w - - 0 1", 2 * ['d7e7', 'e8g6', 'e7d7', 'g6e8'], True, sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "3k5/9/9/9/9/9/9/9/cr1CAK3/9 w - - 0 1", 2 * ['d2d4', 'b2b4', 'd4d2', 'b4b2'], True, sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "5k3/9/9/5C3/5c3/5C3/9/9/5p3/4K4 w - - 0 1", 2 * ['f5d5', 'f6d6', 'd5f5', 'd6f6'], True, -sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "4k4/9/4b4/2c2nR2/9/9/9/9/9/3K5 w - - 0 1", 2 * ['g7g6', 'f7g9', 'g6g7', 'g9f7'], True, sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "3P5/3k5/3nn4/9/9/9/9/9/9/5K3 w - - 0 1", 2 * ['d10e10', 'd9e9', 'e10d10', 'e9d9'], True, sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "4ck3/9/9/9/9/2r1R4/9/9/4A4/3AK4 w - - 0 1", 2 * ['e5e4', 'c5c4', 'e4e5', 'c4c5'], True, sf.VALUE_MATE)
        self._check_optional_game_end("xiangqi", "4k4/9/9/c1c6/9/r8/9/9/C8/3K5 w - - 0 1", 2 * ['a2c2', 'a5c5', 'c2a2', 'c5a5'], True, sf.VALUE_MATE)
        # Mutual perpetual check
        self._check_optional_game_end("xiangqi", "9/4c4/3k5/3r5/9/9/4C4/9/4K4/3R5 w - - 0 1", 2 * ['e4d4', 'd7e7', 'd4e4', 'e7d7'], True, sf.VALUE_DRAW)
        self._check_optional_game_end("xiangqi", "3k5/6c2/9/7P1/6c2/6P2/9/9/9/5K3 w - - 0 1", 2 * ['h7g7', 'g6h6', 'g7h7', 'h6g6'], True, sf.VALUE_DRAW)
        self._check_optional_game_end("xiangqi", "4ck3/9/9/9/9/2r1R1N2/6N2/9/4A4/3AK4 w - - 0 1", 2 * ['e5e4', 'c5c4', 'e4e5', 'c4c5'], True, sf.VALUE_DRAW)
        self._check_optional_game_end("xiangqi", "5k3/9/9/c8/9/P1P6/9/2C6/9/3K5 w - - 0 1", 2 * ['c3a3', 'a7c7', 'a3c3', 'c7a7'], True, sf.VALUE_DRAW)
        self._check_optional_game_end("xiangqi", "4k4/9/r1r6/9/PPPP5/9/9/9/1C7/5K3 w - - 0 1", ['b2a2'] + 2 * ['a8b8', 'a2c2', 'c8d8', 'c2b2', 'b8a8', 'b2d2', 'd8c8', 'd2a2'], True, sf.VALUE_DRAW)

        # Corner cases
        # D106: Chariot chases cannon, but attack actually does not change (draw)
        self._check_optional_game_end("xiangqi", "3k2b2/4P4/4b4/9/8p/6Bc1/6P1P/3AB4/4pp3/1p1K3R1[] w - - 0 1", 2 * ["h1h2", "h5h4", "h2h1", "h4h5"], True, sf.VALUE_DRAW)
        # D39: Chased chariot pinned by horse + mutual chase (controversial if pinned chariot chases)
        self._check_optional_game_end("xiangqi", "2baka1r1/C4rN2/9/1Rp1p4/9/9/4P4/9/4A4/4KA3 w - - 0 1", ["b7b9"] + 2 * ["f10e9", "b9b10", "e9f10", "b10b9"], True, sf.VALUE_MATE)
        # D39: Chased chariot pinned by horse + mutual chase (controversial if pinned chariot chases)
        self._check_optional_game_end("xiangqi", "5k3/9/9/9/9/9/7r1/9/2nRA3c/4K4 w - - 0 1", 2 * ['e2f1', 'h4h2', 'f1e2', 'h2h4'], True, sf.VALUE_MATE)
        # Creating pins to undermine root
        self._check_optional_game_end("xiangqi", "4k4/4c4/9/4p4/9/9/3rn4/3NR4/4K4/9 b - - 0 1", 2 * ['e4g5', 'e2f2', 'g5e4', 'f2e2'], True, -sf.VALUE_MATE)
        # Discovered check capture threat by rook
        self._check_optional_game_end("xiangqi", "5k3/9/9/9/9/1N2P1C2/9/4BC3/9/cr1RK4 w - - 0 1", 2 * ['b5c3', 'b1c1', 'c3b5', 'c1b1'], True, sf.VALUE_MATE)
        # Creating a pin to undermine root + discovered check threat by horse
        self._check_optional_game_end("xiangqi", "5k3/9/9/9/9/4c4/3n5/3NBA3/4A4/4K4 w - - 0 1", 2 * ['e1d1', 'e5d5', 'd1e1', 'd5e5'], True, sf.VALUE_MATE)
        # Creating a pin to undermine root + discovered check threat by rook
        self._check_optional_game_end("xiangqi", "5k3/9/9/9/9/4c4/3r5/3NB4/4A4/4K4 w - - 0 1", 2 * ['e1d1', 'e5d5', 'd1e1', 'd5e5'], True, sf.VALUE_MATE)
        # X-Ray protected discovered check
        self._check_optional_game_end("xiangqi", "5k3/9/9/9/9/9/9/9/9/3NK1cr1 w - - 0 1", 2 * ['d1c3', 'h1h3', 'c3d1', 'h3h1'], True, sf.VALUE_MATE)
        # No overprotection by king
        self._check_optional_game_end("xiangqi", "3k5/9/9/3n5/9/9/3r5/9/9/3NK4 w - - 0 1", 2 * ['d1c3', 'd4c4', 'c3d1', 'c4d4'], True, sf.VALUE_DRAW)
        # Overprotection by king
        self._check_optional_game_end("xiangqi", "3k5/9/9/9/9/9/3r5/9/9/3NK4 w - - 0 1", 2 * ['d1c3', 'd4c4', 'c3d1', 'c4d4'], True, sf.VALUE_MATE)
        # Mutual pins by flying generals
        self._check_optional_game_end("xiangqi", "4k4/9/9/9/4n4/9/5C3/9/4N4/4K4 w - - 0 1", 2 * ['e2g1', 'e10f10', 'g1e2', 'f10e10'], True) #, sf.VALUE_MATE)
        # Fake protection by cannon
        self._check_optional_game_end("xiangqi", "5k3/9/9/9/9/1C7/1r7/9/1C7/4K4 w - - 0 1", 2 * ['b5c5', 'b4c4', 'c5b5', 'c4b4'], True, sf.VALUE_MATE)
        # Fake protection by cannon + mutual chase
        self._check_optional_game_end("xiangqi", "4ka3/c2R1R2c/4b4/9/9/9/9/9/9/4K4 w - - 0 1", 2 * ['f9f7', 'f10e9', 'f7f9', 'e9f10'], True, sf.VALUE_DRAW)

    def test_has_insufficient_material(self):
        for variant, positions in variant_positions.items():
            for fen, expected_result in positions.items():
                with self.subTest(variant=variant, fen=fen):
                    result = sf.has_insufficient_material(variant, fen, [])
                    self.assertEqual(result, expected_result)

    def test_validate_fen(self):
        # valid
        for variant, positions in variant_positions.items():
            for fen in positions:
                with self.subTest(variant=variant, fen=fen):
                    self.assertEqual(sf.validate_fen(fen, variant), sf.FEN_OK)
        # invalid
        for variant, positions in invalid_variant_positions.items():
            for fen in positions:
                with self.subTest(variant=variant, fen=fen):
                    self.assertNotEqual(sf.validate_fen(fen, variant), sf.FEN_OK)
        # chess960
        self.assertEqual(sf.validate_fen(CHESS960, "chess", True), sf.FEN_OK)
        self.assertEqual(sf.validate_fen("nrbqbkrn/pppppppp/8/8/8/8/PPPPPPPP/NRBQBKRN w BGbg - 0 1", "newzealand", True), sf.FEN_OK, "{}: {}".format(variant, fen))
        # all variants starting positions
        for variant in sf.variants():
            with self.subTest(variant=variant):
                fen = sf.start_fen(variant)
                self.assertEqual(sf.validate_fen(fen, variant), sf.FEN_OK)

    def test_validate_fen_promoted_pieces(self):
        # Test promoted piece validation specifically
        
        # Valid promoted pieces should pass
        valid_promoted_fens = {
            "shogi": [
                "lnsgkgsnl/1r5b1/pppppp+ppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1",  # promoted pawn
                "lnsgkgsnl/1r5+b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1",  # promoted bishop
                "lnsgkgsnl/1+r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1",  # promoted rook
                "ln+sgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1",  # promoted silver
                "l+nsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1",  # promoted knight
                "+lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1",  # promoted lance
            ]
        }
        
        # Invalid promoted pieces should fail with FEN_INVALID_PROMOTED_PIECE (-12)
        invalid_promoted_fens = {
            "kyotoshogi": [
                "p+nks+l/5/5/5/+LS+K+NP[-] w 0 1",  # promoted king (+K) - kings cannot be promoted
            ],
            "shogi": [
                "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSG++KGSNL[-] w - - 0 1",  # double promotion (++K)
            ]
        }
        
        # Non-shogi variants should ignore promoted piece syntax ('+' should be invalid character)
        non_shogi_promoted_fens = {
            "chess": [
                "rnb+qkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  # '+' not valid in chess
            ]
        }
        
        # Test valid promoted pieces
        for variant, fens in valid_promoted_fens.items():
            for fen in fens:
                with self.subTest(variant=variant, fen=fen, test_type="valid_promoted"):
                    result = sf.validate_fen(fen, variant)
                    self.assertEqual(result, sf.FEN_OK, f"Expected valid promoted piece FEN to pass: {fen}")
        
        # Test invalid promoted pieces (should return FEN_INVALID_PROMOTED_PIECE = -12)
        for variant, fens in invalid_promoted_fens.items():
            for fen in fens:
                with self.subTest(variant=variant, fen=fen, test_type="invalid_promoted"):
                    result = sf.validate_fen(fen, variant)
                    self.assertEqual(result, sf.FEN_INVALID_PROMOTED_PIECE, 
                                   f"Expected invalid promoted piece FEN to return -12: {fen}, got {result}")
        
        # Test non-shogi variants (should fail with character validation, not promoted piece validation)
        for variant, fens in non_shogi_promoted_fens.items():
            for fen in fens:
                with self.subTest(variant=variant, fen=fen, test_type="non_shogi"):
                    result = sf.validate_fen(fen, variant)
                    # Should fail with character validation (FEN_INVALID_CHAR = -10), not promoted piece validation
                    self.assertEqual(result, -10, 
                                   f"Expected non-shogi variant to fail with character error (-10): {fen}, got {result}")

    def test_get_fog_fen(self):
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"  # startpos
        result = sf.get_fog_fen(fen, "fogofwar")
        self.assertEqual(result, "********/********/********/********/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

        fen = "rnbqkbnr/p1p2ppp/8/Pp1pp3/4P3/8/1PPP1PPP/RNBQKBNR w KQkq b6 0 1"
        result = sf.get_fog_fen(fen, "fogofwar")
        self.assertEqual(result, "********/********/2******/Pp*p***1/4P3/4*3/1PPP1PPP/RNBQKBNR w KQkq b6 0 1")
        

if __name__ == '__main__':
    unittest.main(verbosity=2)
