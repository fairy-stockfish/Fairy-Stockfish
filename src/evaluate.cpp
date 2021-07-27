/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>   // For std::memset
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <streambuf>
#include <vector>

#include "nnue/evaluate_nnue.h"

#include "bitboard.h"
#include "evaluate.h"
#include "material.h"
#include "misc.h"
#include "pawns.h"
#include "thread.h"
#include "timeman.h"
#include "uci.h"
#include "incbin/incbin.h"

// Macro to embed the default efficiently updatable neural network (NNUE) file
// data in the engine binary (using incbin.h, by Dale Weiler).
// This macro invocation will declare the following three variables
//     const unsigned char        gEmbeddedNNUEData[];  // a pointer to the embedded data
//     const unsigned char *const gEmbeddedNNUEEnd;     // a marker to the end
//     const unsigned int         gEmbeddedNNUESize;    // the size of the embedded file
// Note that this does not work in Microsoft Visual Studio.
#if !defined(_MSC_VER) && !defined(NNUE_EMBEDDING_OFF)
  INCBIN(EmbeddedNNUE, EvalFileDefaultName);
#else
  const unsigned char        gEmbeddedNNUEData[1] = {0x0};
  [[maybe_unused]]
  const unsigned char *const gEmbeddedNNUEEnd = &gEmbeddedNNUEData[1];
  const unsigned int         gEmbeddedNNUESize = 1;
#endif

using namespace std;

namespace Stockfish {

NnueFeatures currentNnueFeatures;

namespace Eval {

  namespace NNUE {
    string eval_file_loaded = "None";
    UseNNUEMode useNNUE;

    static UseNNUEMode nnue_mode_from_option(const UCI::Option& mode)
    {
      if (mode == "false")
        return UseNNUEMode::False;
      else if (mode == "true")
         return UseNNUEMode::True;
      else if (mode == "pure")
        return UseNNUEMode::Pure;

      return UseNNUEMode::False;
    }
  }

  /// NNUE::init() tries to load a NNUE network at startup time, or when the engine
  /// receives a UCI command "setoption name EvalFile value nn-[a-z0-9]{12}.nnue"
  /// The name of the NNUE network is always retrieved from the EvalFile option.
  /// We search the given network in three locations: internally (the default
  /// network may be embedded in the binary), in the active working directory and
  /// in the engine directory. Distro packagers may define the DEFAULT_NNUE_DIRECTORY
  /// variable to have the engine search in a special directory in their distro.

  void NNUE::init() {

    useNNUE = nnue_mode_from_option(Options["Use NNUE"]);
    if (useNNUE == UseNNUEMode::False)
        return;

    string eval_file = string(Options["EvalFile"]);

    // Restrict NNUE usage to corresponding variant
    // Support multiple variant networks separated by semicolon(Windows)/colon(Unix)
    stringstream ss(eval_file);
    string variant = string(Options["UCI_Variant"]);
    useNNUE = UseNNUEMode::False;
#ifndef _WIN32
    constexpr char SepChar = ':';
#else
    constexpr char SepChar = ';';
#endif
    while (getline(ss, eval_file, SepChar))
    {
        string basename = eval_file.substr(eval_file.find_last_of("\\/") + 1);
        if (basename.rfind(variant, 0) != string::npos || (variant == "chess" && basename.rfind("nn-", 0) != string::npos))
        {
            useNNUE = UseNNUEMode::True;
            break;
        }
    }
    if (useNNUE == UseNNUEMode::False)
        return;

    currentNnueFeatures = variants.find(variant)->second->nnueFeatures;

    #if defined(DEFAULT_NNUE_DIRECTORY)
    #define stringify2(x) #x
    #define stringify(x) stringify2(x)
    vector<string> dirs = { "<internal>" , "" , CommandLine::binaryDirectory , stringify(DEFAULT_NNUE_DIRECTORY) };
    #else
    vector<string> dirs = { "<internal>" , "" , CommandLine::binaryDirectory };
    #endif

    for (string directory : dirs)
        if (eval_file_loaded != eval_file)
        {
            if (directory != "<internal>")
            {
                ifstream stream(directory + eval_file, ios::binary);
                if (load_eval(eval_file, stream))
                    eval_file_loaded = eval_file;
            }

            if (directory == "<internal>" && eval_file == EvalFileDefaultName)
            {
                // C++ way to prepare a buffer for a memory stream
                class MemoryBuffer : public basic_streambuf<char> {
                    public: MemoryBuffer(char* p, size_t n) { setg(p, p, p + n); setp(p, p + n); }
                };

                MemoryBuffer buffer(const_cast<char*>(reinterpret_cast<const char*>(gEmbeddedNNUEData)),
                                    size_t(gEmbeddedNNUESize));

                istream stream(&buffer);
                if (load_eval(eval_file, stream))
                    eval_file_loaded = eval_file;
            }
        }
  }

  /// NNUE::export_net() exports the currently loaded network to a file
  void NNUE::export_net(const std::optional<std::string>& filename) {
    std::string actualFilename;

    if (filename.has_value())
        actualFilename = filename.value();
    else
    {
        if (eval_file_loaded != EvalFileDefaultName)
        {
             sync_cout << "Failed to export a net. A non-embedded net can only be saved if the filename is specified." << sync_endl;
             return;
        }
        actualFilename = EvalFileDefaultName;
    }

    ofstream stream(actualFilename, std::ios_base::binary);

    if (save_eval(stream))
        sync_cout << "Network saved successfully to " << actualFilename << "." << sync_endl;
    else
        sync_cout << "Failed to export a net." << sync_endl;
  }

  /// NNUE::verify() verifies that the last net used was loaded successfully
  void NNUE::verify() {

    string eval_file = string(Options["EvalFile"]);

    if (useNNUE != UseNNUEMode::False && eval_file.find(eval_file_loaded) == string::npos)
    {
        UCI::OptionsMap defaults;
        UCI::init(defaults);

        string msg1 = "If the UCI option \"Use NNUE\" is set to true, network evaluation parameters compatible with the engine must be available.";
        string msg2 = "The option is set to true, but the network file " + eval_file + " was not loaded successfully.";
        string msg3 = "The UCI option EvalFile might need to specify the full path, including the directory name, to the network file.";
        string msg4 = "The default net can be downloaded from: https://tests.stockfishchess.org/api/nn/" + string(defaults["EvalFile"]);
        string msg5 = "The engine will be terminated now.";

        sync_cout << "info string ERROR: " << msg1 << sync_endl;
        sync_cout << "info string ERROR: " << msg2 << sync_endl;
        sync_cout << "info string ERROR: " << msg3 << sync_endl;
        sync_cout << "info string ERROR: " << msg4 << sync_endl;
        sync_cout << "info string ERROR: " << msg5 << sync_endl;

        exit(EXIT_FAILURE);
    }

    if (useNNUE != UseNNUEMode::False)
        sync_cout << "info string NNUE evaluation using " << eval_file << " enabled" << sync_endl;
    else
        sync_cout << "info string classical evaluation enabled" << sync_endl;
  }
}

namespace Trace {

  enum Tracing { NO_TRACE, TRACE };

  enum Term { // The first PIECE_TYPE_NB entries are reserved for PieceType
    MATERIAL = PIECE_TYPE_NB, IMBALANCE, MOBILITY, THREAT, PASSED, SPACE, VARIANT, WINNABLE, TOTAL, TERM_NB
  };

  Score scores[TERM_NB][COLOR_NB];

  double to_cp(Value v) { return double(v) / PawnValueEg; }

  void add(int idx, Color c, Score s) {
    scores[idx][c] = s;
  }

  void add(int idx, Score w, Score b = SCORE_ZERO) {
    scores[idx][WHITE] = w;
    scores[idx][BLACK] = b;
  }

  std::ostream& operator<<(std::ostream& os, Score s) {
    os << std::setw(5) << to_cp(mg_value(s)) << " "
       << std::setw(5) << to_cp(eg_value(s));
    return os;
  }

  std::ostream& operator<<(std::ostream& os, Term t) {

    if (t == MATERIAL || t == IMBALANCE || t == WINNABLE || t == TOTAL)
        os << " ----  ----"    << " | " << " ----  ----";
    else
        os << scores[t][WHITE] << " | " << scores[t][BLACK];

    os << " | " << scores[t][WHITE] - scores[t][BLACK] << "\n";
    return os;
  }
}

using namespace Trace;

namespace {

  // Threshold for lazy and space evaluation
  constexpr Value LazyThreshold1 =  Value(1565);
  constexpr Value LazyThreshold2 =  Value(1102);
  constexpr Value SpaceThreshold = Value(11551);
  constexpr Value NNUEThreshold1 =   Value(682);
  constexpr Value NNUEThreshold2 =   Value(176);

  // KingAttackWeights[PieceType] contains king attack weights by piece type
  constexpr int KingAttackWeights[PIECE_TYPE_NB] = { 0, 0, 81, 52, 44, 10, 40 };

  // SafeCheck[PieceType][single/multiple] contains safe check bonus by piece type,
  // higher if multiple safe checks are possible for that piece type.
  constexpr int SafeCheck[][2] = {
      {}, {600, 600}, {803, 1292}, {639, 974}, {1087, 1878}, {759, 1132}, {600, 900}
  };

#define S(mg, eg) make_score(mg, eg)

