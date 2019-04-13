/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2019 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

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

Magic RookMagics[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];

namespace {

#ifdef LARGEBOARDS
  Bitboard RookTable[0xA80000];  // To store rook attacks
  Bitboard BishopTable[0x33C00]; // To store bishop attacks
#else
  Bitboard RookTable[0x19000];  // To store rook attacks
  Bitboard BishopTable[0x1480]; // To store bishop attacks
#endif

#ifdef PRECOMPUTED_MAGICS
#define B(a, b) (Bitboard(a) << 64) ^ Bitboard(b)
  // Use precomputed magics if pext is not avaible,
  // since the magics generation is very slow.
  Bitboard RookMagicInit[SQUARE_NB] = {
      B(0x4068000028000894, 0x20006051028),
      B(0x2004000042000004, 0x2810000008004000),
      B(0x1020000220004000, 0x8004290180020200),
      B(0x4310000101000020, 0x8020100004002000),
      B(0x38000080211108, 0x118000104000000),
      B(0x4050000080400004, 0x2011040500118010),
      B(0x4010000048100200, 0x100000840204000),
      B(0x4020000820000100, 0x410106024000804),
      B(0x910000008D00021, 0x800004400100010),
      B(0x6830000821080004, 0x80C00010100000),
      B(0x8040000040500008, 0x200031840804901),
      B(0x4018000042201000, 0x8300002D80000841),
      B(0x100008000044000, 0x1980020094A89400),
      B(0x1000004000082000, 0x4408540710000181),
      B(0x1206010001100802, 0x40004100048),
      B(0x984010000410001, 0x208080028AE),
      B(0x411210000261014, 0x8000414100011208),
      B(0x8200100000B0002, 0x1082008404000008),
      B(0x5004090000400096, 0x10004020001000E2),
      B(0x8899220000020001, 0x8401004904082000),
      B(0x801720000080040, 0x101000200082020),
      B(0x800060000102411, 0xA8200008808400),
      B(0x30C0000081004, 0x1004200082100418),
      B(0x1018010000020102, 0x4100000040014082),
      B(0x24000008000001, 0x4080041809020000),
      B(0x3011200814000104, 0x2110004020000000),
      B(0x1814410050000800, 0x4041010220000400),
      B(0x20B002010000211, 0x100104188808),
      B(0x821020000114, 0x22080A0200304020),
      B(0xC0E0020000082, 0x1004108210042000),
      B(0x9010010000084, 0x2000011014020406),
      B(0x400208018000040, 0x20D000052800018),
      B(0x108008000080, 0x240400000104),
      B(0x8012001080004400, 0x102010420201000),
      B(0x1000000C0001210, 0x821000080441200),
      B(0x1C500000200004D0, 0x20042084400410),
      B(0x4000284000108000, 0x1809420810080140),
      B(0x40804004000, 0x2220005410000000),
      B(0x4007080A1020000, 0x8004020D41200020),
      B(0x1080000800008000, 0x4080210000002002),
      B(0x400000400250000, 0x8000001100410000),
      B(0xC8010090010001, 0x2004000068009022),
      B(0x8300200008210000, 0x410804100111004),
      B(0x42150008210000, 0x81100004A400200),
      B(0x2C1410001810001, 0x8200800000411020),
      B(0x28420004020000, 0x409002880830900),
      B(0x200028000008000, 0x8100000040200004),
      B(0x4001030120000, 0x41008A020140404),
      B(0x288000004240804, 0x2410020000090),
      B(0x104202000004004, 0x2008010000040),
      B(0x800A020008022020, 0x4112108040400),
      B(0x90044001082420, 0x239806020040A),
      B(0x4018000414800030, 0x2001100010000),
      B(0x102820010088120, 0x8002004090008),
      B(0x280100200000410, 0x840441100000),
      B(0x4A020000040020, 0x10820040C0881),
      B(0x20000010020, 0x21000800402010),
      B(0x161A0400212C80, 0x42000400203002),
      B(0xAB00800181208440, 0x3120A01004040),
      B(0x261100084025060, 0x81800A2100400),
      B(0x1008000000A02000, 0x940000010040910),
      B(0x1406400004A010, 0x100011080188C05),
      B(0x400002000011000, 0x2080005008080002),
      B(0x402C1004012, 0x600004200488003),
      B(0x1800080802000409, 0x200012061200104),
      B(0x843020018100800, 0xE2000040100E0A04),
      B(0xA0010200005048, 0x80000851000012),
      B(0x20000900041, 0x200006904002024),
      B(0x20010000080002, 0x100000490211000),
      B(0x1000018070C, 0x4100008200400010),
      B(0x40000004101060, 0x40000109A800112),
      B(0x44000908010404, 0x2200000420010409),
      B(0x169D042288081204, 0x18000040A000),
      B(0x1000041002000, 0x6040040001208000),
      B(0x130040088200DA, 0x81600004010300),
      B(0x3000062004000220, 0x8404200010200020),
      B(0xC000102000020A, 0x108200004001000),
      B(0x2000008606120000, 0xC010200008010821),
      B(0x80040200000100, 0x80080000800440),
      B(0x508028000040100, 0x4280000800124),
      B(0x5AA422000B810100, 0xA802200002004404),
      B(0x10000A0000012100, 0x8080200000400802),
      B(0x240048004008102, 0x200400010800100),
      B(0x4140030108004, 0x8C012000024909A0),
      B(0x1400018A0A8000, 0x90841080008200),
      B(0xA500120088001010, 0x4040020200008042),
      B(0x5008024008888001, 0x1A0200A00005000),
      B(0x4004D13000026, 0x800008008000A012),
      B(0x201501022901000E, 0x8000024100001102),
      B(0xE80010140000800C, 0x20004B1080000820),
      B(0x8011010100046A00, 0x90010000C021),
      B(0x1008020008401440, 0x80900020000E088),
      B(0x100088000040000, 0x8124800080002200),
      B(0x12C04120401200, 0x410500E80004040B),
      B(0x9904A0100020008, 0x800108400020040),
      B(0x4000842405D0004, 0x10220200000080),
      B(0x4000001080802010, 0x4040880A00200001),
      B(0x4000001080802010, 0x4040880A00200001),
      B(0x800002021008000, 0xA04000420020000C),
      B(0x12A6020000304, 0x8290144200000),
      B(0x102803020008, 0x40824014A0200000),
      B(0x1400020080210008, 0x8001004000500000),
      B(0x120000148212, 0x801C001003A00000),
      B(0x2020002908040, 0x2000804140200000),
      B(0x820000010080, 0x10A2020420200000),
      B(0x508200022440, 0xB004001108800040),
      B(0x8202004084010C81, 0xC081A03000400008),
      B(0xB04080040204050, 0x1810028080200000),
      B(0xAA20014010120001, 0x80308004022200),
      B(0xAA20014010120001, 0x80308004022200),
      B(0x208018020814020, 0x2004020200410A00),
      B(0x6000801000002800, 0xC80121082004080),
      B(0x84002080140202, 0x8000004100090100),
      B(0x3100114050000, 0x2000818014000100),
      B(0x280805000008A400, 0x401042000000300),
      B(0x245A20000040401, 0x1000850102080200),
      B(0x4010430000D00240, 0xD0800001201100),
      B(0x20000080040, 0x9053000880500600),
      B(0x1840000610021088, 0x440801002428400),
      B(0x448814040010, 0x8410085020500600)
  };
  Bitboard BishopMagicInit[SQUARE_NB] = {
      B(0x2001040305000010, 0x830200040400082),
      B(0x1042400080E01200, 0x2004904010811400),
      B(0x400010120200, 0x880080D080018000),
      B(0x240190C00100040, 0x100A020140044404),
      B(0x1018010404010004, 0x1001010018081E0),
      B(0x41200A804C0904, 0x40000322000008),
      B(0x4001180A004, 0x8000001106000000),
      B(0x6006020020030600, 0x1840002100004841),
      B(0x4200200100, 0x4001041808002000),
      B(0x4100020050124600, 0x1001802902400CA0),
      B(0x448C0081440161, 0x200206010008000),
      B(0x400008008008408, 0x1000080210100080),
      B(0x200280C01008200, 0x210200813000080),
      B(0x1A000204400, 0x222200401023000),
      B(0x10081040640A00, 0x8410021881400000),
      B(0x1840400318080008, 0x800800840080000),
      B(0x4204050C040, 0x6500600200140000),
      B(0x1012100040204, 0x402404444400000),
      B(0x6000012680008240, 0x410140000004220),
      B(0x1000020810040008, 0x2D0011000060000),
      B(0x1020020400, 0x400108059001001),
      B(0x400020001100808, 0x480204800200000B),
      B(0x10000010030084, 0x2042000848900022),
      B(0x10000010030084, 0x2042000848900022),
      B(0x100D801402400, 0x1512404009000400),
      B(0x8000208005112400, 0xA02040401000000),
      B(0x1000420002800200, 0x4CA000183020000),
      B(0x800811480020, 0x408801010224001),
      B(0xC805200810900100, 0x9000084204004020),
      B(0x8200160204100004, 0x8040004004002022),
      B(0x104514013080080, 0x146410040001000),
      B(0x140844000080002, 0x1008102020040001),
      B(0x4040400041A2002, 0x8040000A8802510),
      B(0x801014041008002, 0x80068008025200),
      B(0xA00540A414040, 0x4101040010A0000),
      B(0x6484008010810002, 0x1100506884024000),
      B(0x2800401008006000, 0x1005420884029020),
      B(0x6822091010004421, 0x2000458080480),
      B(0x40101000200101, 0x10020100001C4E0),
      B(0x100400008C42, 0x4000100009008000),
      B(0x851220018800400, 0x1681800040080080),
      B(0x64200002010, 0x900020200040002),
      B(0x20800080000022, 0x80040810002010),
      B(0xA88408000802080, 0x20808001000000),
      B(0x200000400C005040, 0x100140020290108),
      B(0x224100000800408, 0x4204802004400020),
      B(0x80080620010210, 0x91080088804040),
      B(0x4008002100010, 0x80AC201001000001),
      B(0x10008200902C046, 0x8080D03004000010),
      B(0x3002100081000180, 0x2210002121528408),
      B(0x8C101800804420, 0x1019880200043008),
      B(0x200022000920D0, 0x8000800081300020),
      B(0x1D40800880000, 0x400040001400050),
      B(0x2020004100040, 0x200008040008008),
      B(0x4840800040100001, 0x100100040203040),
      B(0x40084001105, 0x8800080088000089),
      B(0x4000128008020008, 0x4004200200440020),
      B(0x210040008520000, 0x820219001080022),
      B(0x1494040018002116, 0x400101047020008),
      B(0x510008001910C224, 0x80200148118000),
      B(0xC0301002301000, 0x4211A08004801),
      B(0x50008E0C01001080, 0x100C004102845100),
      B(0x400600020060400, 0x88024100250050),
      B(0x8202920002002040, 0x810012000003),
      B(0x800004208800200, 0x18AA00201000048),
      B(0x402100800100002, 0x411000081000400),
      B(0x101000022004044, 0x9000100040000),
      B(0x41068001001, 0xC00400010001),
      B(0x310210001040, 0x1A1200020010000),
      B(0xA082409200004048, 0x490040800124101),
      B(0x18844820E0040212, 0x1000404420D10000),
      B(0x802908A40003348, 0x20200040104140),
      B(0x1800404028205003, 0xC020010401089020),
      B(0x802100044D01000, 0x8C41888000800040),
      B(0x1D0161011410081, 0x10008000100200),
      B(0x401000480040100, 0x286800404002212),
      B(0x821030000100009, 0x2000090200A00000),
      B(0x200020800200800, 0x2000480900841012),
      B(0x80A000048030080, 0x200000120200008),
      B(0x40B1400008020020, 0x148000200008004),
      B(0xA021700002002010, 0x3040E400040100),
      B(0x400242C200200640, 0x20440210200281),
      B(0x80AC140040206240, 0x120000102801401),
      B(0x2020340040832040, 0x10402100A44000),
      B(0x420100400040220, 0x80014C8004000106),
      B(0x504300822421120, 0x8004004008400100),
      B(0x2001100008040, 0x2020104302000000),
      B(0xA500802000A, 0x2008008000114100),
      B(0x8A0020000200, 0x9C00101001002408),
      B(0x104000001001008, 0x9001000204040060),
      B(0x1000820080108200, 0xA401000008100001),
      B(0x2008600009000480, 0x9008020001400000),
      B(0x4000800200040200, 0xA00030400308082),
      B(0x4004300202004709, 0x1000100180010020),
      B(0xC014800100440010, 0x402020280002C010),
      B(0x220208010884680, 0x1040280000042110),
      B(0x40B0018019202801, 0x1008408000100040),
      B(0x8269010206080044, 0x8001810000000040),
      B(0x4000020880081040, 0x208A44000028000),
      B(0x4004004E9004220A, 0x2104004001400024),
      B(0x8035006008C0904, 0x402002001080120),
      B(0x1800884002, 0x404400820000000),
      B(0x8088000004008910, 0x8024100401000000),
      B(0x142200086000100, 0x28021040020002E),
      B(0x1000409141004018, 0x100410820080040A),
      B(0x1800801800140, 0x810801060C0801),
      B(0x1000C00100402220, 0x808023420000000),
      B(0x8A0A202414305008, 0x100040200000021),
      B(0xC0208024050, 0x8003088008020401),
      B(0x8044004201440101, 0x400820080C024022),
      B(0x406018884120099, 0xB00088018002000),
      B(0x2000800010403010, 0xC5A002002010010),
      B(0x800020040840, 0x201800202800200),
      B(0x201280120020008D, 0x258809001000040),
      B(0x9100002020181, 0x80400082204000),
      B(0x104010080201001, 0x40080080181080),
      B(0x8440248092000430, 0xA200804900100000),
      B(0x2031010C01000C20, 0x200310A560082008),
      B(0x400202081811400, 0x40081802050000C),
      B(0x1011002100821300, 0x2400825040804100)
  };
#undef B
#endif

#ifdef PRECOMPUTED_MAGICS
  void init_magics(Bitboard table[], Magic magics[], std::vector<Direction> directions, Bitboard magicsInit[]);
#else
  void init_magics(Bitboard table[], Magic magics[], std::vector<Direction> directions);
#endif

