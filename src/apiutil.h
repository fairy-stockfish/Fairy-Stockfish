/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2020 Fabian Fichter

  Fairy-Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Fairy-Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

namespace PSQT {
  void init(const Variant* v);
}

enum Notation {
    NOTATION_DEFAULT,
    // https://en.wikipedia.org/wiki/Algebraic_notation_(chess)
    NOTATION_SAN,
    NOTATION_LAN,
    // https://en.wikipedia.org/wiki/Shogi_notation#Western_notation
    NOTATION_SHOGI_HOSKING, // Examples: P76, S’34
    NOTATION_SHOGI_HODGES, // Examples: P-7f, S*3d
    NOTATION_SHOGI_HODGES_NUMBER, // Examples: P-76, S*34
    // http://www.janggi.pl/janggi-notation/
    NOTATION_JANGGI,
    // https://en.wikipedia.org/wiki/Xiangqi#Notation
    NOTATION_XIANGQI_WXF,
};

Notation default_notation(const Variant* v) {
    if (v->variantTemplate == "shogi")
        return NOTATION_SHOGI_HODGES_NUMBER;
    return NOTATION_SAN;
}

enum Disambiguation {
    NO_DISAMBIGUATION,
    FILE_DISAMBIGUATION,
    RANK_DISAMBIGUATION,
    SQUARE_DISAMBIGUATION,
};

bool is_shogi(Notation n) {
    return n == NOTATION_SHOGI_HOSKING || n == NOTATION_SHOGI_HODGES || n == NOTATION_SHOGI_HODGES_NUMBER;
}

std::string piece(const Position& pos, Move m, Notation n) {
    Color us = pos.side_to_move();
    Square from = from_sq(m);
    Piece pc = pos.moved_piece(m);
    PieceType pt = type_of(pc);
    // Quiet pawn moves
    if ((n == NOTATION_SAN || n == NOTATION_LAN) && type_of(pc) == PAWN && type_of(m) != DROP)
        return "";
    // Tandem pawns
    else if (n == NOTATION_XIANGQI_WXF && popcount(pos.pieces(us, pt) & file_bb(from)) > 2)
        return std::to_string(popcount(forward_file_bb(us, from) & pos.pieces(us, pt)) + 1);
    // Moves of promoted pieces
    else if (is_shogi(n) && type_of(m) != DROP && pos.unpromoted_piece_on(from))
        return "+" + std::string(1, toupper(pos.piece_to_char()[pos.unpromoted_piece_on(from)]));
    // Promoted drops
    else if (is_shogi(n) && type_of(m) == DROP && dropped_piece_type(m) != in_hand_piece_type(m))
        return "+" + std::string(1, toupper(pos.piece_to_char()[in_hand_piece_type(m)]));
    else if (pos.piece_to_char_synonyms()[pc] != ' ')
        return std::string(1, toupper(pos.piece_to_char_synonyms()[pc]));
    else
        return std::string(1, toupper(pos.piece_to_char()[pc]));
}

std::string file(const Position& pos, Square s, Notation n) {
    switch (n) {
    case NOTATION_SHOGI_HOSKING:
    case NOTATION_SHOGI_HODGES:
    case NOTATION_SHOGI_HODGES_NUMBER:
        return std::to_string(pos.max_file() - file_of(s) + 1);
    case NOTATION_JANGGI:
        return std::to_string(file_of(s) + 1);
    case NOTATION_XIANGQI_WXF:
        return std::to_string((pos.side_to_move() == WHITE ? pos.max_file() - file_of(s) : file_of(s)) + 1);
    default:
        return std::string(1, char('a' + file_of(s)));
    }
}

std::string rank(const Position& pos, Square s, Notation n) {
    switch (n) {
    case NOTATION_SHOGI_HOSKING:
    case NOTATION_SHOGI_HODGES_NUMBER:
        return std::to_string(pos.max_rank() - rank_of(s) + 1);
    case NOTATION_SHOGI_HODGES:
        return std::string(1, char('a' + pos.max_rank() - rank_of(s)));
    case NOTATION_JANGGI:
        return std::to_string((pos.max_rank() - rank_of(s) + 1) % 10);
    case NOTATION_XIANGQI_WXF:
    {
        if (pos.empty(s))
            return std::to_string(relative_rank(pos.side_to_move(), s, pos.max_rank()) + 1);
        else if (pos.pieces(pos.side_to_move(), type_of(pos.piece_on(s))) & forward_file_bb(pos.side_to_move(), s))
            return "-";
        else
            return "+";
    }
    default:
        return std::to_string(rank_of(s) + 1);
    }
}