  // MobilityBonus[PieceType-2][attacked] contains bonuses for middle and end game,
  // indexed by piece type and number of attacked squares in the mobility area.
  constexpr Score MobilityBonus[][4 * RANK_NB] = {
    { S(-62,-79), S(-53,-57), S(-12,-31), S( -3,-17), S(  3,  7), S( 12, 13), // Knight
      S( 21, 16), S( 28, 21), S( 37, 26) },
    { S(-47,-59), S(-20,-25), S( 14, -8), S( 29, 12), S( 39, 21), S( 53, 40), // Bishop
      S( 53, 56), S( 60, 58), S( 62, 65), S( 69, 72), S( 78, 78), S( 83, 87),
      S( 91, 88), S( 96, 98) },
    { S(-60,-82), S(-24,-15), S(  0, 17) ,S(  3, 43), S(  4, 72), S( 14,100), // Rook
      S( 20,102), S( 30,122), S( 41,133), S(41 ,139), S( 41,153), S( 45,160),
      S( 57,165), S( 58,170), S( 67,175) },
    { S(-29,-49), S(-16,-29), S( -8, -8), S( -8, 17), S( 18, 39), S( 25, 54), // Queen
      S( 23, 59), S( 37, 73), S( 41, 76), S( 54, 95), S( 65, 95) ,S( 68,101),
      S( 69,124), S( 70,128), S( 70,132), S( 70,133) ,S( 71,136), S( 72,140),
      S( 74,147), S( 76,149), S( 90,153), S(104,169), S(105,171), S(106,171),
      S(112,178), S(114,185), S(114,187), S(119,221) }
  };
  constexpr Score MaxMobility  = S(150, 200);
  constexpr Score DropMobility = S(10, 10);

  // BishopPawns[distance from edge] contains a file-dependent penalty for pawns on
  // squares of the same color as our bishop.
  constexpr Score BishopPawns[int(FILE_NB) / 2] = {
    S(3, 8), S(3, 9), S(2, 8), S(3, 8)
  };

  // KingProtector[knight/bishop] contains penalty for each distance unit to own king
  constexpr Score KingProtector[] = { S(8, 9), S(6, 9) };

  // Outpost[knight/bishop] contains bonuses for each knight or bishop occupying a
  // pawn protected square on rank 4 to 6 which is also safe from a pawn attack.
  constexpr Score Outpost[] = { S(57, 38), S(31, 24) };

  // PassedRank[Rank] contains a bonus according to the rank of a passed pawn
  constexpr Score PassedRank[RANK_NB] = {
    S(0, 0), S(7, 27), S(16, 32), S(17, 40), S(64, 71), S(170, 174), S(278, 262)
  };

  constexpr Score RookOnClosedFile = S(10, 5);
  constexpr Score RookOnOpenFile[] = { S(19, 6), S(47, 26) };

  // ThreatByMinor/ByRook[attacked PieceType] contains bonuses according to
  // which piece type attacks which one. Attacks on lesser pieces which are
  // pawn-defended are not considered.
  constexpr Score ThreatByMinor[PIECE_TYPE_NB] = {
    S(0, 0), S(5, 32), S(55, 41), S(77, 56), S(89, 119), S(79, 162)
  };

  constexpr Score ThreatByRook[PIECE_TYPE_NB] = {
    S(0, 0), S(3, 44), S(37, 68), S(42, 60), S(0, 39), S(58, 43)
  };

  constexpr Value CorneredBishop = Value(50);

  // Assorted bonuses and penalties
  constexpr Score UncontestedOutpost  = S(  1, 10);
  constexpr Score BishopOnKingRing    = S( 24,  0);
  constexpr Score BishopXRayPawns     = S(  4,  5);
  constexpr Score FlankAttacks        = S(  8,  0);
  constexpr Score Hanging             = S( 69, 36);
  constexpr Score KnightOnQueen       = S( 16, 11);
  constexpr Score LongDiagonalBishop  = S( 45,  0);
  constexpr Score MinorBehindPawn     = S( 18,  3);
  constexpr Score PassedFile          = S( 11,  8);
  constexpr Score PawnlessFlank       = S( 17, 95);
  constexpr Score ReachableOutpost    = S( 31, 22);
  constexpr Score RestrictedPiece     = S(  7,  7);
  constexpr Score RookOnKingRing      = S( 16,  0);
  constexpr Score SliderOnQueen       = S( 60, 18);
  constexpr Score ThreatByKing        = S( 24, 89);
  constexpr Score ThreatByPawnPush    = S( 48, 39);
  constexpr Score ThreatBySafePawn    = S(173, 94);
  constexpr Score TrappedRook         = S( 55, 13);
  constexpr Score WeakQueenProtection = S( 14,  0);
  constexpr Score WeakQueen           = S( 56, 15);


  // Variant and fairy piece bonuses
  constexpr Score KingProximity        = S(2, 6);
  constexpr Score EndgameKingProximity = S(0, 10);
  constexpr Score ConnectedSoldier     = S(20, 20);

  constexpr int VirtualCheck = 600;

#undef S

  // Evaluation class computes and stores attacks tables and other working data
  template<Tracing T>
  class Evaluation {

  public:
    Evaluation() = delete;
    explicit Evaluation(const Position& p) : pos(p) {}
    Evaluation& operator=(const Evaluation&) = delete;
    Value value();

  private:
    template<Color Us> void initialize();
    template<Color Us> Score pieces(PieceType Pt);
    template<Color Us> Score hand(PieceType pt);
    template<Color Us> Score king() const;
    template<Color Us> Score threats() const;
    template<Color Us> Score passed() const;
    template<Color Us> Score space() const;
    template<Color Us> Score variant() const;
    Value winnable(Score score) const;

    const Position& pos;
    Material::Entry* me;
    Pawns::Entry* pe;
    Bitboard mobilityArea[COLOR_NB];
    Score mobility[COLOR_NB] = { SCORE_ZERO, SCORE_ZERO };

    // attackedBy[color][piece type] is a bitboard representing all squares
    // attacked by a given color and piece type. Special "piece types" which
    // is also calculated is ALL_PIECES.
    Bitboard attackedBy[COLOR_NB][PIECE_TYPE_NB];

    // attackedBy2[color] are the squares attacked by at least 2 units of a given
    // color, including x-rays. But diagonal x-rays through pawns are not computed.
    Bitboard attackedBy2[COLOR_NB];

    // kingRing[color] are the squares adjacent to the king plus some other
    // very near squares, depending on king position.
    Bitboard kingRing[COLOR_NB];

    // kingAttackersCount[color] is the number of pieces of the given color
    // which attack a square in the kingRing of the enemy king.
    int kingAttackersCount[COLOR_NB];
    int kingAttackersCountInHand[COLOR_NB];

    // kingAttackersWeight[color] is the sum of the "weights" of the pieces of
    // the given color which attack a square in the kingRing of the enemy king.
    // The weights of the individual piece types are given by the elements in
    // the KingAttackWeights array.
    int kingAttackersWeight[COLOR_NB];
    int kingAttackersWeightInHand[COLOR_NB];

    // kingAttacksCount[color] is the number of attacks by the given color to
    // squares directly adjacent to the enemy king. Pieces which attack more
    // than one square are counted multiple times. For instance, if there is
    // a white knight on g5 and black's king is on g8, this white knight adds 2
    // to kingAttacksCount[WHITE].
    int kingAttacksCount[COLOR_NB];
  };


  // Evaluation::initialize() computes king and pawn attacks, and the king ring
  // bitboard for a given color. This is done at the beginning of the evaluation.

  template<Tracing T> template<Color Us>
  void Evaluation<T>::initialize() {

    constexpr Color     Them = ~Us;
    constexpr Direction Up   = pawn_push(Us);
    constexpr Direction Down = -Up;
    Bitboard LowRanks = rank_bb(relative_rank(Us, RANK_2, pos.max_rank())) | rank_bb(relative_rank(Us, RANK_3, pos.max_rank()));

    const Square ksq = pos.count<KING>(Us) ? pos.square<KING>(Us) : SQ_NONE;

    Bitboard dblAttackByPawn = pawn_double_attacks_bb<Us>(pos.pieces(Us, PAWN));

    // Find our pawns that are blocked or on the first two ranks
    Bitboard b = pos.pieces(Us, PAWN) & (shift<Down>(pos.pieces()) | LowRanks);

    // Squares occupied by those pawns, by our king or queen, by blockers to attacks on our king
    // or controlled by enemy pawns are excluded from the mobility area.
    if (pos.must_capture())
        mobilityArea[Us] = AllSquares;
    else
        mobilityArea[Us] = ~(b | pos.pieces(Us, KING, QUEEN) | pos.blockers_for_king(Us) | pe->pawn_attacks(Them)
                               | (pos.pieces(Us, SHOGI_PAWN) & shift<Down>(pos.pieces(Us)))
                               | shift<Down>(pos.pieces(Them, SHOGI_PAWN, SOLDIER))
                               | shift<EAST>(pos.promoted_soldiers(Them))
                               | shift<WEST>(pos.promoted_soldiers(Them)));

    // Initialize attackedBy[] for king and pawns
    attackedBy[Us][KING] = pos.count<KING>(Us) ? pos.attacks_from(Us, KING, ksq) : Bitboard(0);
    attackedBy[Us][PAWN] = pe->pawn_attacks(Us);
    attackedBy[Us][SHOGI_PAWN] = shift<Up>(pos.pieces(Us, SHOGI_PAWN));
    attackedBy[Us][ALL_PIECES] = attackedBy[Us][KING] | attackedBy[Us][PAWN] | attackedBy[Us][SHOGI_PAWN];
    attackedBy2[Us]            =  (attackedBy[Us][KING] & attackedBy[Us][PAWN])
                                | (attackedBy[Us][KING] & attackedBy[Us][SHOGI_PAWN])
                                | (attackedBy[Us][PAWN] & attackedBy[Us][SHOGI_PAWN])
                                | dblAttackByPawn;

    // Init our king safety tables
    if (!pos.count<KING>(Us))
        kingRing[Us] = Bitboard(0);
    else
    {
        Square s = make_square(std::clamp(file_of(ksq), FILE_B, File(pos.max_file() - 1)),
                               std::clamp(rank_of(ksq), RANK_2, Rank(pos.max_rank() - 1)));
        kingRing[Us] = attacks_bb<KING>(s) | s;
    }

    kingAttackersCount[Them] = popcount(kingRing[Us] & (pe->pawn_attacks(Them) | shift<Down>(pos.pieces(Them, SHOGI_PAWN))));
    kingAttacksCount[Them] = kingAttackersWeight[Them] = 0;
    kingAttackersCountInHand[Them] = kingAttackersWeightInHand[Them] = 0;

    // Remove from kingRing[] the squares defended by two pawns
    kingRing[Us] &= ~dblAttackByPawn;

    kingRing[Us] &= pos.board_bb();
  }


