/*
  Based on Jean-Francois Romang work from
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
#include "variant.h"

static PyObject* PyFFishError;

namespace PSQT {
  void init(const Variant* v);
}

using namespace std;

extern "C" PyObject* pyffish_info(PyObject* self) {
    return Py_BuildValue("s", engine_info().c_str());
}

// INPUT option name, option value
extern "C" PyObject* pyffish_setOption(PyObject* self, PyObject *args) {
    const char *name;
    PyObject *valueObj;
    if (!PyArg_ParseTuple(args, "sO", &name, &valueObj)) return NULL;

    if (Options.count(name))
        Options[name] = string(PyBytes_AS_STRING(PyUnicode_AsEncodedString(PyObject_Str(valueObj), "UTF-8", "strict")));
    else
    {
        PyErr_SetString(PyExc_ValueError, (string("No such option ")+name+"'").c_str());
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
    bool sfen = strcmp(variant,"shogi")==0;
    Options["Protocol"] = (sfen) ? string("usi") : string("uci");

    return Py_BuildValue("s", variants.find(string(variant))->second->startFen.c_str());
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_legalMoves(PyObject* self, PyObject *args) {
    PyObject* legalMoves = PyList_New(0), *moveList;
    StateListPtr states = StateListPtr(new std::deque<StateInfo>(1));
    Position pos;
    const char *fen, *variant;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen,  &PyList_Type, &moveList)) {
        return NULL;
    }
    if(strcmp(fen,"startpos")==0) fen=variants.find(string(variant))->second->startFen.c_str();
    bool sfen = strcmp(variant,"shogi")==0;
    Options["Protocol"] = (sfen) ? string("usi") : string("uci");
    pos.set(variants.find(string(variant))->second, string(fen), Options["UCI_Chess960"], &states->back(), Threads.main(), sfen);

    // parse move list
    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyBytes_AS_STRING(PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict")) );
        Move m;
        if((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            // do the move
            states->emplace_back();
            pos.do_move(m, states->back());
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
            return NULL;
        }
    }

    for (const auto& m : MoveList<LEGAL>(pos))
    {
        PyObject *moveStr;
        moveStr=Py_BuildValue("s", UCI::move(pos, m).c_str());
        PyList_Append(legalMoves, moveStr);
        Py_XDECREF(moveStr);
    }

    return legalMoves;
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_getFEN(PyObject* self, PyObject *args) {
    PyObject *moveList;
    StateListPtr states = StateListPtr(new std::deque<StateInfo>(1));
    Position pos;
    const char *fen, *variant;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen,  &PyList_Type, &moveList)) {
        return NULL;
    }
    if(strcmp(fen,"startpos")==0) fen=variants.find(string(variant))->second->startFen.c_str();
    bool sfen = strcmp(variant,"shogi")==0;
    Options["Protocol"] = (sfen) ? string("usi") : string("uci");
    pos.set(variants.find(string(variant))->second, string(fen), Options["UCI_Chess960"], &states->back(), Threads.main(), sfen);

    // parse move list
    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyBytes_AS_STRING(PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict")) );
        Move m;
        if((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            // do the move
            states->emplace_back();
            pos.do_move(m, states->back());
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
            return NULL;
        }
    }
    return Py_BuildValue("s", pos.fen().c_str());
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_givesCheck(PyObject* self, PyObject *args) {
    PyObject *moveList;
    StateListPtr states = StateListPtr(new std::deque<StateInfo>(1));
    Position pos;
    const char *fen, *variant;
    bool givesCheck;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen,  &PyList_Type, &moveList)) {
        return NULL;
    }
    if(strcmp(fen,"startpos")==0) fen=variants.find(string(variant))->second->startFen.c_str();
    bool sfen = strcmp(variant,"shogi")==0;
    Options["Protocol"] = (sfen) ? string("usi") : string("uci");
    pos.set(variants.find(string(variant))->second, string(fen), Options["UCI_Chess960"], &states->back(), Threads.main(), sfen);

    // parse move list
    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyBytes_AS_STRING(PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict")) );
        Move m;
        if((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            // do the move
            givesCheck = pos.gives_check(m);
            states->emplace_back();
            pos.do_move(m, states->back());
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
            return NULL;
        }
    }
    return Py_BuildValue("O", givesCheck ? Py_True : Py_False);
}

static PyMethodDef PyFFishMethods[] = {
    {"info", (PyCFunction)pyffish_info, METH_NOARGS, "Get Stockfish version info."},
    {"set_option", (PyCFunction)pyffish_setOption, METH_VARARGS, "Set UCI option."},
    {"start_fen", (PyCFunction)pyffish_startFen, METH_VARARGS, "Get starting position FEN."},
    {"legal_moves", (PyCFunction)pyffish_legalMoves, METH_VARARGS, "Get legal moves from given FEN and movelist."},
    {"get_fen", (PyCFunction)pyffish_getFEN, METH_VARARGS, "Get resulting FEN from given FEN and movelist."},
    {"gives_check", (PyCFunction)pyffish_givesCheck, METH_VARARGS, "Get check status from given FEN and movelist."},
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

    // initialize stockfish
    variants.init();
    UCI::init(Options);
    PSQT::init(variants.find("chess")->second);
    Bitboards::init();
    Position::init();
    Bitbases::init();
    Search::init();
    Pawns::init();
    Threads.set(Options["Threads"]);
    Search::clear(); // After threads are up

    return module;
};
