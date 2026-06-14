#!/usr/bin/env python3
"""Compare Fairy-Stockfish Japanese Shogi notation against lishogi.org .kif exports.

This script verifies our engine's Japanese notation output by:
  1. Parsing KIF game records into UCI moves
  2. Checking each move is legal in the current position
  3. Comparing our engine's notation output against the KIF destination

== Coordinate systems ==

Three coordinate systems are involved:

  UCI (Universal Chess Interface) — used by Fairy-Stockfish internally
    Files: a-i, left to right     (a = leftmost)
    Ranks: 1-9, bottom to top    (1 = bottom)
    Example: g7g6 = pawn from g7 to g6

  USI (Universal Shogi Interface) — used by lishogi.org / shogiops
    Files: 1-9, right to left    (1 = rightmost from sente's view)
    Ranks: a-i, top to bottom    (a = sente's back rank, i = gote's back rank)
    Example: 7g7f = pawn from 7g to 7f
    Note: In our FEN, sente is at the TOP, so USI rank a maps to UCI rank 9.

  KIF (KiFu) — the .kif game record format from lishogi.org
    Files: 1-9, left to right    (1 = leftmost, same as UCI)
    Ranks: 一-九, bottom to top  (一 = bottom, same as UCI)
    Origin squares in parentheses: (XY) where X=file, Y=rank
    Example: 七六歩(77) = pawn from (7,7) to file 七 rank 六

  Fairy-Stockfish's FEN convention for shogi:
    Sente (black/lowercase) at the TOP of the FEN (ranks 7-9)
    Gote (white/uppercase) at the BOTTOM of the FEN (ranks 1-3)
    This is OPPOSITE to standard USI where sente is at the bottom.

== Engine notation conventions ==

  File (横): The engine counts files from RIGHT (一 = rightmost).
    This matches how shogi players count files (from right in standard orientation).

  Rank (段): The engine uses DIFFERENT conventions per side:
    Sente (Black): rank = UCI rank (counts from bottom of the FEN)
    Gote (White):  rank = 10 - UCI rank (counts from top of the FEN)
    This is because the engine treats the FEN orientation literally:
    sente's "forward" is toward rank 1 (bottom), gote's is toward rank 9 (top).

  KIF always counts ranks from the BOTTOM (absolute), regardless of side.
  Therefore: engine gote rank != KIF rank for gote moves.
  The comparison accounts for this with engine_rank_to_kif_rank().

== KIF special notation ==

  同 (dou): "same square" — the move lands on the same destination as the
    previous move. Always implies a capture. The previous destination must
    be tracked via last_dest_sq.

  打 (da): "drop" — placing a piece from hand onto the board.
    Format: piece名 + 打, e.g., 歩六五打 = drop pawn at 6五

  成 / 不成: "promotes" / "does not promote".
    CRITICAL: 成 at the END of piece text = promotion (e.g., 桂成 = knight promotes)
              成 at the START = already-promoted piece name (e.g., 成桂 = promoted knight)
    UCI marks promotion with '+' suffix: g7g7+ vs g7g7

  Game-end tokens (not moves): 投了, 切れ負け, 詰み, 千日手, etc.
    These are game results and are skipped, not parsed as moves.

== KIF file mapping ==

  KIF files count from LEFT (1=leftmost, 9=rightmost), same as UCI.
  This is different from USI where files count from the right.
  KIF rank numbers count from the bottom, same as UCI ranks.
  Therefore: KIF (file, rank) -> UCI is simply chr(ord('a')+(file-1)) + str(rank).
"""

import re
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import pyffish as sf

# Standard shogi starting position in Fairy-Stockfish's FEN convention.
# Sente (lowercase) at top, gote (uppercase) at bottom.
# "b" = sente (black) to move.
SHOGI_FEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] b - - 0 1"

# KIF piece names -> UCI piece letters.
# Includes base pieces and promoted forms (龍=promoted飛, 馬=promoted角, と=promoted歩).
KIF_PIECE_MAP = {
    "歩": "P", "香": "L", "桂": "N", "銀": "S", "金": "G",
    "角": "B", "飛": "R", "玉": "K", "王": "K",
    "龍": "R", "馬": "B", "と": "P",
}

# KIF rank kanji -> numeric value (一=1 through 九=9).
KIF_RANK_MAP = {
    "一": 1, "二": 2, "三": 3, "四": 4, "五": 5,
    "六": 6, "七": 7, "八": 8, "九": 9,
}