  Bitboard sliding_attack(std::vector<Direction> directions, Square sq, Bitboard occupied, Color c = WHITE) {

    Bitboard attack = 0;

    for (Direction d : directions)
        for (Square s = sq + (c == WHITE ? d : -d);
             is_ok(s) && distance(s, s - (c == WHITE ? d : -d)) == 1;
             s += (c == WHITE ? d : -d))
        {
            attack |= s;

            if (occupied & s)
                break;
        }

    return attack;
  }
}


/// Bitboards::pretty() returns an ASCII representation of a bitboard suitable
/// to be printed to standard output. Useful for debugging.

const std::string Bitboards::pretty(Bitboard b) {

  std::string s = "+---+---+---+---+---+---+---+---+---+---+---+---+\n";

  for (Rank r = RANK_MAX; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_MAX; ++f)
          s += b & make_square(f, r) ? "| X " : "|   ";

      s += "|\n+---+---+---+---+---+---+---+---+---+---+---+---+\n";
  }

  return s;
}


/// Bitboards::init() initializes various bitboard tables. It is called at
/// startup and relies on global objects to be already zero-initialized.

void Bitboards::init() {

  // Initialize rider types
  for (PieceType pt = PAWN; pt <= KING; ++pt)
  {
      const PieceInfo* pi = pieceMap.find(pt)->second;

      for (Direction d : pi->sliderCapture)
      {
          if (d == NORTH_EAST || d == SOUTH_WEST || d == NORTH_WEST || d == SOUTH_EAST)
              AttackRiderTypes[pt] |= RIDER_BISHOP;
          if (d == NORTH || d == SOUTH || d == EAST || d == WEST)
              AttackRiderTypes[pt] |= RIDER_ROOK;
      }
      for (Direction d : pi->sliderQuiet)
      {
          if (d == NORTH_EAST || d == SOUTH_WEST || d == NORTH_WEST || d == SOUTH_EAST)
              MoveRiderTypes[pt] |= RIDER_BISHOP;
          if (d == NORTH || d == SOUTH || d == EAST || d == WEST)
              MoveRiderTypes[pt] |= RIDER_ROOK;
      }
  }

  for (unsigned i = 0; i < (1 << 16); ++i)
      PopCnt16[i] = std::bitset<16>(i).count();

  for (Square s = SQ_A1; s <= SQ_MAX; ++s)
      SquareBB[s] = make_bitboard(s);

  for (File f = FILE_A; f <= FILE_MAX; ++f)
      for (Rank r = RANK_1; r <= RANK_MAX; ++r)
          BoardSizeBB[f][r] = forward_file_bb(BLACK, make_square(f, r)) | SquareBB[make_square(f, r)] | (f > FILE_A ? BoardSizeBB[f - 1][r] : Bitboard(0));

  for (Square s1 = SQ_A1; s1 <= SQ_MAX; ++s1)
      for (Square s2 = SQ_A1; s2 <= SQ_MAX; ++s2)
              SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));

  // Piece moves
  std::vector<Direction> RookDirections = { NORTH,  EAST,  SOUTH,  WEST };
  std::vector<Direction> BishopDirections = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

