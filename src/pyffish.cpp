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

using namespace std;

namespace
{

const string move_to_san(Position& pos, Move m) {
  Bitboard others, b;
  string san;
  Color us = pos.side_to_move();
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = pos.piece_on(from);
  PieceType pt = type_of(pc);

  if (type_of(m) == CASTLING)
      {
      san = to > from ? "O-O" : "O-O-O";

        if (is_gating(m))
        {
          san += string("/") + pos.piece_to_char()[make_piece(WHITE, gating_type(m))];
          san += gating_square(m) == to ? UCI::square(pos, to) : UCI::square(pos, from);
        }
      }
  else
  {
      san = type_of(m) == DROP ? UCI::dropped_piece(pos, m) + (Options["Protocol"] == "usi" ? '*' : '@') : "";

      if (pt != PAWN && type_of(m) != DROP)
      {
          if (type_of(m) != DROP) san += pos.piece_to_char()[make_piece(WHITE, pt)];

          // A disambiguation occurs if we have more then one piece of type 'pt'
          // that can reach 'to' with a legal move.
          if (pt == SHOGI_PAWN
             || pt == LANCE
             || (pt == SILVER && UCI::square(pos, from)[0] == UCI::square(pos, to)[0])
             )
            others = b = Bitboard(0);
          else
            others = b = (attacks_bb(~us, pt, to, pos.pieces()) & pos.pieces(us, pt)) ^ from;

          while (b)
          {
              Square s = pop_lsb(&b);
              if (!pos.legal(make_move(s, to)))
                  others ^= s;
          }

          if (!others)
              { /* disambiguation is not needed */ }
          else if (!(others & file_bb(from)))
              san += UCI::square(pos, from)[0];
          else if (!(others & rank_bb(from)))
              san += UCI::square(pos, from)[1];
          else
              san += UCI::square(pos, from);
      }
      else if (pos.capture(m))
          san += UCI::square(pos, from)[0];

      if (pos.capture(m))
          san += 'x';

      san += UCI::square(pos, to);

      if (type_of(m) == PROMOTION)
          san += string("=") + pos.piece_to_char()[make_piece(WHITE, promotion_type(m))];
      else if (is_gating(m))
          san += string("/") + pos.piece_to_char()[make_piece(WHITE, gating_type(m))];
  }

  if (pos.gives_check(m))
  {
      StateInfo st;
      pos.do_move(m, st);
      san += MoveList<LEGAL>(pos).size() ? "+" : "#";
      pos.undo_move(m);
  }
  return san;
}

bool hasInsufficientMaterial(Color c, Position *p) {
    if (p->count(c, PAWN) > 0 || p->count(c, ROOK) > 0 || p->count(c, QUEEN) > 0 || p->count(c, ARCHBISHOP) > 0 || p->count(c, CHANCELLOR) > 0)
        return false;

    if (p->count(c, KNIGHT) + p->count(c, BISHOP) < 2)
        return true;

    if (p->count(c, BISHOP) > 1 && p->count(c, KNIGHT) == 0)
    {
        bool sameColor;
        sameColor = ((DarkSquares & (p->pieces(c, BISHOP))) == 0) || ((~DarkSquares & (p->pieces(c, BISHOP))) == 0);
        return sameColor;
    }
    return false;
}

void buildPosition(Position& pos, StateListPtr& states, const char *variant, const char *fen, PyObject *moveList) {
    states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one

    if (strcmp(fen,"startpos")==0) fen=variants.find(string(variant))->second->startFen.c_str();
    bool sfen = strcmp(variant,"shogi")==0;
    Options["Protocol"] = (sfen) ? string("usi") : string("uci");
    pos.set(variants.find(string(variant))->second, string(fen), Options["UCI_Chess960"], &states->back(), Threads.main(), sfen);

    // parse move list
    int numMoves = PyList_Size(moveList);
    for (int i=0; i<numMoves ; i++) {
        string moveStr( PyBytes_AS_STRING(PyUnicode_AsEncodedString( PyList_GetItem(moveList, i), "UTF-8", "strict")) );
        Move m;
        if ((m = UCI::to_move(pos, moveStr)) != MOVE_NONE)
        {
            // do the move
            states->emplace_back();
            pos.do_move(m, states->back());
        }
        else
        {
            PyErr_SetString(PyExc_ValueError, (string("Invalid move '")+moveStr+"'").c_str());
        }
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

// INPUT variant, fen, move
extern "C" PyObject* pyffish_getSAN(PyObject* self, PyObject *args) {
    PyObject* moveList = PyList_New(0);
    Position pos;
    const char *fen, *variant, *move;

    if (!PyArg_ParseTuple(args, "sss", &variant, &fen,  &move)) {
        return NULL;
    }
    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
    string moveStr = move;
    return Py_BuildValue("s", move_to_san(pos, UCI::to_move(pos, moveStr)).c_str());
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_legalMoves(PyObject* self, PyObject *args) {
    PyObject* legalMoves = PyList_New(0), *moveList;
    Position pos;
    const char *fen, *variant;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen,  &PyList_Type, &moveList)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
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
    Position pos;
    const char *fen, *variant;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen,  &PyList_Type, &moveList)) {
        return NULL;
    }
    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
    return Py_BuildValue("s", pos.fen().c_str());
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_givesCheck(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen,  &PyList_Type, &moveList)) {
        return NULL;
    }
    if (PyList_Size(moveList) < 1) {
        PyErr_SetString(PyExc_ValueError, (string("Move list can't be empty.")).c_str());
        return NULL;
    }
    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
    return Py_BuildValue("O", pos.checkers() ? Py_True : Py_False);
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_isImmediateGameEnd(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    bool gameEnd;
    Value result;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen, &PyList_Type, &moveList)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
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

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen, &PyList_Type, &moveList)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
    gameEnd = pos.is_optional_game_end(result);
    return Py_BuildValue("(Oi)", gameEnd ? Py_True : Py_False, result);
}

