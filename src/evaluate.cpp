/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2020 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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
#include <cstring>   // For std::memset
#include <iomanip>
#include <sstream>

#include "bitboard.h"
#include "evaluate.h"
#include "material.h"
#include "pawns.h"
#include "thread.h"
#include "uci.h"

namespace Trace {

  enum Tracing { NO_TRACE, TRACE };

  enum Term { // The first PIECE_TYPE_NB entries are reserved for PieceType
    MATERIAL = PIECE_TYPE_NB, IMBALANCE, MOBILITY, THREAT, PASSED, SPACE, INITIATIVE, VARIANT, TOTAL, TERM_NB
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

    if (t == MATERIAL || t == IMBALANCE || t == INITIATIVE || t == TOTAL)
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
  constexpr Value LazyThreshold  = Value(1400);
  constexpr Value SpaceThreshold = Value(12222);

  // KingAttackWeights[PieceType] contains king attack weights by piece type
  constexpr int KingAttackWeights[PIECE_TYPE_NB] = { 0, 0, 81, 52, 44, 10, 40 };

  // Penalties for enemy's safe checks
  constexpr int QueenSafeCheck  = 772;
  constexpr int RookSafeCheck   = 1084;
  constexpr int BishopSafeCheck = 645;
  constexpr int KnightSafeCheck = 792;

  constexpr int OtherSafeCheck  = 600;

#define S(mg, eg) make_score(mg, eg)

  // MobilityBonus[PieceType-2][attacked] contains bonuses for middle and end game,
  // indexed by piece type and number of attacked squares in the mobility area.
  constexpr Score MobilityBonus[][4 * RANK_NB] = {
    { S(-62,-81), S(-53,-56), S(-12,-31), S( -4,-16), S(  3,  5), S( 13, 11), // Knight
      S( 22, 17), S( 28, 20), S( 33, 25) },
    { S(-48,-59), S(-20,-23), S( 16, -3), S( 26, 13), S( 38, 24), S( 51, 42), // Bishop
      S( 55, 54), S( 63, 57), S( 63, 65), S( 68, 73), S( 81, 78), S( 81, 86),
      S( 91, 88), S( 98, 97) },
    { S(-60,-78), S(-20,-17), S(  2, 23), S(  3, 39), S(  3, 70), S( 11, 99), // Rook
      S( 22,103), S( 31,121), S( 40,134), S( 40,139), S( 41,158), S( 48,164),
      S( 57,168), S( 57,169), S( 62,172) },
    { S(-30,-48), S(-12,-30), S( -8, -7), S( -9, 19), S( 20, 40), S( 23, 55), // Queen
      S( 23, 59), S( 35, 75), S( 38, 78), S( 53, 96), S( 64, 96), S( 65,100),
      S( 65,121), S( 66,127), S( 67,131), S( 67,133), S( 72,136), S( 72,141),
      S( 77,147), S( 79,150), S( 93,151), S(108,168), S(108,168), S(108,171),
      S(110,182), S(114,182), S(114,192), S(116,219) }
  };
  constexpr Score MaxMobility  = S(150, 200);
  constexpr Score DropMobility = S(10, 10);

  // RookOnFile[semiopen/open] contains bonuses for each rook when there is
  // no (friendly) pawn on the rook file.
  constexpr Score RookOnFile[] = { S(19, 7), S(48, 29) };

  // ThreatByMinor/ByRook[attacked PieceType] contains bonuses according to
  // which piece type attacks which one. Attacks on lesser pieces which are
  // pawn-defended are not considered.
  constexpr Score ThreatByMinor[PIECE_TYPE_NB] = {
    S(0, 0), S(5, 32), S(57, 41), S(77, 56), S(88, 119), S(79, 161)
  };

  constexpr Score ThreatByRook[PIECE_TYPE_NB] = {
    S(0, 0), S(3, 46), S(37, 68), S(42, 60), S(0, 38), S(58, 41)
  };

  // PassedRank[Rank] contains a bonus according to the rank of a passed pawn
  constexpr Score PassedRank[RANK_NB] = {
    S(0, 0), S(10, 28), S(17, 33), S(15, 41), S(62, 72), S(168, 177), S(276, 260)
  };