  // Evaluation::pieces() scores pieces of a given color and type

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::pieces(PieceType Pt) {

    constexpr Color     Them = ~Us;
    constexpr Direction Down = -pawn_push(Us);
    constexpr Bitboard OutpostRanks = (Us == WHITE ? Rank4BB | Rank5BB | Rank6BB
                                                   : Rank5BB | Rank4BB | Rank3BB);
    Bitboard b1 = pos.pieces(Us, Pt);
    Bitboard b, bb;
    Score score = SCORE_ZERO;

    attackedBy[Us][Pt] = 0;

    while (b1)
    {
        Square s = pop_lsb(b1);

        // Find attacked squares, including x-ray attacks for bishops and rooks
        b = Pt == BISHOP ? attacks_bb<BISHOP>(s, pos.pieces() ^ pos.pieces(QUEEN))
          : Pt ==   ROOK && !pos.diagonal_lines() ? attacks_bb<  ROOK>(s, pos.pieces() ^ pos.pieces(QUEEN) ^ pos.pieces(Us, ROOK))
                         : pos.attacks_from(Us, Pt, s);

        // Restrict mobility to actual squares of board
        b &= pos.board_bb(Us, Pt);

        if (pos.blockers_for_king(Us) & s)
            b &= line_bb(pos.square<KING>(Us), s);

        attackedBy2[Us] |= attackedBy[Us][ALL_PIECES] & b;
        attackedBy[Us][Pt] |= b;
        attackedBy[Us][ALL_PIECES] |= b;

        if (b & kingRing[Them])
        {
            kingAttackersCount[Us]++;
            kingAttackersWeight[Us] += KingAttackWeights[std::min(Pt, FAIRY_PIECES)];
            kingAttacksCount[Us] += popcount(b & attackedBy[Them][KING]);
        }

        else if (Pt == ROOK && (file_bb(s) & kingRing[Them]))
            score += RookOnKingRing;

        else if (Pt == BISHOP && (attacks_bb<BISHOP>(s, pos.pieces(PAWN)) & kingRing[Them]))
            score += BishopOnKingRing;

        if (Pt > QUEEN)
             b = (b & pos.pieces()) | (pos.moves_from(Us, Pt, s) & ~pos.pieces() & pos.board_bb());

        int mob = popcount(b & mobilityArea[Us]);
        if (Pt <= QUEEN)
            mobility[Us] += MobilityBonus[Pt - 2][mob];
        else
            mobility[Us] += MaxMobility * (mob - 2) / (8 + mob);

        // Piece promotion bonus
        if (pos.promoted_piece_type(Pt) != NO_PIECE_TYPE)
        {
            Bitboard zone = zone_bb(Us, pos.promotion_rank(), pos.max_rank());
            if (zone & (b | s))
                score += make_score(PieceValue[MG][pos.promoted_piece_type(Pt)] - PieceValue[MG][Pt],
                                    PieceValue[EG][pos.promoted_piece_type(Pt)] - PieceValue[EG][Pt]) / (zone & s && b ? 6 : 12);
        }
        else if (pos.piece_demotion() && pos.unpromoted_piece_on(s))
            score -= make_score(PieceValue[MG][Pt] - PieceValue[MG][pos.unpromoted_piece_on(s)],
                                PieceValue[EG][Pt] - PieceValue[EG][pos.unpromoted_piece_on(s)]) / 4;
        else if (pos.captures_to_hand() && pos.unpromoted_piece_on(s))
            score += make_score(PieceValue[MG][Pt] - PieceValue[MG][pos.unpromoted_piece_on(s)],
                                PieceValue[EG][Pt] - PieceValue[EG][pos.unpromoted_piece_on(s)]) / 8;

        // Penalty if the piece is far from the kings in drop variants
        if ((pos.captures_to_hand() || pos.two_boards()) && pos.count<KING>(Them) && pos.count<KING>(Us))
        {
            if (!(b & (kingRing[Us] | kingRing[Them])))
                score -= KingProximity * distance(s, pos.square<KING>(Us)) * distance(s, pos.square<KING>(Them));
        }

        else if (pos.count<KING>(Us) && (Pt == FERS || Pt == SILVER))
            score -= EndgameKingProximity * (distance(s, pos.square<KING>(Us)) - 2);

        if (Pt == SOLDIER && (pos.pieces(Us, SOLDIER) & rank_bb(s) & adjacent_files_bb(s)))
            score += ConnectedSoldier;

        if (Pt == BISHOP || Pt == KNIGHT)
        {
            // Bonus if the piece is on an outpost square or can reach one
            // Bonus for knights (UncontestedOutpost) if few relevant targets
            bb = OutpostRanks & (attackedBy[Us][PAWN] | shift<Down>(pos.pieces(PAWN)))
                              & ~pe->pawn_attacks_span(Them);
            Bitboard targets = pos.pieces(Them) & ~pos.pieces(PAWN);

            if (   Pt == KNIGHT
                && bb & s & ~CenterFiles // on a side outpost
                && !(b & targets)        // no relevant attacks
                && (!more_than_one(targets & (s & QueenSide ? QueenSide : KingSide))))
                score += UncontestedOutpost * popcount(pos.pieces(PAWN) & (s & QueenSide ? QueenSide : KingSide));
            else if (bb & s)
                score += Outpost[Pt == BISHOP];
            else if (Pt == KNIGHT && bb & b & ~pos.pieces(Us))
                score += ReachableOutpost;

            // Bonus for a knight or bishop shielded by pawn
            if (shift<Down>(pos.pieces(PAWN)) & s)
                score += MinorBehindPawn;

            // Penalty if the piece is far from the king
            if (pos.count<KING>(Us))
            score -= KingProtector[Pt == BISHOP] * distance(pos.square<KING>(Us), s);

            if (Pt == BISHOP)
            {
                // Penalty according to the number of our pawns on the same color square as the
                // bishop, bigger when the center files are blocked with pawns and smaller
                // when the bishop is outside the pawn chain.
                Bitboard blocked = pos.pieces(Us, PAWN) & shift<Down>(pos.pieces());

                score -= BishopPawns[edge_distance(file_of(s), pos.max_file())] * pos.pawns_on_same_color_squares(Us, s)
                                     * (!(attackedBy[Us][PAWN] & s) + popcount(blocked & CenterFiles));

                // Penalty for all enemy pawns x-rayed
                score -= BishopXRayPawns * popcount(attacks_bb<BISHOP>(s) & pos.pieces(Them, PAWN));

                // Bonus for bishop on a long diagonal which can "see" both center squares
                if (more_than_one(attacks_bb<BISHOP>(s, pos.pieces(PAWN)) & Center))
                    score += LongDiagonalBishop;

                // An important Chess960 pattern: a cornered bishop blocked by a friendly
                // pawn diagonally in front of it is a very serious problem, especially
                // when that pawn is also blocked.
                if (   pos.is_chess960()
                    && (s == relative_square(Us, SQ_A1) || s == relative_square(Us, SQ_H1)))
                {
                    Direction d = pawn_push(Us) + (file_of(s) == FILE_A ? EAST : WEST);
                    if (pos.piece_on(s + d) == make_piece(Us, PAWN))
                        score -= !pos.empty(s + d + pawn_push(Us)) ? 4 * make_score(CorneredBishop, CorneredBishop)
                                                                   : 3 * make_score(CorneredBishop, CorneredBishop);
                }
            }
        }

        if (Pt == ROOK)
        {
            // Bonuses for rook on a (semi-)open or closed file
            if (pos.is_on_semiopen_file(Us, s))
            {
                score += RookOnOpenFile[pos.is_on_semiopen_file(Them, s)];
            }
            else
            {
                // If our pawn on this file is blocked, increase penalty
                if ( pos.pieces(Us, PAWN)
                   & shift<Down>(pos.pieces())
                   & file_bb(s))
                {
                    score -= RookOnClosedFile;
                }

                // Penalty when trapped by the king, even more if the king cannot castle
                if (mob <= 3 && pos.count<KING>(Us))
                {
                    File kf = file_of(pos.square<KING>(Us));
                    if ((kf < FILE_E) == (file_of(s) < kf))
                        score -= TrappedRook * (1 + !pos.castling_rights(Us));
                }
            }
        }

        if (Pt == QUEEN)
        {
            // Penalty if any relative pin or discovered attack against the queen
            Bitboard queenPinners;
            if (pos.slider_blockers(pos.pieces(Them, ROOK, BISHOP), s, queenPinners, Them))
                score -= WeakQueen;
        }
    }
    if constexpr (T)
        Trace::add(Pt, Us, score);

    return score;
  }

