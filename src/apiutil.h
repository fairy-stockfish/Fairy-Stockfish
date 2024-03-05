/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2022 Fabian Fichter

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

#ifndef APIUTIL_H_INCLUDED
#define APIUTIL_H_INCLUDED

#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <iostream>
#include <math.h>

#include "types.h"
#include "position.h"
#include "variant.h"

namespace Stockfish {

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
    // https://web.archive.org/web/20180817205956/http://bgsthai.com/2018/05/07/lawofthaichessc/
    NOTATION_THAI_SAN,
    NOTATION_THAI_LAN,
};

inline Notation default_notation(const Variant* v) {
    if (v->variantTemplate == "shogi")
        return NOTATION_SHOGI_HODGES_NUMBER;
    return NOTATION_SAN;
}

enum Termination {
    ONGOING,
    CHECKMATE,
    STALEMATE,
    INSUFFICIENT_MATERIAL,
    N_MOVE_RULE,
    N_FOLD_REPETITION,
    VARIANT_END,
};

const std::array<std::string, 12> THAI_FILES = {"ก", "ข", "ค", "ง", "จ", "ฉ", "ช", "ญ", "ต", "ถ", "ธ", "น"};
const std::array<std::string, 12> THAI_RANKS = {"๑", "๒", "๓", "๔", "๕", "๖", "๗", "๘", "๙", "๑๐", "๑๑", "๑๒"};

namespace SAN {

enum Disambiguation {
    NO_DISAMBIGUATION,
    FILE_DISAMBIGUATION,
    RANK_DISAMBIGUATION,
    SQUARE_DISAMBIGUATION,
};

inline bool is_shogi(Notation n) {
    return n == NOTATION_SHOGI_HOSKING || n == NOTATION_SHOGI_HODGES || n == NOTATION_SHOGI_HODGES_NUMBER;
}

inline bool is_thai(Notation n) {
    return n == NOTATION_THAI_SAN || n == NOTATION_THAI_LAN;
}

// is there more than one file with a pair of pieces?
inline bool multi_tandem(Bitboard b) {
    int tandems = 0;
    for (File f = FILE_A; f <= FILE_MAX; ++f)
        if (more_than_one(b & file_bb(f)))
            tandems++;
    return tandems >= 2;
}

inline std::string piece_to_thai_char(Piece pc, bool promoted) {
    switch(type_of(pc)) {
        case KING:
            return "ข";
        case KHON:
            return "ค";
        case FERS:
            return promoted ? "ง" : "ม็";
        case KNIGHT:
            return "ม";
        case ROOK:
            return "ร";
        case PAWN:
            return "บ";
        case AIWOK:
            return "ว";
        default:
            return "X";
    }
}

inline std::string piece(const Position& pos, Move m, Notation n) {
    Color us = pos.side_to_move();
    Square from = from_sq(m);
    Piece pc = pos.moved_piece(m);
    PieceType pt = type_of(pc);
    // Quiet pawn moves
    if ((n == NOTATION_SAN || n == NOTATION_LAN || n == NOTATION_THAI_SAN) && type_of(pc) == PAWN && type_of(m) != DROP)
        return "";
    // Tandem pawns
    else if (n == NOTATION_XIANGQI_WXF && popcount(pos.pieces(us, pt) & file_bb(from)) >= 3 - multi_tandem(pos.pieces(us, pt)))
        return std::to_string(popcount(forward_file_bb(us, from) & pos.pieces(us, pt)) + 1);
    // Moves of promoted pieces
    else if (is_shogi(n) && type_of(m) != DROP && pos.unpromoted_piece_on(from))
        return "+" + std::string(1, toupper(pos.piece_to_char()[pos.unpromoted_piece_on(from)]));
    // Promoted drops
    else if (is_shogi(n) && type_of(m) == DROP && dropped_piece_type(m) != in_hand_piece_type(m))
        return "+" + std::string(1, toupper(pos.piece_to_char()[in_hand_piece_type(m)]));
    else if (is_thai(n))
        return piece_to_thai_char(pc, pos.is_promoted(from));
    else if (pos.piece_to_char_synonyms()[pc] != ' ')
        return std::string(1, toupper(pos.piece_to_char_synonyms()[pc]));
    else
        return std::string(1, toupper(pos.piece_to_char()[pc]));
}

inline std::string file(const Position& pos, Square s, Notation n) {
    switch (n)
    {
    case NOTATION_SHOGI_HOSKING:
    case NOTATION_SHOGI_HODGES:
    case NOTATION_SHOGI_HODGES_NUMBER:
        return std::to_string(pos.max_file() - file_of(s) + 1);
    case NOTATION_JANGGI:
        return std::to_string(file_of(s) + 1);
    case NOTATION_XIANGQI_WXF:
        return std::to_string((pos.side_to_move() == WHITE ? pos.max_file() - file_of(s) : file_of(s)) + 1);
    case NOTATION_THAI_SAN:
    case NOTATION_THAI_LAN:
        return THAI_FILES[file_of(s)];
    default:
        return std::string(1, char('a' + file_of(s)));
    }
}

inline std::string rank(const Position& pos, Square s, Notation n) {
    switch (n)
    {
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
            // Handle piece drops
            return std::to_string(relative_rank(pos.side_to_move(), s, pos.max_rank()) + 1);
        else if (pos.pieces(pos.side_to_move(), type_of(pos.piece_on(s))) & forward_file_bb(pos.side_to_move(), s))
            return "-";
        else
            return "+";
    }
    case NOTATION_THAI_SAN:
    case NOTATION_THAI_LAN:
        return THAI_RANKS[rank_of(s)];
    default:
        return std::to_string(rank_of(s) + 1);
    }
}

