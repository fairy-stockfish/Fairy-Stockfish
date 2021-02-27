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
#include <bitset>

#include "bitboard.h"
#include "magic.h"
#include "misc.h"
#include "piece.h"

uint8_t PopCnt16[1 << 16];
uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard SquareBB[SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard PseudoAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard PseudoMoves[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard LeaperAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard LeaperMoves[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard BoardSizeBB[FILE_NB][RANK_NB];
RiderType AttackRiderTypes[PIECE_TYPE_NB];
RiderType MoveRiderTypes[PIECE_TYPE_NB];

Magic RookMagicsH[SQUARE_NB];
Magic RookMagicsV[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];
Magic CannonMagicsH[SQUARE_NB];
Magic CannonMagicsV[SQUARE_NB];
Magic HorseMagics[SQUARE_NB];
Magic ElephantMagics[SQUARE_NB];
Magic JanggiElephantMagics[SQUARE_NB];

Magic* magics[] = {BishopMagics, RookMagicsH, RookMagicsV, CannonMagicsH, CannonMagicsV,
                   HorseMagics, ElephantMagics, JanggiElephantMagics};

namespace {

#ifdef LARGEBOARDS
  Bitboard RookTableH[0x11800];  // To store horizontalrook attacks
  Bitboard RookTableV[0x4800];  // To store vertical rook attacks
  Bitboard BishopTable[0x33C00]; // To store bishop attacks
  Bitboard CannonTableH[0x11800];  // To store horizontal cannon attacks
  Bitboard CannonTableV[0x4800];  // To store vertical cannon attacks
  Bitboard HorseTable[0x500];  // To store horse attacks
  Bitboard ElephantTable[0x400];  // To store elephant attacks
  Bitboard JanggiElephantTable[0x1C000];  // To store janggi elephant attacks
#else
  Bitboard RookTableH[0xA00];  // To store horizontal rook attacks
  Bitboard RookTableV[0xA00];  // To store vertical rook attacks
  Bitboard BishopTable[0x1480]; // To store bishop attacks
  Bitboard CannonTableH[0xA00];  // To store horizontal cannon attacks
  Bitboard CannonTableV[0xA00];  // To store vertical cannon attacks
  Bitboard HorseTable[0x240];  // To store horse attacks
  Bitboard ElephantTable[0x1A0];  // To store elephant attacks
  Bitboard JanggiElephantTable[0x5C00];  // To store janggi elephant attacks
#endif

  enum MovementType { RIDER, HOPPER, LAME_LEAPER };

  template <MovementType MT>
#ifdef PRECOMPUTED_MAGICS
  void init_magics(Bitboard table[], Magic magics[], std::vector<Direction> directions, Bitboard magicsInit[]);
#else
  void init_magics(Bitboard table[], Magic magics[], std::vector<Direction> directions);
#endif

  template <MovementType MT>
  Bitboard sliding_attack(std::vector<Direction> directions, Square sq, Bitboard occupied, Color c = WHITE) {
    assert(MT != LAME_LEAPER);

    Bitboard attack = 0;

    for (Direction d : directions)
    {
        bool hurdle = false;
        for (Square s = sq + (c == WHITE ? d : -d);
             is_ok(s) && distance(s, s - (c == WHITE ? d : -d)) == 1;
             s += (c == WHITE ? d : -d))
        {
            if (MT != HOPPER || hurdle)
                attack |= s;

            if (occupied & s)
            {
                if (MT == HOPPER && !hurdle)
                    hurdle = true;
                else
                    break;
            }
        }
    }

    return attack;
  }

  Bitboard lame_leaper_path(Direction d, Square s) {
    Direction dr = d > 0 ? NORTH : SOUTH;
    Direction df = (std::abs(d % NORTH) < NORTH / 2 ? d % NORTH : -(d % NORTH)) < 0 ? WEST : EAST;
    Square to = s + d;
    Bitboard b = 0;
    if (!is_ok(to) || distance(s, to) >= 4)
        return b;
    while (s != to)
    {
        int diff = std::abs(file_of(to) - file_of(s)) - std::abs(rank_of(to) - rank_of(s));
        if (diff > 0)
            s += df;
        else if (diff < 0)
            s += dr;
        else
            s += df + dr;

        if (s != to)
            b |= s;
    }
    return b;
  }

  Bitboard lame_leaper_path(std::vector<Direction> directions, Square s) {
    Bitboard b = 0;
    for (Direction d : directions)
        b |= lame_leaper_path(d, s);
    return b;
  }

  Bitboard lame_leaper_attack(std::vector<Direction> directions, Square s, Bitboard occupied) {
    Bitboard b = 0;
    for (Direction d : directions)
    {
        Square to = s + d;
        if (is_ok(to) && distance(s, to) < 4 && !(lame_leaper_path(d, s) & occupied))
            b |= to;
    }
    return b;
  }

}


/// safe_destination() returns the bitboard of target square for the given step
/// from the given square. If the step is off the board, returns empty bitboard.

inline Bitboard safe_destination(Square s, int step) {
    Square to = Square(s + step);
    return is_ok(to) && distance(s, to) <= 3 ? square_bb(to) : Bitboard(0);
}


/// Bitboards::pretty() returns an ASCII representation of a bitboard suitable
/// to be printed to standard output. Useful for debugging.

const std::string Bitboards::pretty(Bitboard b) {

  std::string s = "+---+---+---+---+---+---+---+---+---+---+---+---+\n";

  for (Rank r = RANK_MAX; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_MAX; ++f)
          s += b & make_square(f, r) ? "| X " : "|   ";

      s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h   i   j   k\n";

  return s;
}


/// Bitboards::init() initializes various bitboard tables. It is called at
/// startup and relies on global objects to be already zero-initialized.

void Bitboards::init() {

  // Piece moves
  std::vector<Direction> RookDirectionsV = { NORTH, SOUTH};
  std::vector<Direction> RookDirectionsH = { EAST, WEST };
  std::vector<Direction> BishopDirections = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };
  std::vector<Direction> HorseDirections = {2 * SOUTH + WEST, 2 * SOUTH + EAST, SOUTH + 2 * WEST, SOUTH + 2 * EAST,
                                            NORTH + 2 * WEST, NORTH + 2 * EAST, 2 * NORTH + WEST, 2 * NORTH + EAST };
  std::vector<Direction> ElephantDirections = { 2 * NORTH_EAST, 2 * SOUTH_EAST, 2 * SOUTH_WEST, 2 * NORTH_WEST };
  std::vector<Direction> JanggiElephantDirections = { NORTH + 2 * NORTH_EAST, EAST  + 2 * NORTH_EAST,
                                                      EAST  + 2 * SOUTH_EAST, SOUTH + 2 * SOUTH_EAST,
                                                      SOUTH + 2 * SOUTH_WEST, WEST + 2 * SOUTH_WEST,
                                                      WEST  + 2 * NORTH_WEST, NORTH + 2 * NORTH_WEST };

  // Initialize rider types
  for (PieceType pt = PAWN; pt <= KING; ++pt)
  {
      const PieceInfo* pi = pieceMap.find(pt)->second;

      if (pi->lameLeaper)
      {
          for (Direction d : pi->stepsCapture)
          {
              if (std::find(HorseDirections.begin(), HorseDirections.end(), d) != HorseDirections.end())
                  AttackRiderTypes[pt] |= RIDER_HORSE;
              if (std::find(ElephantDirections.begin(), ElephantDirections.end(), d) != ElephantDirections.end())
                  AttackRiderTypes[pt] |= RIDER_ELEPHANT;
              if (std::find(JanggiElephantDirections.begin(), JanggiElephantDirections.end(), d) != JanggiElephantDirections.end())
                  AttackRiderTypes[pt] |= RIDER_JANGGI_ELEPHANT;
          }
          for (Direction d : pi->stepsQuiet)
          {
              if (std::find(HorseDirections.begin(), HorseDirections.end(), d) != HorseDirections.end())
                  MoveRiderTypes[pt] |= RIDER_HORSE;
              if (std::find(ElephantDirections.begin(), ElephantDirections.end(), d) != ElephantDirections.end())
                  MoveRiderTypes[pt] |= RIDER_ELEPHANT;
              if (std::find(JanggiElephantDirections.begin(), JanggiElephantDirections.end(), d) != JanggiElephantDirections.end())
                  MoveRiderTypes[pt] |= RIDER_JANGGI_ELEPHANT;
          }
      }
      for (Direction d : pi->sliderCapture)
      {
          if (std::find(BishopDirections.begin(), BishopDirections.end(), d) != BishopDirections.end())
              AttackRiderTypes[pt] |= RIDER_BISHOP;
          if (std::find(RookDirectionsH.begin(), RookDirectionsH.end(), d) != RookDirectionsH.end())
              AttackRiderTypes[pt] |= RIDER_ROOK_H;
          if (std::find(RookDirectionsV.begin(), RookDirectionsV.end(), d) != RookDirectionsV.end())
              AttackRiderTypes[pt] |= RIDER_ROOK_V;
      }
      for (Direction d : pi->sliderQuiet)
      {
          if (std::find(BishopDirections.begin(), BishopDirections.end(), d) != BishopDirections.end())
              MoveRiderTypes[pt] |= RIDER_BISHOP;
          if (std::find(RookDirectionsH.begin(), RookDirectionsH.end(), d) != RookDirectionsH.end())
              MoveRiderTypes[pt] |= RIDER_ROOK_H;
          if (std::find(RookDirectionsV.begin(), RookDirectionsV.end(), d) != RookDirectionsV.end())
              MoveRiderTypes[pt] |= RIDER_ROOK_V;
      }
      for (Direction d : pi->hopperCapture)
      {
          if (std::find(RookDirectionsH.begin(), RookDirectionsH.end(), d) != RookDirectionsH.end())
              AttackRiderTypes[pt] |= RIDER_CANNON_H;
          if (std::find(RookDirectionsV.begin(), RookDirectionsV.end(), d) != RookDirectionsV.end())
              AttackRiderTypes[pt] |= RIDER_CANNON_V;
      }
      for (Direction d : pi->hopperQuiet)
      {
          if (std::find(RookDirectionsH.begin(), RookDirectionsH.end(), d) != RookDirectionsH.end())
              MoveRiderTypes[pt] |= RIDER_CANNON_H;
          if (std::find(RookDirectionsV.begin(), RookDirectionsV.end(), d) != RookDirectionsV.end())
              MoveRiderTypes[pt] |= RIDER_CANNON_V;
      }
  }

  for (unsigned i = 0; i < (1 << 16); ++i)
      PopCnt16[i] = uint8_t(std::bitset<16>(i).count());

  for (Square s = SQ_A1; s <= SQ_MAX; ++s)
      SquareBB[s] = make_bitboard(s);

  for (File f = FILE_A; f <= FILE_MAX; ++f)
      for (Rank r = RANK_1; r <= RANK_MAX; ++r)
          BoardSizeBB[f][r] = forward_file_bb(BLACK, make_square(f, r)) | SquareBB[make_square(f, r)] | (f > FILE_A ? BoardSizeBB[f - 1][r] : Bitboard(0));

  for (Square s1 = SQ_A1; s1 <= SQ_MAX; ++s1)
      for (Square s2 = SQ_A1; s2 <= SQ_MAX; ++s2)
              SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));

#ifdef PRECOMPUTED_MAGICS
  init_magics<RIDER>(RookTableH, RookMagicsH, RookDirectionsH, RookMagicHInit);
  init_magics<RIDER>(RookTableV, RookMagicsV, RookDirectionsV, RookMagicVInit);
  init_magics<RIDER>(BishopTable, BishopMagics, BishopDirections, BishopMagicInit);
  init_magics<HOPPER>(CannonTableH, CannonMagicsH, RookDirectionsH, CannonMagicHInit);
  init_magics<HOPPER>(CannonTableV, CannonMagicsV, RookDirectionsV, CannonMagicVInit);
  init_magics<LAME_LEAPER>(HorseTable, HorseMagics, HorseDirections, HorseMagicInit);
  init_magics<LAME_LEAPER>(ElephantTable, ElephantMagics, ElephantDirections, ElephantMagicInit);
  init_magics<LAME_LEAPER>(JanggiElephantTable, JanggiElephantMagics, JanggiElephantDirections, JanggiElephantMagicInit);
#else
  init_magics<RIDER>(RookTableH, RookMagicsH, RookDirectionsH);
  init_magics<RIDER>(RookTableV, RookMagicsV, RookDirectionsV);
  init_magics<RIDER>(BishopTable, BishopMagics, BishopDirections);
  init_magics<HOPPER>(CannonTableH, CannonMagicsH, RookDirectionsH);
  init_magics<HOPPER>(CannonTableV, CannonMagicsV, RookDirectionsV);
  init_magics<LAME_LEAPER>(HorseTable, HorseMagics, HorseDirections);
  init_magics<LAME_LEAPER>(ElephantTable, ElephantMagics, ElephantDirections);
  init_magics<LAME_LEAPER>(JanggiElephantTable, JanggiElephantMagics, JanggiElephantDirections);
#endif

  for (Color c : { WHITE, BLACK })
      for (PieceType pt = PAWN; pt <= KING; ++pt)
      {
          const PieceInfo* pi = pieceMap.find(pt)->second;

          for (Square s = SQ_A1; s <= SQ_MAX; ++s)
          {
              for (Direction d : pi->stepsCapture)
              {
                  PseudoAttacks[c][pt][s] |= safe_destination(s, c == WHITE ? d : -d);
                  if (!pi->lameLeaper)
                      LeaperAttacks[c][pt][s] |= safe_destination(s, c == WHITE ? d : -d);
              }
              for (Direction d : pi->stepsQuiet)
              {
                  PseudoMoves[c][pt][s] |= safe_destination(s, c == WHITE ? d : -d);
                  if (!pi->lameLeaper)
                      LeaperMoves[c][pt][s] |= safe_destination(s, c == WHITE ? d : -d);
              }
              PseudoAttacks[c][pt][s] |= sliding_attack<RIDER>(pi->sliderCapture, s, 0, c);
              PseudoAttacks[c][pt][s] |= sliding_attack<RIDER>(pi->hopperCapture, s, 0, c);
              PseudoMoves[c][pt][s] |= sliding_attack<RIDER>(pi->sliderQuiet, s, 0, c);
              PseudoMoves[c][pt][s] |= sliding_attack<RIDER>(pi->hopperQuiet, s, 0, c);
          }
      }

  for (Square s1 = SQ_A1; s1 <= SQ_MAX; ++s1)
  {
      for (PieceType pt : { BISHOP, ROOK })
          for (Square s2 = SQ_A1; s2 <= SQ_MAX; ++s2)
              if (PseudoAttacks[WHITE][pt][s1] & s2)
                  LineBB[s1][s2] = (attacks_bb(WHITE, pt, s1, 0) & attacks_bb(WHITE, pt, s2, 0)) | s1 | s2;
  }
}


