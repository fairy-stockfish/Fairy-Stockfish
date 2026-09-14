import re
import unittest

import pyffish as sf


class SparkChessTest(unittest.TestCase):
    variant = "sparkchess"

    def moves(self, fen):
        moves = sf.legal_moves(self.variant, fen, [])
        self.assertEqual(len(moves), len(set(moves)))
        return set(moves)

    def test_start(self):
        fen = sf.start_fen(self.variant)
        expected = {f + "2" + f + "3" for f in "abcdefgh"}
        expected.update({"b1a3", "b1c3", "g1f3", "g1h3", "b1d2", "g1e2"})
        self.assertEqual(self.moves(fen), expected)

    def test_shift_and_enemy_blocker(self):
        clear = "4k3/8/8/8/8/8/R2P4/R3K3 w - - 0 1"
        blocked = "4k3/8/8/8/p7/8/R2P4/R3K3 w - - 0 1"
        self.assertEqual({m for m in self.moves(clear) if m.startswith("d2a")},
                         {"d2a3", "d2a4", "d2a5", "d2a6", "d2a7"})
        self.assertEqual({m for m in self.moves(blocked) if m.startswith("d2a")},
                         {"d2a3"})

    def test_single_piece_is_not_two_influences(self):
        fen = "4k3/8/8/8/8/8/3P4/Q3K3 w - - 0 1"
        self.assertNotIn("d2c3", self.moves(fen))

    def test_direct_swap(self):
        fen = "4k3/8/8/8/8/1N6/3P4/4K3 w - - 0 1"
        self.assertIn("b3d2", self.moves(fen))
        self.assertEqual(sf.get_san(self.variant, fen, "b3d2"), "Nb3>d2")
        self.assertEqual(sf.get_fen(self.variant, fen, ["b3d2"]),
                         "4k3/8/8/8/8/1P6/3N4/4K3 b - - 1 1")

    def test_relay_and_multiple_witnesses(self):
        # Both c3 and d4 attack b5 and e2. The exchange is still one move.
        fen = "7k/8/8/1P6/3N4/2N5/4R3/4K3 w - - 0 1"
        self.assertIn("e2b5", self.moves(fen))
        self.assertEqual(sf.get_san(self.variant, fen, "e2b5"), "Re2>b5")
        self.assertEqual(sf.get_fen(self.variant, fen, ["e2b5"]).split()[0],
                         "7k/8/8/1R6/3N4/2N5/4P3/4K3")

    def test_attacked_king_relay(self):
        safe = "4k3/8/8/8/8/5N2/3P4/4K3 w - - 0 1"
        attacked = "4k3/8/8/8/8/5N2/3P4/4K2r w - - 0 1"
        self.assertIn("e1d2", self.moves(safe))
        self.assertNotIn("e1d2", self.moves(attacked))
        self.assertIn("f3d2", self.moves(attacked))
        self.assertIn("e1f1", self.moves(attacked))
        self.assertIn("d2d3", self.moves(attacked))

    def test_promotion_rank(self):
        fen = "R2QK3/3P4/8/8/8/8/8/4k3 w - - 0 1"
        moves = self.moves(fen)
        self.assertNotIn("d7b8", moves)
        self.assertNotIn("d7c8", moves)
        promotion = "4k3/P7/8/8/8/8/8/4K3 w - - 0 1"
        self.assertEqual({m for m in self.moves(promotion) if m.startswith("a7a8")},
                         {"a7a8q", "a7a8r", "a7a8b", "a7a8n"})

    def test_san_attack_and_capture(self):
        fen = "4k3/8/3R4/8/8/8/8/4K3 w - - 0 1"
        self.assertEqual(sf.get_san(self.variant, fen, "d6e6"), "Re6+")
        fen = "3Rk3/8/8/8/8/8/8/4K3 w - - 0 1"
        self.assertEqual(sf.get_san(self.variant, fen, "d8e8"), "Rxe8")
        self.assertTrue(sf.is_immediate_game_end(self.variant, fen, ["d8e8"])[0])

    def test_spark_san_attack(self):
        # Shift to d7 attacks e8 with the relocated pawn.
        fen = "4k3/8/8/8/B7/8/1P6/3RK3 w - - 0 1"
        self.assertIn("b2d7", self.moves(fen))
        self.assertEqual(sf.get_san(self.variant, fen, "b2d7"), "b2>d7+")

    def test_black_san_attack(self):
        fen = "4k3/8/8/8/8/3r4/8/4K3 b - - 0 1"
        self.assertEqual(sf.get_san(self.variant, fen, "d3e3"), "Re3+")

    def test_clock_and_repetition(self):
        fen = sf.start_fen(self.variant)
        self.assertEqual(sf.get_fen(self.variant, fen, ["a2a3"]).split()[4], "1")
        fen = "7k/8/8/8/8/1N6/3P4/K7 w - - 0 1"
        moves = ["b3d2", "h8h7", "d2b3", "h7h8"]
        self.assertTrue(sf.is_optional_game_end(self.variant, fen, moves)[0])

    def test_king_count(self):
        self.assertEqual(sf.validate_fen(sf.start_fen(self.variant), self.variant), sf.FEN_OK)
        for fen in ("8/8/8/8/8/8/8/4K3 w - - 0 1",
                    "4k3/8/8/8/8/8/8/3KK3 w - - 0 1"):
            self.assertNotEqual(sf.validate_fen(fen, self.variant), sf.FEN_OK)

    def test_all_two_king_placements(self):
        for white in range(64):
            for black in range(64):
                if white == black:
                    continue
                squares = ["1"] * 64
                squares[white], squares[black] = "K", "k"
                ranks = ["".join(squares[i:i + 8]) for i in range(0, 64, 8)]
                ranks = [re.sub("1+", lambda match: str(len(match.group())), rank) for rank in ranks]
                for side in ("w", "b"):
                    fen = "/".join(ranks) + " " + side + " - - 0 1"
                    self.assertEqual(sf.validate_fen(fen, self.variant), sf.FEN_OK, fen)

    def test_unrestricted_material_and_attacks(self):
        for fen in (
            "4k3/8/8/8/8/8/4K3/8 w - - 0 1",
            "4k3/8/8/8/8/8/4K3/4r3 w - - 0 1",
            "4k3/4R3/8/8/8/8/4r3/4K3 w - - 0 1",
            "QQQQkQQQ/PPPPPPPP/PPPPPPPP/8/8/pppppppp/pppppppp/qqqqKqqq b - - 0 1",
            "p3k3/8/8/8/8/8/8/4K2P w - - 0 1",
        ):
            self.assertEqual(sf.validate_fen(fen, self.variant), sf.FEN_OK, fen)

    def test_pawns_cannot_start_on_promotion_rank(self):
        for fen in ("P3k3/8/8/8/8/8/8/4K3 w - - 0 1",
                    "4k3/8/8/8/8/8/8/4K2p b - - 0 1"):
            self.assertEqual(sf.validate_fen(fen, self.variant), sf.FEN_INVALID_PAWN_PLACEMENT)

    def test_imported_castling_flags_are_ignored(self):
        for board, flags in (
            ("r3k2r/8/8/8/8/8/8/R3K2R", "KQkq"),
            ("r3k2r/8/8/8/8/8/8/R3K2R", "HAha"),
            ("1r2k1r1/8/8/8/8/8/8/1R2K1R1", "GBgb"),
        ):
            fen = board + " w " + flags + " - 4 12"
            plain = board + " w - - 4 12"
            for chess960 in (False, True):
                self.assertEqual(sf.validate_fen(fen, self.variant, chess960), sf.FEN_OK)
                self.assertEqual(sf.get_fen(self.variant, fen, [], chess960), plain)
                moves = sf.legal_moves(self.variant, fen, [], chess960)
                self.assertEqual(sorted(moves), sorted(sf.legal_moves(self.variant, plain, [], chess960)))
                self.assertNotIn("e1g1", moves)
                self.assertNotIn("e1c1", moves)

    def test_no_legal_moves_is_a_draw(self):
        fen = "BBBBBBBB/PPPPPPPP/PP6/PP6/PP6/PP6/PP6/KP5k w - - 0 1"
        self.assertEqual(sf.validate_fen(fen, self.variant), sf.FEN_OK)
        self.assertEqual(sf.legal_moves(self.variant, fen, []), [])
        self.assertEqual(sf.game_result(self.variant, fen, []), 0)

    def test_bare_kings_draw_unless_adjacent(self):
        fen = "8/8/8/8/8/8/2k5/K7 w - - 0 1"
        self.assertEqual(sf.is_immediate_game_end(self.variant, fen, []), (True, 0))
        adjacent = "8/8/8/8/8/8/2k5/1K6 b - - 0 1"
        self.assertFalse(sf.is_immediate_game_end(self.variant, adjacent, [])[0])
        self.assertIn("c2b1", self.moves(adjacent))
        self.assertTrue(sf.is_immediate_game_end(self.variant, adjacent, ["c2b1"])[0])

    def test_minor_draw_zones(self):
        def fen_for(pieces, turn):
            rows = []
            for rank in reversed(range(8)):
                row, empty = "", 0
                for file in range(8):
                    piece = pieces.get(rank * 8 + file)
                    if piece:
                        row += (str(empty) if empty else "") + piece
                        empty = 0
                    else:
                        empty += 1
                rows.append(row + (str(empty) if empty else ""))
            return "/".join(rows) + " " + turn + " - - 0 1"

        petals = {"b2", "g2", "b7", "g7"}
        knight_zone = set("a1 b1 c1 f1 g1 h1 a8 b8 c8 f8 g8 h8 a2 a3 a6 a7 h2 h3 h6 h7".split()) | petals
        for minor in ("B", "N"):
            for weak in range(64):
                file, rank = weak % 8, weak // 8
                square = "abcdefgh"[file] + str(rank + 1)
                danger = (file in (0, 7) or rank in (0, 7) or square in petals) if minor == "B" else square in knight_zone
                strong = next(s for s in range(64)
                              if max(abs(s % 8 - file), abs(s // 8 - rank)) > 1)
                piece = next(s for s in range(64) if s not in (weak, strong)
                             and (abs(s % 8 - file) != abs(s // 8 - rank) if minor == "B"
                                  else sorted((abs(s % 8 - file), abs(s // 8 - rank))) != [1, 2]))
                for black_strong in (False, True):
                    pieces = {weak: "K" if black_strong else "k",
                              strong: "k" if black_strong else "K",
                              piece: minor.lower() if black_strong else minor}
                    for turn in ("w", "b"):
                        fen = fen_for(pieces, turn)
                        with self.subTest(minor=minor, square=square, fen=fen):
                            self.assertEqual(sf.is_immediate_game_end(self.variant, fen, [])[0], not danger)
                            if not danger:
                                self.assertEqual(sf.game_result(self.variant, fen, []), 0)

    def test_minor_draw_exceptions_and_transition(self):
        # An attacked lone king, adjacent kings, extra material, or a rook
        # must not trigger the minor-piece draw rule.
        for fen in (
            "8/8/8/8/4k3/8/8/KB6 w - - 0 1",
            "8/8/8/8/4k3/2N5/8/K7 w - - 0 1",
            "8/8/8/8/4kK2/8/8/B7 w - - 0 1",
            "8/8/8/8/4k3/8/P7/KN6 w - - 0 1",
            "8/8/8/8/4k3/8/8/KR6 w - - 0 1",
        ):
            with self.subTest(fen=fen):
                self.assertFalse(sf.is_immediate_game_end(self.variant, fen, [])[0])
        fen = "8/8/8/8/8/k7/8/6NK b - - 0 1"
        self.assertFalse(sf.is_immediate_game_end(self.variant, fen, [])[0])
        self.assertEqual(sf.is_immediate_game_end(self.variant, fen, ["a3b4"]), (True, 0))

    def test_chess_unchanged(self):
        self.assertFalse(sf.is_immediate_game_end("chess", "8/8/8/8/4k3/8/8/KN6 w - - 0 1", [])[0])
        self.assertEqual(len(sf.legal_moves("chess", sf.start_fen("chess"), [])), 20)
        fen = "4k3/8/3R4/8/8/8/8/4K3 w - - 0 1"
        self.assertEqual(sf.get_san("chess", fen, "d6e6"), "Re6+")
        fen = "7k/8/5KQ1/8/8/8/8/8 w - - 0 1"
        self.assertEqual(sf.get_san("chess", fen, "g6g7"), "Qg7#")


if __name__ == "__main__":
    unittest.main()