inline std::string square(const Position& pos, Square s, Notation n) {
    switch (n)
    {
    case NOTATION_JANGGI:
        return rank(pos, s, n) + file(pos, s, n);
    default:
        return file(pos, s, n) + rank(pos, s, n);
    }
}

inline Disambiguation disambiguation_level(const Position& pos, Move m, Notation n) {
    // Drops never need disambiguation
    if (type_of(m) == DROP)
        return NO_DISAMBIGUATION;

    // NOTATION_LAN and Janggi always use disambiguation
    if (n == NOTATION_LAN || n == NOTATION_THAI_LAN || n == NOTATION_JANGGI)
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
        if (popcount(pos.pieces(us, pt) & file_bb(from)) == 2 && !multi_tandem(pos.pieces(us, pt)))
        {
            Square otherFrom = lsb((pos.pieces(us, pt) & file_bb(from)) ^ from);
            Square otherTo = otherFrom + Direction(to) - Direction(from);
            if (is_ok(otherTo) && (pos.board_bb(us, pt) & otherTo))
                return RANK_DISAMBIGUATION;
        }
        return FILE_DISAMBIGUATION;
    }

    // Pawn captures always use disambiguation
    if ((n == NOTATION_SAN || n == NOTATION_THAI_SAN) && pt == PAWN)
    {
        if (pos.capture(m))
            return FILE_DISAMBIGUATION;
        if (type_of(m) == PROMOTION && from != to && pos.sittuyin_promotion())
            return SQUARE_DISAMBIGUATION;
    }

    // A disambiguation occurs if we have more than one piece of type 'pt'
    // that can reach 'to' with a legal move.
    Bitboard b = pos.pieces(us, pt) ^ from;
    Bitboard others = 0;

    while (b)
    {
        Square s = pop_lsb(b);
        // Construct a potential move with identical special move flags
        // and only a different "from" square.
        Move testMove = Move(m ^ make_move(from, to) ^ make_move(s, to));
        if (      pos.pseudo_legal(testMove)
               && pos.legal(testMove)
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

inline std::string disambiguation(const Position& pos, Square s, Notation n, Disambiguation d) {
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

inline const std::string move_to_san(Position& pos, Move m, Notation n) {
    std::string san = "";
    Color us = pos.side_to_move();
    Square from = from_sq(m);
    Square to = to_sq(m);

    if (type_of(m) == CASTLING)
    {
        san = to > from ? "O-O" : "O-O-O";

        if (is_gating(m))
        {
            san += std::string("/") + (char)toupper(pos.piece_to_char()[make_piece(us, gating_type(m))]);
            san += square(pos, gating_square(m), n);
        }
    }
    else
    {
        // Piece
        san += piece(pos, m, n);

        if (n == NOTATION_THAI_LAN)
            san += " ";

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
        else if (n == NOTATION_LAN || n == NOTATION_THAI_LAN || (is_shogi(n) && (n != NOTATION_SHOGI_HOSKING || d == SQUARE_DISAMBIGUATION)) || n == NOTATION_JANGGI || (n == NOTATION_THAI_SAN && type_of(pos.moved_piece(m)) != PAWN))
            san += '-';

        // Destination square
        if (n == NOTATION_XIANGQI_WXF && type_of(m) != DROP)
            san += file_of(to) == file_of(from) ? std::to_string(std::abs(rank_of(to) - rank_of(from))) : file(pos, to, n);
        else
            san += square(pos, to, n);

        // Suffix
        if (type_of(m) == PROMOTION)
            san += std::string("=") + (char)toupper(pos.piece_to_char()[make_piece(us, promotion_type(m))]);
        else if (type_of(m) == PIECE_PROMOTION)
            san += is_shogi(n) ? std::string("+") : std::string("=") + (char)toupper(pos.piece_to_char()[make_piece(us, pos.promoted_piece_type(type_of(pos.moved_piece(m))))]);
        else if (type_of(m) == PIECE_DEMOTION)
            san += is_shogi(n) ? std::string("-") : std::string("=") + std::string(1, toupper(pos.piece_to_char()[pos.unpromoted_piece_on(from)]));
        else if (type_of(m) == NORMAL && is_shogi(n) && pos.pseudo_legal(make<PIECE_PROMOTION>(from, to)))
            san += std::string("=");
        if (is_gating(m))
            san += std::string("/") + (char)toupper(pos.piece_to_char()[make_piece(us, gating_type(m))]);
    }

    // Wall square
    if (pos.walling())
        san += "," + square(pos, gating_square(m), n);

    // Check and checkmate
    if (pos.gives_check(m) && !is_shogi(n) && n != NOTATION_XIANGQI_WXF)
    {
        StateInfo st;
        pos.do_move(m, st);
        san += MoveList<LEGAL>(pos).size() ? "+" : "#";
        pos.undo_move(m);
    }

    return san;
}

} // namespace SAN

inline bool has_insufficient_material(Color c, const Position& pos) {

    // Other win rules
    if (   pos.captures_to_hand()
        || pos.count_in_hand(c, ALL_PIECES)
        || (pos.extinction_value() != VALUE_NONE && !pos.extinction_pseudo_royal())
        || (pos.flag_region(c) && pos.count(c, pos.flag_piece(c))))
        return false;

    // Restricted pieces
    Bitboard restricted = pos.pieces(~c, KING);
    // Atomic kings can not help checkmating
    if (pos.extinction_pseudo_royal() && pos.blast_on_capture() && (pos.extinction_piece_types() & COMMONER))
        restricted |= pos.pieces(c, COMMONER);
    for (PieceSet ps = pos.piece_types(); ps;)
    {
        PieceType pt = pop_lsb(ps);
        if (pt == KING || !(pos.board_bb(c, pt) & pos.board_bb(~c, KING)))
            restricted |= pos.pieces(c, pt);
        else if (is_custom(pt) && pos.count(c, pt) > 0)
            // to be conservative, assume any custom piece has mating potential
            return false;
    }

    // Mating pieces
    for (PieceType pt : { ROOK, QUEEN, ARCHBISHOP, CHANCELLOR, SILVER, GOLD, COMMONER, CENTAUR })
        if ((pos.pieces(c, pt) & ~restricted) || (pos.count(c, pos.promotion_pawn_type(c)) && (pos.promotion_piece_types(c) & pt)))
            return false;

    // Color-bound pieces
    Bitboard colorbound = 0, unbound;
    for (PieceType pt : { BISHOP, FERS, FERS_ALFIL, ALFIL, ELEPHANT })
        colorbound |= pos.pieces(pt) & ~restricted;
    unbound = pos.pieces() ^ restricted ^ colorbound;
    if ((colorbound & pos.pieces(c)) && (((DarkSquares & colorbound) && (~DarkSquares & colorbound)) || unbound || pos.stalemate_value() != VALUE_DRAW || pos.check_counting() || pos.makpong()))
        return false;

    // Unbound pieces require one helper piece of either color
    if ((pos.pieces(c) & unbound) && (popcount(pos.pieces() ^ restricted) >= 2 || pos.stalemate_value() != VALUE_DRAW || pos.check_counting() || pos.makpong()))
        return false;

    // Non-draw stalemate with lone custom king
    if (   pos.stalemate_value() != VALUE_DRAW && pos.king_type() != KING
        && pos.pieces(c, KING) && (pos.board_bb(c, KING) & pos.board_bb(~c, KING)))
        return false;

    return true;
}

inline Bitboard checked(const Position& pos) {
    return (pos.checkers() ? square_bb(pos.square<KING>(pos.side_to_move())) : Bitboard(0))
        | (pos.extinction_pseudo_royal() ? pos.checked_pseudo_royals(pos.side_to_move()) : Bitboard(0));
}

namespace FEN {

enum FenValidation : int {
    FEN_INVALID_COUNTING_RULE = -14,
    FEN_INVALID_CHECK_COUNT = -13,
    FEN_INVALID_NB_PARTS = -11,
    FEN_INVALID_CHAR = -10,
    FEN_TOUCHING_KINGS = -9,
    FEN_INVALID_BOARD_GEOMETRY = -8,
    FEN_INVALID_POCKET_INFO = -7,
    FEN_INVALID_SIDE_TO_MOVE = -6,
    FEN_INVALID_CASTLING_INFO = -5,
    FEN_INVALID_EN_PASSANT_SQ = -4,
    FEN_INVALID_NUMBER_OF_KINGS = -3,
    FEN_INVALID_HALF_MOVE_COUNTER = -2,
    FEN_INVALID_MOVE_COUNTER = -1,
    FEN_EMPTY = 0,
    FEN_OK = 1
};
enum Validation : int {
    NOK,
    OK
};

struct CharSquare {
    int rowIdx;
    int fileIdx;
    CharSquare() : rowIdx(-1), fileIdx(-1) {}
    CharSquare(int row, int file) : rowIdx(row), fileIdx(file) {}
};

inline bool operator==(const CharSquare& s1, const CharSquare& s2) {
    return s1.rowIdx == s2.rowIdx && s1.fileIdx == s2.fileIdx;
}

inline bool operator!=(const CharSquare& s1, const CharSquare& s2) {
    return !(s1 == s2);
}

inline int non_root_euclidian_distance(const CharSquare& s1, const CharSquare& s2) {
    return pow(s1.rowIdx - s2.rowIdx, 2) + pow(s1.fileIdx - s2.fileIdx, 2);
}

class CharBoard {
private:
    int nbRanks;
    int nbFiles;
    std::vector<char> board;  // fill an array where the pieces are for later geometry checks
public:
    CharBoard(int ranks, int files) : nbRanks(ranks), nbFiles(files) {
        assert(nbFiles > 0 && nbRanks > 0);
        board = std::vector<char>(nbRanks * nbFiles, ' ');
    }
    void set_piece(int rankIdx, int fileIdx, char c) {
        board[rankIdx * nbFiles + fileIdx] = c;
    }
    char get_piece(int rowIdx, int fileIdx) const {
        return board[rowIdx * nbFiles + fileIdx];
    }
    int get_nb_ranks() const {
        return nbRanks;
    }
    int get_nb_files() const {
        return nbFiles;
    }
    /// Returns the square of a given character
    CharSquare get_square_for_piece(char piece) const {
        CharSquare s;
        for (int r = 0; r < nbRanks; ++r)
        {
            for (int c = 0; c < nbFiles; ++c)
            {
                if (get_piece(r, c) == piece)
                {
                    s.rowIdx = r;
                    s.fileIdx = c;
                    return s;
                }
            }
        }
        return s;
    }
    /// Returns all square positions for a given piece
    std::vector<CharSquare> get_squares_for_pieces(Color color, PieceSet ps, const std::string& pieceChars) const {
        std::vector<CharSquare> squares;
        size_t pcIdx;
        for (int r = 0; r < nbRanks; ++r)
            for (int c = 0; c < nbFiles; ++c)
                if ((pcIdx = pieceChars.find(get_piece(r, c))) != std::string::npos && (ps & type_of(Piece(pcIdx))) && color_of(Piece(pcIdx)) == color)
                    squares.emplace_back(CharSquare(r, c));
        return squares;
    }
    friend std::ostream& operator<<(std::ostream& os, const CharBoard& board);
};

inline std::ostream& operator<<(std::ostream& os, const CharBoard& board) {
    for (int r = 0; r < board.nbRanks; ++r)
    {
        for (int c = 0; c < board.nbFiles; ++c)
            os << "[" << board.get_piece(r, c) << "] ";
        os << std::endl;
    }
    return os;
}

inline bool contains(const std::string& str, char c) {
    return str.find(c) != std::string::npos;
}

inline bool in_any(const std::vector<std::string>& vec, char c) {
    for (std::string str : vec)
        if (contains(str, c))
            return true;
    return false;
}

inline Validation check_for_valid_characters(const std::string& firstFenPart, const std::string& validSpecialCharactersFirstField, const Variant* v) {
    for (char c : firstFenPart)
    {
        if (!isdigit(c) && !in_any({v->pieceToChar, v->pieceToCharSynonyms, validSpecialCharactersFirstField}, c))
        {
            std::cerr << "Invalid piece character: '" << c << "'." << std::endl;
            return NOK;
        }
    }
    return OK;
}

inline std::vector<std::string> get_fen_parts(const std::string& fullFen, char delim) {
    std::vector<std::string> fenParts;
    std::string curPart;
    std::stringstream ss(fullFen);
    while (std::getline(ss, curPart, delim))
        fenParts.emplace_back(curPart);
    return fenParts;
}

/// fills the character board according to a given FEN string
inline Validation fill_char_board(CharBoard& board, const std::string& fenBoard, const std::string& validSpecialCharactersFirstField, const Variant* v) {
    int rankIdx = 0;
    int fileIdx = 0;

    char prevChar = '?';
    for (char c : fenBoard)
    {
        if (c == ' ' || c == '[')
            break;
        if (c == '*')
            ++fileIdx;
        else if (isdigit(c))
        {
            fileIdx += c - '0';
            // if we have multiple digits attached we can add multiples of 9 to compute the resulting number (e.g. -> 21 = 2 + 2 * 9 + 1)
            if (isdigit(prevChar))
                fileIdx += 9 * (prevChar - '0');
        }
        else if (c == '/')
        {
            ++rankIdx;
            if (fileIdx != board.get_nb_files())
            {
                std::cerr << "curRankWidth != nbFiles: " << fileIdx << " != " << board.get_nb_files() << std::endl;
                return NOK;
            }
            if (rankIdx == board.get_nb_ranks())
                break;
            fileIdx = 0;
        }
        else if (!contains(validSpecialCharactersFirstField, c))
        {  // normal piece
            if (fileIdx == board.get_nb_files())
            {
                std::cerr << "File index: " << fileIdx << " for piece '" << c << "' exceeds maximum of allowed number of files: " << board.get_nb_files() << "." << std::endl;
                return NOK;
            }
            board.set_piece(v->maxRank-rankIdx, fileIdx, c);  // we mirror the rank index because the black pieces are given first in the FEN
            ++fileIdx;
        }
        prevChar = c;
    }

    if (v->pieceDrops)
    { // pockets can either be defined by [] or /
        if (rankIdx+1 != board.get_nb_ranks() && rankIdx != board.get_nb_ranks())
        {
            std::cerr << "Invalid number of ranks. Expected: " << board.get_nb_ranks() << " Actual: " << rankIdx+1 << std::endl;
            return NOK;
        }
    }
    else
    {
        if (rankIdx+1 != board.get_nb_ranks())
        {
            std::cerr << "Invalid number of ranks. Expected: " << board.get_nb_ranks() << " Actual: " << rankIdx+1 << std::endl;
            return NOK;
        }
    }
    return OK;
}

inline Validation check_touching_kings(const CharBoard& board, const std::array<CharSquare, 2>& kingPositions) {
    if (non_root_euclidian_distance(kingPositions[WHITE], kingPositions[BLACK]) <= 2)
    {
        std::cerr << "King pieces are next to each other." << std::endl;
        std::cerr << board << std::endl;
        return NOK;
    }
    return OK;
}

inline Validation fill_castling_info_splitted(const std::string& castlingInfo, std::array<std::string, 2>& castlingInfoSplitted) {
    for (char c : castlingInfo)
    {
        if (c != '-')
        {
            if (!isalpha(c))
            {
                std::cerr << "Invalid castling specification: '" << c << "'." << std::endl;
                return NOK;
            }
            else if (isupper(c))
                castlingInfoSplitted[WHITE] += tolower(c);
            else
                castlingInfoSplitted[BLACK] += c;
        }
    }
    return OK;
}

inline std::string color_to_string(Color c) {
    switch (c)
    {
    case WHITE:
        return "WHITE";
    case BLACK:
        return "BLACK";
    case COLOR_NB:
        return "COLOR_NB";
    default:
        return "INVALID_COLOR";
    }
}

inline std::string castling_rights_to_string(CastlingRights castlingRights) {
    switch (castlingRights)
    {
    case KING_SIDE:
        return "KING_SIDE";
    case QUEEN_SIDE:
        return "QUEENS_SIDE";
    case WHITE_OO:
        return "WHITE_OO";
    case WHITE_OOO:
        return "WHITE_OOO";
    case BLACK_OO:
        return "BLACK_OO";
    case BLACK_OOO:
        return "BLACK_OOO";
    case WHITE_CASTLING:
        return "WHITE_CASTLING";
    case BLACK_CASTLING:
        return "BLACK_CASTLING";
    case ANY_CASTLING:
        return "ANY_CASTLING";
    case CASTLING_RIGHT_NB:
        return "CASTLING_RIGHT_NB";
    default:
        return "INVALID_CASTLING_RIGHTS";
    }
}

inline Validation check_castling_rank(const std::array<std::string, 2>& castlingInfoSplitted, const CharBoard& board,
                            const std::array<CharSquare, 2>& kingPositions, const Variant* v) {

    for (Color c : {WHITE, BLACK})
    {
        const Rank castlingRank = relative_rank(c, v->castlingRank, v->maxRank);
        for (char castlingFlag : castlingInfoSplitted[c])
        {
            if (tolower(castlingFlag) == 'k' || tolower(castlingFlag) == 'q')
            {
                if (kingPositions[c].rowIdx != castlingRank)
                {
                    std::cerr << "The " << color_to_string(c) << " king must be on rank " << castlingRank << " if castling is enabled for " << color_to_string(c) << "." << std::endl;
                    return NOK;
                }
                bool kingside = tolower(castlingFlag) == 'k';
                bool castlingRook = false;
                size_t pcIdx;
                for (int f = kingside ? board.get_nb_files() - 1 : 0; f != kingPositions[c].fileIdx; kingside ? f-- : f++)
                    if (   (pcIdx = v->pieceToChar.find(board.get_piece(castlingRank, f))) != std::string::npos
                        && (v->castlingRookPieces[c] & type_of(Piece(pcIdx)))
                        && color_of(Piece(pcIdx)) == c)
                    {
                        castlingRook = true;
                        break;
                    }
                if (!castlingRook)
                {
                    std::cerr << "No castling rook for flag " << castlingFlag << std::endl;
                    return NOK;
                }
            }
            else if (board.get_piece(castlingRank, tolower(castlingFlag) - 'a') == ' ')
            {
                std::cerr << "No gating piece for flag " << castlingFlag << std::endl;
                return NOK;
            }
        }
    }
    return OK;
}

inline Validation check_standard_castling(std::array<std::string, 2>& castlingInfoSplitted, const CharBoard& board,
                             const std::array<CharSquare, 2>& kingPositions, const std::array<CharSquare, 2>& kingPositionsStart,
                             const std::array<std::vector<CharSquare>, 2>& rookPositionsStart, const Variant* v) {

    for (Color c : {WHITE, BLACK})
    {
        if (castlingInfoSplitted[c].size() == 0)
            continue;
        if (kingPositions[c] != kingPositionsStart[c])
        {
            std::cerr << "The " << color_to_string(c) << " KING has moved. Castling is no longer valid for " << color_to_string(c) << "." << std::endl;
            return NOK;
        }

        for (CastlingRights castling: {KING_SIDE, QUEEN_SIDE})
        {
            CharSquare rookStartingSquare = castling == QUEEN_SIDE ? rookPositionsStart[c][0] : rookPositionsStart[c][1];
            char targetChar = castling == QUEEN_SIDE ? 'q' : 'k';
            size_t pcIdx;
            if (castlingInfoSplitted[c].find(targetChar) != std::string::npos)
            {
                if (   (pcIdx = v->pieceToChar.find(board.get_piece(rookStartingSquare.rowIdx, rookStartingSquare.fileIdx))) == std::string::npos
                    || !(v->castlingRookPieces[c] & type_of(Piece(pcIdx)))
                    || color_of(Piece(pcIdx)) != c)
                {
                    std::cerr << "The " << color_to_string(c) << " ROOK on the "<<  castling_rights_to_string(castling) << " has moved. "
                              << castling_rights_to_string(castling) << " castling is no longer valid for " << color_to_string(c) << "." << std::endl;
                    return NOK;
                }
            }

        }
    }
    return OK;
}

inline Validation check_pocket_info(const std::string& fenBoard, int nbRanks, const Variant* v, std::string& pocket) {

    char stopChar;
    int offset = 0;
    if (std::count(fenBoard.begin(), fenBoard.end(), '/') == nbRanks)
    {
        // look for last '/'
        stopChar = '/';
    }
    else if (std::count(fenBoard.begin(), fenBoard.end(), '[') == 1)
    {
        // pocket is defined as [ and ]
        stopChar = '[';
        offset = 1;
        if (*(fenBoard.end()-1) != ']')
        {
            std::cerr << "Pocket specification does not end with ']'." << std::endl;
            return NOK;
        }
    }
    else
        // allow to skip pocket
        return OK;

    // look for last '/'
    for (auto it = fenBoard.rbegin()+offset; it != fenBoard.rend(); ++it)
    {
        const char c = *it;
        if (c == stopChar)
            return OK;
        if (c != '-')
        {
            if (!in_any({v->pieceToChar, v->pieceToCharSynonyms}, c))
            {
                std::cerr << "Invalid pocket piece: '" << c << "'." << std::endl;
                return NOK;
            }
            else
                pocket += c;
        }
    }
    std::cerr << "Pocket piece closing character '" << stopChar << "' was not found." << std::endl;
    return NOK;
}

inline int piece_count(const std::string& fenBoard, Color c, PieceType pt, const Variant* v) {
    return std::count(fenBoard.begin(), fenBoard.end(), v->pieceToChar[make_piece(c, pt)]);
}

inline Validation check_number_of_kings(const std::string& fenBoard, const std::string& startFenBoard, const Variant* v) {
    int nbWhiteKings = piece_count(fenBoard, WHITE, KING, v);
    int nbBlackKings = piece_count(fenBoard, BLACK, KING, v);
    int nbWhiteKingsStart = piece_count(startFenBoard, WHITE, KING, v);
    int nbBlackKingsStart = piece_count(startFenBoard, BLACK, KING, v);

    if (nbWhiteKings > 1)
    {
        std::cerr << "Invalid number of white kings. Maximum: 1. Given: " << nbWhiteKings << std::endl;
        return NOK;
    }
    if (nbBlackKings > 1)
    {
        std::cerr << "Invalid number of black kings. Maximum: 1. Given: " << nbBlackKings << std::endl;
        return NOK;
    }
    if (nbWhiteKings != nbWhiteKingsStart)
    {
        std::cerr << "Invalid number of white kings. Expected: " << nbWhiteKingsStart << ". Given: " << nbWhiteKings << std::endl;
        return NOK;
    }
    if (nbBlackKings != nbBlackKingsStart)
    {
        std::cerr << "Invalid number of black kings. Expected: " << nbBlackKingsStart << ". Given: " << nbBlackKings << std::endl;
        return NOK;
    }
    return OK;
}


inline Validation check_en_passant_square(const std::string& enPassantInfo) {
    if (enPassantInfo.size() != 1 || enPassantInfo[0] != '-')
    {
        if (enPassantInfo.size() != 2)
        {
            std::cerr << "Invalid en-passant square '" << enPassantInfo << "'. Expects 2 characters. Actual: " << enPassantInfo.size() << " character(s)." << std::endl;
            return NOK;
        }
        if (!isalpha(enPassantInfo[0]))
        {
            std::cerr << "Invalid en-passant square '" << enPassantInfo << "'. Expects 1st character to be a letter." << std::endl;
            return NOK;
        }
        if (!isdigit(enPassantInfo[1]))
        {
            std::cerr << "Invalid en-passant square '" << enPassantInfo << "'. Expects 2nd character to be a digit." << std::endl;
            return NOK;
        }
    }
    return OK;
}


inline Validation check_check_count(const std::string& checkCountInfo) {
    if (checkCountInfo.size() != 3)
    {
        std::cerr << "Invalid check count '" << checkCountInfo << "'. Expects 3 characters. Actual: " << checkCountInfo.size() << " character(s)." << std::endl;
        return NOK;
    }
    if (!isdigit(checkCountInfo[0]))
    {
        std::cerr << "Invalid check count '" << checkCountInfo << "'. Expects 1st character to be a digit." << std::endl;
        return NOK;
    }
    if (!isdigit(checkCountInfo[2])) {
        std::cerr << "Invalid check count '" << checkCountInfo << "'. Expects 3rd character to be a digit." << std::endl;
        return NOK;
    }
    return OK;
}

inline Validation check_lichess_check_count(const std::string& checkCountInfo) {
    if (checkCountInfo.size() != 4)
    {
        std::cerr << "Invalid check count '" << checkCountInfo << "'. Expects 4 characters. Actual: " << checkCountInfo.size() << " character(s)." << std::endl;
        return NOK;
    }
    if (!isdigit(checkCountInfo[1]) || checkCountInfo[1] - '0' > 3)
    {
        std::cerr << "Invalid check count '" << checkCountInfo << "'. Expects 2nd character to be a digit up to 3." << std::endl;
        return NOK;
    }
    if (!isdigit(checkCountInfo[3]) || checkCountInfo[3] - '0' > 3) {
        std::cerr << "Invalid check count '" << checkCountInfo << "'. Expects 4th character to be a digit up to 3." << std::endl;
        return NOK;
    }
    return OK;
}

inline Validation check_digit_field(const std::string& field) {
    if (field.size() == 1 && field[0] == '-')
        return OK;
    for (char c : field)
        if (!isdigit(c))
            return NOK;
    return OK;
}

inline std::string get_valid_special_chars(const Variant* v) {
    std::string validSpecialCharactersFirstField = "/*";
    // Whether or not '-', '+', '~', '[', ']' are valid depends on the variant being played.
    if (v->shogiStylePromotions)
        validSpecialCharactersFirstField += '+';
    if (v->promotionPieceTypes[WHITE] || v->promotionPieceTypes[BLACK])
        validSpecialCharactersFirstField += '~';
    if (!v->freeDrops && (v->pieceDrops || v->seirawanGating))
        validSpecialCharactersFirstField += "[-]";
    return validSpecialCharactersFirstField;
}

inline FenValidation validate_fen(const std::string& fen, const Variant* v, bool chess960 = false) {

    const std::string validSpecialCharactersFirstField = get_valid_special_chars(v);
    // 0) Layout
    // check for empty fen
    if (fen.empty())
    {
        std::cerr << "Fen is empty." << std::endl;
        return FEN_EMPTY;
    }

    std::vector<std::string> fenParts = get_fen_parts(fen, ' ');
    std::vector<std::string> startFenParts = get_fen_parts(v->startFen, ' ');

    // check for number of parts
    const unsigned int maxNumberFenParts = 6 + v->checkCounting;
    if (fenParts.size() < 1 || fenParts.size() > maxNumberFenParts)
    {
        std::cerr << "Invalid number of fen parts. Expected: >= 1 and <= " << maxNumberFenParts
                  << " Actual: " << fenParts.size() << std::endl;
        return FEN_INVALID_NB_PARTS;
    }

    // 1) Part
    // check for valid characters
    if (check_for_valid_characters(fenParts[0], validSpecialCharactersFirstField, v) == NOK)
        return FEN_INVALID_CHAR;

    // check for number of ranks
    const int nbRanks = v->maxRank + 1;
    // check for number of files
    const int nbFiles = v->maxFile + 1;
    CharBoard board(nbRanks, nbFiles);  // create a 2D character board for later geometry checks

    if (fill_char_board(board, fenParts[0], validSpecialCharactersFirstField, v) == NOK)
        return FEN_INVALID_BOARD_GEOMETRY;

    // check for pocket
    std::string pocket = "";
    if (v->pieceDrops || v->seirawanGating)
    {
        if (check_pocket_info(fenParts[0], nbRanks, v, pocket) == NOK)
            return FEN_INVALID_POCKET_INFO;
    }

    // check for number of kings
    if (v->pieceTypes & KING)
    {
        // we have a royal king in this variant,
        // ensure that each side has exactly as many kings as in the starting position
        // (variants like giveaway use the COMMONER piece type instead)
        if (check_number_of_kings(fenParts[0], startFenParts[0], v) == NOK)
            return FEN_INVALID_NUMBER_OF_KINGS;

        // check for touching kings if there are exactly two royal kings on the board (excluding pocket)
        if (   v->kingType == KING
            && piece_count(fenParts[0], WHITE, KING, v) - piece_count(pocket, WHITE, KING, v) == 1
            && piece_count(fenParts[0], BLACK, KING, v) - piece_count(pocket, BLACK, KING, v) == 1)
        {
            std::array<CharSquare, 2> kingPositions;
            kingPositions[WHITE] = board.get_square_for_piece(v->pieceToChar[make_piece(WHITE, KING)]);
            kingPositions[BLACK] = board.get_square_for_piece(v->pieceToChar[make_piece(BLACK, KING)]);
            if (check_touching_kings(board, kingPositions) == NOK)
                return FEN_TOUCHING_KINGS;
        }
    }

    // 2) Part
    // check side to move char
    if (fenParts.size() >= 2 && fenParts[1][0] != 'w' && fenParts[1][0] != 'b')
    {
        std::cerr << "Invalid side to move specification: '" << fenParts[1][0] << "'." << std::endl;
        return FEN_INVALID_SIDE_TO_MOVE;
    }

    // Castling and en passant can be skipped
    bool skipCastlingAndEp = fenParts.size() >= 4 && fenParts.size() <= 5 && isdigit(fenParts[2][0]);

    // 3) Part
    // check castling rights
    if (fenParts.size() >= 3 && !skipCastlingAndEp && v->castling)
    {
        std::array<std::string, 2> castlingInfoSplitted;
        if (fill_castling_info_splitted(fenParts[2], castlingInfoSplitted) == NOK)
            return FEN_INVALID_CASTLING_INFO;

        if (castlingInfoSplitted[WHITE].size() != 0 || castlingInfoSplitted[BLACK].size() != 0)
        {
            std::array<CharSquare, 2> kingPositions;
            kingPositions[WHITE] = board.get_square_for_piece(toupper(v->pieceToChar[v->castlingKingPiece[WHITE]]));
            kingPositions[BLACK] = board.get_square_for_piece(tolower(v->pieceToChar[v->castlingKingPiece[BLACK]]));

            CharBoard startBoard(board.get_nb_ranks(), board.get_nb_files());
            fill_char_board(startBoard, v->startFen, validSpecialCharactersFirstField, v);

            // Check pieces present on castling rank against castling/gating rights
            if (check_castling_rank(castlingInfoSplitted, board, kingPositions, v) == NOK)
                return FEN_INVALID_CASTLING_INFO;

            // only check exact squares if starting position of castling pieces is known
            if (!v->chess960 && !v->castlingDroppedPiece && !chess960)
            {
                std::array<CharSquare, 2> kingPositionsStart;
                kingPositionsStart[WHITE] = startBoard.get_square_for_piece(v->pieceToChar[make_piece(WHITE, v->castlingKingPiece[WHITE])]);
                kingPositionsStart[BLACK] = startBoard.get_square_for_piece(v->pieceToChar[make_piece(BLACK, v->castlingKingPiece[BLACK])]);
                std::array<std::vector<CharSquare>, 2> rookPositionsStart;
                rookPositionsStart[WHITE] = startBoard.get_squares_for_pieces(WHITE, v->castlingRookPieces[WHITE], v->pieceToChar);
                rookPositionsStart[BLACK] = startBoard.get_squares_for_pieces(BLACK, v->castlingRookPieces[BLACK], v->pieceToChar);

                if (check_standard_castling(castlingInfoSplitted, board, kingPositions, kingPositionsStart, rookPositionsStart, v) == NOK)
                    return FEN_INVALID_CASTLING_INFO;
            }
        }
    }

    // 4) Part
    // check en-passant square
    if (fenParts.size() >= 4 && !skipCastlingAndEp)
    {
        if (v->doubleStep && (v->pieceTypes & PAWN))
        {
            if (check_en_passant_square(fenParts[3]) == NOK)
                return FEN_INVALID_EN_PASSANT_SQ;
        }
        else if (v->countingRule && !check_digit_field(fenParts[3]))
            return FEN_INVALID_COUNTING_RULE;
    }

    // 5) Part
    // check check count
    unsigned int optionalInbetweenFields = 2 * !skipCastlingAndEp;
    unsigned int optionalTrailingFields = 0;
    if (fenParts.size() >= 3 + optionalInbetweenFields && v->checkCounting && fenParts.size() % 2)
    {
        if (check_check_count(fenParts[2 + optionalInbetweenFields]) == NOK)
        {
            // allow valid lichess style check as alternative
            if (fenParts.size() < 5 + optionalInbetweenFields || check_lichess_check_count(fenParts[fenParts.size() - 1]) == NOK)
                return FEN_INVALID_CHECK_COUNT;
            else
                optionalTrailingFields++;
        }
        else
            optionalInbetweenFields++;
    }

    // 6) Part
    // check half move counter
    if (fenParts.size() >= 3 + optionalInbetweenFields && !check_digit_field(fenParts[fenParts.size() - 2 - optionalTrailingFields]))
    {
        std::cerr << "Invalid half move counter: '" << fenParts[fenParts.size()-2] << "'." << std::endl;
        return FEN_INVALID_HALF_MOVE_COUNTER;
    }

    // 7) Part
    // check move counter
    if (fenParts.size() >= 4 + optionalInbetweenFields && !check_digit_field(fenParts[fenParts.size() - 1 - optionalTrailingFields]))
    {
        std::cerr << "Invalid move counter: '" << fenParts[fenParts.size()-1] << "'." << std::endl;
        return FEN_INVALID_MOVE_COUNTER;
    }

    return FEN_OK;
}
} // namespace FEN

} // namespace Stockfish

#endif // #ifndef APIUTIL_H_INCLUDED