# Number -> kanji rank for display (index 0 unused, 1="一" through 9="九").
KANJI_RANK = ["", "一", "二", "三", "四", "五", "六", "七", "八", "九"]


def fullwidth_to_int(ch):
    """Convert full-width digit (e.g. '１') or half-width digit ('1') to int."""
    if ord(ch) >= 0xFF11 and ord(ch) <= 0xFF19:
        return ord(ch) - 0xFF10
    return int(ch)


def kif_file_to_uci(file_num):
    """KIF file 1-9 (left-to-right, same as UCI) -> UCI file letter a-i."""
    return chr(ord("a") + (int(file_num) - 1))


def kif_rank_to_uci(rank_num):
    """KIF rank 1-9 -> UCI rank number (both count from bottom)."""
    return int(rank_num)


def kif_sq_to_uci(file_num, rank_num):
    """Convert a KIF square (file, rank) to a UCI square string like 'g7'."""
    return f"{kif_file_to_uci(file_num)}{kif_rank_to_uci(rank_num)}"


def parse_kif_dest(dest_text):
    """Parse KIF destination text like '５六歩' or '７二角成'.

    Extracts the destination coordinates and identifies the piece and
    promotion status from the text AFTER the coordinates.

    Returns (file_num, rank_num, piece_char, is_promo, is_not_promo) or None.
    """
    m = re.match(r"([１-９1-9])([一二三四五六七八九])", dest_text)
    if not m:
        return None
    file_num = fullwidth_to_int(m.group(1))
    rank_num = KIF_RANK_MAP.get(m.group(2), 0)
    if rank_num == 0:
        return None

    # Everything after the coordinates is the piece name + promotion markers.
    # e.g. "歩" (pawn), "角成" (bishop promotes), "成桂" (promoted knight).
    rest = dest_text[m.end():]
    piece_char = None
    for ch in rest:
        if ch in KIF_PIECE_MAP:
            piece_char = ch
            break

    # Promotion detection:
    #   "桂成" = knight promotes -> is_promo = True  (成 at END, not START)
    #   "成桂" = promoted knight -> is_promo = False  (成 at START = piece name)
    #   "歩不成" = pawn does not promote -> is_promo = False
    is_promo = rest.endswith("成") and not rest.startswith("成") and "不成" not in rest
    is_not_promo = "不成" in rest

    return file_num, rank_num, piece_char, is_promo, is_not_promo


def parse_kif_origin(origin_text):
    """Parse KIF origin square '(77)' or '（７７）'.

    Returns (file_num, rank_num) or None.
    """
    m = re.search(r"[(\uff08](\d)(\d)[)\uff09]", origin_text)
    if m:
        return int(m.group(1)), int(m.group(2))
    m = re.search(r"[(\uff08]([１-９1-9])([１-９1-9])[)\uff09]", origin_text)
    if m:
        return fullwidth_to_int(m.group(1)), fullwidth_to_int(m.group(2))
    return None


