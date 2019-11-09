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
SHOGI = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] b 0 1"
SEIRAWAN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1"
GRAND = "r8r/1nbqkcabn1/pppppppppp/10/10/10/10/PPPPPPPPPP/1NBQKCABN1/R8R w - - 0 1"
XIANGQI = "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1"

variant_positions = {
    "chess": {
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
    },
    "seirawan": {
        "k7/8/8/8/8/8/8/K7[] w - - 0 1": (True, True),  # K vs K
        "k7/8/8/8/8/8/8/KH6[] w - - 0 1": (False, True),  # KH vs K
        "k7/8/8/8/8/8/8/4K3[E] w E - 0 1": (False, True),  # KE vs K
    },
    "sittuyin": {
        "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1": (False, False),  # starting position
        "k7/8/8/8/8/8/8/K7[] w - - 0 1": (True, True),  # K vs K
        "k6P/8/8/8/8/8/8/K7[] w - - 0 1": (True, True),  # KP vs K
        "k6P/8/8/8/8/8/8/K6p[] w - - 0 1": (False, False),  # KP vs KP
        "k7/8/8/8/8/8/8/KFF5[] w - - 0 1": (False, True),  # KFF vs K
        "k7/8/8/8/8/8/8/KS6[] w - - 0 1": (False, True),  # KS vs K
    },
    "xiangqi": {
        "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1": (False, False),  # starting position
        "5k3/4a4/3CN4/9/1PP5p/9/8P/4C4/4A4/2B1K4 w - - 0 46": (False, False),  # issue #53
        "4k4/9/9/9/9/9/9/9/9/4K4 w - - 0 1": (True, True),  # K vs K
        "4k4/9/9/4p4/9/9/9/9/9/4KR3 w - - 0 1": (False, False),  # KR vs KP
        "4k4/9/9/9/9/9/9/9/9/3KN4 w - - 0 1": (False, True),  # KN vs K
        "4k4/9/4b4/9/9/9/9/4B4/9/4K4 w - - 0 1": (True, True),  # KB vs KB
        "4k4/9/9/9/9/9/9/9/4A4/4KC3 w - - 0 1": (False, True),  # KCA vs K
    },
}