std::string square(const Position& pos, Square s, Notation n) {
    switch (n) {
    case NOTATION_JANGGI:
        return rank(pos, s, n) + file(pos, s, n);
    default:
        return file(pos, s, n) + rank(pos, s, n);
    }
}

Disambiguation disambiguation_level(const Position& pos, Move m, Notation n) {
    // Drops never need disambiguation
    if (type_of(m) == DROP)
        return NO_DISAMBIGUATION;

    // NOTATION_LAN and Janggi always use disambiguation
    if (n == NOTATION_LAN || n == NOTATION_JANGGI)
        return SQUARE_DISAMBIGUATION;

    Color us = pos.side_to_move();
    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = pos.moved_piece(m);
    PieceType pt = type_of(pc);

    // Xiangqi uses either file disambiguation or +/- if two pieces on file
    if (n == NOTATION_XIANGQI_WXF)
    {
        // Disambiguate by rank (+/-) if target square of other piece is valid
        if (popcount(pos.pieces(us, pt) & file_bb(from)) == 2)
        {
            Square otherFrom = lsb((pos.pieces(us, pt) & file_bb(from)) ^ from);
            Square otherTo = otherFrom + Direction(to) - Direction(from);
            if (is_ok(otherTo) && (pos.board_bb(us, pt) & otherTo))
                return RANK_DISAMBIGUATION;
        }
        return FILE_DISAMBIGUATION;
    }

    // Pawn captures always use disambiguation
    if (n == NOTATION_SAN && pt == PAWN)
    {
        if (pos.capture(m))
            return FILE_DISAMBIGUATION;
        if (type_of(m) == PROMOTION && from != to && pos.sittuyin_promotion())
            return SQUARE_DISAMBIGUATION;
    }

    // A disambiguation occurs if we have more then one piece of type 'pt'
    // that can reach 'to' with a legal move.
    Bitboard b = pos.pieces(us, pt) ^ from;
    Bitboard others = 0;

    while (b)
    {
        Square s = pop_lsb(&b);
        if (   pos.pseudo_legal(make_move(s, to))
            && pos.legal(make_move(s, to))
            && !(is_shogi(n) && pos.unpromoted_piece_on(s) != pos.unpromoted_piece_on(from)))
            others |= s;
    }

    if (!others)
        return NO_DISAMBIGUATION;
    else if (is_shogi(n))
        return SQUARE_DISAMBIGUATION;
    else if (!(others & file_bb(from)))
        return FILE_DISAMBIGUATION;
    else if (!(others & rank_bb(from)))
        return RANK_DISAMBIGUATION;
    else
        return SQUARE_DISAMBIGUATION;
}

std::string disambiguation(const Position& pos, Square s, Notation n, Disambiguation d) {
    switch (d)
    {
    case FILE_DISAMBIGUATION:
        return file(pos, s, n);
    case RANK_DISAMBIGUATION:
        return rank(pos, s, n);
    case SQUARE_DISAMBIGUATION:
        return square(pos, s, n);
    default:
        assert(d == NO_DISAMBIGUATION);
        return "";
    }
}

