#!/usr/bin/env python3
"""Compare our Japanese Shogi notation against lishogi.org .kif exports.

Usage: python3 tests/shogi_compare.py <game.kif>
"""

import re
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))
import pyffish as sf

SHOGI_FEN = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL[-] b - - 0 1"

# KIF piece names -> UCI piece letters
KIF_PIECE_MAP = {
    '歩': 'P', '香': 'L', '桂': 'N', '銀': 'S', '金': 'G',
    '角': 'B', '飛': 'R', '玉': 'K', '王': 'K',
}

# KIF rank kanji -> our rank numbers
# KIF: 一=bottom(our 1), 九=top(our 9) — same as UCI
KIF_RANK_MAP = {'一': 1, '二': 2, '三': 3, '四': 4, '五': 5, '六': 6, '七': 7, '八': 8, '九': 9}

def kif_sq_to_uci(file_num, rank_num):
    """Convert KIF square (file 1-9 right-to-left, rank 1-9 same as UCI) to UCI."""
    uci_file = chr(ord('a') + (9 - int(file_num)))
    uci_rank = int(rank_num)
    return f"{uci_file}{uci_rank}"

def kif_kanji_dest_to_uci(dest_text):
    """Convert KIF destination like '５六' to UCI square."""
    # Full-width or half-width file number + kanji rank
    m = re.match(r'([１-９1-9])([一二三四五六七八九])', dest_text)
    if m:
        file_char = m.group(1)
        # Normalize full-width to half-width
        if ord(file_char) >= 0xFF11:
            file_num = ord(file_char) - 0xFF10
        else:
            file_num = int(file_char)
        rank_kanji = m.group(2)
        uci_file = chr(ord('a') + (9 - file_num))
        uci_rank = KIF_RANK_MAP.get(rank_kanji, 0)
        return f"{uci_file}{uci_rank}"
    return None

def parse_kif_moves(filepath):
    """Parse .kif file, return list of (move_num, raw_move_text)."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    moves = []
    for line in content.split('\n'):
        line = line.strip()
        # Match: "   1   ５六歩(57)   (00:00/00:00:00)"
        m = re.match(r'^\s*(\d+)\s+(.+?)(?:\s+\(.*\))?$', line)
        if m:
            move_num = int(m.group(1))
            move_text = m.group(2).strip()
            moves.append((move_num, move_text))
    return moves

def kif_move_to_uci(move_text, last_dest_sq=None):
    """Convert KIF move text to UCI. Returns (uci, is_promotion, is_drop)."""
    # 同 (same square capture)
    if move_text.startswith('同'):
        piece_match = re.search(r'([歩香桂銀金角飛玉龍馬と成])', move_text)
        if not piece_match:
            return None, False, False
        piece_char = piece_match.group(1)
        # Strip 成 for promotion detection
        is_promo = '成' in move_text.replace(piece_char, '', 1)
        piece = KIF_PIECE_MAP.get(piece_char, '?')

        # Check for drop (同打)
        if '打' in move_text:
            if last_dest_sq:
                return f"{piece}@{last_dest_sq}", False, True
            return None, False, False

        # 同 piece - capture on last destination
        if last_dest_sq:
            uci = last_dest_sq  # Will be prepended with from_sq later
            return None, False, False  # Need more info
        return None, False, False

    # Drop: 歩打, 金打, etc.
    drop_match = re.search(r'([歩香桂銀金角飛])打', move_text)
    if drop_match:
        piece = KIF_PIECE_MAP.get(drop_match.group(1), '?')
        # Extract destination (before 打)
        dest = re.match(r'([^\(]+?)打', move_text)
        if dest:
            uci_dest = kif_kanji_dest_to_uci(dest.group(1))
            if uci_dest:
                return f"{piece}@{uci_dest}", False, True
        return None, False, False

    # Normal move: ５六歩(57)
    origin_match = re.search(r'\((\d)(\d)\)', move_text)
    if not origin_match:
        return None, False, False

    origin_file = origin_match.group(1)
    origin_rank = origin_match.group(2)
    from_sq = kif_sq_to_uci(origin_file, origin_rank)

    # Destination: extract from move text (before the parenthesized origin)
    dest_text = re.match(r'(.+?)[\(（]', move_text)
    if not dest_text:
        return None, False, False
    dest_str = dest_text.group(1).strip()

    # Strip piece name and promotion markers to get destination coordinates
    # e.g., "５六歩" -> "５六", "９三桂成" -> "９三"
    coord_match = re.match(r'([１-９1-9])([一二三四五六七八九])', dest_str)
    if not coord_match:
        return None, False, False

    uci_dest = kif_kanji_dest_to_uci(dest_str)
    if not uci_dest:
        return None, False, False

    is_promo = '成' in move_text and '不成' not in move_text

    return f"{from_sq}{uci_dest}" + ('+' if is_promo else ''), is_promo, False

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 tests/shogi_compare.py <game.kif>")
        sys.exit(1)

    filepath = sys.argv[1]
    moves = parse_kif_moves(filepath)
    print(f"Parsed {len(moves)} moves from {filepath}\n")

    fen = SHOGI_FEN
    last_dest_sq = None
    ok = 0
    fail = 0
    skip = 0

    for move_num, kif_text in moves:
        # Strip .kif origin suffix for display/comparison
        kif_notation = re.sub(r'[\(（]\d+[\)）]', '', kif_text).strip()

        # Convert to UCI
        uci, is_promo, is_drop = kif_move_to_uci(kif_text, last_dest_sq)

        # Handle 同 by looking at last destination
        if kif_text.startswith('同') and not uci:
            origin_match = re.search(r'\((\d)(\d)\)', kif_text)
            if origin_match and last_dest_sq:
                from_sq = kif_sq_to_uci(origin_match.group(1), origin_match.group(2))
                is_promo = '成' in kif_text and '不成' not in kif_text
                uci = from_sq + last_dest_sq + ('+' if is_promo else '')

        if not uci:
            print(f"{move_num:3d}  {kif_notation:20s}  {'?':10s}  SKIP (parse)")
            skip += 1
            continue

        # Get our notation
        try:
            our_san = sf.get_san('shogi', fen, uci, False, sf.NOTATION_SHOGI_JAPANESE)
        except Exception as e:
            our_san = f"ERROR({e})"

        # Check legality
        legal = sf.legal_moves('shogi', fen, [])
        if uci not in legal:
            print(f"{move_num:3d}  {kif_notation:20s}  {uci:10s}  {our_san:20s}  ILLEGAL")
            fail += 1
            continue

        # Compare
        match = (our_san == kif_notation or
                 our_san.replace('同　', '') == kif_notation.replace('同', '') or
                 our_san in kif_notation or kif_notation in our_san)
        status = "OK" if match else "MISMATCH"
        if not match:
            fail += 1
        else:
            ok += 1

        print(f"{move_num:3d}  {kif_notation:20s}  {uci:10s}  {our_san:20s}  {status}")

        # Track last destination for 同
        if len(uci) >= 4 and not is_drop:
            last_dest_sq = uci[2:4]

        # Update position
        try:
            fen = sf.get_fen('shogi', fen, [uci], False, False)
        except:
            pass

    print(f"\nOK: {ok}  MISMATCH: {fail}  SKIP: {skip}  Total: {len(moves)}")

if __name__ == '__main__':
    main()
