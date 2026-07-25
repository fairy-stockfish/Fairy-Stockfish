#!/usr/bin/env python3
"""Compare Fairy-Stockfish Japanese Shogi notation against lishogi.org .kif exports.

This script verifies our engine's Japanese notation output by:
  1. Parsing KIF game records into UCI moves
  2. Checking each move is legal in the current position
  3. Comparing our engine's notation output against the exported notation

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
    Files: 1-9, right to left    (1 = rightmost from sente's view)
    Ranks: 一-九, bottom to top  (一 = bottom from sente's view)
    Origin squares in parentheses: (XY) where X=file, Y=rank
    Example: 七六歩(77) = pawn from (7,7) to file 七 rank 六

  Fairy-Stockfish's FEN convention for shogi:
    Sente (black/lowercase) at the TOP of the FEN (ranks 7-9)
    Gote (white/uppercase) at the BOTTOM of the FEN (ranks 1-3)
    This is OPPOSITE to standard USI where sente is at the bottom.

== Engine notation conventions ==

  File (筋): The engine outputs full-width digits, e.g. "７".
  Rank (段): The engine outputs kanji, e.g. "六".

  For Japanese shogi notation these coordinates are ABSOLUTE board coordinates
  from sente's perspective for both sides, matching the KIF destination text.

== Comparison policy ==

  The checker distinguishes between semantic mismatches and notation-style
  differences.

  Semantic mismatches are treated as failures:
    - destination or 同 mismatch
    - wrong piece name (e.g. 歩 vs と)
    - wrong promotion marker when the export explicitly says 成
    - wrong explicit qualifier when the export explicitly says 右/左/直/...
    - unparseable or placeholder engine output such as '?'

  Style differences are reported but do not fail the run:
    - lishogi KIF exports always write 打 for drops, while the engine omits 打
      for unambiguous drops
    - lishogi often omits optional qualifiers like 右/左/直/上/引/寄/行
    - lishogi often omits optional 不成 where the engine spells it out

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

  KIF coordinates are standard shogi coordinates from sente's perspective.
  Fairy-Stockfish's shogi FEN is mirrored relative to that view because sente
  starts at the TOP of the FEN.

  Therefore KIF (file, rank) -> UCI requires flipping both axes:
    file = 10 - KIF_file
    rank = 10 - KIF_rank
"""

import re
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import pyffish as sf

# Standard shogi starting position in Fairy-Stockfish's FEN convention.
# Sente (lowercase) at top, gote (uppercase) at bottom.
# "w" is sente to move in Fairy-Stockfish's shogi FEN convention.
SHOGI_FEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] w - - 0 1"

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

GAME_END_MARKERS = (
    "投了", "切れ負け", "詰み", "千日手", "入玉宣言", "反則勝ち", "反則負け", "中断"
)

PIECE_TOKENS = [
    ("成銀", "+S"),
    ("成桂", "+N"),
    ("成香", "+L"),
    ("龍", "+R"),
    ("竜", "+R"),
    ("馬", "+B"),
    ("と", "+P"),
    ("歩", "P"),
    ("香", "L"),
    ("桂", "N"),
    ("銀", "S"),
    ("金", "G"),
    ("角", "B"),
    ("飛", "R"),
    ("玉", "K"),
    ("王", "K"),
]

ALLOWED_QUALIFIERS = {"右", "左", "直", "上", "引", "寄", "行", "中", "跳"}


def normalize_notation_text(text):
    """Remove KIF spacing that is not semantically relevant."""
    return text.replace(" ", "").replace("\u3000", "")


def parse_japanese_notation(text):
    """Parse Japanese move text into semantic components.

    The parser is intentionally lightweight: it understands standard shogi move
    text well enough to classify whether a difference is semantic (piece,
    destination, promotion, explicit qualifier) or merely stylistic
    (omitted 打/不成/qualifier).
    """
    raw = normalize_notation_text(text)
    parsed = {
        "raw": raw,
        "same": False,
        "dest": None,
        "piece": None,
        "piece_token": None,
        "qualifiers": "",
        "drop": False,
        "promote": False,
        "not_promote": False,
        "unknown": False,
        "parse_error": None,
    }

    if not raw:
        parsed["unknown"] = True
        parsed["parse_error"] = "empty"
        return parsed

    if "?" in raw:
        parsed["unknown"] = True
        parsed["parse_error"] = "placeholder"
        return parsed

    idx = 0
    if raw.startswith("同"):
        parsed["same"] = True
        idx = 1
    else:
        m = re.match(r"([１-９1-9])([一二三四五六七八九])", raw)
        if not m:
            parsed["unknown"] = True
            parsed["parse_error"] = "destination"
            return parsed
        parsed["dest"] = (
            fullwidth_to_int(m.group(1)),
            KIF_RANK_MAP.get(m.group(2), 0),
        )
        idx = m.end()

    rest = raw[idx:]
    for token, canonical_piece in PIECE_TOKENS:
        if rest.startswith(token):
            parsed["piece"] = canonical_piece
            parsed["piece_token"] = token
            rest = rest[len(token):]
            break

    if parsed["piece"] is None:
        parsed["unknown"] = True
        parsed["parse_error"] = "piece"
        return parsed

    if "不成" in rest:
        parsed["not_promote"] = True
        rest = rest.replace("不成", "")

    if rest.endswith("成"):
        parsed["promote"] = True
        rest = rest[:-1]

    if rest.endswith("打"):
        parsed["drop"] = True
        rest = rest[:-1]

    parsed["qualifiers"] = rest
    unknown_qualifiers = [ch for ch in rest if ch not in ALLOWED_QUALIFIERS]
    if unknown_qualifiers:
        parsed["unknown"] = True
        parsed["parse_error"] = "qualifier"

    return parsed