  // KingProximity contains a penalty according to distance from king
  constexpr Score KingProximity = S(1, 3);
  constexpr Score EndgameKingProximity = S(0, 10);

  // Assorted bonuses and penalties
  constexpr Score BishopPawns         = S(  3,  7);
  constexpr Score BishopXRayPawns     = S(  4,  5);
  constexpr Score CorneredBishop      = S( 50, 50);
  constexpr Score FlankAttacks        = S(  8,  0);
  constexpr Score Hanging             = S( 69, 36);
  constexpr Score BishopKingProtector = S(  6,  9);
  constexpr Score KnightKingProtector = S(  8,  9);
  constexpr Score KnightOnQueen       = S( 16, 11);
  constexpr Score LongDiagonalBishop  = S( 45,  0);
  constexpr Score MinorBehindPawn     = S( 18,  3);
  constexpr Score KnightOutpost       = S( 56, 36);
  constexpr Score BishopOutpost       = S( 30, 23);
  constexpr Score ReachableOutpost    = S( 31, 22);
  constexpr Score PassedFile          = S( 11,  8);
  constexpr Score PawnlessFlank       = S( 17, 95);
  constexpr Score RestrictedPiece     = S(  7,  7);
  constexpr Score RookOnKingRing      = S( 16,  0);
  constexpr Score RookOnQueenFile     = S(  5,  9);
  constexpr Score SliderOnQueen       = S( 59, 18);
  constexpr Score ThreatByKing        = S( 24, 89);
  constexpr Score ThreatByPawnPush    = S( 48, 39);
  constexpr Score ThreatBySafePawn    = S(173, 94);
  constexpr Score TrappedRook         = S( 55, 13);
  constexpr Score WeakQueen           = S( 51, 14);
  constexpr Score WeakQueenProtection = S( 15,  0);

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
    ScaleFactor scale_factor(Value eg) const;
    Score initiative(Score score) const;

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
        Square s = make_square(Utility::clamp(file_of(ksq), FILE_B, File(pos.max_file() - 1)),
                               Utility::clamp(rank_of(ksq), RANK_2, Rank(pos.max_rank() - 1)));
        kingRing[Us] = attacks_bb<KING>(s) | s;
    }

    kingAttackersCount[Them] = popcount(kingRing[Us] & pe->pawn_attacks(Them));
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
    const Square* pl = pos.squares(Us, Pt);

    Bitboard b, bb;
    Score score = SCORE_ZERO;

    attackedBy[Us][Pt] = 0;

    for (Square s = *pl; s != SQ_NONE; s = *++pl)
    {
        // Find attacked squares, including x-ray attacks for bishops and rooks
        b = Pt == BISHOP ? attacks_bb<BISHOP>(s, pos.pieces() ^ pos.pieces(QUEEN))
          : Pt ==   ROOK ? attacks_bb<  ROOK>(s, pos.pieces() ^ pos.pieces(QUEEN) ^ pos.pieces(Us, ROOK))
                         : pos.attacks_from(Us, Pt, s);

        // Restrict mobility to actual squares of board
        b &= pos.board_bb(Us, Pt);

        if (pos.blockers_for_king(Us) & s)
            b &= LineBB[pos.square<KING>(Us)][s];

        attackedBy2[Us] |= attackedBy[Us][ALL_PIECES] & b;
        attackedBy[Us][Pt] |= b;
        attackedBy[Us][ALL_PIECES] |= b;

        if (b & kingRing[Them])
        {
            kingAttackersCount[Us]++;
            kingAttackersWeight[Us] += KingAttackWeights[std::min(int(Pt), QUEEN + 1)];
            kingAttacksCount[Us] += popcount(b & attackedBy[Them][KING]);
        }
        else if (Pt == ROOK && (file_bb(s) & kingRing[Them]))
            score += RookOnKingRing;

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
            if (promotion_zone_bb(Us, pos.promotion_rank(), pos.max_rank()) & (b | s))
                score += make_score(PieceValue[MG][pos.promoted_piece_type(Pt)] - PieceValue[MG][Pt],
                                    PieceValue[EG][pos.promoted_piece_type(Pt)] - PieceValue[EG][Pt]) / 10;
        }
        else if (pos.piece_demotion() && pos.unpromoted_piece_on(s))
            score -= make_score(PieceValue[MG][Pt] - PieceValue[MG][pos.unpromoted_piece_on(s)],
                                PieceValue[EG][Pt] - PieceValue[EG][pos.unpromoted_piece_on(s)]) / 4;
        else if (pos.captures_to_hand() && pos.unpromoted_piece_on(s))
            score += make_score(PieceValue[MG][Pt] - PieceValue[MG][pos.unpromoted_piece_on(s)],
                                PieceValue[EG][Pt] - PieceValue[EG][pos.unpromoted_piece_on(s)]) / 8;