#ifdef PRECOMPUTED_MAGICS
  init_magics(RookTable, RookMagics, RookDirections, RookMagicInit);
  init_magics(BishopTable, BishopMagics, BishopDirections, BishopMagicInit);
#else
  init_magics(RookTable, RookMagics, RookDirections);
  init_magics(BishopTable, BishopMagics, BishopDirections);
#endif

  for (Color c : { WHITE, BLACK })
      for (PieceType pt = PAWN; pt <= KING; ++pt)
      {
          const PieceInfo* pi = pieceMap.find(pt)->second;

          for (Square s = SQ_A1; s <= SQ_MAX; ++s)
          {
              for (Direction d : pi->stepsCapture)
              {
                  Square to = s + Direction(c == WHITE ? d : -d);

                  if (is_ok(to) && distance(s, to) < 4)
                  {
                      PseudoAttacks[c][pt][s] |= to;
                      LeaperAttacks[c][pt][s] |= to;
                  }
              }
              for (Direction d : pi->stepsQuiet)
              {
                  Square to = s + Direction(c == WHITE ? d : -d);

                  if (is_ok(to) && distance(s, to) < 4)
                  {
                      PseudoMoves[c][pt][s] |= to;
                      LeaperMoves[c][pt][s] |= to;
                  }
              }
              PseudoAttacks[c][pt][s] |= sliding_attack(pi->sliderCapture, s, 0, c);
              PseudoMoves[c][pt][s] |= sliding_attack(pi->sliderQuiet, s, 0, c);
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
        m.mask  = sliding_attack(directions, s, 0) & ~edges;
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
            reference[size] = sliding_attack(directions, s, b);

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
            for (m.magic = 0; popcount(((m.magic * m.mask) & AllSquares) >> (SQUARE_NB - FILE_NB)) < FILE_NB - 2; )
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