  // Evaluation::hand() scores pieces of a given color and type in hand
  template<Tracing T> template<Color Us>
  Score Evaluation<T>::hand(PieceType pt) {

    constexpr Color Them = ~Us;

    Score score = SCORE_ZERO;

    if (pos.count_in_hand(Us, pt) > 0 && pt != KING)
    {
        Bitboard b = pos.drop_region(Us, pt) & ~pos.pieces() & (~attackedBy2[Them] | attackedBy[Us][ALL_PIECES]);
        if ((b & kingRing[Them]) && pt != SHOGI_PAWN)
        {
            kingAttackersCountInHand[Us] += pos.count_in_hand(Us, pt);
            kingAttackersWeightInHand[Us] += KingAttackWeights[std::min(pt, FAIRY_PIECES)] * pos.count_in_hand(Us, pt);
            kingAttacksCount[Us] += popcount(b & attackedBy[Them][KING]);
        }
        Bitboard theirHalf = pos.board_bb() & ~forward_ranks_bb(Them, relative_rank(Them, Rank((pos.max_rank() - 1) / 2), pos.max_rank()));
        mobility[Us] += DropMobility * popcount(b & theirHalf & ~attackedBy[Them][ALL_PIECES]);

        // Bonus for Kyoto shogi style drops of promoted pieces
        if (pos.promoted_piece_type(pt) != NO_PIECE_TYPE && pos.drop_promoted())
            score += make_score(std::max(PieceValue[MG][pos.promoted_piece_type(pt)] - PieceValue[MG][pt], VALUE_ZERO),
                                std::max(PieceValue[EG][pos.promoted_piece_type(pt)] - PieceValue[EG][pt], VALUE_ZERO)) / 4 * pos.count_in_hand(Us, pt);

        // Mobility bonus for reversi variants
        if (pos.enclosing_drop())
            mobility[Us] += make_score(500, 500) * popcount(b);

        // Reduce score if there is a deficit of gates
        if (pos.seirawan_gating() && !pos.piece_drops() && pos.count_in_hand(Us, ALL_PIECES) > popcount(pos.gates(Us)))
            score -= make_score(200, 900) / pos.count_in_hand(Us, ALL_PIECES) * (pos.count_in_hand(Us, ALL_PIECES) - popcount(pos.gates(Us)));

        // Redundant pieces that can not be doubled per file (e.g., shogi pawns)
        if (pt == pos.drop_no_doubled())
            score -= make_score(50, 20) * std::max(pos.count_with_hand(Us, pt) - pos.max_file() - 1, 0);
    }

    return score;
  }

  // Evaluation::king() assigns bonuses and penalties to a king of a given color

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::king() const {

    constexpr Color    Them = ~Us;
    Rank r = relative_rank(Us, std::min(Rank((pos.max_rank() - 1) / 2 + 1), pos.max_rank()), pos.max_rank());
    Bitboard Camp = pos.board_bb() & ~forward_ranks_bb(Us, r);

    if (!pos.count<KING>(Us) || !pos.checking_permitted() || pos.checkmate_value() != -VALUE_MATE)
        return SCORE_ZERO;

    Bitboard weak, b1, b2, b3, safe, unsafeChecks = 0;
    Bitboard queenChecks, knightChecks, pawnChecks, otherChecks;
    int kingDanger = 0;
    const Square ksq = pos.square<KING>(Us);

    // Init the score with king shelter and enemy pawns storm
    Score score = pe->king_safety<Us>(pos);

    // Attacked squares defended at most once by our queen or king
    weak =  attackedBy[Them][ALL_PIECES]
          & ~attackedBy2[Us]
          & (~attackedBy[Us][ALL_PIECES] | attackedBy[Us][KING] | attackedBy[Us][QUEEN]);

    // Analyse the safe enemy's checks which are possible on next move
    safe  = ~pos.pieces(Them);
    if (!pos.check_counting() || pos.checks_remaining(Them) > 1)
    safe &= ~attackedBy[Us][ALL_PIECES] | (weak & attackedBy2[Them]);

    b1 = attacks_bb<ROOK  >(ksq, pos.pieces() ^ pos.pieces(Us, QUEEN));
    b2 = attacks_bb<BISHOP>(ksq, pos.pieces() ^ pos.pieces(Us, QUEEN));

    std::function <Bitboard (Color, PieceType)> get_attacks = [this](Color c, PieceType pt) {
        return attackedBy[c][pt] | (pos.piece_drops() && pos.count_in_hand(c, pt) > 0 ? pos.drop_region(c, pt) & ~pos.pieces() : Bitboard(0));
    };
    for (PieceType pt : pos.piece_types())
    {
        switch (pt)
        {
        case QUEEN:
            // Enemy queen safe checks: we count them only if they are from squares from
            // which we can't give a rook check, because rook checks are more valuable.
            queenChecks = (b1 | b2)
                        & get_attacks(Them, QUEEN)
                        & pos.board_bb()
                        & safe
                        & ~attackedBy[Us][QUEEN]
                        & ~(b1 & attackedBy[Them][ROOK]);

            if (queenChecks)
                kingDanger += SafeCheck[QUEEN][more_than_one(queenChecks)];
            break;
        case ROOK:
        case BISHOP:
        case KNIGHT:
            knightChecks = attacks_bb(Us, pt, ksq, pos.pieces() ^ pos.pieces(Us, QUEEN)) & get_attacks(Them, pt) & pos.board_bb();
            if (knightChecks & safe)
                kingDanger += SafeCheck[pt][more_than_one(knightChecks & safe)];
            else
                unsafeChecks |= knightChecks;
            break;
        case PAWN:
            if (pos.piece_drops() && pos.count_in_hand(Them, pt) > 0)
            {
                pawnChecks = attacks_bb(Us, pt, ksq, pos.pieces()) & ~pos.pieces() & pos.board_bb();
                if (pawnChecks & safe)
                    kingDanger += SafeCheck[PAWN][more_than_one(pawnChecks & safe)];
                else
                    unsafeChecks |= pawnChecks;
            }
            break;
        case SHOGI_PAWN:
            if (pos.promoted_piece_type(pt))
            {
                otherChecks = attacks_bb(Us, pos.promoted_piece_type(pt), ksq, pos.pieces()) & attackedBy[Them][pt]
                                 & zone_bb(Them, pos.promotion_rank(), pos.max_rank()) & pos.board_bb();
                if (otherChecks & safe)
                    kingDanger += SafeCheck[FAIRY_PIECES][more_than_one(otherChecks & safe)];
                else
                    unsafeChecks |= otherChecks;
            }
            break;
        case KING:
            break;
        default:
            otherChecks = attacks_bb(Us, pt, ksq, pos.pieces()) & get_attacks(Them, pt) & pos.board_bb();
            if (otherChecks & safe)
                kingDanger += SafeCheck[FAIRY_PIECES][more_than_one(otherChecks & safe)];
            else
                unsafeChecks |= otherChecks;
        }
    }

    // Virtual piece drops
    if (pos.two_boards() && pos.piece_drops())
    {
        for (PieceType pt : pos.piece_types())
            if (pos.count_in_hand(Them, pt) <= 0 && (attacks_bb(Us, pt, ksq, pos.pieces()) & safe & pos.drop_region(Them, pt) & ~pos.pieces()))
            {
                kingDanger += VirtualCheck * 500 / (500 + PieceValue[MG][pt]);
                // Presumably a mate threat
                if (!(attackedBy[Us][KING] & ~(attackedBy[Them][ALL_PIECES] | pos.pieces(Us))))
                    kingDanger += 2000;
            }
    }

    if (pos.check_counting())
        kingDanger += kingDanger * 7 / (3 + pos.checks_remaining(Them));

    Square s = file_of(ksq) == FILE_A ? ksq + EAST : file_of(ksq) == pos.max_file() ? ksq + WEST : ksq;
    Bitboard kingFlank = pos.max_file() == FILE_H ? KingFlank[file_of(ksq)] : file_bb(s) | adjacent_files_bb(s);

    // Find the squares that opponent attacks in our king flank, the squares
    // which they attack twice in that flank, and the squares that we defend.
    b1 = attackedBy[Them][ALL_PIECES] & kingFlank & Camp;
    b2 = b1 & attackedBy2[Them];
    b3 = attackedBy[Us][ALL_PIECES] & kingFlank & Camp;

    int kingFlankAttack  = popcount(b1) + popcount(b2);
    int kingFlankDefense = popcount(b3);

    kingDanger +=        kingAttackersCount[Them] * kingAttackersWeight[Them]
                 +       kingAttackersCountInHand[Them] * kingAttackersWeight[Them]
                 +       kingAttackersCount[Them] * kingAttackersWeightInHand[Them]
                 + 183 * popcount(kingRing[Us] & (weak | ~pos.board_bb(Us, KING))) * (1 + pos.captures_to_hand() + pos.check_counting())
                 + 148 * popcount(unsafeChecks) * (1 + pos.check_counting())
                 +  98 * popcount(pos.blockers_for_king(Us))
                 +  69 * kingAttacksCount[Them] * (2 + 8 * pos.check_counting() + pos.captures_to_hand()) / 2
                 +   3 * kingFlankAttack * kingFlankAttack / 8
                 +       mg_value(mobility[Them] - mobility[Us]) * int(!pos.captures_to_hand())
                 - 873 * !(pos.major_pieces(Them) || pos.captures_to_hand())
                       * 2 / (2 + 2 * pos.check_counting() + 2 * pos.two_boards() + 2 * pos.makpong()
                                + (pos.king_type() != KING) * (pos.diagonal_lines() ? 1 : 2))
                 - 100 * bool(attackedBy[Us][KNIGHT] & attackedBy[Us][KING])
                 -   6 * mg_value(score) / 8
                 -   4 * kingFlankDefense
                 +  37;

    // Transform the kingDanger units into a Score, and subtract it from the evaluation
    if (kingDanger > 100)
        score -= make_score(std::min(kingDanger, 3500) * kingDanger / 4096, kingDanger / 16);

    // Penalty when our king is on a pawnless flank
    if (!(pos.pieces(PAWN) & kingFlank))
        score -= PawnlessFlank;

    // Penalty if king flank is under attack, potentially moving toward the king
    score -= FlankAttacks * kingFlankAttack * (1 + 5 * pos.captures_to_hand() + pos.check_counting());

    if (pos.check_counting())
        score += make_score(0, mg_value(score) * 2 / (2 + pos.checks_remaining(Them)));

    if (pos.king_type() == WAZIR)
        score += make_score(0, mg_value(score) / 2);

    // For drop games, king danger is independent of game phase, but dependent on material density
    if (pos.captures_to_hand() || pos.two_boards())
        score = make_score(mg_value(score) * me->material_density() / 11000,
                           mg_value(score) * me->material_density() / 11000);

    if constexpr (T)
        Trace::add(KING, Us, score);

    return score;
  }


