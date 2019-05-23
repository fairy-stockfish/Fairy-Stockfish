# -*- coding: utf-8 -*-

import unittest
import pyffish as sf

CAPA = "rnabqkbcnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNABQKBCNR w KQkq - 0 1"

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

    def test_get_fen(self):
        result = sf.get_fen("capablanca", CAPA, [])
        self.assertEqual(result, CAPA)

    def test_get_san(self):
        result = sf.get_san("capablanca", CAPA, "e2e4")
        self.assertEqual(result, "e4")

        result = sf.get_san("capablanca", CAPA, "h1i3")
        self.assertEqual(result, "Ci3")

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
