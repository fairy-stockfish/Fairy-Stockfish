# -*- coding: utf-8 -*-

import unittest
import pyffish as sf

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
    def test_has_insufficient_material(self):
        for fen in standard:
            print(fen, standard[fen])
            result = sf.has_insufficient_material("standard", fen, [])
            self.assertEqual(result, standard[fen])


if __name__ == '__main__':
    unittest.main()
