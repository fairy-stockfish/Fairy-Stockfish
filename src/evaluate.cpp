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
  constexpr int QueenSafeCheck  = 780;
  constexpr int RookSafeCheck   = 1080;
  constexpr int BishopSafeCheck = 635;
  constexpr int KnightSafeCheck = 790;
  constexpr int OtherSafeCheck  = 600;

#define S(mg, eg) make_score(mg, eg)

  // MobilityBonus[PieceType-2][attacked] contains bonuses for middle and end game,
  // indexed by piece type and number of attacked squares in the mobility area.
#ifdef LARGEBOARDS
  constexpr Score MobilityBonus[][38] = {
#else
  constexpr Score MobilityBonus[][32] = {
#endif
    { S(-62,-81), S(-53,-56), S(-12,-30), S( -4,-14), S(  3,  8), S( 13, 15), // Knights
      S( 22, 23), S( 28, 27), S( 33, 33) },
    { S(-48,-59), S(-20,-23), S( 16, -3), S( 26, 13), S( 38, 24), S( 51, 42), // Bishops
      S( 55, 54), S( 63, 57), S( 63, 65), S( 68, 73), S( 81, 78), S( 81, 86),
      S( 91, 88), S( 98, 97) },
    { S(-58,-76), S(-27,-18), S(-15, 28), S(-10, 55), S( -5, 69), S( -2, 82), // Rooks
      S(  9,112), S( 16,118), S( 30,132), S( 29,142), S( 32,155), S( 38,165),
      S( 46,166), S( 48,169), S( 58,171) },
    { S(-39,-36), S(-21,-15), S(  3,  8), S(  3, 18), S( 14, 34), S( 22, 54), // Queens
      S( 28, 61), S( 41, 73), S( 43, 79), S( 48, 92), S( 56, 94), S( 60,104),
      S( 60,113), S( 66,120), S( 67,123), S( 70,126), S( 71,133), S( 73,136),
      S( 79,140), S( 88,143), S( 88,148), S( 99,166), S(102,170), S(102,175),
      S(106,184), S(109,191), S(113,206), S(116,212) }
  };
  constexpr Score MaxMobility  = S(250, 250);
  constexpr Score DropMobility = S(10, 10);

  // RookOnFile[semiopen/open] contains bonuses for each rook when there is
  // no (friendly) pawn on the rook file.
  constexpr Score RookOnFile[] = { S(21, 4), S(47, 25) };

  // ThreatByMinor/ByRook[attacked PieceType] contains bonuses according to
  // which piece type attacks which one. Attacks on lesser pieces which are
  // pawn-defended are not considered.
  constexpr Score ThreatByMinor[PIECE_TYPE_NB] = {
    S(0, 0), S(6, 32), S(59, 41), S(79, 56), S(90, 119), S(79, 161)
  };

  constexpr Score ThreatByRook[PIECE_TYPE_NB] = {
    S(0, 0), S(3, 44), S(38, 71), S(38, 61), S(0, 38), S(51, 38)
  };

  // PassedRank[Rank] contains a bonus according to the rank of a passed pawn
  constexpr Score PassedRank[RANK_NB] = {
    S(0, 0), S(10, 28), S(17, 33), S(15, 41), S(62, 72), S(168, 177), S(276, 260)
  };

  // KingProximity contains a penalty according to distance from king
  constexpr Score KingProximity = S(1, 3);

  // Assorted bonuses and penalties
  constexpr Score BishopPawns        = S(  3,  7);
  constexpr Score CorneredBishop     = S( 50, 50);
  constexpr Score FlankAttacks       = S(  8,  0);
  constexpr Score Hanging            = S( 69, 36);
  constexpr Score KingProtector      = S(  7,  8);
  constexpr Score KnightOnQueen      = S( 16, 12);
  constexpr Score LongDiagonalBishop = S( 45,  0);
  constexpr Score MinorBehindPawn    = S( 18,  3);
  constexpr Score Outpost            = S( 30, 21);
  constexpr Score PassedFile         = S( 11,  8);
  constexpr Score PawnlessFlank      = S( 17, 95);
  constexpr Score RestrictedPiece    = S(  7,  7);
  constexpr Score ReachableOutpost   = S( 32, 10);
  constexpr Score RookOnQueenFile    = S(  7,  6);
  constexpr Score SliderOnQueen      = S( 59, 18);
  constexpr Score ThreatByKing       = S( 24, 89);
  constexpr Score ThreatByPawnPush   = S( 48, 39);
  constexpr Score ThreatBySafePawn   = S(173, 94);
  constexpr Score TrappedRook        = S( 52, 10);
  constexpr Score WeakQueen          = S( 49, 15);

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

    constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
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
        mobilityArea[Us] = ~(b | pos.pieces(Us, KING, QUEEN) | pos.blockers_for_king(Us) | pe->pawn_attacks(Them) | shift<Down>(pos.pieces(Them, SHOGI_PAWN, SOLDIER)));

    // Initialize attackedBy[] for king and pawns
    attackedBy[Us][KING] = pos.count<KING>(Us) ? pos.attacks_from<KING>(ksq, Us) : Bitboard(0);
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
        Square s = make_square(clamp(file_of(ksq), FILE_B, File(pos.max_file() - 1)),
                               clamp(rank_of(ksq), RANK_2, Rank(pos.max_rank() - 1)));
        kingRing[Us] = PseudoAttacks[Us][KING][s] | s;
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

    constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
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
        b &= pos.board_bb();

        if (Pt == SOLDIER && pos.unpromoted_soldier(Us, s))
        {
            b &= file_bb(s);
            score -= make_score(PieceValue[MG][Pt], PieceValue[EG][Pt]) / 3;
        }

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

        if (Pt > QUEEN)
             b = (b & pos.pieces()) | (pos.moves_from(Us, Pt, s) & ~pos.pieces() & pos.board_bb());

        int mob = popcount(b & mobilityArea[Us]);

        if (Pt <= QUEEN)
            mobility[Us] += MobilityBonus[Pt - 2][mob];
        else
            mobility[Us] += MaxMobility * (mob - 1) / (8 + mob);

        // Piece promotion bonus
        if (pos.promoted_piece_type(Pt) != NO_PIECE_TYPE)
        {
            if (promotion_zone_bb(Us, pos.promotion_rank(), pos.max_rank()) & (b | s))
                score += make_score(PieceValue[MG][pos.promoted_piece_type(Pt)] - PieceValue[MG][Pt],
                                    PieceValue[EG][pos.promoted_piece_type(Pt)] - PieceValue[EG][Pt]) / 10;
        }
        else if (pos.captures_to_hand() && pos.unpromoted_piece_on(s))
            score += make_score(PieceValue[MG][Pt] - PieceValue[MG][pos.unpromoted_piece_on(s)],
                                PieceValue[EG][Pt] - PieceValue[EG][pos.unpromoted_piece_on(s)]) / 8;

        // Penalty if the piece is far from the kings in drop variants
        if (pos.captures_to_hand() && pos.count<KING>(Them) && pos.count<KING>(Us))
            score -= KingProximity * distance(s, pos.square<KING>(Us)) * distance(s, pos.square<KING>(Them));

        if (Pt == BISHOP || Pt == KNIGHT)
        {
            // Bonus if piece is on an outpost square or can reach one
            bb = OutpostRanks & attackedBy[Us][PAWN] & ~pe->pawn_attacks_span(Them);
            if (bb & s)
                score += Outpost * (Pt == KNIGHT ? 2 : 1);

            else if (Pt == KNIGHT && bb & b & ~pos.pieces(Us))
                score += ReachableOutpost;

            // Knight and Bishop bonus for being right behind a pawn
            if (shift<Down>(pos.pieces(PAWN)) & s)
                score += MinorBehindPawn;

            // Penalty if the piece is far from the king
            if (pos.count<KING>(Us))
            score -= KingProtector * distance(s, pos.square<KING>(Us));

            if (Pt == BISHOP)
            {
                // Penalty according to number of pawns on the same color square as the
                // bishop, bigger when the center files are blocked with pawns.
                Bitboard blocked = pos.pieces(Us, PAWN) & shift<Down>(pos.pieces());

                score -= BishopPawns * pos.pawns_on_same_color_squares(Us, s)
                                     * (1 + popcount(blocked & CenterFiles));

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

    constexpr Color Them = (Us == WHITE ? BLACK : WHITE);

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
    }

    return score;
  }

  // Evaluation::king() assigns bonuses and penalties to a king of a given color
  template<Tracing T> template<Color Us>
  Score Evaluation<T>::king() const {

    constexpr Color    Them = (Us == WHITE ? BLACK : WHITE);
    Rank r = relative_rank(Us, std::min(Rank((pos.max_rank() - 1) / 2 + 1), pos.max_rank()), pos.max_rank());
    Bitboard Camp = AllSquares ^ forward_ranks_bb(Us, r);

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
    safe &= ~attackedBy[Us][ALL_PIECES] | (weak & attackedBy2[Them]);

    b1 = attacks_bb<ROOK  >(ksq, pos.pieces() ^ pos.pieces(Us, QUEEN));
    b2 = attacks_bb<BISHOP>(ksq, pos.pieces() ^ pos.pieces(Us, QUEEN));

    std::function <Bitboard (Color, PieceType)> get_attacks = [this](Color c, PieceType pt) {
        return attackedBy[c][pt] | (pos.captures_to_hand() && pos.count_in_hand(c, pt) ? ~pos.pieces() : Bitboard(0));
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
                        & safe
                        & ~attackedBy[Us][QUEEN]
                        & ~(b1 & attackedBy[Them][ROOK]);

            if (queenChecks)
                kingDanger += QueenSafeCheck;
            break;
        case ROOK:
        case BISHOP:
        case KNIGHT:
            knightChecks = attacks_bb(Us, pt, ksq, pos.pieces() ^ pos.pieces(Us, QUEEN)) & get_attacks(Them, pt) & pos.board_bb();
            if (knightChecks & safe)
                kingDanger +=  pt == ROOK   ? RookSafeCheck
                             : pt == BISHOP ? BishopSafeCheck
                                            : KnightSafeCheck;
            else
                unsafeChecks |= knightChecks;
            break;
        case PAWN:
            if (pos.captures_to_hand() && pos.count_in_hand(Them, pt))
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
                kingDanger += OtherSafeCheck;
            else
                unsafeChecks |= otherChecks;
        }
    }

    if (pos.check_counting())
        kingDanger *= 2;

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
                 + 185 * popcount(kingRing[Us] & weak) * (1 + pos.captures_to_hand() + pos.check_counting())
                 + 148 * popcount(unsafeChecks)
                 +  98 * popcount(pos.blockers_for_king(Us))
                 +  69 * kingAttacksCount[Them] * (2 + 8 * pos.check_counting() + pos.captures_to_hand()) / 2
                 +   3 * kingFlankAttack * kingFlankAttack / 8
                 +       mg_value(mobility[Them] - mobility[Us])
                 - 873 * !(pos.major_pieces(Them) || pos.captures_to_hand() || pos.king_type() == WAZIR) / (1 + pos.check_counting())
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

    if (pos.check_counting() || pos.king_type() == WAZIR)
        score += make_score(0, mg_value(score) / 2);

    // For drop games, king danger is independent of game phase
    if (pos.captures_to_hand())
        score = make_score(mg_value(score), mg_value(score)) * 2 / (2 + 3 * !pos.shogi_doubled_pawn());

    if (T)
        Trace::add(KING, Us, score);

    return score;
  }


  // Evaluation::threats() assigns bonuses according to the types of the
  // attacking and the attacked pieces.
  template<Tracing T> template<Color Us>
  Score Evaluation<T>::threats() const {

    constexpr Color     Them     = (Us == WHITE ? BLACK   : WHITE);
    constexpr Direction Up       = pawn_push(Us);
    constexpr Bitboard  TRank3BB = (Us == WHITE ? Rank3BB : Rank6BB);

    Bitboard b, weak, defended, nonPawnEnemies, stronglyProtected, safe;
    Score score = SCORE_ZERO;

    // Bonuses for variants with mandatory captures
    if (pos.must_capture())
    {
        // Penalties for possible captures
        score -= make_score(100, 100) * popcount(attackedBy[Us][ALL_PIECES] & pos.pieces(Them));

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

        b = attackedBy[Us][KNIGHT] & pos.attacks_from<KNIGHT>(s, Us);

        score += KnightOnQueen * popcount(b & safe);

        b =  (attackedBy[Us][BISHOP] & pos.attacks_from<BISHOP>(s, Us))
           | (attackedBy[Us][ROOK  ] & pos.attacks_from<ROOK  >(s, Us));

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

    constexpr Color     Them = (Us == WHITE ? BLACK : WHITE);
    constexpr Direction Up   = pawn_push(Us);

    auto king_proximity = [&](Color c, Square s) {
      return pos.count<KING>(c) ? std::min(distance(pos.square<KING>(c), s), 5) : 5;
    };

    Bitboard b, bb, squaresToQueen, unsafeSquares;
    Score score = SCORE_ZERO;

    b = pe->passed_pawns(Us);

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

            // Skip bonus for antichess variants
            if (pos.extinction_value() != VALUE_MATE)
            {
                // Adjust bonus based on the king's proximity
                bonus += make_score(0, (  (king_proximity(Them, blockSq) * 19) / 4
                                         - king_proximity(Us,   blockSq) *  2) * w);

                // If blockSq is not the queening square then consider also a second push
                if (r != RANK_7)
                    bonus -= make_score(0, king_proximity(Us, blockSq + Up) * w);
            }

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

        // Scale down bonus for candidate passers which need more than one
        // pawn push to become passed, or have a pawn in front of them.
        if (   !pos.pawn_passed(Us, s + Up)
            || (pos.pieces(PAWN) & (s + Up)))
            bonus = bonus / 2;

        score += bonus - PassedFile * std::min(file_of(s), File(pos.max_file() - file_of(s)));
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

    constexpr Color Them     = (Us == WHITE ? BLACK : WHITE);
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
    int weight = pos.count<ALL_PIECES>(Us) - 1;
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

    constexpr Color Them = (Us == WHITE ? BLACK : WHITE);

    Score score = SCORE_ZERO;

    // Capture the flag
    if (pos.capture_the_flag(Us))
    {
        bool isKingCTF = pos.capture_the_flag_piece() == KING;
        Bitboard ctfPieces = pos.pieces(Us, pos.capture_the_flag_piece());
        int scale = pos.count(Us, pos.capture_the_flag_piece());
        while (ctfPieces)
        {
            Square s1 = pop_lsb(&ctfPieces);
            Bitboard target_squares = pos.capture_the_flag(Us) & pos.board_bb();
            while (target_squares)
            {
                Square s2 = pop_lsb(&target_squares);
                int dist =  distance(s1, s2)
                          + (isKingCTF || pos.flag_move() ? popcount(pos.attackers_to(s2, Them)) : 0)
                          + !!(pos.pieces(Us) & s2);
                score += make_score(2500, 2500) / (1 + scale * dist * dist);
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
                score += make_score(1100, 1100) / pos.count(Us, pt) * (pos.extinction_value() / VALUE_MATE);
            else if (pos.extinction_value() == VALUE_MATE && !pos.count<KING>(Us))
                score += make_score(5000, pos.non_pawn_material(Us)) / pos.count<ALL_PIECES>(Us);
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

    if (T)
        Trace::add(VARIANT, Us, score);

    return score;
  }


  // Evaluation::initiative() computes the initiative correction value
  // for the position. It is a second order bonus/malus based on the
  // known attacking/defending status of the players.

  template<Tracing T>
  Score Evaluation<T>::initiative(Score score) const {

    Value mg = mg_value(score);
    Value eg = eg_value(score);

    // No initiative bonus for extinction variants
    if (pos.extinction_value() != VALUE_NONE || pos.captures_to_hand() || pos.connect_n())
      return SCORE_ZERO;

    int outflanking = !pos.count<KING>(WHITE) || !pos.count<KING>(BLACK) ? 0
                     :  distance<File>(pos.square<KING>(WHITE), pos.square<KING>(BLACK))
                      - distance<Rank>(pos.square<KING>(WHITE), pos.square<KING>(BLACK));

    bool infiltration =   (pos.count<KING>(WHITE) && rank_of(pos.square<KING>(WHITE)) > RANK_4)
                       || (pos.count<KING>(BLACK) && rank_of(pos.square<KING>(BLACK)) < RANK_5);

    bool pawnsOnBothFlanks =   (pos.pieces(PAWN) & QueenSide)
                            && (pos.pieces(PAWN) & KingSide);

    bool almostUnwinnable =   !pe->passed_count()
                           &&  outflanking < 0
                           && pos.stalemate_value() == VALUE_DRAW
                           && !pawnsOnBothFlanks;

    // Compute the initiative bonus for the attacking side
    int complexity =   9 * pe->passed_count()
                    + 11 * pos.count<PAWN>()
                    + 15 * pos.count<SOLDIER>()
                    +  9 * outflanking
                    + 12 * infiltration
                    + 21 * pawnsOnBothFlanks
                    + 51 * !pos.non_pawn_material()
                    - 43 * almostUnwinnable
                    - 100 ;

    // Now apply the bonus: note that we find the attacking side by extracting the
    // sign of the midgame or endgame values, and that we carefully cap the bonus
    // so that the midgame and endgame scores do not change sign after the bonus.
    int u = ((mg > 0) - (mg < 0)) * std::max(std::min(complexity + 50, 0), -abs(mg));
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
        if (   pos.opposite_bishops()
            && pos.non_pawn_material() == 2 * BishopValueMg)
            sf = 22 ;
        else
            sf = std::min(sf, 36 + (pos.opposite_bishops() ? 2 : 7) * (pos.count<PAWN>(strongSide) + pos.count<SOLDIER>(strongSide)));

        sf = std::max(0, sf - (pos.rule50_count() - 12) / 4);
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

    score += (mobility[WHITE] - mobility[BLACK]) * (1 + pos.captures_to_hand() + pos.must_capture());

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

    return  (pos.side_to_move() == WHITE ? v : -v) // Side to move point of view
           + Eval::tempo_value(pos);
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