class TestPyffish(unittest.TestCase):
    def test_info(self):
        result = sf.info()
        self.assertEqual(result[:15], "Fairy-Stockfish")

    def test_set_option(self):
        result = sf.set_option("UCI_Variant", "capablanca")
        self.assertIsNone(result)

    def test_start_fen(self):
        result = sf.start_fen("capablanca")
        self.assertEqual(result, CAPA)

        result = sf.start_fen("capahouse")
        self.assertEqual(result, CAPAHOUSE)

        result = sf.start_fen("xiangqi")
        self.assertEqual(result, XIANGQI)

    def test_legal_moves(self):
        fen = "10/10/10/10/10/k9/10/K9 w - - 0 1"
        result = sf.legal_moves("capablanca", fen, [])
        self.assertEqual(result, ["a1b1"])

        result = sf.legal_moves("grand", GRAND, ["a3a5"])
        self.assertIn("a10b10", result)

        result = sf.legal_moves("xiangqi", XIANGQI, ["h3h10"])
        self.assertIn("i10h10", result)

    def test_short_castling(self):
        legals = ['f5f4', 'a7a6', 'b7b6', 'c7c6', 'd7d6', 'e7e6', 'i7i6', 'j7j6', 'a7a5', 'b7b5', 'c7c5', 'e7e5', 'i7i5', 'j7j5', 'b8a6', 'b8c6', 'h6g4', 'h6i4', 'h6j5', 'h6f7', 'h6g8', 'h6i8', 'd5a2', 'd5b3', 'd5f3', 'd5c4', 'd5e4', 'd5c6', 'd5e6', 'd5f7', 'd5g8', 'j8g8', 'j8h8', 'j8i8', 'e8f7', 'c8b6', 'c8d6', 'g6g2', 'g6g3', 'g6f4', 'g6g4', 'g6h4', 'g6e5', 'g6g5', 'g6i5', 'g6a6', 'g6b6', 'g6c6', 'g6d6', 'g6e6', 'g6f6', 'g6h8', 'f8f7', 'f8g8', 'f8i8']
        moves = ['b2b4', 'f7f5', 'c2c3', 'g8d5', 'a2a4', 'h8g6', 'f2f3', 'i8h6', 'h2h3']
        result = sf.legal_moves("capablanca", CAPA, moves)
        self.assertEqual(legals, result)
        self.assertIn("f8i8", result)

        moves = ['a2a4', 'f7f5', 'b2b3', 'g8d5', 'b1a3', 'i8h6', 'c1a2', 'h8g6', 'c2c4']
        result = sf.legal_moves("capablanca", CAPA, moves)
        self.assertIn("f8i8", result)

        moves = ['f2f4', 'g7g6', 'g1d4', 'j7j6', 'h1g3', 'b8a6', 'i1h3', 'h7h6']
        result = sf.legal_moves("capablanca", CAPA, moves)
        self.assertIn("f1i1", result)

    def test_get_fen(self):
        result = sf.get_fen("chess", CHESS, [])
        self.assertEqual(result, CHESS)

        result = sf.get_fen("capablanca", CAPA, [])
        self.assertEqual(result, CAPA)

        result = sf.get_fen("xiangqi", XIANGQI, [])
        self.assertEqual(result, XIANGQI)

        fen = "rnab1kbcnr/ppppPppppp/10/4q5/10/10/PPPPP1PPPP/RNABQKBCNR[p] b KQkq - 0 3"
        result = sf.get_fen("capahouse", CAPA, ["f2f4", "e7e5", "f4e5", "e8e5", "P@e7"])
        self.assertEqual(result, fen)

        fen0 = "reb1k2r/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] b KQkqac - 2 6"
        fen1 = "reb2rk1/ppppqppp/2nb1n2/4p3/4P3/N1P2N2/PB1PQPPP/RE2KBHR[h] w KQac - 3 7"
        result = sf.get_fen("seirawan", fen0, ["e8g8"])
        self.assertEqual(result, fen1)

        result = sf.get_fen("chess", CHESS, [], True)
        self.assertEqual(result, CHESS960)

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

        result = sf.get_san("capablanca", CAPA, "e2e4")
        self.assertEqual(result, "e4")

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

        result = sf.get_san("shogi", SHOGI, "1g1f")
        self.assertEqual(result, "P-1f")

        result = sf.get_san("shogi", SHOGI, "4i5h")
        self.assertEqual(result, "G4i-5h")

        fen = "lnsgkgsnl/1r5b1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL b -"
        result = sf.get_san("shogi", fen, "8h2b")
        self.assertEqual(result, "Bx2b=")
        result = sf.get_san("shogi", fen, "8h2b+")
        self.assertEqual(result, "Bx2b+")

        fen = "lnsgkg1nl/1r5s1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/7R1/LNSGKGSNL b Bb"
        result = sf.get_san("shogi", fen, "B*3c")
        self.assertEqual(result, "B*3c")

        fen = "lnsgkg1nl/1r4s+B1/pppppp1pp/6p2/9/2P6/PP1PPPPPP/7R1/LNSGKGSNL b B"
        result = sf.get_san("shogi", fen, "2b3c")
        self.assertEqual(result, "+B-3c")

        fen = "lnk2gsnl/7b1/p1p+SGp1pp/6p2/1pP6/4P4/PP3PPPP/1S2G2R1/L2GK1bNL b PRppns"
        result = sf.get_san("shogi", fen, "6c6b")
        self.assertEqual(result, "+S-6b")

        result = sf.get_san("xiangqi", XIANGQI, "h1g3")
        self.assertEqual(result, "Hg3")

        result = sf.get_san("xiangqi", XIANGQI, "c1e3")
        self.assertEqual(result, "Ece3")

        fen = "rnsm1s1r/4n1k1/1ppppppp/p7/2PPP3/PP3PPP/4N2R/RNSKMS2 b - - 1 5"
        result = sf.get_san("makruk", fen, "f8f7")
        self.assertEqual(result, "Sf7")

        fen = "4k3/8/8/4S3/8/2S5/8/4K3 w - - 0 1"
        result = sf.get_san("makruk", fen, "e5d4")
        self.assertEqual(result, "Sed4")

        result = sf.get_san("makruk", fen, "c3d4")
        self.assertEqual(result, "Scd4")

        fen = "4k3/8/8/3S4/8/3S4/8/4K3 w - - 0 1"
        result = sf.get_san("makruk", fen, "d3d4")
        self.assertEqual(result, "Sd4")

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

    def test_get_san_moves(self):
        UCI_moves = ["e2e4", "e7e5", "g1f3", "b8c6h", "f1c4", "f8c5e"]
        SAN_moves = ["e4", "e5", "Nf3", "Nc6/H", "Bc4", "Bc5/E"]

        result = sf.get_san_moves("seirawan", SEIRAWAN, UCI_moves)
        self.assertEqual(result, SAN_moves)

    def test_gives_check(self):
        self.assertRaises(ValueError, sf.gives_check, "makruk", MAKRUK, [])
        self.assertRaises(ValueError, sf.gives_check, "capablanca", CAPA, [])

        result = sf.gives_check("capablanca", CAPA, ["e2e4"])
        self.assertFalse(result)

        moves = ["g2g3", "d7d5", "a2a3", "c8h3"]
        result = sf.gives_check("capablanca", CAPA, moves)
        self.assertTrue(result)

    def test_is_immediate_game_end(self):
        result = sf.is_immediate_game_end("capablanca", CAPA, [])
        self.assertNotEqual(result, 0)

    def test_is_optional_game_end(self):
        result = sf.is_optional_game_end("capablanca", CAPA, [])
        self.assertNotEqual(result, 0)

    def test_has_insufficient_material(self):
        for variant, positions in variant_positions.items():
            for fen, expected_result in positions.items():
                result = sf.has_insufficient_material(variant, fen, [])
                self.assertEqual(result, expected_result, "{}: {}".format(variant, fen))


if __name__ == '__main__':
    unittest.main(verbosity=2)
