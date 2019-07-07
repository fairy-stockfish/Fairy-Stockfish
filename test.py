# -*- coding: utf-8 -*-

import unittest
import pyffish as sf

CAPA = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq - 0 1"
SITTUYIN = "8/8/4pppp/pppp4/4PPPP/PPPP4/8/8[KFRRSSNNkfrrssnn] w - - 0 1"
MAKRUK = "rnsmksnr/8/pppppppp/8/8/PPPPPPPP/8/RNSKMSNR w - - 0 1"
SHOGI = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] b 0 1"

standard = {
    "k7/8/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs K
    "k7/b7/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs KB
    "k7/n7/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs KN
    "k7/p7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KP
    "k7/r7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KR
    "k7/q7/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KQ
    "k7/nn6/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vsNN K
    "k7/bb6/8/8/8/8/8/K7 w - - 0 1": (True, False),  # K vs KBB opp color
    "k7/b1b5/8/8/8/8/8/K7 w - - 0 1": (True, True),  # K vs KBB same color
    # TODO: implement more lichess/python-chess adjudication rule
    #    "kb6/8/8/8/8/8/8/K1B6 w - - 0 1": (True, True),  # KB vs KB same color
    #    "kb6/8/8/8/8/8/8/KB7 w - - 0 1": (False, False),  # KB vs KB opp color
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

    def test_legal_moves(self):
        fen = "10/10/10/10/10/k9/10/K9 w - - 0 1"
        result = sf.legal_moves("capablanca", fen, [])
        self.assertEqual(result, ["a1b1"])

    def test_short_castling(self):
        for i in range(10000):
            print(i)
            moves = ['a2a4', 'f7f5', 'b2b3', 'g8d5', 'b1a3', 'i8h6', 'c1a2', 'h8g6', 'c2c4']
            result = sf.legal_moves("capablanca", CAPA, moves)
            self.assertIn("f8i8", result)

            moves = ['f2f4', 'g7g6', 'g1d4', 'j7j6', 'h1g3', 'b8a6', 'i1h3', 'h7h6']
            result = sf.legal_moves("capablanca", CAPA, moves)
            self.assertIn("f1i1", result)

    def test_get_fen(self):
        result = sf.get_fen("capablanca", CAPA, [])
        self.assertEqual(result, CAPA)

    def test_get_san(self):
        fen = "4k3/8/3R4/8/1R3R2/8/3R4/4K3 w - - 0 1"
        result = sf.get_san("standard", fen, "b4d4")
        self.assertEqual(result, "Rbd4")

        result = sf.get_san("standard", fen, "f4d4")
        self.assertEqual(result, "Rfd4")

        result = sf.get_san("standard", fen, "d2d4")
        self.assertEqual(result, "R2d4")

        result = sf.get_san("standard", fen, "d6d4")
        self.assertEqual(result, "R6d4")

        fen = "4k3/8/3R4/3P4/1RP1PR2/8/3R4/4K3 w - - 0 1"
        result = sf.get_san("standard", fen, "d2d4")
        self.assertEqual(result, "Rd4")

        result = sf.get_san("capablanca", CAPA, "e2e4")
        self.assertEqual(result, "e4")

        result = sf.get_san("capablanca", CAPA, "h1i3")
        self.assertEqual(result, "Ci3")

        result = sf.get_san("sittuyin", SITTUYIN, "R@a1")
        self.assertEqual(result, "R@a1")

        result = sf.get_san("shogi", SHOGI, "1g1f")
        self.assertEqual(result, "P1f")

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

    def test_gives_check(self):
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
        for fen in standard:
            # print(fen, standard[fen])
            result = sf.has_insufficient_material("standard", fen, [])
            self.assertEqual(result, standard[fen])


if __name__ == '__main__':
    unittest.main()