def compare_japanese_notation(expected_text, actual_text):
    """Compare exported notation with engine notation.

    Returns (hard_reasons, style_reasons). Hard reasons indicate a semantic
    mismatch; style reasons indicate acceptable convention differences.
    """
    expected = parse_japanese_notation(expected_text)
    actual = parse_japanese_notation(actual_text)

    hard_reasons = []
    style_reasons = []

    if expected["unknown"]:
        hard_reasons.append(f"expected_{expected['parse_error']}")
        return hard_reasons, style_reasons
    if actual["unknown"]:
        hard_reasons.append(f"actual_{actual['parse_error']}")
        return hard_reasons, style_reasons

    if expected["same"] != actual["same"]:
        hard_reasons.append("same")

    if not expected["same"] and expected["dest"] != actual["dest"]:
        hard_reasons.append("destination")

    if expected["piece"] != actual["piece"]:
        hard_reasons.append("piece")

    if expected["promote"] != actual["promote"]:
        hard_reasons.append("promotion")

    if expected["not_promote"] and not actual["not_promote"]:
        hard_reasons.append("not_promote")
    elif actual["not_promote"] and not expected["not_promote"]:
        style_reasons.append("not_promote")

    if expected["qualifiers"]:
        if expected["qualifiers"] != actual["qualifiers"]:
            hard_reasons.append("qualifier")
    elif actual["qualifiers"]:
        style_reasons.append("qualifier")

    # KIF exports include 打 for drops, while the engine intentionally omits it
    # for unambiguous drops. That is informative but not a semantic mismatch.
    if expected["drop"] != actual["drop"]:
        style_reasons.append("drop")

    return hard_reasons, style_reasons


def print_usage():
    print(
        "Usage: python3 tests/shogi_compare.py <game.kif> [--max N] [--summary-only]"
    )


def print_move(verbose, text):
    if verbose:
        print(text)


def process_game(game_idx, handicap, moves, verbose):
    """Compare one game and return aggregate counters.

    The summary-only path intentionally uses the same legality, SAN, and FEN
    checks as the verbose mode. It only suppresses the per-move log so large
    exports can be validated without spending most of the runtime on I/O.
    """
    if handicap != "平手":
        print_move(
            verbose,
            f"=== Game {game_idx + 1} ({len(moves)} moves) [SKIPPED: {handicap}] ===",
        )
        return 0, 0, 0, len(moves), True

    print_move(verbose, f"=== Game {game_idx + 1} ({len(moves)} moves) ===")
    fen = SHOGI_FEN
    last_dest_sq = None
    last_move_uci = None
    exact = 0
    style = 0
    fail = 0
    skip = 0

    for move_num, kif_text in moves:
        # Strip origin suffix for display: "七六歩(77)" -> "七六歩"
        kif_notation = re.sub(r"[\(（]\d+[\)）]", "", kif_text).strip()

        if kif_text in GAME_END_MARKERS:
            print_move(
                verbose,
                f"{move_num:4d}  {kif_notation:20s}  {'(end)':10s}  SKIP (end)",
            )
            skip += 1
            continue

        uci, is_promo, is_drop = kif_move_to_uci(kif_text, last_dest_sq)

        if not uci:
            print_move(
                verbose,
                f"{move_num:4d}  {kif_notation:20s}  {'?':10s}  SKIP (parse)",
            )
            skip += 1
            continue

        legal = sf.legal_moves("shogi", fen, [])
        if uci not in legal:
            print_move(verbose, f"{move_num:4d}  {kif_notation:20s}  {uci:10s}  ILLEGAL")
            fail += 1
            continue

        try:
            our_san = sf.get_san(
                "shogi", fen, uci, False, sf.NOTATION_SHOGI_JAPANESE, last_move_uci
            )
        except Exception as e:
            our_san = f"ERROR({e})"

        hard_reasons, style_reasons = compare_japanese_notation(kif_notation, our_san)
        if hard_reasons:
            status = f"MISMATCH ({'+'.join(hard_reasons)})"
            fail += 1
        elif style_reasons:
            status = f"STYLE ({'+'.join(sorted(set(style_reasons)))})"
            style += 1
        else:
            status = "OK"
            exact += 1

        print_move(
            verbose,
            f"{move_num:4d}  {kif_notation:20s}  {uci:10s}  "
            f"{our_san:20s}  {status}",
        )

        if len(uci) >= 4:
            if is_drop:
                last_dest_sq = uci[2:]
            else:
                last_dest_sq = uci[2:4]
        last_move_uci = uci

        try:
            fen = sf.get_fen("shogi", fen, [uci], False, False)
        except Exception:
            pass

    print_move(
        verbose,
        f"\nOK: {exact}  STYLE: {style}  MISMATCH: {fail}  "
        f"SKIP: {skip}  Total: {len(moves)}",
    )
    print_move(verbose, "")
    return exact, style, fail, skip, False