  // Evaluation::threats() assigns bonuses according to the types of the
  // attacking and the attacked pieces.

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::threats() const {

    constexpr Color     Them     = ~Us;
    constexpr Direction Up       = pawn_push(Us);
    constexpr Bitboard  TRank3BB = (Us == WHITE ? Rank3BB : Rank6BB);

    Bitboard b, weak, defended, nonPawnEnemies, stronglyProtected, safe;
    Score score = SCORE_ZERO;

    // Bonuses for variants with mandatory captures
    if (pos.must_capture())
    {
        // Penalties for possible captures
        Bitboard captures = attackedBy[Us][ALL_PIECES] & pos.pieces(Them);
        if (captures)
            score -= make_score(2000, 2000) / (1 + popcount(captures & attackedBy[Them][ALL_PIECES] & ~attackedBy2[Us]));

        // Bonus if we threaten to force captures
        Bitboard moves = 0, piecebb = pos.pieces(Us);
        while (piecebb)
        {
            Square s = pop_lsb(piecebb);
            if (type_of(pos.piece_on(s)) != KING)
                moves |= pos.moves_from(Us, type_of(pos.piece_on(s)), s);
        }
        score += make_score(200, 200) * popcount(attackedBy[Them][ALL_PIECES] & moves & ~pos.pieces());
        score += make_score(200, 220) * popcount(attackedBy[Them][ALL_PIECES] & moves & ~pos.pieces() & ~attackedBy2[Us]);
    }

    // Extinction threats
    if (pos.extinction_value() == -VALUE_MATE)
    {
        Bitboard bExt = attackedBy[Us][ALL_PIECES] & pos.pieces(Them);
        for (PieceType pt : pos.extinction_piece_types())
        {
            if (pt == ALL_PIECES)
                continue;
            int denom = std::max(pos.count_with_hand(Them, pt) - pos.extinction_piece_count(), 1);
            // Explosion threats
            if (pos.blast_on_capture())
            {
                int evasions = popcount(((attackedBy[Them][pt] & ~pos.pieces(Them)) | pos.pieces(Them, pt)) & ~attackedBy[Us][ALL_PIECES]) * denom;
                int attacks = popcount((attackedBy[Them][pt] | pos.pieces(Them, pt)) & attackedBy[Us][ALL_PIECES]);
                int explosions = 0;

                Bitboard bExtBlast = bExt & (attackedBy2[Us] | ~attackedBy[Us][pt]);
                while (bExtBlast)
                {
                    Square s = pop_lsb(bExtBlast);
                    if (((attacks_bb<KING>(s) | s) & pos.pieces(Them, pt)) && !(attacks_bb<KING>(s) & pos.pieces(Us, pt)))
                        explosions++;
                }
                int danger = 20 * attacks / (evasions + 1) + 40 * explosions;
                score += make_score(danger * (100 + danger), 0);
            }
            else
                // Direct extinction threats
                score += make_score(1000, 1000) / (denom * denom) * popcount(bExt & pos.pieces(Them, pt));
        }
    }

    // Non-pawn enemies
    nonPawnEnemies = pos.pieces(Them) & ~pos.pieces(PAWN, SHOGI_PAWN) & ~pos.pieces(SOLDIER);

    // Squares strongly protected by the enemy, either because they defend the
    // square with a pawn, or because they defend the square twice and we don't.
    stronglyProtected =  (attackedBy[Them][PAWN] | attackedBy[Them][SHOGI_PAWN] | attackedBy[Them][SOLDIER])
                       | (attackedBy2[Them] & ~attackedBy2[Us]);

    // Non-pawn enemies, strongly protected
    defended = nonPawnEnemies & stronglyProtected;

    // Enemies not strongly protected and under our attack
    weak = pos.pieces(Them) & ~stronglyProtected & attackedBy[Us][ALL_PIECES];

    // Bonus according to the kind of attacking pieces
    if (defended | weak)
    {
        b = (defended | weak) & (attackedBy[Us][KNIGHT] | attackedBy[Us][BISHOP]);
        while (b)
            score += ThreatByMinor[type_of(pos.piece_on(pop_lsb(b)))];

        b = weak & attackedBy[Us][ROOK];
        while (b)
            score += ThreatByRook[type_of(pos.piece_on(pop_lsb(b)))];

        if (weak & attackedBy[Us][KING])
            score += ThreatByKing;

        b =  ~attackedBy[Them][ALL_PIECES]
           | (nonPawnEnemies & attackedBy2[Us]);
        score += Hanging * popcount(weak & b);

        // Additional bonus if weak piece is only protected by a queen
        score += WeakQueenProtection * popcount(weak & attackedBy[Them][QUEEN]);
    }

    // Bonus for restricting their piece moves
    b =   attackedBy[Them][ALL_PIECES]
       & ~stronglyProtected
       &  attackedBy[Us][ALL_PIECES];
    score += RestrictedPiece * popcount(b);

    // Protected or unattacked squares
    safe = ~attackedBy[Them][ALL_PIECES] | attackedBy[Us][ALL_PIECES];

    // Bonus for attacking enemy pieces with our relatively safe pawns
    b = pos.pieces(Us, PAWN) & safe;
    b = pawn_attacks_bb<Us>(b) & nonPawnEnemies;
    score += ThreatBySafePawn * popcount(b);

    // Find squares where our pawns can push on the next move
    b  = shift<Up>(pos.pieces(Us, PAWN)) & ~pos.pieces();
    b |= shift<Up>(b & TRank3BB) & ~pos.pieces();

    // Keep only the squares which are relatively safe
    b &= ~attackedBy[Them][PAWN] & safe;

    // Bonus for safe pawn threats on the next move
    b = (pawn_attacks_bb<Us>(b) | shift<Up>(shift<Up>(pos.pieces(Us, SHOGI_PAWN, SOLDIER)))) & nonPawnEnemies;
    score += ThreatByPawnPush * popcount(b);

    // Bonus for threats on the next moves against enemy queen
    if (pos.count<QUEEN>(Them) == 1)
    {
        bool queenImbalance = pos.count<QUEEN>() == 1;

        Square s = pos.square<QUEEN>(Them);
        safe =   mobilityArea[Us]
              & ~pos.pieces(Us, PAWN)
              & ~stronglyProtected;

        b = attackedBy[Us][KNIGHT] & attacks_bb<KNIGHT>(s);

        score += KnightOnQueen * popcount(b & safe) * (1 + queenImbalance);

        b =  (attackedBy[Us][BISHOP] & attacks_bb<BISHOP>(s, pos.pieces()))
           | (attackedBy[Us][ROOK  ] & attacks_bb<ROOK  >(s, pos.pieces()));

        score += SliderOnQueen * popcount(b & safe & attackedBy2[Us]) * (1 + queenImbalance);
    }

    if constexpr (T)
        Trace::add(THREAT, Us, score);

    return score;
  }