        // Penalty if the piece is far from the kings in drop variants
        if ((pos.captures_to_hand() || pos.two_boards()) && pos.count<KING>(Them) && pos.count<KING>(Us))
            score -= KingProximity * distance(s, pos.square<KING>(Us)) * distance(s, pos.square<KING>(Them));

        else if (pos.count<KING>(Us) && (Pt == FERS || Pt == SILVER))
            score -= EndgameKingProximity * (distance(s, pos.square<KING>(Us)) - 2);

        if (Pt == BISHOP || Pt == KNIGHT)
        {
            // Bonus if piece is on an outpost square or can reach one
            bb = OutpostRanks & attackedBy[Us][PAWN] & ~pe->pawn_attacks_span(Them);
            if (bb & s)
                score += (Pt == KNIGHT) ? KnightOutpost : BishopOutpost;
            else if (Pt == KNIGHT && bb & b & ~pos.pieces(Us))
                score += ReachableOutpost;

            // Bonus for a knight or bishop shielded by pawn
            if (shift<Down>(pos.pieces(PAWN)) & s)
                score += MinorBehindPawn;

            // Penalty if the piece is far from the king
            if (pos.count<KING>(Us))
            score -= (Pt == KNIGHT ? KnightKingProtector
                                   : BishopKingProtector) * distance(pos.square<KING>(Us), s);

            if (Pt == BISHOP)
            {
                // Penalty according to the number of our pawns on the same color square as the
                // bishop, bigger when the center files are blocked with pawns and smaller
                // when the bishop is outside the pawn chain.
                Bitboard blocked = pos.pieces(Us, PAWN) & shift<Down>(pos.pieces());

                score -= BishopPawns * pos.pawns_on_same_color_squares(Us, s)
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
                        score -= !pos.empty(s + d + pawn_push(Us))                ? CorneredBishop * 4
                                : pos.piece_on(s + d + d) == make_piece(Us, PAWN) ? CorneredBishop * 2
                                                                                  : CorneredBishop;
                }
            }
        }

        if (Pt == ROOK)
        {
            // Bonus for rook on the same file as a queen
            if (file_bb(s) & pos.pieces(QUEEN))
                score += RookOnQueenFile;

            // Bonus for rook on an open or semi-open file
            if (pos.is_on_semiopen_file(Us, s))
                score += RookOnFile[pos.is_on_semiopen_file(Them, s)];

            // Penalty when trapped by the king, even more if the king cannot castle
            else if (mob <= 3 && pos.count<KING>(Us))
            {
                File kf = file_of(pos.square<KING>(Us));
                if ((kf < FILE_E) == (file_of(s) < kf))
                    score -= TrappedRook * (1 + !pos.castling_rights(Us));
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
    if (T)
        Trace::add(Pt, Us, score);

    return score;
  }

  // Evaluation::hand() scores pieces of a given color and type in hand
  template<Tracing T> template<Color Us>
  Score Evaluation<T>::hand(PieceType pt) {

    constexpr Color Them = ~Us;

    Score score = SCORE_ZERO;

    if (pos.count_in_hand(Us, pt))
    {
        Bitboard b = pos.drop_region(Us, pt) & ~pos.pieces() & (~attackedBy2[Them] | attackedBy[Us][ALL_PIECES]);
        if ((b & kingRing[Them]) && pt != SHOGI_PAWN)
        {
            kingAttackersCountInHand[Us] += pos.count_in_hand(Us, pt);
            kingAttackersWeightInHand[Us] += KingAttackWeights[std::min(int(pt), QUEEN + 1)] * pos.count_in_hand(Us, pt);
            kingAttacksCount[Us] += popcount(b & attackedBy[Them][KING]);
        }
        Bitboard theirHalf = pos.board_bb() & ~forward_ranks_bb(Them, relative_rank(Them, Rank((pos.max_rank() - 1) / 2), pos.max_rank()));
        mobility[Us] += DropMobility * popcount(b & theirHalf & ~attackedBy[Them][ALL_PIECES]);
        if (pos.promoted_piece_type(pt) != NO_PIECE_TYPE && pos.drop_promoted())
            score += make_score(std::max(PieceValue[MG][pos.promoted_piece_type(pt)] - PieceValue[MG][pt], VALUE_ZERO),
                                std::max(PieceValue[EG][pos.promoted_piece_type(pt)] - PieceValue[EG][pt], VALUE_ZERO)) / 4 * pos.count_in_hand(Us, pt);
        if (pos.enclosing_drop())
            mobility[Us] += make_score(500, 500) * popcount(b);
    }

    return score;
  }

  // Evaluation::king() assigns bonuses and penalties to a king of a given color
  template<Tracing T> template<Color Us>
  Score Evaluation<T>::king() const {

    constexpr Color    Them = ~Us;
    Rank r = relative_rank(Us, std::min(Rank((pos.max_rank() - 1) / 2 + 1), pos.max_rank()), pos.max_rank());
    Bitboard Camp = pos.board_bb() & ~forward_ranks_bb(Us, r);

    if (!pos.count<KING>(Us) || !pos.checking_permitted())
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
        return attackedBy[c][pt] | (pos.piece_drops() && pos.count_in_hand(c, pt) ? pos.drop_region(c, pt) & ~pos.pieces() : Bitboard(0));
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
                kingDanger += more_than_one(queenChecks) ? QueenSafeCheck * 145/100
                                                         : QueenSafeCheck;
            break;
        case ROOK:
        case BISHOP:
        case KNIGHT:
            knightChecks = attacks_bb(Us, pt, ksq, pos.pieces() ^ pos.pieces(Us, QUEEN)) & get_attacks(Them, pt) & pos.board_bb();
            if (knightChecks & safe)
                kingDanger +=  pt == ROOK   ? RookSafeCheck * (more_than_one(knightChecks & safe) ? 175 : 100) / 100
                             : pt == BISHOP ? BishopSafeCheck * (more_than_one(knightChecks & safe) ? 150 : 100) / 100
                                            : KnightSafeCheck * (more_than_one(knightChecks & safe) ? 162 : 100) / 100;
            else
                unsafeChecks |= knightChecks;
            break;
        case PAWN:
            if (pos.piece_drops() && pos.count_in_hand(Them, pt))
            {
                pawnChecks = attacks_bb(Us, pt, ksq, pos.pieces()) & ~pos.pieces() & pos.board_bb();
                if (pawnChecks & safe)
                    kingDanger += OtherSafeCheck;
                else
                    unsafeChecks |= pawnChecks;
            }
            break;
        case SHOGI_PAWN:
        case KING:
            break;
        default:
            otherChecks = attacks_bb(Us, pt, ksq, pos.pieces()) & get_attacks(Them, pt) & pos.board_bb();
            if (otherChecks & safe)
                kingDanger += OtherSafeCheck * (more_than_one(otherChecks & safe) ? 3 : 2) / 2;
            else
                unsafeChecks |= otherChecks;
        }
    }

    // Virtual piece drops
    if (pos.two_boards() && pos.piece_drops())
    {
        for (PieceType pt : pos.piece_types())
            if (!pos.count_in_hand(Them, pt) && (attacks_bb(Us, pt, ksq, pos.pieces()) & safe & pos.drop_region(Them, pt) & ~pos.pieces()))
            {
                kingDanger += OtherSafeCheck * 500 / (500 + PieceValue[MG][pt]);
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

    int kingFlankAttack = popcount(b1) + popcount(b2);
    int kingFlankDefense = popcount(b3);

    kingDanger +=        kingAttackersCount[Them] * kingAttackersWeight[Them]
                 +       kingAttackersCountInHand[Them] * kingAttackersWeight[Them]
                 +       kingAttackersCount[Them] * kingAttackersWeightInHand[Them]
                 + 185 * popcount(kingRing[Us] & (weak | ~pos.board_bb(Us, KING))) * (1 + pos.captures_to_hand() + pos.check_counting())
                 + 148 * popcount(unsafeChecks) * (1 + pos.check_counting())
                 +  98 * popcount(pos.blockers_for_king(Us))
                 +  69 * kingAttacksCount[Them] * (2 + 8 * pos.check_counting() + pos.captures_to_hand()) / 2
                 +   3 * kingFlankAttack * kingFlankAttack / 8
                 +       mg_value(mobility[Them] - mobility[Us])
                 - 873 * !(pos.major_pieces(Them) || pos.captures_to_hand())
                       * 2 / (2 + 2 * pos.check_counting() + 2 * pos.two_boards() + 2 * pos.makpong()
                                + (pos.king_type() != KING) * (pos.diagonal_lines() ? 1 : 2))
                 - 100 * bool(attackedBy[Us][KNIGHT] & attackedBy[Us][KING])
                 -   6 * mg_value(score) / 8
                 -   4 * kingFlankDefense
                 +  37;

    // Transform the kingDanger units into a Score, and subtract it from the evaluation
    if (kingDanger > 100)
        score -= make_score(std::min(kingDanger * kingDanger / 4096, 3000), kingDanger / 16);

    // Penalty when our king is on a pawnless flank
    if (!(pos.pieces(PAWN) & kingFlank))
        score -= PawnlessFlank;

    // Penalty if king flank is under attack, potentially moving toward the king
    score -= FlankAttacks * kingFlankAttack * (1 + 5 * pos.captures_to_hand() + pos.check_counting());

    if (pos.check_counting())
        score += make_score(0, mg_value(score) * 2 / (2 + pos.checks_remaining(Them)));

    if (pos.king_type() == WAZIR)
        score += make_score(0, mg_value(score) / 2);

    // For drop games, king danger is independent of game phase
    if (pos.captures_to_hand() || pos.two_boards())
        score = make_score(mg_value(score), mg_value(score)) * 2 / (2 + 16 / pos.max_rank() * !pos.shogi_doubled_pawn());

    if (T)
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
            Square s = pop_lsb(&piecebb);
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
        while (bExt)
        {
            PieceType pt = type_of(pos.piece_on(pop_lsb(&bExt)));
            int denom = std::max(pos.count_with_hand(Them, pt) - pos.extinction_piece_count(), 1);
            if (pos.extinction_piece_types().find(pt) != pos.extinction_piece_types().end())
                score += make_score(1000, 1000) / (denom * denom);
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
            score += ThreatByMinor[type_of(pos.piece_on(pop_lsb(&b)))];

        b = weak & attackedBy[Us][ROOK];
        while (b)
            score += ThreatByRook[type_of(pos.piece_on(pop_lsb(&b)))];

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
        Square s = pos.square<QUEEN>(Them);
        safe = mobilityArea[Us] & ~stronglyProtected;

        b = attackedBy[Us][KNIGHT] & attacks_bb<KNIGHT>(s);

        score += KnightOnQueen * popcount(b & safe);

        b =  (attackedBy[Us][BISHOP] & attacks_bb<BISHOP>(s, pos.pieces()))
           | (attackedBy[Us][ROOK  ] & attacks_bb<ROOK  >(s, pos.pieces()));

        score += SliderOnQueen * popcount(b & safe & attackedBy2[Us]);
    }

    if (T)
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
        Square s = pop_lsb(&b);

        assert(!(pos.pieces(Them, PAWN) & forward_file_bb(Us, s + Up)));

        int r = std::max(RANK_8 - std::max(pos.promotion_rank() - relative_rank(Us, s, pos.max_rank()), 0), 0);

        Score bonus = PassedRank[r];

        if (r > RANK_3)
        {
            int w = 5 * r - 13;
            Square blockSq = s + Up;

            // Adjust bonus based on the king's proximity
            bonus += make_score(0, ( (king_proximity(Them, blockSq) * 19) / 4
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
                    unsafeSquares &= attackedBy[Them][ALL_PIECES];

                // If there are no enemy attacks on passed pawn span, assign a big bonus.
                // Otherwise assign a smaller bonus if the path to queen is not attacked
                // and even smaller bonus if it is attacked but block square is not.
                int k = !unsafeSquares                    ? 35 :
                        !(unsafeSquares & squaresToQueen) ? 20 :
                        !(unsafeSquares & blockSq)        ?  9 :
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
    const Square* pl = pos.squares(Us, SHOGI_PAWN);
    Square s;

    PieceType pt = pos.promoted_piece_type(SHOGI_PAWN);
    if (pt != NO_PIECE_TYPE)
    {
        while ((s = *pl++) != SQ_NONE)
        {
            if ((pos.pieces(Them, SHOGI_PAWN) & forward_file_bb(Us, s)) || relative_rank(Us, s, pos.max_rank()) == pos.max_rank())
                continue;

            Square blockSq = s + Up;
            int d = std::max(pos.promotion_rank() - relative_rank(Us, s, pos.max_rank()), 1);
            d += !!(attackedBy[Them][ALL_PIECES] & ~attackedBy2[Us] & blockSq);
            score += make_score(PieceValue[MG][pt], PieceValue[EG][pt]) / (4 * d * d);
        }
    }

    if (T)
        Trace::add(PASSED, Us, score);

    return score;
  }


  // Evaluation::space() computes the space evaluation for a given side. The
  // space evaluation is a simple bonus based on the number of safe squares
  // available for minor pieces on the central four files on ranks 2--4. Safe
  // squares one, two or three squares behind a friendly pawn are counted
  // twice. Finally, the space bonus is multiplied by a weight. The aim is to
  // improve play on game opening.

  template<Tracing T> template<Color Us>
  Score Evaluation<T>::space() const {

    bool pawnsOnly = !(pos.pieces(Us) ^ pos.pieces(Us, PAWN));

    if (pos.non_pawn_material() < SpaceThreshold && !pos.captures_to_hand() && !pawnsOnly)
        return SCORE_ZERO;

    constexpr Color Them     = ~Us;
    constexpr Direction Down = -pawn_push(Us);
    constexpr Bitboard SpaceMask =
      Us == WHITE ? CenterFiles & (Rank2BB | Rank3BB | Rank4BB)
                  : CenterFiles & (Rank7BB | Rank6BB | Rank5BB);

    // Find the available squares for our pieces inside the area defined by SpaceMask
    Bitboard safe =   SpaceMask
                   & ~pos.pieces(Us, PAWN, SHOGI_PAWN)
                   & ~attackedBy[Them][PAWN]
                   & ~attackedBy[Them][SHOGI_PAWN];

    // Find all squares which are at most three squares behind some friendly pawn
    Bitboard behind = pos.pieces(Us, PAWN, SHOGI_PAWN);
    behind |= shift<Down>(behind);
    behind |= shift<Down+Down>(behind);

    if (pawnsOnly)
    {
        safe = behind & ~attackedBy[Them][ALL_PIECES];
        behind = 0;
    }
    int bonus = popcount(safe) + popcount(behind & safe & ~attackedBy[Them][ALL_PIECES]);
    int weight = pos.count<ALL_PIECES>(Us) - 3 + std::min(pe->blocked_count(), 9);
    Score score = make_score(bonus * weight * weight / 16, 0);

    if (pos.capture_the_flag(Us))
        score += make_score(200, 200) * popcount(behind & safe & pos.capture_the_flag(Us));

    if (T)
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
                Square s = pop_lsb(&current);
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
                int denom = std::max(pos.count(Us, pt) - pos.extinction_piece_count(), 1);
                if (pos.count(Them, pt) >= pos.extinction_opponent_piece_count() || pos.two_boards())
                    score += make_score(1000000 / (500 + PieceValue[MG][pt]),
                                        1000000 / (500 + PieceValue[EG][pt])) / (denom * denom)
                            * (pos.extinction_value() / VALUE_MATE);
            }
            else if (pos.extinction_value() == VALUE_MATE)
                score += make_score(pos.non_pawn_material(Us), pos.non_pawn_material(Us)) / pos.count<ALL_PIECES>(Us);
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
                Square s = pop_lsb(&b);
                int c = 0;
                for (int j = 0; j < pos.connect_n(); j++)
                    if (pos.pieces(Us) & (s - j * d))
                        c++;
                score += make_score(200, 200)  * c / (pos.connect_n() - c) / (pos.connect_n() - c);
            }
        }
    }

    // Potential piece flips
    if (pos.flip_enclosed_pieces())
    {
        // Stable pieces
        if (pos.flip_enclosed_pieces() == REVERSI)
        {
            Bitboard edges = (FileABB | file_bb(pos.max_file()) | Rank1BB | rank_bb(pos.max_rank())) & pos.board_bb();
            Bitboard edgePieces = pos.pieces(Us) & edges;
            while (edgePieces)
            {
                Bitboard connectedEdge = attacks_bb(Us, ROOK, pop_lsb(&edgePieces), ~(pos.pieces(Us) & edges)) & edges;
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
            Square s = pop_lsb(&drops);
            if (pos.flip_enclosed_pieces() == REVERSI)
            {
                Bitboard b = attacks_bb(Them, QUEEN, s, ~pos.pieces(Us)) & ~PseudoAttacks[Them][KING][s] & pos.pieces(Them);
                while(b)
                    unstable |= between_bb(s, pop_lsb(&b));
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


  // Evaluation::initiative() computes the initiative correction value
  // for the position. It is a second order bonus/malus based on the
  // known attacking/defending status of the players.

  template<Tracing T>
  Score Evaluation<T>::initiative(Score score) const {

    // No initiative bonus for extinction variants
    if (pos.extinction_value() != VALUE_NONE || pos.captures_to_hand() || pos.connect_n())
      return SCORE_ZERO;

    int outflanking = !pos.count<KING>(WHITE) || !pos.count<KING>(BLACK) ? 0
                     :  distance<File>(pos.square<KING>(WHITE), pos.square<KING>(BLACK))
                      - distance<Rank>(pos.square<KING>(WHITE), pos.square<KING>(BLACK));

    bool pawnsOnBothFlanks =   (pos.pieces(PAWN) & QueenSide)
                            && (pos.pieces(PAWN) & KingSide);

    bool almostUnwinnable =   outflanking < 0
                           && pos.stalemate_value() == VALUE_DRAW
                           && !pawnsOnBothFlanks;

    bool infiltration =   (pos.count<KING>(WHITE) && rank_of(pos.square<KING>(WHITE)) > RANK_4)
                       || (pos.count<KING>(BLACK) && rank_of(pos.square<KING>(BLACK)) < RANK_5);

    // Compute the initiative bonus for the attacking side
    int complexity =   9 * pe->passed_count()
                    + 12 * pos.count<PAWN>()
                    + 15 * pos.count<SOLDIER>()
                    +  9 * outflanking
                    + 21 * pawnsOnBothFlanks
                    + 24 * infiltration
                    + 51 * !pos.non_pawn_material()
                    - 43 * almostUnwinnable
                    -  2 * pos.rule50_count()
                    -110 ;

    Value mg = mg_value(score);
    Value eg = eg_value(score);

    // Now apply the bonus: note that we find the attacking side by extracting the
    // sign of the midgame or endgame values, and that we carefully cap the bonus
    // so that the midgame and endgame scores do not change sign after the bonus.
    int u = ((mg > 0) - (mg < 0)) * Utility::clamp(complexity + 50, -abs(mg), 0);
    int v = ((eg > 0) - (eg < 0)) * std::max(complexity, -abs(eg));

    if (T)
        Trace::add(INITIATIVE, make_score(u, v));

    return make_score(u, v);
  }


  // Evaluation::scale_factor() computes the scale factor for the winning side

  template<Tracing T>
  ScaleFactor Evaluation<T>::scale_factor(Value eg) const {

    Color strongSide = eg > VALUE_DRAW ? WHITE : BLACK;
    int sf = me->scale_factor(pos, strongSide);

    // If scale is not already specific, scale down the endgame via general heuristics
    if (sf == SCALE_FACTOR_NORMAL && !pos.captures_to_hand())
    {
        if (pos.opposite_bishops())
        {
            if (   pos.non_pawn_material(WHITE) == BishopValueMg
                && pos.non_pawn_material(BLACK) == BishopValueMg)
                sf = 18 + 4 * popcount(pe->passed_pawns(strongSide));
            else
                sf = 22 + 3 * pos.count<ALL_PIECES>(strongSide);
        }
        else
            sf = std::min(sf, 36 + 7 * (pos.count<PAWN>(strongSide) + pos.count<SOLDIER>(strongSide)));
    }

    return ScaleFactor(sf);
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
    Value v = (mg_value(score) + eg_value(score)) / 2;
    if (abs(v) > LazyThreshold + pos.non_pawn_material() / 64 && Options["UCI_Variant"] == "chess")
       return pos.side_to_move() == WHITE ? v : -v;

    // Main evaluation begins here

    initialize<WHITE>();
    initialize<BLACK>();

    // Pieces should be evaluated first (populate attack tables).
    // For unused piece types, we still need to set attack bitboard to zero.
    for (PieceType pt = KNIGHT; pt < KING; ++pt)
        if (pt != SHOGI_PAWN)
            score += pieces<WHITE>(pt) - pieces<BLACK>(pt);

    // Evaluate pieces in hand once attack tables are complete
    if (pos.piece_drops() || pos.seirawan_gating())
        for (PieceType pt = PAWN; pt < KING; ++pt)
            score += hand<WHITE>(pt) - hand<BLACK>(pt);

    score += (mobility[WHITE] - mobility[BLACK]) * (1 + pos.captures_to_hand() + pos.must_capture() + pos.check_counting());

    // More complex interactions that require fully populated attack bitboards
    score +=  king<   WHITE>() - king<   BLACK>()
            + threats<WHITE>() - threats<BLACK>()
            + passed< WHITE>() - passed< BLACK>()
            + space<  WHITE>() - space<  BLACK>()
            + variant<WHITE>() - variant<BLACK>();

    score += initiative(score);

    // Interpolate between a middlegame and a (scaled by 'sf') endgame score
    ScaleFactor sf = scale_factor(eg_value(score));
    v =  mg_value(score) * int(me->game_phase())
       + eg_value(score) * int(PHASE_MIDGAME - me->game_phase()) * sf / SCALE_FACTOR_NORMAL;

    v /= PHASE_MIDGAME;

    // In case of tracing add all remaining individual evaluation terms
    if (T)
    {
        Trace::add(IMBALANCE, me->imbalance());
        Trace::add(PAWN, pe->pawn_score(WHITE), pe->pawn_score(BLACK));
        Trace::add(MOBILITY, mobility[WHITE], mobility[BLACK]);
        Trace::add(TOTAL, score);
    }

    // Side to move point of view
    return (pos.side_to_move() == WHITE ? v : -v) + Eval::tempo_value(pos);
  }

} // namespace


/// tempo_value() returns the evaluation offset for the side to move

Value Eval::tempo_value(const Position& pos) {
  return Tempo * (1 + 4 * pos.captures_to_hand());
}


/// evaluate() is the evaluator for the outer world. It returns a static
/// evaluation of the position from the point of view of the side to move.

Value Eval::evaluate(const Position& pos) {
  return Evaluation<NO_TRACE>(pos).value();
}


/// trace() is like evaluate(), but instead of returning a value, it returns
/// a string (suitable for outputting to stdout) that contains the detailed
/// descriptions and values of each evaluation term. Useful for debugging.

std::string Eval::trace(const Position& pos) {

  if (pos.checkers())
      return "Total evaluation: none (in check)";

  std::memset(scores, 0, sizeof(scores));

  pos.this_thread()->contempt = SCORE_ZERO; // Reset any dynamic contempt

  Value v = Evaluation<TRACE>(pos).value();

  v = pos.side_to_move() == WHITE ? v : -v; // Trace scores are from white's point of view

  std::stringstream ss;
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
     << "  Initiative | " << Term(INITIATIVE)
     << "     Variant | " << Term(VARIANT)
     << " ------------+-------------+-------------+------------\n"
     << "       Total | " << Term(TOTAL);

  ss << "\nTotal evaluation: " << to_cp(v) << " (white side)\n";

  return ss.str();
}