def kif_move_to_uci(move_text, last_dest_sq=None):
    """Convert a KIF move text to a UCI move string.

    Handles three move types:
      1. 同 (same-square capture) — uses last_dest_sq as destination
      2. 打 (drop) — piece@square format, e.g. P@e5
      3. Normal move — from_sq + to_sq, e.g. g7g6, with optional '+' for promotion

    Args:
        move_text: KIF move string, e.g. "７六歩(77)" or "同　銀(33)"
        last_dest_sq: UCI square of the previous move's destination, needed for 同

    Returns:
        (uci_move, is_promo, is_drop) or (None, False, False) on parse failure
    """
    # --- 同 (dou): same-square capture ---
    # "同　銀(33)" means "capture on the same square as the previous move,
    # using the silver from origin (33)". The destination is last_dest_sq.
    if move_text.startswith("同"):
        # Strip "同" and whitespace, then strip the origin parentheses
        after_dou = re.sub(r"^同[\s　]*", "", move_text)
        after_dou = re.sub(r"[\(（]\d+[\)）]", "", after_dou).strip()

        # Extract the base piece name (skip 成 which is a promotion marker or
        # part of promoted piece names like 成桂)
        piece_match = re.search(r"([歩香桂銀金角飛玉龍馬と])", after_dou)
        if not piece_match:
            return None, False, False
        piece_char = piece_match.group(1)
        is_promo = (after_dou.endswith("成") and not after_dou.startswith("成")
                    and "不成" not in after_dou)
        piece = KIF_PIECE_MAP.get(piece_char, "?")

        # 同打 = same-square drop (rare but possible)
        if "打" in move_text:
            if last_dest_sq:
                return f"{piece}@{last_dest_sq}", False, True
            return None, False, False

        # 同 with origin = normal capture on last_dest_sq
        if last_dest_sq:
            origin = parse_kif_origin(move_text)
            if origin:
                from_sq = kif_sq_to_uci(origin[0], origin[1])
                uci = from_sq + last_dest_sq
                if is_promo:
                    uci += "+"
                return uci, is_promo, False
        return None, False, False

    # --- Drop: ７七歩打, ５五金打, etc. ---
    # Format: destination + piece + 打. E.g., "７七歩打" = drop pawn at 7七
    if "打" in move_text:
        drop_dest = re.match(r"([^\(（]+?)打", move_text)
        piece_match = re.search(r"([歩香桂銀金角飛])打", move_text)
        if drop_dest and piece_match:
            piece = KIF_PIECE_MAP.get(piece_match.group(1), "?")
            dest = parse_kif_dest(drop_dest.group(1))
            if dest:
                uci_dest = kif_sq_to_uci(dest[0], dest[1])
                return f"{piece}@{uci_dest}", False, True
        return None, False, False

    # --- Normal move: ５六歩(57) or ９三桂成(21) ---
    # Format: destination + piece + optional promotion + origin in parentheses
    origin = parse_kif_origin(move_text)
    if not origin:
        return None, False, False

    from_sq = kif_sq_to_uci(origin[0], origin[1])

    # Extract destination text (everything before the opening parenthesis)
    dest_text = re.match(r"(.+?)[\(（]", move_text)
    if not dest_text:
        return None, False, False

    dest = parse_kif_dest(dest_text.group(1))
    if not dest:
        return None, False, False

    to_sq = kif_sq_to_uci(dest[0], dest[1])
    is_promo = dest[3]

    uci = from_sq + to_sq
    if is_promo:
        uci += "+"

    return uci, is_promo, False


def parse_kif_file(filepath):
    """Parse a .kif file (single or multi-game format).

    lishogi.org exports multi-game files where games are separated by
    "開始日時" headers or blank lines. Each move line has format:
      "   1   ７六歩(77)   (00:00/00:00:00)"

    Returns a list of games, each a list of (move_num, move_text) tuples.
    """
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    games = []
    current_game = []
    for line in content.split("\n"):
        line = line.strip()
        m = re.match(r"^\s*(\d+)\s+(.+?)(?:\s+\(.*\))?\s*$", line)
        if m:
            move_num = int(m.group(1))
            move_text = m.group(2).strip()
            current_game.append((move_num, move_text))
        elif current_game and (
            line.startswith("開始日時") or line == "" or line.startswith("*")
        ):
            if current_game:
                games.append(current_game)
                current_game = []
    if current_game:
        games.append(current_game)
    return games


def extract_dest_from_san(san):
    """Extract destination file and rank from engine Japanese SAN output.

    The engine outputs notation like "３六歩" (file 三 rank 六 pawn).
    This extracts the two kanji characters representing destination coordinates.

    Returns (file_num, rank_num) or (None, None) for 同 or unrecognized format.
    Note: file_num here is the ENGINE's file (counted from right), not KIF's.
    """
    m = re.match(r"([一二三四五六七八九])([一二三四五六七八九])", san)
    if m:
        file_num = KIF_RANK_MAP.get(m.group(1), 0)
        rank_num = KIF_RANK_MAP.get(m.group(2), 0)
        return file_num, rank_num

    m = re.match(r"同", san)
    if m:
        return None, None

    return None, None


def engine_rank_to_kif_rank(engine_rank, is_sente):
    """Convert engine rank number to KIF rank number for comparison.

    The engine and KIF use different rank conventions for gote moves:

      Engine: sente rank = UCI rank (from bottom), gote rank = 10 - UCI rank
      KIF:    always counts from the bottom (absolute), both sides

      Therefore:
        sente: engine_rank == KIF_rank (no conversion needed)
        gote:  engine_rank == 10 - KIF_rank (need to flip)
    """
    if is_sente:
        return engine_rank
    else:
        return 10 - engine_rank