  // Evaluation::passed() evaluates the passed pawns and candidate passed
  // pawns of the given color.

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::passed() const {

    constexpr Color     Them = ~Us;
    constexpr Direction Up   = pawn_push(Us);
    constexpr Direction Down = -Up;

    auto king_proximity = [&](Color c, Square s) {
      return pos.extinction_value() == VALUE_MATE ? 0 : pos.count<KING>(c) ? std::min(distance(pos.square<KING>(c), s), 5) : 5;
    };

    Bitboard b, bb, squaresToQueen, unsafeSquares, blockedPassers, helpers;
    Score score = SCORE_ZERO;

    b = pe->passed_pawns(Us);

    blockedPassers = b & shift<Down>(pos.pieces(Them, PAWN));
    if (blockedPassers)
    {
        helpers =  shift<Up>(pos.pieces(Us, PAWN))
                 & ~pos.pieces(Them)
                 & (~attackedBy2[Them] | attackedBy[Us][ALL_PIECES]);

        // Remove blocked candidate passers that don't have help to pass
        b &=  ~blockedPassers
            | shift<WEST>(helpers)
            | shift<EAST>(helpers);
    }

    while (b)
    {
        Square s = pop_lsb(b);

        assert(!(pos.pieces(Them, PAWN) & forward_file_bb(Us, s + Up)));

        int r = std::max(RANK_8 - std::max(pos.promotion_rank() - relative_rank(Us, s, pos.max_rank()), 0), 0);

        Score bonus = PassedRank[r];

        if (r > RANK_3)
        {
            int w = 5 * r - 13;
            Square blockSq = s + Up;

            // Adjust bonus based on the king's proximity
            bonus += make_score(0, (  king_proximity(Them, blockSq) * 19 / 4
                                    - king_proximity(Us,   blockSq) *  2) * w);

            // If blockSq is not the queening square then consider also a second push
            if (r != RANK_7)
                bonus -= make_score(0, king_proximity(Us, blockSq + Up) * w);

            // If the pawn is free to advance, then increase the bonus
            if (pos.empty(blockSq))
            {
                squaresToQueen = forward_file_bb(Us, s);
                unsafeSquares = passed_pawn_span(Us, s);

                bb = forward_file_bb(Them, s) & pos.pieces(ROOK, QUEEN);

                if (!(pos.pieces(Them) & bb))
                    unsafeSquares &= attackedBy[Them][ALL_PIECES] | pos.pieces(Them);

                // If there are no enemy pieces or attacks on passed pawn span, assign a big bonus.
                // Or if there is some, but they are all attacked by our pawns, assign a bit smaller bonus.
                // Otherwise assign a smaller bonus if the path to queen is not attacked
                // and even smaller bonus if it is attacked but block square is not.
                int k = !unsafeSquares                    ? 36 :
                !(unsafeSquares & ~attackedBy[Us][PAWN])  ? 30 :
                        !(unsafeSquares & squaresToQueen) ? 17 :
                        !(unsafeSquares & blockSq)        ?  7 :
                                                             0 ;

                // Assign a larger bonus if the block square is defended
                if ((pos.pieces(Us) & bb) || (attackedBy[Us][ALL_PIECES] & blockSq))
                    k += 5;

                bonus += make_score(k * w, k * w);
            }
        } // r > RANK_3

        score += bonus - PassedFile * edge_distance(file_of(s), pos.max_file());
    }

    // Scale by maximum promotion piece value
    Value maxMg = VALUE_ZERO, maxEg = VALUE_ZERO;
    for (PieceType pt : pos.promotion_piece_types())
    {
        maxMg = std::max(maxMg, PieceValue[MG][pt]);
        maxEg = std::max(maxEg, PieceValue[EG][pt]);
    }
    score = make_score(mg_value(score) * int(maxMg - PawnValueMg) / (QueenValueMg - PawnValueMg),
                       eg_value(score) * int(maxEg - PawnValueEg) / (QueenValueEg - PawnValueEg));

    // Score passed shogi pawns
    PieceType pt = pos.promoted_piece_type(SHOGI_PAWN);
    if (pt != NO_PIECE_TYPE)
    {
        b = pos.pieces(Us, SHOGI_PAWN);
        while (b)
        {
            Square s = pop_lsb(b);
            if ((pos.pieces(Them, SHOGI_PAWN) & forward_file_bb(Us, s)) || relative_rank(Us, s, pos.max_rank()) == pos.max_rank())
                continue;

            Square blockSq = s + Up;
            int d = 2 * std::max(pos.promotion_rank() - relative_rank(Us, s, pos.max_rank()), 1);
            d += !!(attackedBy[Them][ALL_PIECES] & ~attackedBy2[Us] & blockSq);
            score += make_score(PieceValue[MG][pt], PieceValue[EG][pt]) / (d * d);
        }
    }

    if constexpr (T)
        Trace::add(PASSED, Us, score);

    return score;
  }


  // Evaluation::space() computes a space evaluation for a given side, aiming to improve game
  // play in the opening. It is based on the number of safe squares on the four central files
  // on ranks 2 to 4. Completely safe squares behind a friendly pawn are counted twice.
  // Finally, the space bonus is multiplied by a weight which decreases according to occupancy.

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::space() const {

    bool pawnsOnly = !(pos.pieces(Us) ^ pos.pieces(Us, PAWN));

    // Early exit if, for example, both queens or 6 minor pieces have been exchanged
    if (pos.non_pawn_material() < SpaceThreshold && !pawnsOnly && pos.double_step_enabled())
        return SCORE_ZERO;

    constexpr Color Them     = ~Us;
    constexpr Direction Down = -pawn_push(Us);
    constexpr Bitboard SpaceMask =
      Us == WHITE ? CenterFiles & (Rank2BB | Rank3BB | Rank4BB)
                  : CenterFiles & (Rank7BB | Rank6BB | Rank5BB);

    // Find the available squares for our pieces inside the area defined by SpaceMask
    Bitboard safe =   SpaceMask
                   & ~pos.pieces(Us, PAWN)
                   & ~attackedBy[Them][PAWN];

    // Find all squares which are at most three squares behind some friendly pawn
    Bitboard behind = pos.pieces(Us, PAWN);
    behind |= shift<Down>(behind);
    behind |= shift<Down+Down>(behind);

    if (pawnsOnly)
    {
        safe = pos.board_bb() & ((attackedBy2[Us] & ~attackedBy2[Them]) | (attackedBy[Us][PAWN] & ~pos.pieces(Us, PAWN)));
        behind = 0;
    }

    // Compute space score based on the number of safe squares and number of our pieces
    // increased with number of total blocked pawns in position.
    int bonus = popcount(safe) + popcount(behind & safe & ~attackedBy[Them][ALL_PIECES]);
    int weight = pos.count<ALL_PIECES>(Us) - 3 + std::min(pe->blocked_count(), 9);
    Score score = make_score(bonus * weight * weight / 16, 0);

    if (pos.capture_the_flag(Us))
        score += make_score(200, 200) * popcount(behind & safe & pos.capture_the_flag(Us));

    if constexpr (T)
        Trace::add(SPACE, Us, score);

    return score;
  }


  // Evaluation::variant() computes variant-specific evaluation bonuses for a given side.

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::variant() const {

    constexpr Color Them = ~Us;
    constexpr Direction Down = pawn_push(Them);

    Score score = SCORE_ZERO;

    // Capture the flag
    if (pos.capture_the_flag(Us))
    {
        PieceType ptCtf = pos.capture_the_flag_piece();
        Bitboard ctfPieces = pos.pieces(Us, ptCtf);
        Bitboard ctfTargets = pos.capture_the_flag(Us) & pos.board_bb();
        Bitboard onHold = 0;
        Bitboard onHold2 = 0;
        Bitboard processed = 0;
        Bitboard blocked = pos.pieces(Us, PAWN) | attackedBy[Them][ALL_PIECES];
        Bitboard doubleBlocked =  attackedBy2[Them]
                                | (pos.pieces(Us, PAWN) & (shift<Down>(pos.pieces()) | attackedBy[Them][ALL_PIECES]))
                                | (pos.pieces(Them) & pe->pawn_attacks(Them))
                                | (pawn_attacks_bb<Them>(pos.pieces(Them, PAWN) & pe->pawn_attacks(Them)));
        Bitboard inaccessible = pos.pieces(Us, PAWN) & shift<Down>(pos.pieces(Them, PAWN));
        // Traverse all paths of the CTF pieces to the CTF targets.
        // Put squares that are attacked or occupied on hold for one iteration.
        // This reflects that likely a move will be needed to block or capture the attack.
        for (int dist = 0; (ctfPieces || onHold || onHold2) && (ctfTargets & ~processed); dist++)
        {
            int wins = popcount(ctfTargets & ctfPieces);
            if (wins)
                score += make_score(4000, 4000) * wins / (wins + dist * dist);
            Bitboard current = ctfPieces & ~ctfTargets;
            processed |= ctfPieces;
            ctfPieces = onHold & ~processed;
            onHold = onHold2 & ~processed;
            onHold2 = 0;
            while (current)
            {
                Square s = pop_lsb(current);
                Bitboard attacks = (  (PseudoAttacks[Us][ptCtf][s] & pos.pieces())
                                    | (PseudoMoves[Us][ptCtf][s] & ~pos.pieces())) & ~processed & pos.board_bb();
                ctfPieces |= attacks & ~blocked;
                onHold |= attacks & ~doubleBlocked;
                onHold2 |= attacks & ~inaccessible;
            }
        }
    }

    // nCheck
    if (pos.check_counting())
    {
        int remainingChecks = pos.checks_remaining(Us);
        assert(remainingChecks > 0);
        score += make_score(3600, 1000) / (remainingChecks * remainingChecks);
    }

    // Extinction
    if (pos.extinction_value() != VALUE_NONE)
    {
        for (PieceType pt : pos.extinction_piece_types())
            if (pt != ALL_PIECES)
            {
                // Single piece type extinction bonus
                int denom = std::max(pos.count(Us, pt) - pos.extinction_piece_count(), 1);
                if (pos.count(Them, pt) >= pos.extinction_opponent_piece_count() || pos.two_boards())
                    score += make_score(1000000 / (500 + PieceValue[MG][pt]),
                                        1000000 / (500 + PieceValue[EG][pt])) / (denom * denom)
                            * (pos.extinction_value() / VALUE_MATE);
            }
            else if (pos.extinction_value() == VALUE_MATE)
            {
                // Losing chess variant bonus
                score += make_score(pos.non_pawn_material(Us), pos.non_pawn_material(Us)) / pos.count<ALL_PIECES>(Us);
            }
            else if (pos.count<PAWN>(Us) == pos.count<ALL_PIECES>(Us))
            {
                // Pawns easy to stop/capture
                int l = 0, m = 0, r = popcount(pos.pieces(Us, PAWN) & file_bb(FILE_A));
                for (File f = FILE_A; f <= pos.max_file(); ++f)
                {
                    l = m; m = r; r = popcount(pos.pieces(Us, PAWN) & shift<EAST>(file_bb(f)));
                    score -= make_score(80 - 10 * (edge_distance(f, pos.max_file()) % 2),
                                        80 - 15 * (edge_distance(f, pos.max_file()) % 2)) * m / (1 + l * r);
                }
            }
            else if (pos.count<PAWN>(Them) == pos.count<ALL_PIECES>(Them))
            {
                // Add a bonus according to how close we are to breaking through the pawn wall
                int dist = 8;
                Bitboard breakthroughs = attackedBy[Us][ALL_PIECES] & rank_bb(relative_rank(Us, pos.max_rank(), pos.max_rank()));
                if (breakthroughs)
                    dist = attackedBy[Us][QUEEN] & breakthroughs ? 0 : 1;
                else for (File f = FILE_A; f <= pos.max_file(); ++f)
                    dist = std::min(dist, popcount(pos.pieces(PAWN) & file_bb(f)));
                score += make_score(70, 70) * pos.count<PAWN>(Them) / (1 + dist * dist) / (pos.pieces(Us, QUEEN) ? 2 : 4);
            }
    }

    // Connect-n
    if (pos.connect_n() > 0)
    {
        for (Direction d : {NORTH, NORTH_EAST, EAST, SOUTH_EAST})
        {
            // Find sufficiently large gaps
            Bitboard b = pos.board_bb() & ~pos.pieces(Them);
            for (int i = 1; i < pos.connect_n(); i++)
                b &= shift(d, b);
            // Count number of pieces per gap
            while (b)
            {
                Square s = pop_lsb(b);
                int c = 0;
                for (int j = 0; j < pos.connect_n(); j++)
                    if (pos.pieces(Us) & (s - j * d))
                        c++;
                score += make_score(200, 200)  * c / (pos.connect_n() - c) / (pos.connect_n() - c);
            }
        }
    }

    // Potential piece flips (Reversi)
    if (pos.flip_enclosed_pieces())
    {
        // Stable pieces
        if (pos.flip_enclosed_pieces() == REVERSI)
        {
            Bitboard edges = (FileABB | file_bb(pos.max_file()) | Rank1BB | rank_bb(pos.max_rank())) & pos.board_bb();
            Bitboard edgePieces = pos.pieces(Us) & edges;
            while (edgePieces)
            {
                Bitboard connectedEdge = attacks_bb(Us, ROOK, pop_lsb(edgePieces), ~(pos.pieces(Us) & edges)) & edges;
                if (!more_than_one(connectedEdge & ~pos.pieces(Us)))
                    score += make_score(300, 300);
                else if (!(connectedEdge & ~pos.pieces()))
                    score += make_score(200, 200);
            }
        }

        // Unstable
        Bitboard unstable = 0;
        Bitboard drops = pos.drop_region(Them, IMMOBILE_PIECE);
        while (drops)
        {
            Square s = pop_lsb(drops);
            if (pos.flip_enclosed_pieces() == REVERSI)
            {
                Bitboard b = attacks_bb(Them, QUEEN, s, ~pos.pieces(Us)) & ~PseudoAttacks[Them][KING][s] & pos.pieces(Them);
                while(b)
                    unstable |= between_bb(s, pop_lsb(b));
            }
            else
                unstable |= PseudoAttacks[Them][KING][s] & pos.pieces(Us);
        }
        score -= make_score(200, 200) * popcount(unstable);
    }

    if (T)
        Trace::add(VARIANT, Us, score);

    return score;
  }