def fullwidth_to_int(ch):
    """Convert full-width digit (e.g. '１') or half-width digit ('1') to int."""
    if ord(ch) >= 0xFF11 and ord(ch) <= 0xFF19:
        return ord(ch) - 0xFF10
    return int(ch)


def kif_file_to_uci(file_num):
    """KIF file 1-9 -> UCI file letter a-i for Fairy-Stockfish's flipped FEN."""
    return chr(ord("a") + (9 - int(file_num)))


def kif_rank_to_uci(rank_num):
    """KIF rank 1-9 -> UCI rank number for Fairy-Stockfish's flipped FEN."""
    return 10 - int(rank_num)


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

    Returns a list of (game_type, moves) tuples, where game_type is the
    手合割 value (e.g. "平手", "王手将棋") and moves is a list of
    (move_num, move_text) tuples.

    Games without a 手合割 header that have non-standard first "moves"
    (e.g. file header lines like "８ ７ ６ ５ ４ ３ ２ １") are
    detected as variant games automatically.
    """
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    games = []
    current_game = []
    current_handicap = "平手"  # default to standard game
    for line in content.split("\n"):
        line = line.strip()
        # Extract game type from 手合割 header
        hm = re.match(r"^手合割[：:](.+)$", line)
        if hm:
            current_handicap = hm.group(1).strip()
        m = re.match(r"^\s*(\d+)\s+(.+?)(?:\s+\(.*\))?\s*$", line)
        if m:
            move_num = int(m.group(1))
            move_text = m.group(2).strip()
            current_game.append((move_num, move_text))
        elif current_game and (
            line.startswith("開始日時") or line == "" or line.startswith("*")
        ):
            if current_game:
                # Auto-detect variant games by checking first move.
                # Variant games often have a file header line like
                # "8  ８ ７ ６ ５ ４ ３ ２ １" parsed as a "move".
                # A real KIF move never has spaces between kanji digits.
                first_text = current_game[0][1]
                if " " in first_text or "　" in first_text:
                    current_handicap = "(variant)"
                games.append((current_handicap, current_game))
                current_game = []
                current_handicap = "平手"
    if current_game:
        first_text = current_game[0][1]
        if " " in first_text or "　" in first_text:
            current_handicap = "(variant)"
        games.append((current_handicap, current_game))
    return games


def extract_dest_from_san(san):
    """Extract destination file and rank from engine Japanese SAN output.

    The engine outputs notation like "７六歩" (full-width digit + kanji rank).

    Returns (file_num, rank_num) or (None, None) for 同 or unrecognized format.
    """
    m = re.match(r"([１-９1-9])([一二三四五六七八九])", san)
    if m:
        file_num = fullwidth_to_int(m.group(1))
        rank_num = KIF_RANK_MAP.get(m.group(2), 0)
        return file_num, rank_num

    m = re.match(r"同", san)
    if m:
        return None, None

    return None, None


def main():
    max_games = None
    filepath = None
    summary_only = False
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == "--max" and i + 1 < len(args):
            max_games = int(args[i + 1])
            i += 2
        elif args[i] == "--summary-only":
            summary_only = True
            i += 1
        elif not args[i].startswith("-"):
            filepath = args[i]
            i += 1
        else:
            print_usage()
            sys.exit(1)

    if not filepath:
        print_usage()
        sys.exit(1)

    games = parse_kif_file(filepath)
    if max_games:
        games = games[:max_games]
    print(f"Parsed {len(games)} game(s) from {filepath}\n")

    total_exact = 0
    total_style = 0
    total_fail = 0
    total_skip = 0
    skipped_variants = 0
    verbose = not summary_only

    for game_idx, (handicap, moves) in enumerate(games):
        exact, style, fail, skip, variant_skipped = process_game(
            game_idx, handicap, moves, verbose
        )
        total_exact += exact
        total_style += style
        total_fail += fail
        total_skip += skip
        if variant_skipped:
            skipped_variants += 1

    print(f"=== Summary ===")
    print(f"Games: {len(games)}  (平手: {len(games) - skipped_variants}, "
          f"variant: {skipped_variants} skipped)")
    print(f"Total moves: {total_exact + total_style + total_fail + total_skip}")
    print(
        f"OK: {total_exact}  STYLE: {total_style}  "
        f"MISMATCH: {total_fail}  SKIP: {total_skip}"
    )

    sys.exit(1 if total_fail > 0 else 0)


if __name__ == "__main__":
    main()