// INPUT variant, fen, move list
extern "C" PyObject* pyffish_hasInsufficientMaterial(PyObject* self, PyObject *args) {
    PyObject *moveList;
    Position pos;
    const char *fen, *variant;
    bool wInsufficient, bInsufficient;

    if (!PyArg_ParseTuple(args, "ssO!", &variant, &fen, &PyList_Type, &moveList)) {
        return NULL;
    }

    StateListPtr states(new std::deque<StateInfo>(1));
    buildPosition(pos, states, variant, fen, moveList);
    if (strcmp(variant,"crazyhouse")==0 || strcmp(variant,"shogi")==0) {
        wInsufficient = false;
        bInsufficient = false;
    } else {
        wInsufficient = hasInsufficientMaterial(WHITE, &pos);
        bInsufficient = hasInsufficientMaterial(BLACK, &pos);
    }
    return Py_BuildValue("(OO)", wInsufficient ? Py_True : Py_False, bInsufficient ? Py_True : Py_False);
}

static PyMethodDef PyFFishMethods[] = {
    {"info", (PyCFunction)pyffish_info, METH_NOARGS, "Get Stockfish version info."},
    {"set_option", (PyCFunction)pyffish_setOption, METH_VARARGS, "Set UCI option."},
    {"start_fen", (PyCFunction)pyffish_startFen, METH_VARARGS, "Get starting position FEN."},
    {"get_san", (PyCFunction)pyffish_getSAN, METH_VARARGS, "Get SAN move from given FEN and UCI move."},
    {"legal_moves", (PyCFunction)pyffish_legalMoves, METH_VARARGS, "Get legal moves from given FEN and movelist."},
    {"get_fen", (PyCFunction)pyffish_getFEN, METH_VARARGS, "Get resulting FEN from given FEN and movelist."},
    {"gives_check", (PyCFunction)pyffish_givesCheck, METH_VARARGS, "Get check status from given FEN and movelist."},
    {"is_immediate_game_end", (PyCFunction)pyffish_isImmediateGameEnd, METH_VARARGS, "Get result from given FEN if variant rules ends the game."},
    {"is_optional_game_end", (PyCFunction)pyffish_isOptionalGameEnd, METH_VARARGS, "Get result from given FEN it rules enable game end by player."},
    {"has_insufficient_material", (PyCFunction)pyffish_hasInsufficientMaterial, METH_VARARGS, "Set UCI option."},
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
    pieceMap.init();
    variants.init();
    UCI::init(Options);
    PSQT::init(variants.find("chess")->second);
    Bitboards::init();
    Position::init();
    Bitbases::init();
    Search::init();
    Threads.set(Options["Threads"]);
    Search::clear(); // After threads are up

    return module;
};
