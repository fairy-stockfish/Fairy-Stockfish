/*
  Based on Jean-Francois Romang work
  https://github.com/jromang/Stockfish/blob/pyfish/src/pyfish.cpp
*/

#include <Python.h>
#include <sstream>

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
#include "apiutil.h"

using namespace Stockfish;

static PyObject* PyFFishError;

void buildPosition(Position& pos, StateListPtr& states, const char *variant, const char *fen, PyObject *moveList, const bool chess960) {
    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one

    const Variant* v = variants.find(std::string(variant))->second;
    UCI::init_variant(v);
    if (strcmp(fen, "startpos") == 0)
        fen = v->startFen.c_str();
    pos.set(v, std::string(fen), chess960, &states->back(), Threads.main());

    // parse move list
    int numMoves = PyList_Size(moveList);
    for (int i = 0; i < numMoves ; i++)
    {
        PyObject *MoveStr = PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict");
        std::string moveStr(PyBytes_AS_STRING(MoveStr));
        Py_XDECREF(MoveStr);
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

extern "C" PyObject* pyffish_version(PyObject* self) {
    return Py_BuildValue("(iii)", 0, 0, 73);
}

extern "C" PyObject* pyffish_info(PyObject* self) {
    return Py_BuildValue("s", engine_info().c_str());
}

extern "C" PyObject* pyffish_variants(PyObject* self, PyObject *args) {
    PyObject* varList = PyList_New(0);

    for (std::string v : variants.get_keys())
    {
        PyObject* variant = Py_BuildValue("s", v.c_str());
        PyList_Append(varList, variant);
        Py_XDECREF(variant);
    }

    PyObject* Result = Py_BuildValue("O", varList);
    Py_XDECREF(varList);
    return Result;
}

// INPUT option name, option value
extern "C" PyObject* pyffish_setOption(PyObject* self, PyObject *args) {
    const char *name;
    PyObject *valueObj;
    if (!PyArg_ParseTuple(args, "sO", &name, &valueObj)) return NULL;

    if (Options.count(name))
    {
        PyObject *Value = PyUnicode_AsEncodedString( PyObject_Str(valueObj), "UTF-8", "strict");
        Options[name] = std::string(PyBytes_AS_STRING(Value));
        Py_XDECREF(Value);
    }
    else
    {
        PyErr_SetString(PyExc_ValueError, (std::string("No such option ") + name + "'").c_str());
        return NULL;
    }
    Py_RETURN_NONE;
}

// INPUT variant config
extern "C" PyObject* pyffish_loadVariantConfig(PyObject* self, PyObject *args) {
    const char *config;
    if (!PyArg_ParseTuple(args, "s", &config))
        return NULL;
    std::stringstream ss(config);
    variants.parse_istream<false>(ss);
    Options["UCI_Variant"].set_combo(variants.get_keys());
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

    Py_XDECREF(moveList);
    return Py_BuildValue("s", SAN::move_to_san(pos, UCI::to_move(pos, moveStr), notation).c_str());
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
        PyObject *MoveStr = PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict");
        std::string moveStr(PyBytes_AS_STRING(MoveStr));
        Py_XDECREF(MoveStr);
        Move m;
        if ((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            //add to the san move list
            PyObject *move = Py_BuildValue("s", SAN::move_to_san(pos, m, notation).c_str());
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
    PyObject *Result = Py_BuildValue("O", sanMoves);  
    Py_XDECREF(sanMoves);
    return Result;
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

    PyObject *Result = Py_BuildValue("O", legalMoves);  
    Py_XDECREF(legalMoves);
    return Result;
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
    return Py_BuildValue("O", Stockfish::is_check(pos) ? Py_True : Py_False);
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

    bool wInsufficient = has_insufficient_material(WHITE, pos);
    bool bInsufficient = has_insufficient_material(BLACK, pos);

    return Py_BuildValue("(OO)", wInsufficient ? Py_True : Py_False, bInsufficient ? Py_True : Py_False);
}

// INPUT variant, fen
extern "C" PyObject* pyffish_validateFen(PyObject* self, PyObject *args) {
    const char *fen, *variant;
    int chess960 = false;
    if (!PyArg_ParseTuple(args, "ss|p", &fen, &variant, &chess960)) {
        return NULL;
    }

    return Py_BuildValue("i", FEN::validate_fen(std::string(fen), variants.find(std::string(variant))->second, chess960));
}


static PyMethodDef PyFFishMethods[] = {
    {"version", (PyCFunction)pyffish_version, METH_NOARGS, "Get package version."},
    {"info", (PyCFunction)pyffish_info, METH_NOARGS, "Get Stockfish version info."},
    {"variants", (PyCFunction)pyffish_variants, METH_NOARGS, "Get supported variants."},
    {"set_option", (PyCFunction)pyffish_setOption, METH_VARARGS, "Set UCI option."},
    {"load_variant_config", (PyCFunction)pyffish_loadVariantConfig, METH_VARARGS, "Load variant configuration."},
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
    {"validate_fen", (PyCFunction)pyffish_validateFen, METH_VARARGS, "Validate an input FEN."},
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

    // validation
    PyModule_AddObject(module, "FEN_OK", PyLong_FromLong(FEN::FEN_OK));

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