  // Evaluation::winnable() adjusts the midgame and endgame score components, based on
  // the known attacking/defending status of the players. The final value is derived
  // by interpolation from the midgame and endgame values.

  template<Tracing T>
  Value Evaluation<T>::winnable(Score score) const {

    // No initiative bonus for extinction variants
    int complexity = 0;
    bool pawnsOnBothFlanks = true;
    if (pos.extinction_value() == VALUE_NONE && !pos.captures_to_hand() && !pos.connect_n() && !pos.material_counting())
    {
    int outflanking = !pos.count<KING>(WHITE) || !pos.count<KING>(BLACK) ? 0
                     :  distance<File>(pos.square<KING>(WHITE), pos.square<KING>(BLACK))
                    + int(rank_of(pos.square<KING>(WHITE)) - rank_of(pos.square<KING>(BLACK)));

        pawnsOnBothFlanks =   (pos.pieces(PAWN) & QueenSide)
                            && (pos.pieces(PAWN) & KingSide);

    bool almostUnwinnable =   outflanking < 0
                           && pos.stalemate_value() == VALUE_DRAW
                           && !pawnsOnBothFlanks;

    bool infiltration =   (pos.count<KING>(WHITE) && rank_of(pos.square<KING>(WHITE)) > RANK_4)
                       || (pos.count<KING>(BLACK) && rank_of(pos.square<KING>(BLACK)) < RANK_5);

    // Compute the initiative bonus for the attacking side
    complexity =       9 * pe->passed_count()
                    + 12 * pos.count<PAWN>()
                    + 15 * pos.count<SOLDIER>()
                    +  9 * outflanking
                    + 21 * pawnsOnBothFlanks
                    + 24 * infiltration
                    + 51 * !pos.non_pawn_material()
                    - 43 * almostUnwinnable
                    -110 ;
    }

    Value mg = mg_value(score);
    Value eg = eg_value(score);

    // Now apply the bonus: note that we find the attacking side by extracting the
    // sign of the midgame or endgame values, and that we carefully cap the bonus
    // so that the midgame and endgame scores do not change sign after the bonus.
    int u = ((mg > 0) - (mg < 0)) * std::clamp(complexity + 50, -abs(mg), 0);
    int v = ((eg > 0) - (eg < 0)) * std::max(complexity, -abs(eg));

    mg += u;
    eg += v;

    // Compute the scale factor for the winning side
    Color strongSide = eg > VALUE_DRAW ? WHITE : BLACK;
    int sf = me->scale_factor(pos, strongSide);

    // If scale factor is not already specific, scale up/down via general heuristics
    if (sf == SCALE_FACTOR_NORMAL && !pos.captures_to_hand() && !pos.material_counting())
    {
        if (pos.opposite_bishops())
        {
            // For pure opposite colored bishops endgames use scale factor
            // based on the number of passed pawns of the strong side.
            if (   pos.non_pawn_material(WHITE) == BishopValueMg
                && pos.non_pawn_material(BLACK) == BishopValueMg)
                sf = 18 + 4 * popcount(pe->passed_pawns(strongSide));
            // For every other opposite colored bishops endgames use scale factor
            // based on the number of all pieces of the strong side.
            else
                sf = 22 + 3 * pos.count<ALL_PIECES>(strongSide);
        }
        // For rook endgames with strong side not having overwhelming pawn number advantage
        // and its pawns being on one flank and weak side protecting its pieces with a king
        // use lower scale factor.
        else if (  pos.non_pawn_material(WHITE) == RookValueMg
                && pos.non_pawn_material(BLACK) == RookValueMg
                && pos.count<PAWN>(strongSide) - pos.count<PAWN>(~strongSide) <= 1
                && bool(KingSide & pos.pieces(strongSide, PAWN)) != bool(QueenSide & pos.pieces(strongSide, PAWN))
                && pos.count<KING>(~strongSide)
                && (attacks_bb<KING>(pos.square<KING>(~strongSide)) & pos.pieces(~strongSide, PAWN)))
            sf = 36;
        // For queen vs no queen endgames use scale factor
        // based on number of minors of side that doesn't have queen.
        else if (pos.count<QUEEN>() == 1)
            sf = 37 + 3 * (pos.count<QUEEN>(WHITE) == 1 ? pos.count<BISHOP>(BLACK) + pos.count<KNIGHT>(BLACK)
                                                        : pos.count<BISHOP>(WHITE) + pos.count<KNIGHT>(WHITE));
        // In every other case use scale factor based on
        // the number of pawns of the strong side reduced if pawns are on a single flank.
        else
            sf = std::min(sf, 36 + 7 * (pos.count<PAWN>(strongSide) + pos.count<SOLDIER>(strongSide))) - 4 * !pawnsOnBothFlanks;

        // Reduce scale factor in case of pawns being on a single flank
        sf -= 4 * !pawnsOnBothFlanks;
    }

    // Interpolate between the middlegame and (scaled by 'sf') endgame score
    v =  mg * int(me->game_phase())
       + eg * int(PHASE_MIDGAME - me->game_phase()) * ScaleFactor(sf) / SCALE_FACTOR_NORMAL;
    v /= PHASE_MIDGAME;

    if constexpr (T)
    {
        Trace::add(WINNABLE, make_score(u, eg * ScaleFactor(sf) / SCALE_FACTOR_NORMAL - eg_value(score)));
        Trace::add(TOTAL, make_score(mg, eg * ScaleFactor(sf) / SCALE_FACTOR_NORMAL));
    }

    return Value(v);
  }


  // Evaluation::value() is the main function of the class. It computes the various
  // parts of the evaluation and returns the value of the position from the point
  // of view of the side to move.