namespace {

  // init_magics() computes all rook and bishop attacks at startup. Magic
  // bitboards are used to look up attacks of sliding pieces. As a reference see
  // www.chessprogramming.org/Magic_Bitboards. In particular, here we use the so
  // called "fancy" approach.

  template <MovementType MT>
#ifdef PRECOMPUTED_MAGICS
  void init_magics(Bitboard table[], Magic magics[], std::vector<Direction> directions, Bitboard magicsInit[]) {
#else
  void init_magics(Bitboard table[], Magic magics[], std::vector<Direction> directions) {
#endif

    // Optimal PRNG seeds to pick the correct magics in the shortest time
#ifndef PRECOMPUTED_MAGICS
#ifdef LARGEBOARDS
    int seeds[][RANK_NB] = { { 734, 10316, 55013, 32803, 12281, 15100,  16645, 255, 346, 89123 },
                             { 734, 10316, 55013, 32803, 12281, 15100,  16645, 255, 346, 89123 } };
#else
    int seeds[][RANK_NB] = { { 8977, 44560, 54343, 38998,  5731, 95205, 104912, 17020 },
                             {  728, 10316, 55013, 32803, 12281, 15100,  16645,   255 } };
#endif
#endif

    Bitboard* occupancy = new Bitboard[1 << (FILE_NB + RANK_NB - 4)];
    Bitboard* reference = new Bitboard[1 << (FILE_NB + RANK_NB - 4)];
    Bitboard edges, b;
    int* epoch = new int[1 << (FILE_NB + RANK_NB - 4)]();
    int cnt = 0, size = 0;


    for (Square s = SQ_A1; s <= SQ_MAX; ++s)
    {
        // Board edges are not considered in the relevant occupancies
        edges = ((Rank1BB | rank_bb(RANK_MAX)) & ~rank_bb(s)) | ((FileABB | file_bb(FILE_MAX)) & ~file_bb(s));

        // Given a square 's', the mask is the bitboard of sliding attacks from
        // 's' computed on an empty board. The index must be big enough to contain
        // all the attacks for each possible subset of the mask and so is 2 power
        // the number of 1s of the mask. Hence we deduce the size of the shift to
        // apply to the 64 or 32 bits word to get the index.
        Magic& m = magics[s];
        m.mask  = (MT == LAME_LEAPER ? lame_leaper_path(directions, s) : sliding_attack<MT == HOPPER ? RIDER : MT>(directions, s, 0)) & ~edges;
#ifdef LARGEBOARDS
        m.shift = 128 - popcount(m.mask);
#else
        m.shift = (Is64Bit ? 64 : 32) - popcount(m.mask);
#endif

        // Set the offset for the attacks table of the square. We have individual
        // table sizes for each square with "Fancy Magic Bitboards".
        m.attacks = s == SQ_A1 ? table : magics[s - 1].attacks + size;

        // Use Carry-Rippler trick to enumerate all subsets of masks[s] and
        // store the corresponding sliding attack bitboard in reference[].
        b = size = 0;
        do {
            occupancy[size] = b;
            reference[size] = MT == LAME_LEAPER ? lame_leaper_attack(directions, s, b) : sliding_attack<MT>(directions, s, b);

            if (HasPext)
                m.attacks[pext(b, m.mask)] = reference[size];

            size++;
            b = (b - m.mask) & m.mask;
        } while (b);

        if (HasPext)
            continue;

#ifndef PRECOMPUTED_MAGICS
        PRNG rng(seeds[Is64Bit][rank_of(s)]);
#endif

        // Find a magic for square 's' picking up an (almost) random number
        // until we find the one that passes the verification test.
        for (int i = 0; i < size; )
        {
            for (m.magic = 0; popcount((m.magic * m.mask) >> (SQUARE_NB - FILE_NB)) < FILE_NB - 2; )
            {
#ifdef LARGEBOARDS
#ifdef PRECOMPUTED_MAGICS
                m.magic = magicsInit[s];
#else
                m.magic = (rng.sparse_rand<Bitboard>() << 64) ^ rng.sparse_rand<Bitboard>();
#endif
#else
                m.magic = rng.sparse_rand<Bitboard>();
#endif
            }

            // A good magic must map every possible occupancy to an index that
            // looks up the correct sliding attack in the attacks[s] database.
            // Note that we build up the database for square 's' as a side
            // effect of verifying the magic. Keep track of the attempt count
            // and save it in epoch[], little speed-up trick to avoid resetting
            // m.attacks[] after every failed attempt.
            for (++cnt, i = 0; i < size; ++i)
            {
                unsigned idx = m.index(occupancy[i]);

                if (epoch[idx] < cnt)
                {
                    epoch[idx] = cnt;
                    m.attacks[idx] = reference[i];
                }
                else if (m.attacks[idx] != reference[i])
                    break;
            }
        }
    }

    delete[] occupancy;
    delete[] reference;
    delete[] epoch;
  }
}