const std::string move_to_san(Position& pos, Move m, Notation n) {
    std::string san = "";
    Color us = pos.side_to_move();
    Square from = from_sq(m);
    Square to = to_sq(m);

    if (type_of(m) == CASTLING)
    {
        san = to > from ? "O-O" : "O-O-O";

        if (is_gating(m))
        {
            san += std::string("/") + pos.piece_to_char()[make_piece(WHITE, gating_type(m))];
            san += square(pos, gating_square(m), n);
        }
    }
    else
    {
        // Piece
        san += piece(pos, m, n);

        // Origin square, disambiguation
        Disambiguation d = disambiguation_level(pos, m, n);
        san += disambiguation(pos, from, n, d);

        // Separator/Operator
        if (type_of(m) == DROP)
            san += n == NOTATION_SHOGI_HOSKING ? '\'' : is_shogi(n) ? '*' : '@';
        else if (n == NOTATION_XIANGQI_WXF)
        {
            if (rank_of(from) == rank_of(to))
                san += '=';
            else if (relative_rank(us, to, pos.max_rank()) > relative_rank(us, from, pos.max_rank()))
                san += '+';
            else
                san += '-';
        }
        else if (pos.capture(m))
            san += 'x';
        else if (n == NOTATION_LAN || (is_shogi(n) && (n != NOTATION_SHOGI_HOSKING || d == SQUARE_DISAMBIGUATION)) || n == NOTATION_JANGGI)
            san += '-';

        // Destination square
        if (n == NOTATION_XIANGQI_WXF && type_of(m) != DROP)
            san += file_of(to) == file_of(from) ? std::to_string(std::abs(rank_of(to) - rank_of(from))) : file(pos, to, n);
        else
            san += square(pos, to, n);

        // Suffix
        if (type_of(m) == PROMOTION)
            san += std::string("=") + pos.piece_to_char()[make_piece(WHITE, promotion_type(m))];
        else if (type_of(m) == PIECE_PROMOTION)
            san += is_shogi(n) ? std::string("+") : std::string("=") + pos.piece_to_char()[make_piece(WHITE, pos.promoted_piece_type(type_of(pos.moved_piece(m))))];
        else if (type_of(m) == PIECE_DEMOTION)
            san += is_shogi(n) ? std::string("-") : std::string("=") + std::string(1, pos.piece_to_char()[pos.unpromoted_piece_on(from)]);
        else if (type_of(m) == NORMAL && is_shogi(n) && pos.pseudo_legal(make<PIECE_PROMOTION>(from, to)))
            san += std::string("=");
        if (is_gating(m))
            san += std::string("/") + pos.piece_to_char()[make_piece(WHITE, gating_type(m))];
    }

    // Check and checkmate
    if (pos.gives_check(m) && !is_shogi(n))
    {
        StateInfo st;
        pos.do_move(m, st);
        san += MoveList<LEGAL>(pos).size() ? "+" : "#";
        pos.undo_move(m);
    }

    return san;
}

bool hasInsufficientMaterial(Color c, const Position& pos) {

    // Other win rules
    if (   pos.captures_to_hand()
        || pos.count_in_hand(c, ALL_PIECES)
        || pos.extinction_value() != VALUE_NONE
        || (pos.capture_the_flag_piece() && pos.count(c, pos.capture_the_flag_piece())))
        return false;

    // Restricted pieces
    Bitboard restricted = pos.pieces(~c, KING);
    for (PieceType pt : pos.piece_types())
        if (pt == KING || !(pos.board_bb(c, pt) & pos.board_bb(~c, KING)))
            restricted |= pos.pieces(c, pt);

    // Mating pieces
    for (PieceType pt : { ROOK, QUEEN, ARCHBISHOP, CHANCELLOR, SILVER, GOLD, COMMONER, CENTAUR })
        if ((pos.pieces(c, pt) & ~restricted) || (pos.count(c, PAWN) && pos.promotion_piece_types().find(pt) != pos.promotion_piece_types().end()))
            return false;

    // Color-bound pieces
    Bitboard colorbound = 0, unbound;
    for (PieceType pt : { BISHOP, FERS, FERS_ALFIL, ALFIL, ELEPHANT })
        colorbound |= pos.pieces(pt) & ~restricted;
    unbound = pos.pieces() ^ restricted ^ colorbound;
    if ((colorbound & pos.pieces(c)) && (((DarkSquares & colorbound) && (~DarkSquares & colorbound)) || unbound))
        return false;

    // Unbound pieces require one helper piece of either color
    if ((pos.pieces(c) & unbound) && (popcount(pos.pieces() ^ restricted) >= 2 || pos.stalemate_value() != VALUE_DRAW))
        return false;

    return true;
}
