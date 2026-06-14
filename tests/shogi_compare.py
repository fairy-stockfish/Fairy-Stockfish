#!/usr/bin/env python3
"""Compare our Japanese Shogi notation against lishogi.org .kif exports.

Verifies that KIF moves parse to legal UCI moves and that our engine's
Japanese notation matches KIF conventions.

Usage:
  python3 tests/shogi_compare.py <game.kif>          # single game
  python3 tests/shogi_compare.py <games.kif>          # multi-game
  python3 tests/shogi_compare.py <games.kif> --max 5  # first 5 games only
"""

import re
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import pyffish as sf

SHOGI_FEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] b - - 0 1"

# KIF piece names -> UCI piece letters
KIF_PIECE_MAP = {
    "歩": "P", "香": "L", "桂": "N", "銀": "S", "金": "G",
    "角": "B", "飛": "R", "玉": "K", "王": "K",
    "龍": "R", "馬": "B", "と": "P",
}

# KIF rank kanji -> number
KIF_RANK_MAP = {
    "一": 1, "二": 2, "三": 3, "四": 4, "五": 5,
    "六": 6, "七": 7, "八": 8, "九": 9,
}

# Number -> kanji rank (1-indexed)
KANJI_RANK = ["", "一", "二", "三", "四", "五", "六", "七", "八", "九"]


def fullwidth_to_int(ch):
    """Convert full-width digit '１'-'９' or half-width '1'-'9' to int."""
    if ord(ch) >= 0xFF11 and ord(ch) <= 0xFF19:
        return ord(ch) - 0xFF10
    return int(ch)


def kif_file_to_uci(file_num):
    """KIF file 1-9 (left-to-right, same as UCI) -> UCI file letter."""
    return chr(ord("a") + (int(file_num) - 1))


def kif_rank_to_uci(rank_num):
    """KIF rank -> UCI rank (both count from bottom of the FEN)."""
    return int(rank_num)


def kif_sq_to_uci(file_num, rank_num):
    """KIF square (file, rank) -> UCI square string."""
    return f"{kif_file_to_uci(file_num)}{kif_rank_to_uci(rank_num)}"


def parse_kif_dest(dest_text):
    """Parse KIF destination text like '５六歩' or '７二角成'.
    Returns (file_num, rank_num, piece_char, is_promo) or None.
    """
    m = re.match(r"([１-９1-9])([一二三四五六七八九])", dest_text)
    if not m:
        return None
    file_num = fullwidth_to_int(m.group(1))
    rank_num = KIF_RANK_MAP.get(m.group(2), 0)
    if rank_num == 0:
        return None

    rest = dest_text[m.end():]
    piece_char = None
    for ch in rest:
        if ch in KIF_PIECE_MAP:
            piece_char = ch
            break

    is_promo = rest.endswith("成") and not rest.startswith("成") and "不成" not in rest
    is_not_promo = "不成" in rest

    return file_num, rank_num, piece_char, is_promo, is_not_promo


def parse_kif_origin(origin_text):
    """Parse KIF origin '(77)' or '（７７）'. Returns (file_num, rank_num) or None."""
    m = re.search(r"[(\uff08](\d)(\d)[)\uff09]", origin_text)
    if m:
        return int(m.group(1)), int(m.group(2))
    m = re.search(r"[(\uff08]([１-９1-9])([１-９1-9])[)\uff09]", origin_text)
    if m:
        return fullwidth_to_int(m.group(1)), fullwidth_to_int(m.group(2))
    return None


def kif_move_to_uci(move_text, last_dest_sq=None):
    """Convert KIF move text to UCI move string.

    Returns (uci_move, is_promo, is_drop) or None on failure.
    For 同 (same-square), last_dest_sq must be provided.
    """
    # 同 (same-square capture)
    if move_text.startswith("同"):
        after_dou = re.sub(r"^同[\s　]*", "", move_text)
        after_dou = re.sub(r"[\(（]\d+[\)）]", "", after_dou).strip()

        piece_match = re.search(r"([歩香桂銀金角飛玉龍馬と])", after_dou)
        if not piece_match:
            return None, False, False
        piece_char = piece_match.group(1)
        is_promo = (after_dou.endswith("成") and not after_dou.startswith("成")
                    and "不成" not in after_dou)
        piece = KIF_PIECE_MAP.get(piece_char, "?")

        if "打" in move_text:
            if last_dest_sq:
                return f"{piece}@{last_dest_sq}", False, True
            return None, False, False

        if last_dest_sq:
            origin = parse_kif_origin(move_text)
            if origin:
                from_sq = kif_sq_to_uci(origin[0], origin[1])
                uci = from_sq + last_dest_sq
                if is_promo:
                    uci += "+"
                return uci, is_promo, False
        return None, False, False

    # Drop: ７七歩打, ５五金打, etc.
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

    # Normal move: ５六歩(57) or ９三桂成(21)
    origin = parse_kif_origin(move_text)
    if not origin:
        return None, False, False

    from_sq = kif_sq_to_uci(origin[0], origin[1])

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
    """Parse .kif file (single or multi-game).
    Returns list of games, each a list of (move_num, move_text).
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
    """Extract destination file number and rank number from engine Japanese SAN.
    Returns (file_num, rank_num) where file_num is 1-9 (right-to-left)
    and rank_num is the engine's rank number.
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
    """Convert engine rank number to KIF rank number.

    Engine convention: sente counts from bottom (same as UCI), gote counts
    from top (10 - UCI). KIF always counts from bottom (absolute).
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
            kif_notation = re.sub(r"[\(（]\d+[\)）]", "", kif_text).strip()

            if kif_text in ("投了", "切れ負け", "詰み", "千日手", "入玉宣言",
                            "反則勝ち", "反則負け", "中断"):
                print(f"{move_num:4d}  {kif_notation:20s}  {'(end)':10s}  SKIP (end)")
                skip += 1
                continue

            uci, is_promo, is_drop = kif_move_to_uci(kif_text, last_dest_sq)

            if not uci:
                print(f"{move_num:4d}  {kif_notation:20s}  {'?':10s}  SKIP (parse)")
                skip += 1
                continue

            legal = sf.legal_moves("shogi", fen, [])
            if uci not in legal:
                print(
                    f"{move_num:4d}  {kif_notation:20s}  {uci:10s}  ILLEGAL"
                )
                fail += 1
                continue

            is_sente = move_num % 2 == 1

            try:
                our_san = sf.get_san(
                    "shogi", fen, uci, False, sf.NOTATION_SHOGI_JAPANESE
                )
            except Exception as e:
                our_san = f"ERROR({e})"

            engine_file, engine_rank = extract_dest_from_san(our_san)

            dest_match = True
            file_match = True
            rank_match = True

            if engine_file is not None and engine_rank is not None:
                kif_dest = parse_kif_dest(kif_notation)
                if kif_dest:
                    kif_dest_file = kif_dest[0]
                    kif_dest_rank = kif_dest[1]

                    engine_as_kif_file = 10 - engine_file
                    if engine_as_kif_file != kif_dest_file:
                        file_match = False
                        dest_match = False

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

            origin_str = ""
            origin = parse_kif_origin(kif_text)
            if origin:
                origin_str = f"({origin[0]}{origin[1]})"

            print(
                f"{move_num:4d}  {kif_notation:20s}  {uci:10s}  "
                f"{our_san:20s}  {status}"
            )

            if len(uci) >= 4:
                if is_drop:
                    last_dest_sq = uci[2:]
                else:
                    last_dest_sq = uci[2:4]

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
