/*
  Based on Jean-Francois Romang work
  https://github.com/jromang/Stockfish/blob/pyfish/src/pyfish.cpp
*/

#include <Python.h>

#include "misc.h"
#include "types.h"
#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "syzygy/tbprobe.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "piece.h"
#include "variant.h"

static PyObject* PyFFishError;

namespace PSQT {
  void init(const Variant* v);
}

namespace
{

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
    Bitboard others, b;
    others = b = ((pos.capture(m) ? attacks_bb(~us, pt, to, AttackRiderTypes[pt] & ASYMMETRICAL_RIDERS ? Bitboard(0) : pos.pieces())
                                  : moves_bb(  ~us, pt, to, MoveRiderTypes[pt] & ASYMMETRICAL_RIDERS ? Bitboard(0) : pos.pieces()))
                & pos.pieces(us, pt)) & ~square_bb(from);

    while (b)
    {
        Square s = pop_lsb(&b);
        if (   !pos.pseudo_legal(make_move(s, to))
            || !pos.legal(make_move(s, to))
            || (is_shogi(n) && pos.unpromoted_piece_on(s) != pos.unpromoted_piece_on(from)))
            others ^= s;
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

void buildPosition(Position& pos, StateListPtr& states, const char *variant, const char *fen, PyObject *moveList, const bool chess960) {
    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one

    const Variant* v = variants.find(std::string(variant))->second;
    if (strcmp(fen, "startpos") == 0)
        fen = v->startFen.c_str();
    Options["UCI_Chess960"] = chess960;
    pos.set(v, std::string(fen), chess960, &states->back(), Threads.main());

    // parse move list
    int numMoves = PyList_Size(moveList);
    for (int i = 0; i < numMoves ; i++)
    {
        std::string moveStr(PyBytes_AS_STRING(PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict")));
        Move m;
        if ((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            // do the move
            states->emplace_back();
            pos.do_move(m, states->back());
        }
        else
            PyErr_SetString(PyExc_ValueError, (std::string("Invalid move '") + moveStr + "'").c_str());
    }
    return;
}

}

extern "C" PyObject* pyffish_info(PyObject* self) {
    return Py_BuildValue("s", engine_info().c_str());
}

// INPUT option name, option value
extern "C" PyObject* pyffish_setOption(PyObject* self, PyObject *args) {
    const char *name;
    PyObject *valueObj;
    if (!PyArg_ParseTuple(args, "sO", &name, &valueObj)) return NULL;

    if (Options.count(name))
        Options[name] = std::string(PyBytes_AS_STRING(PyUnicode_AsEncodedString(PyObject_Str(valueObj), "UTF-8", "strict")));
    else
    {
        PyErr_SetString(PyExc_ValueError, (std::string("No such option ") + name + "'").c_str());
        return NULL;
    }
    Py_RETURN_NONE;
}

// INPUT variant
extern "C" PyObject* pyffish_startFen(PyObject* self, PyObject *args) {
    const char *variant;

    if (!PyArg_ParseTuple(args, "s", &variant)) {
        return NULL;
    }

    return Py_BuildValue("s", variants.find(std::string(variant))->second->startFen.c_str());
}

// INPUT variant
extern "C" PyObject* pyffish_twoBoards(PyObject* self, PyObject *args) {
    const char *variant;

    if (!PyArg_ParseTuple(args, "s", &variant)) {
        return NULL;
    }

    return Py_BuildValue("O", variants.find(std::string(variant))->second->twoBoards ? Py_True : Py_False);
}

// INPUT variant, fen, move
extern "C" PyObject* pyffish_getSAN(PyObject* self, PyObject *args) {
    PyObject* moveList = PyList_New(0);
    Position pos;
    const char *fen, *variant, *move;

    int chess960 = false;
    Notation notation = NOTATION_DEFAULT;
    if (!PyArg_ParseTuple(args, "sss|pi", &variant, &fen,  &move, &chess960, &notation)) {
        return NULL;
    }
    if (notation == NOTATION_DEFAULT)
        notation = default_notation(variants.find(std::string(variant))->second);
    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    std::string moveStr = move;
    return Py_BuildValue("s", move_to_san(pos, UCI::to_move(pos, moveStr), notation).c_str());
}

// INPUT variant, fen, movelist
extern "C" PyObject* pyffish_getSANmoves(PyObject* self, PyObject *args) {
    PyObject* sanMoves = PyList_New(0), *moveList;
    Position pos;
    const char *fen, *variant;

    int chess960 = false;
    Notation notation = NOTATION_DEFAULT;
    if (!PyArg_ParseTuple(args, "ssO!|pi", &variant, &fen, &PyList_Type, &moveList, &chess960, &notation)) {
        return NULL;
    }
    if (notation == NOTATION_DEFAULT)
        notation = default_notation(variants.find(std::string(variant))->second);
    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, sanMoves, chess960);

    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++) {
        std::string moveStr(PyBytes_AS_STRING(PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict")));
        Move m;
        if ((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            //add to the san move list
            PyObject *move = Py_BuildValue("s", move_to_san(pos, m, notation).c_str());
            PyList_Append(sanMoves, move);
            Py_XDECREF(move);

            //do the move
            states->emplace_back();
            pos.do_move(m, states->back());
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, (std::string("Invalid move '") + moveStr + "'").c_str());
            return NULL;
        }
    }
    return sanMoves;
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_legalMoves(PyObject* self, PyObject *args) {
    PyObject* legalMoves = PyList_New(0), *moveList;
    Position pos;
    const char *fen, *variant;

    int chess960 = false;
    if (!PyArg_ParseTuple(args, "ssO!|p", &variant, &fen, &PyList_Type, &moveList, &chess960)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    for (const auto& m : MoveList<LEGAL>(pos))
    {
        PyObject *moveStr;
        moveStr = Py_BuildValue("s", UCI::move(pos, m).c_str());
        PyList_Append(legalMoves, moveStr);
        Py_XDECREF(moveStr);
    }
    return legalMoves;
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_getFEN(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;

    int chess960 = false, sfen = false, showPromoted = false, countStarted = 0;
    if (!PyArg_ParseTuple(args, "ssO!|pppi", &variant, &fen, &PyList_Type, &moveList, &chess960, &sfen, &showPromoted, &countStarted)) {
        return NULL;
    }
    countStarted = std::min<unsigned int>(countStarted, INT_MAX); // pseudo-unsigned

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    return Py_BuildValue("s", pos.fen(sfen, showPromoted, countStarted).c_str());
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_givesCheck(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    int chess960 = false;
    if (!PyArg_ParseTuple(args, "ssO!|p", &variant, &fen,  &PyList_Type, &moveList, &chess960)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    return Py_BuildValue("O", pos.checkers() ? Py_True : Py_False);
}

// INPUT variant, fen, move list
// should only be called when the move list is empty
extern "C" PyObject* pyffish_gameResult(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    bool gameEnd;
    Value result;
    int chess960 = false;
    if (!PyArg_ParseTuple(args, "ssO!|p", &variant, &fen, &PyList_Type, &moveList, &chess960)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    assert(!MoveList<LEGAL>(pos).size());
    gameEnd = pos.is_immediate_game_end(result);
    if (!gameEnd)
        result = pos.checkers() ? pos.checkmate_value() : pos.stalemate_value();

    return Py_BuildValue("i", result);
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_isImmediateGameEnd(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    bool gameEnd;
    Value result;
    int chess960 = false;
    if (!PyArg_ParseTuple(args, "ssO!|p", &variant, &fen, &PyList_Type, &moveList, &chess960)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    gameEnd = pos.is_immediate_game_end(result);
    return Py_BuildValue("(Oi)", gameEnd ? Py_True : Py_False, result);
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_isOptionalGameEnd(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    bool gameEnd;
    Value result;
    int chess960 = false, countStarted = 0;
    if (!PyArg_ParseTuple(args, "ssO!|pi", &variant, &fen, &PyList_Type, &moveList, &chess960, &countStarted)) {
        return NULL;
    }
    countStarted = std::min<unsigned int>(countStarted, INT_MAX); // pseudo-unsigned

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);
    gameEnd = pos.is_optional_game_end(result, 0, countStarted);
    return Py_BuildValue("(Oi)", gameEnd ? Py_True : Py_False, result);
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_hasInsufficientMaterial(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    int chess960 = false;
    if (!PyArg_ParseTuple(args, "ssO!|p", &variant, &fen, &PyList_Type, &moveList, &chess960)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList, chess960);

    bool wInsufficient = hasInsufficientMaterial(WHITE, pos);
    bool bInsufficient = hasInsufficientMaterial(BLACK, pos);

    return Py_BuildValue("(OO)", wInsufficient ? Py_True : Py_False, bInsufficient ? Py_True : Py_False);
}


static PyMethodDef PyFFishMethods[] = {
    {"info", (PyCFunction)pyffish_info, METH_NOARGS, "Get Stockfish version info."},
    {"set_option", (PyCFunction)pyffish_setOption, METH_VARARGS, "Set UCI option."},
    {"start_fen", (PyCFunction)pyffish_startFen, METH_VARARGS, "Get starting position FEN."},
    {"two_boards", (PyCFunction)pyffish_twoBoards, METH_VARARGS, "Checks whether the variant is played on two boards."},
    {"get_san", (PyCFunction)pyffish_getSAN, METH_VARARGS, "Get SAN move from given FEN and UCI move."},
    {"get_san_moves", (PyCFunction)pyffish_getSANmoves, METH_VARARGS, "Get SAN movelist from given FEN and UCI movelist."},
    {"legal_moves", (PyCFunction)pyffish_legalMoves, METH_VARARGS, "Get legal moves from given FEN and movelist."},
    {"get_fen", (PyCFunction)pyffish_getFEN, METH_VARARGS, "Get resulting FEN from given FEN and movelist."},
    {"gives_check", (PyCFunction)pyffish_givesCheck, METH_VARARGS, "Get check status from given FEN and movelist."},
    {"game_result", (PyCFunction)pyffish_gameResult, METH_VARARGS, "Get result from given FEN, considering variant end, checkmate, and stalemate."},
    {"is_immediate_game_end", (PyCFunction)pyffish_isImmediateGameEnd, METH_VARARGS, "Get result from given FEN if variant rules ends the game."},
    {"is_optional_game_end", (PyCFunction)pyffish_isOptionalGameEnd, METH_VARARGS, "Get result from given FEN it rules enable game end by player."},
    {"has_insufficient_material", (PyCFunction)pyffish_hasInsufficientMaterial, METH_VARARGS, "Checks for insufficient material."},
    {NULL, NULL, 0, NULL},  // sentinel
};

static PyModuleDef pyffishmodule = {
    PyModuleDef_HEAD_INIT,
    "pyffish",
    "Fairy-Stockfish extension module.",
    -1,
    PyFFishMethods,
};

PyMODINIT_FUNC PyInit_pyffish() {
    PyObject* module;

    module = PyModule_Create(&pyffishmodule);
    if (module == NULL) {
        return NULL;
    }
    PyFFishError = PyErr_NewException("pyffish.error", NULL, NULL);
    Py_INCREF(PyFFishError);
    PyModule_AddObject(module, "error", PyFFishError);

    // values
    PyModule_AddObject(module, "VALUE_MATE", PyLong_FromLong(VALUE_MATE));
    PyModule_AddObject(module, "VALUE_DRAW", PyLong_FromLong(VALUE_DRAW));

    // notations
    PyModule_AddObject(module, "NOTATION_DEFAULT", PyLong_FromLong(NOTATION_DEFAULT));
    PyModule_AddObject(module, "NOTATION_SAN", PyLong_FromLong(NOTATION_SAN));
    PyModule_AddObject(module, "NOTATION_LAN", PyLong_FromLong(NOTATION_LAN));
    PyModule_AddObject(module, "NOTATION_SHOGI_HOSKING", PyLong_FromLong(NOTATION_SHOGI_HOSKING));
    PyModule_AddObject(module, "NOTATION_SHOGI_HODGES", PyLong_FromLong(NOTATION_SHOGI_HODGES));
    PyModule_AddObject(module, "NOTATION_SHOGI_HODGES_NUMBER", PyLong_FromLong(NOTATION_SHOGI_HODGES_NUMBER));
    PyModule_AddObject(module, "NOTATION_JANGGI", PyLong_FromLong(NOTATION_JANGGI));
    PyModule_AddObject(module, "NOTATION_XIANGQI_WXF", PyLong_FromLong(NOTATION_XIANGQI_WXF));

    // initialize stockfish
    pieceMap.init();
    variants.init();
    UCI::init(Options);
    PSQT::init(variants.find(Options["UCI_Variant"])->second);
    Bitboards::init();
    Position::init();
    Bitbases::init();
    Search::init();
    Threads.set(Options["Threads"]);
    Search::clear(); // After threads are up

    return module;
};