  template<Tracing T>
  Value Evaluation<T>::value() {

    assert(!pos.checkers());
    assert(!pos.is_immediate_game_end());

    // Probe the material hash table
    me = Material::probe(pos);

    // If we have a specialized evaluation function for the current material
    // configuration, call it and return.
    if (me->specialized_eval_exists())
        return me->evaluate(pos);

    // Initialize score by reading the incrementally updated scores included in
    // the position object (material + piece square tables) and the material
    // imbalance. Score is computed internally from the white point of view.
    Score score = pos.psq_score();
    if (T)
        Trace::add(MATERIAL, score);
    score += me->imbalance() + pos.this_thread()->contempt;

    // Probe the pawn hash table
    pe = Pawns::probe(pos);
    score += pe->pawn_score(WHITE) - pe->pawn_score(BLACK);

    // Early exit if score is high
    auto lazy_skip = [&](Value lazyThreshold) {
        return abs(mg_value(score) + eg_value(score)) / 2 > lazyThreshold + pos.non_pawn_material() / 64;
    };

    if (lazy_skip(LazyThreshold1) && Options["UCI_Variant"] == "chess")
        goto make_v;

    // Main evaluation begins here
    std::memset(attackedBy, 0, sizeof(attackedBy));
    initialize<WHITE>();
    initialize<BLACK>();

    // Pieces evaluated first (also populates attackedBy, attackedBy2).
    // For unused piece types, we still need to set attack bitboard to zero.
    for (PieceType pt : pos.piece_types())
        if (pt != SHOGI_PAWN && pt != PAWN && pt != KING)
            score += pieces<WHITE>(pt) - pieces<BLACK>(pt);

    // Evaluate pieces in hand once attack tables are complete
    if (pos.piece_drops() || pos.seirawan_gating())
        for (PieceType pt : pos.piece_types())
            score += hand<WHITE>(pt) - hand<BLACK>(pt);

    score += (mobility[WHITE] - mobility[BLACK]) * (1 + pos.captures_to_hand() + pos.must_capture() + pos.check_counting());

    // More complex interactions that require fully populated attack bitboards
    score +=  king<   WHITE>() - king<   BLACK>()
            + passed< WHITE>() - passed< BLACK>()
            + variant<WHITE>() - variant<BLACK>();

    if (lazy_skip(LazyThreshold2) && Options["UCI_Variant"] == "chess")
        goto make_v;

    score +=  threats<WHITE>() - threats<BLACK>()
            + space<  WHITE>() - space<  BLACK>();

make_v:
    // Derive single value from mg and eg parts of score
    Value v = winnable(score);

    // In case of tracing add all remaining individual evaluation terms
    if constexpr (T)
    {
        Trace::add(IMBALANCE, me->imbalance());
        Trace::add(PAWN, pe->pawn_score(WHITE), pe->pawn_score(BLACK));
        Trace::add(MOBILITY, mobility[WHITE], mobility[BLACK]);
    }

    // Evaluation grain
    v = (v / 16) * 16;

    // Side to move point of view
    v = (pos.side_to_move() == WHITE ? v : -v);

    return v;
  }


  /// Fisher Random Chess: correction for cornered bishops, to fix chess960 play with NNUE

  Value fix_FRC(const Position& pos) {

    constexpr Bitboard Corners =  Bitboard(1ULL) << SQ_A1 | Bitboard(1ULL) << SQ_H1 | Bitboard(1ULL) << SQ_A8 | Bitboard(1ULL) << SQ_H8;

    if (!(pos.pieces(BISHOP) & Corners))
        return VALUE_ZERO;

    int correction = 0;

    if (   pos.piece_on(SQ_A1) == W_BISHOP
        && pos.piece_on(SQ_B2) == W_PAWN)
        correction += !pos.empty(SQ_B3) ? -CorneredBishop * 4
                                        : -CorneredBishop * 3;

    if (   pos.piece_on(SQ_H1) == W_BISHOP
        && pos.piece_on(SQ_G2) == W_PAWN)
        correction += !pos.empty(SQ_G3) ? -CorneredBishop * 4
                                        : -CorneredBishop * 3;

    if (   pos.piece_on(SQ_A8) == B_BISHOP
        && pos.piece_on(SQ_B7) == B_PAWN)
        correction += !pos.empty(SQ_B6) ? CorneredBishop * 4
                                        : CorneredBishop * 3;

    if (   pos.piece_on(SQ_H8) == B_BISHOP
        && pos.piece_on(SQ_G7) == B_PAWN)
        correction += !pos.empty(SQ_G6) ? CorneredBishop * 4
                                        : CorneredBishop * 3;

    return pos.side_to_move() == WHITE ?  Value(correction)
                                       : -Value(correction);
  }

} // namespace Eval


/// evaluate() is the evaluator for the outer world. It returns a static
/// evaluation of the position from the point of view of the side to move.

Value Eval::evaluate(const Position& pos) {

  pos.this_thread()->on_eval();

  Value v;

  if (NNUE::useNNUE == NNUE::UseNNUEMode::Pure) {
      v = NNUE::evaluate(pos);

      // Guarantee evaluation does not hit the tablebase range
      v = std::clamp(v, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);

      return v;
  }
  else if (NNUE::useNNUE == NNUE::UseNNUEMode::False)
      v = Evaluation<NO_TRACE>(pos).value();
  else
  {
      // Scale and shift NNUE for compatibility with search and classical evaluation
      auto  adjusted_NNUE = [&]()
      {

         int scale = 903 + 28 * pos.count<PAWN>() + 28 * pos.non_pawn_material() / 1024;

         Value nnue = NNUE::evaluate(pos, true) * scale / 1024;

         if (pos.is_chess960())
             nnue += fix_FRC(pos);

         if (pos.check_counting())
         {
             Color us = pos.side_to_move();
             nnue +=  6 * scale / (5 * pos.checks_remaining( us))
                    - 6 * scale / (5 * pos.checks_remaining(~us));
         }

         return nnue;
      };

      // If there is PSQ imbalance we use the classical eval. We also introduce
      // a small probability of using the classical eval when PSQ imbalance is small.
      Value psq = Value(abs(eg_value(pos.psq_score())));
      int   r50 = 16 + pos.rule50_count();
      bool  pure = !pos.check_counting();
      bool  largePsq = psq * 16 > (NNUEThreshold1 + pos.non_pawn_material() / 64) * r50 && !pure;
      bool  classical = largePsq;

      // Use classical evaluation for really low piece endgames.
      // One critical case is the draw for bishop + A/H file pawn vs naked king.
      bool lowPieceEndgame =   pos.non_pawn_material() == BishopValueMg
                            || (pos.non_pawn_material() < 2 * RookValueMg && pos.count<PAWN>() < 2);

      v = classical || lowPieceEndgame ? Evaluation<NO_TRACE>(pos).value()
                                       : adjusted_NNUE();

      // If the classical eval is small and imbalance large, use NNUE nevertheless.
      // For the case of opposite colored bishops, switch to NNUE eval with small
      // probability if the classical eval is less than the threshold.
      if (    largePsq
          && !lowPieceEndgame
          && (   abs(v) * 16 < NNUEThreshold2 * r50
              || (   pos.opposite_bishops()
                  && abs(v) * 16 < (NNUEThreshold1 + pos.non_pawn_material() / 64) * r50)))
          v = adjusted_NNUE();
  }

  // Damp down the evaluation linearly when shuffling
  if (pos.n_move_rule())
  {
      v = v * (2 * pos.n_move_rule() - pos.rule50_count()) / (2 * pos.n_move_rule());
      if (pos.material_counting())
          v += pos.material_counting_result() / (10 * std::max(2 * pos.n_move_rule() - pos.rule50_count(), 1));
  }

  // Guarantee evaluation does not hit the virtual win/loss range
  if (pos.two_boards() && std::abs(v) >= VALUE_VIRTUAL_MATE_IN_MAX_PLY)
      v += v > VALUE_ZERO ? MAX_PLY + 1 : -MAX_PLY - 1;

  // Guarantee evaluation does not hit the tablebase range
  v = std::clamp(v, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);

  return v;
}

/// trace() is like evaluate(), but instead of returning a value, it returns
/// a string (suitable for outputting to stdout) that contains the detailed
/// descriptions and values of each evaluation term. Useful for debugging.
/// Trace scores are from white's point of view

std::string Eval::trace(const Position& pos) {

  if (pos.checkers())
      return "Final evaluation: none (in check)";

  std::stringstream ss;
  ss << std::showpoint << std::noshowpos << std::fixed << std::setprecision(2);

  Value v;

  std::memset(scores, 0, sizeof(scores));

  pos.this_thread()->contempt = SCORE_ZERO; // Reset any dynamic contempt

  v = Evaluation<TRACE>(pos).value();

  ss << std::showpoint << std::noshowpos << std::fixed << std::setprecision(2)
     << "     Term    |    White    |    Black    |    Total   \n"
     << "             |   MG    EG  |   MG    EG  |   MG    EG \n"
     << " ------------+-------------+-------------+------------\n"
     << "    Material | " << Term(MATERIAL)
     << "   Imbalance | " << Term(IMBALANCE)
     << "       Pawns | " << Term(PAWN)
     << "     Knights | " << Term(KNIGHT)
     << "     Bishops | " << Term(BISHOP)
     << "       Rooks | " << Term(ROOK)
     << "      Queens | " << Term(QUEEN)
     << "    Mobility | " << Term(MOBILITY)
     << " King safety | " << Term(KING)
     << "     Threats | " << Term(THREAT)
     << "      Passed | " << Term(PASSED)
     << "       Space | " << Term(SPACE)
     << "     Variant | " << Term(VARIANT)
     << "    Winnable | " << Term(WINNABLE)
     << " ------------+-------------+-------------+------------\n"
     << "       Total | " << Term(TOTAL);

  v = pos.side_to_move() == WHITE ? v : -v;

  ss << "\nClassical evaluation: " << to_cp(v) << " (white side)\n";

  if (NNUE::useNNUE != NNUE::UseNNUEMode::False)
  {
      v = NNUE::evaluate(pos);
      v = pos.side_to_move() == WHITE ? v : -v;
      ss << "\nNNUE evaluation:      " << to_cp(v) << " (white side)\n";
  }

  v = evaluate(pos);
  v = pos.side_to_move() == WHITE ? v : -v;
  ss << "\nFinal evaluation:     " << to_cp(v) << " (white side)\n";

  return ss.str();
}

} // namespace Stockfish