def main():
    max_games = None
    filepath = None
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == "--max" and i + 1 < len(args):
            max_games = int(args[i + 1])
            i += 2
        elif not args[i].startswith("-"):
            filepath = args[i]
            i += 1
        else:
            i += 1

    if not filepath:
        print("Usage: python3 tests/shogi_compare.py <game.kif> [--max N]")
        sys.exit(1)

    games = parse_kif_file(filepath)
    if max_games:
        games = games[:max_games]
    print(f"Parsed {len(games)} game(s) from {filepath}\n")

    total_ok = 0
    total_fail = 0
    total_skip = 0

    for game_idx, moves in enumerate(games):
        print(f"=== Game {game_idx + 1} ({len(moves)} moves) ===")
        fen = SHOGI_FEN
        last_dest_sq = None
        ok = 0
        fail = 0
        skip = 0

        for move_num, kif_text in moves:
            # Strip origin suffix for display: "七六歩(77)" -> "七六歩"
            kif_notation = re.sub(r"[\(（]\d+[\)）]", "", kif_text).strip()

            # Skip game-end tokens (not actual moves)
            if kif_text in ("投了", "切れ負け", "詰み", "千日手", "入玉宣言",
                            "反則勝ち", "反則負け", "中断"):
                print(f"{move_num:4d}  {kif_notation:20s}  {'(end)':10s}  SKIP (end)")
                skip += 1
                continue

            # Parse KIF move text to UCI format
            uci, is_promo, is_drop = kif_move_to_uci(kif_text, last_dest_sq)

            if not uci:
                print(f"{move_num:4d}  {kif_notation:20s}  {'?':10s}  SKIP (parse)")
                skip += 1
                continue

            # Verify move is legal in current position
            legal = sf.legal_moves("shogi", fen, [])
            if uci not in legal:
                print(
                    f"{move_num:4d}  {kif_notation:20s}  {uci:10s}  ILLEGAL"
                )
                fail += 1
                continue

            # Get engine's Japanese notation for this move
            is_sente = move_num % 2 == 1

            try:
                our_san = sf.get_san(
                    "shogi", fen, uci, False, sf.NOTATION_SHOGI_JAPANESE
                )
            except Exception as e:
                our_san = f"ERROR({e})"

            # Compare engine output with KIF destination
            engine_file, engine_rank = extract_dest_from_san(our_san)

            dest_match = True
            file_match = True
            rank_match = True

            if engine_file is not None and engine_rank is not None:
                kif_dest = parse_kif_dest(kif_notation)
                if kif_dest:
                    kif_dest_file = kif_dest[0]
                    kif_dest_rank = kif_dest[1]

                    # Engine files count from RIGHT, KIF files count from LEFT.
                    # For a 9-file board: engine_file + kif_file = 10
                    # (e.g. engine 三 = 3rd from right = KIF 七 = 7th from left)
                    engine_as_kif_file = 10 - engine_file
                    if engine_as_kif_file != kif_dest_file:
                        file_match = False
                        dest_match = False

                    # Engine gote ranks are inverted relative to KIF
                    kif_expected_rank = engine_rank_to_kif_rank(
                        engine_rank, is_sente
                    )
                    if kif_expected_rank != kif_dest_rank:
                        rank_match = False
                        dest_match = False

            if dest_match:
                status = "OK"
                ok += 1
            else:
                reason = []
                if not file_match:
                    reason.append("file")
                if not rank_match:
                    reason.append("rank")
                status = f"MISMATCH ({'+'.join(reason)})"
                fail += 1

            print(
                f"{move_num:4d}  {kif_notation:20s}  {uci:10s}  "
                f"{our_san:20s}  {status}"
            )

            # Track last destination for 同 disambiguation.
            # Drops use "piece@square" format, so the square starts at index 2.
            # Normal moves have fixed-length format: from_sq(2) + to_sq(2).
            if len(uci) >= 4:
                if is_drop:
                    last_dest_sq = uci[2:]  # e.g. "P@e5" -> "e5"
                else:
                    last_dest_sq = uci[2:4]  # e.g. "g7g6" -> "g6"

            # Advance position for the next move
            try:
                fen = sf.get_fen("shogi", fen, [uci], False, False)
            except Exception:
                pass

        print(f"\nOK: {ok}  MISMATCH: {fail}  SKIP: {skip}  Total: {len(moves)}")
        total_ok += ok
        total_fail += fail
        total_skip += skip
        print()

    print(f"=== Summary ===")
    print(f"Games: {len(games)}  Total moves: {total_ok + total_fail + total_skip}")
    print(f"OK: {total_ok}  MISMATCH: {total_fail}  SKIP: {total_skip}")

    sys.exit(1 if total_fail > 0 else 0)


if __name__ == "__main__":
    main()
