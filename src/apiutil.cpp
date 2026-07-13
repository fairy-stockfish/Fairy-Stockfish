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

#include "apiutil.h"

#include <iomanip>

namespace Stockfish {

namespace {

std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 0x20)
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec;
            else
                out << c;
        }
    }
    return out.str();
}

std::string quote(const std::string& value) { return "\"" + json_escape(value) + "\""; }
const char* boolean(bool value) { return value ? "true" : "false"; }

std::string piece_type_name(PieceType pt) {
    static const char* names[] = {
        "none", "pawn", "knight", "bishop", "rook", "queen", "fers", "alfil",
        "fersAlfil", "silver", "aiwok", "bers", "archbishop", "chancellor", "amazon",
        "knibis", "biskni", "kniroo", "rookni", "shogiPawn", "lance", "shogiKnight",
        "gold", "dragonHorse", "clobber", "breakthrough", "immobile", "cannon",
        "janggiCannon", "soldier", "horse", "elephant", "janggiElephant", "banner",
        "wazir", "commoner", "centaur"
    };
    if (pt == KING) return "king";
    if (pt >= CUSTOM_PIECES && pt <= CUSTOM_PIECES_END)
        return "custom" + std::to_string(int(pt - CUSTOM_PIECES + 1));
    if (int(pt) >= 0 && int(pt) < int(sizeof(names) / sizeof(names[0])))
        return names[int(pt)];
    if (pt == ALL_PIECES) return "all";
    return "pieceType" + std::to_string(int(pt));
}

std::string piece_type_json(PieceType pt) { return quote(piece_type_name(pt)); }

std::string piece_set_json(PieceSet pieces) {
    std::ostringstream out;
    out << '[';
    bool first = true;
    for (int i = 1; i < PIECE_TYPE_NB; ++i) {
        PieceType pt = PieceType(i);
        if (!(pieces & piece_set(pt))) continue;
        if (!first) out << ',';
        first = false;
        out << piece_type_json(pt);
    }
    out << ']';
    return out.str();
}

std::string square_name(Square s) {
    return std::string(1, char('a' + file_of(s))) + std::to_string(int(rank_of(s)) + 1);
}

std::string region_json(Bitboard region, File maxFile, Rank maxRank) {
    std::ostringstream out;
    out << '[';
    bool first = true;
    for (int r = 0; r <= int(maxRank); ++r)
        for (int f = 0; f <= int(maxFile); ++f) {
            Square sq = make_square(File(f), Rank(r));
            if (!(region & sq)) continue;
            if (!first) out << ',';
            first = false;
            out << quote(square_name(sq));
        }
    out << ']';
    return out.str();
}

std::string value_name(Value value) {
    if (value == VALUE_NONE) return "none";
    if (value == VALUE_DRAW) return "draw";
    if (value >= VALUE_MATE_IN_MAX_PLY) return "win";
    if (value <= VALUE_MATED_IN_MAX_PLY) return "loss";
    return std::to_string(int(value));
}

const char* enclosing_name(EnclosingRule v) {
    static const char* names[] = {"none", "reversi", "ataxx", "quadwrangle", "snort", "anySide", "top"};
    return names[int(v)];
}
const char* walling_name(WallingRule v) {
    static const char* names[] = {"none", "arrow", "duck", "edge", "past", "static"};
    return names[int(v)];
}
const char* chasing_name(ChasingRule v) { return v == AXF_CHASING ? "axf" : "none"; }
const char* material_counting_name(MaterialCounting v) {
    static const char* names[] = {"none", "janggi", "unweighted", "whiteDrawOdds", "blackDrawOdds"};
    return names[int(v)];
}
const char* counting_name(CountingRule v) {
    static const char* names[] = {"none", "makruk", "cambodian", "asean"};
    return names[int(v)];
}

void field(std::ostringstream& out, bool& first, const char* name, const std::string& value) {
    if (!first) out << ',';
    first = false;
    out << quote(name) << ':' << value;
}

std::string color_piece_sets(PieceSet white, PieceSet black) {
    return std::string("{\"white\":") + piece_set_json(white) + ",\"black\":" + piece_set_json(black) + '}';
}
std::string color_piece_types(PieceType white, PieceType black) {
    return std::string("{\"white\":") + piece_type_json(white) + ",\"black\":" + piece_type_json(black) + '}';
}
std::string color_regions(Bitboard white, Bitboard black, File maxFile, Rank maxRank) {
    return std::string("{\"white\":") + region_json(white, maxFile, maxRank) + ",\"black\":" + region_json(black, maxFile, maxRank) + '}';
}
std::string color_bools(bool white, bool black) {
    return std::string("{\"white\":") + boolean(white) + ",\"black\":" + boolean(black) + '}';
}
std::string castling_rights_json(CastlingRights rights) {
    return std::string("{\"white\":{\"kingSide\":")
         + boolean(bool(rights & WHITE_OO))
         + ",\"queenSide\":" + boolean(bool(rights & WHITE_OOO))
         + "},\"black\":{\"kingSide\":" + boolean(bool(rights & BLACK_OO))
         + ",\"queenSide\":" + boolean(bool(rights & BLACK_OOO)) + "}}";
}

} // namespace

std::string variant_info_json(const std::string& name) {
    auto it = variants.find(name);
    if (it == variants.end())
        return "";

    const Variant& v = *it->second;
    std::ostringstream out;
    out << '{';
    bool first = true;
    field(out, first, "schemaVersion", "1");
    field(out, first, "name", quote(name));
    field(out, first, "template", quote(v.variantTemplate));

    std::ostringstream board;
    board << '{'; bool b = true;
    field(board, b, "width", std::to_string(int(v.maxFile) + 1));
    field(board, b, "height", std::to_string(int(v.maxRank) + 1));
    field(board, b, "startFen", quote(v.startFen));
    field(board, b, "chess960", boolean(v.chess960));
    field(board, b, "twoBoards", boolean(v.twoBoards));
    field(board, b, "diagonalLines", region_json(v.diagonalLines, v.maxFile, v.maxRank));
    board << '}'; field(out, first, "board", board.str());

    std::ostringstream pieces;
    pieces << '['; bool pf = true;
    for (int i = 1; i < PIECE_TYPE_NB; ++i) {
        PieceType pt = PieceType(i);
        if (!(v.pieceTypes & piece_set(pt))) continue;
        if (!pf)
            pieces << ',';
        pf = false;
        pieces << "{\"type\":" << piece_type_json(pt)
               << ",\"fen\":{\"white\":" << quote(std::string(1, v.pieceToChar[make_piece(WHITE, pt)]))
               << ",\"black\":" << quote(std::string(1, v.pieceToChar[make_piece(BLACK, pt)])) << '}';
        char ws = v.pieceToCharSynonyms[make_piece(WHITE, pt)];
        char bs = v.pieceToCharSynonyms[make_piece(BLACK, pt)];
        pieces << ",\"synonym\":";
        if (ws == ' ' && bs == ' ') pieces << "null";
        else pieces << "{\"white\":" << quote(std::string(1, ws)) << ",\"black\":" << quote(std::string(1, bs)) << '}';
        pieces << ",\"customBetza\":";
        if (pt == KING && is_custom(v.kingType))
            pieces << quote(v.customPiece[v.kingType - CUSTOM_PIECES]);
        else if (is_custom(pt))
            pieces << quote(v.customPiece[pt - CUSTOM_PIECES]);
        else
            pieces << "null";
        pieces << ",\"value\":{\"midgame\":" << v.pieceValue[MG][pt]
               << ",\"endgame\":" << v.pieceValue[EG][pt] << "}}";
    }
    pieces << ']'; field(out, first, "pieces", pieces.str());
    field(out, first, "pieceTypes", piece_set_json(v.pieceTypes));
    PieceSet royalPieceTypes = (v.pieceTypes & piece_set(KING))
                                | (v.extinctionPseudoRoyal ? v.extinctionPieceTypes : NO_PIECE_SET);
    field(out, first, "royalPieceTypes", piece_set_json(royalPieceTypes));

    std::ostringstream movement; movement << '{'; b = true;
    std::ostringstream mobility; mobility << '{'; bool mf = true;
    for (int i = 1; i < PIECE_TYPE_NB; ++i) {
        PieceType pt = PieceType(i);
        if (!(v.pieceTypes & piece_set(pt))) continue;
        std::string entry = std::string("{\"white\":") + region_json(v.mobilityRegion[WHITE][pt], v.maxFile, v.maxRank)
                          + ",\"black\":" + region_json(v.mobilityRegion[BLACK][pt], v.maxFile, v.maxRank) + '}';
        field(mobility, mf, piece_type_name(pt).c_str(), entry);
    }
    mobility << '}'; field(movement, b, "mobilityRegions", mobility.str());
    field(movement, b, "doubleStep", boolean(v.doubleStep));
    field(movement, b, "doubleStepRegions", color_regions(v.doubleStepRegion[WHITE], v.doubleStepRegion[BLACK], v.maxFile, v.maxRank));
    field(movement, b, "tripleStepRegions", color_regions(v.tripleStepRegion[WHITE], v.tripleStepRegion[BLACK], v.maxFile, v.maxRank));
    field(movement, b, "enPassantRegions", color_regions(v.enPassantRegion[WHITE], v.enPassantRegion[BLACK], v.maxFile, v.maxRank));
    field(movement, b, "enPassantTypes", color_piece_sets(v.enPassantTypes[WHITE], v.enPassantTypes[BLACK]));
    field(movement, b, "pass", color_bools(v.pass[WHITE], v.pass[BLACK]));
    field(movement, b, "passOnStalemate", color_bools(v.passOnStalemate[WHITE], v.passOnStalemate[BLACK]));
    field(movement, b, "mustCapture", boolean(v.mustCapture));
    field(movement, b, "immobilityIllegal", boolean(v.immobilityIllegal));
    field(movement, b, "cambodianMoves", boolean(v.cambodianMoves));
    field(movement, b, "makpongRule", boolean(v.makpongRule));
    field(movement, b, "flyingGeneral", boolean(v.flyingGeneral));
    field(movement, b, "soldierPromotionRank", std::to_string(int(v.soldierPromotionRank) + 1));
    movement << '}'; field(out, first, "movement", movement.str());

    std::ostringstream promotion; promotion << '{'; b = true;
    field(promotion, b, "regions", color_regions(v.promotionRegion[WHITE], v.promotionRegion[BLACK], v.maxFile, v.maxRank));
    field(promotion, b, "mainPawnTypes", color_piece_types(v.mainPromotionPawnType[WHITE], v.mainPromotionPawnType[BLACK]));
    field(promotion, b, "pawnTypes", color_piece_sets(v.promotionPawnTypes[WHITE], v.promotionPawnTypes[BLACK]));
    field(promotion, b, "pieceTypes", color_piece_sets(v.promotionPieceTypes[WHITE], v.promotionPieceTypes[BLACK]));
    std::ostringstream promoted; promoted << '{'; bool prf = true;
    for (int i = 1; i < PIECE_TYPE_NB; ++i) if (v.promotedPieceType[i] != NO_PIECE_TYPE)
        field(promoted, prf, piece_type_name(PieceType(i)).c_str(), piece_type_json(v.promotedPieceType[i]));
    promoted << '}'; field(promotion, b, "promotedPieceTypes", promoted.str());
    std::ostringstream limits; limits << '{'; bool lf = true;
    for (int i = 1; i < PIECE_TYPE_NB; ++i) if (v.promotionLimit[i])
        field(limits, lf, piece_type_name(PieceType(i)).c_str(), std::to_string(v.promotionLimit[i]));
    limits << '}'; field(promotion, b, "limits", limits.str());
    field(promotion, b, "sittuyin", boolean(v.sittuyinPromotion));
    field(promotion, b, "onCapture", boolean(v.piecePromotionOnCapture));
    field(promotion, b, "mandatoryPawn", boolean(v.mandatoryPawnPromotion));
    field(promotion, b, "mandatoryPiece", boolean(v.mandatoryPiecePromotion));
    field(promotion, b, "demotion", boolean(v.pieceDemotion));
    field(promotion, b, "shogiStyle", boolean(v.shogiStylePromotions));
    promotion << '}'; field(out, first, "promotion", promotion.str());

    std::ostringstream capture; capture << '{'; b = true;
    field(capture, b, "blast", boolean(v.blastOnCapture));
    field(capture, b, "blastImmuneTypes", piece_set_json(v.blastImmuneTypes));
    field(capture, b, "mutuallyImmuneTypes", piece_set_json(v.mutuallyImmuneTypes));
    field(capture, b, "petrifyTypes", piece_set_json(v.petrifyOnCaptureTypes));
    field(capture, b, "petrifyBlastPieces", boolean(v.petrifyBlastPieces));
    capture << '}'; field(out, first, "capture", capture.str());

    std::ostringstream castling; castling << '{'; b = true;
    field(castling, b, "enabled", boolean(v.castling));
    field(castling, b, "droppedPiece", boolean(v.castlingDroppedPiece));
    field(castling, b, "kingSideFile", std::to_string(int(v.castlingKingsideFile) + 1));
    field(castling, b, "queenSideFile", std::to_string(int(v.castlingQueensideFile) + 1));
    field(castling, b, "rank", std::to_string(int(v.castlingRank) + 1));
    field(castling, b, "kingFile", std::to_string(int(v.castlingKingFile) + 1));
    field(castling, b, "rookKingSideFile", std::to_string(int(v.castlingRookKingsideFile) + 1));
    field(castling, b, "rookQueenSideFile", std::to_string(int(v.castlingRookQueensideFile) + 1));
    field(castling, b, "kingPieces", color_piece_types(v.castlingKingPiece[WHITE], v.castlingKingPiece[BLACK]));
    field(castling, b, "rookPieces", color_piece_sets(v.castlingRookPieces[WHITE], v.castlingRookPieces[BLACK]));
    field(castling, b, "opposite", boolean(v.oppositeCastling));
    field(castling, b, "wins", castling_rights_json(v.castlingWins));
    castling << '}'; field(out, first, "castling", castling.str());

    std::ostringstream drops; drops << '{'; b = true;
    field(drops, b, "enabled", boolean(v.pieceDrops));
    field(drops, b, "capturesToHand", boolean(v.capturesToHand));
    field(drops, b, "mustDrop", boolean(v.mustDrop));
    field(drops, b, "mustDropType", piece_type_json(v.mustDropType));
    field(drops, b, "dropLoop", boolean(v.dropLoop));
    field(drops, b, "firstRankPawnDrops", boolean(v.firstRankPawnDrops));
    field(drops, b, "promotionZonePawnDrops", boolean(v.promotionZonePawnDrops));
    field(drops, b, "regions", color_regions(v.dropRegion[WHITE], v.dropRegion[BLACK], v.maxFile, v.maxRank));
    field(drops, b, "enclosingRule", quote(enclosing_name(v.enclosingDrop)));
    field(drops, b, "enclosingStart", region_json(v.enclosingDropStart, v.maxFile, v.maxRank));
    field(drops, b, "sittuyinRook", boolean(v.sittuyinRookDrop));
    field(drops, b, "oppositeColoredBishop", boolean(v.dropOppositeColoredBishop));
    field(drops, b, "promoted", boolean(v.dropPromoted));
    field(drops, b, "noDoubledType", piece_type_json(v.dropNoDoubled));
    field(drops, b, "noDoubledCount", std::to_string(v.dropNoDoubledCount));
    field(drops, b, "free", boolean(v.freeDrops));
    drops << '}'; field(out, first, "drops", drops.str());

    std::ostringstream gating; gating << '{'; b = true;
    field(gating, b, "enabled", boolean(v.gating));
    field(gating, b, "seirawan", boolean(v.seirawanGating));
    field(gating, b, "wallingRule", quote(walling_name(v.wallingRule)));
    field(gating, b, "wallingRegions", color_regions(v.wallingRegion[WHITE], v.wallingRegion[BLACK], v.maxFile, v.maxRank));
    field(gating, b, "wallOrMove", boolean(v.wallOrMove));
    gating << '}'; field(out, first, "gating", gating.str());

    std::ostringstream end; end << '{'; b = true;
    field(end, b, "checking", boolean(v.checking));
    field(end, b, "dropChecks", boolean(v.dropChecks));
    field(end, b, "kingType", piece_type_json(v.kingType));
    field(end, b, "nMoveRule", std::to_string(v.nMoveRule));
    field(end, b, "nMoveRuleTypes", color_piece_sets(v.nMoveRuleTypes[WHITE], v.nMoveRuleTypes[BLACK]));
    field(end, b, "nFoldRule", std::to_string(v.nFoldRule));
    field(end, b, "nFoldValue", quote(value_name(v.nFoldValue)));
    field(end, b, "nFoldValueAbsolute", boolean(v.nFoldValueAbsolute));
    field(end, b, "perpetualCheckIllegal", boolean(v.perpetualCheckIllegal));
    field(end, b, "moveRepetitionIllegal", boolean(v.moveRepetitionIllegal));
    field(end, b, "chasingRule", quote(chasing_name(v.chasingRule)));
    field(end, b, "stalemateValue", quote(value_name(v.stalemateValue)));
    field(end, b, "stalematePieceCount", boolean(v.stalematePieceCount));
    field(end, b, "checkmateValue", quote(value_name(v.checkmateValue)));
    field(end, b, "shogiPawnDropMateIllegal", boolean(v.shogiPawnDropMateIllegal));
    field(end, b, "shatarMateRule", boolean(v.shatarMateRule));
    field(end, b, "bikjangRule", boolean(v.bikjangRule));
    field(end, b, "dupleCheck", boolean(v.dupleCheck));
    field(end, b, "checkCounting", boolean(v.checkCounting));
    field(end, b, "materialCounting", quote(material_counting_name(v.materialCounting)));
    field(end, b, "adjudicateFullBoard", boolean(v.adjudicateFullBoard));
    field(end, b, "countingRule", quote(counting_name(v.countingRule)));
    end << '}'; field(out, first, "gameEnd", end.str());

    std::ostringstream extinction; extinction << '{'; b = true;
    field(extinction, b, "value", quote(value_name(v.extinctionValue)));
    field(extinction, b, "claim", boolean(v.extinctionClaim));
    field(extinction, b, "pseudoRoyal", boolean(v.extinctionPseudoRoyal));
    field(extinction, b, "pieceTypes", piece_set_json(v.extinctionPieceTypes));
    field(extinction, b, "pieceCount", std::to_string(v.extinctionPieceCount));
    field(extinction, b, "opponentPieceCount", std::to_string(v.extinctionOpponentPieceCount));
    extinction << '}'; field(out, first, "extinction", extinction.str());

    std::ostringstream flag; flag << '{'; b = true;
    field(flag, b, "pieces", color_piece_types(v.flagPiece[WHITE], v.flagPiece[BLACK]));
    field(flag, b, "regions", color_regions(v.flagRegion[WHITE], v.flagRegion[BLACK], v.maxFile, v.maxRank));
    field(flag, b, "pieceCount", std::to_string(v.flagPieceCount));
    field(flag, b, "blockedWin", boolean(v.flagPieceBlockedWin));
    field(flag, b, "move", boolean(v.flagMove));
    field(flag, b, "safe", boolean(v.flagPieceSafe));
    flag << '}'; field(out, first, "flag", flag.str());

    std::ostringstream connect; connect << '{'; b = true;
    field(connect, b, "n", std::to_string(v.connectN));
    field(connect, b, "pieceTypes", piece_set_json(v.connectPieceTypes));
    field(connect, b, "horizontal", boolean(v.connectHorizontal));
    field(connect, b, "vertical", boolean(v.connectVertical));
    field(connect, b, "diagonal", boolean(v.connectDiagonal));
    field(connect, b, "region1", color_regions(v.connectRegion1[WHITE], v.connectRegion1[BLACK], v.maxFile, v.maxRank));
    field(connect, b, "region2", color_regions(v.connectRegion2[WHITE], v.connectRegion2[BLACK], v.maxFile, v.maxRank));
    field(connect, b, "nxn", std::to_string(v.connectNxN));
    field(connect, b, "collinearN", std::to_string(v.collinearN));
    field(connect, b, "value", quote(value_name(v.connectValue)));
    connect << '}'; field(out, first, "connect", connect.str());

    std::ostringstream enclosing; enclosing << '{'; b = true;
    field(enclosing, b, "flipRule", quote(enclosing_name(v.flipEnclosedPieces)));
    enclosing << '}'; field(out, first, "enclosing", enclosing.str());

    std::ostringstream protocol; protocol << '{'; b = true;
    field(protocol, b, "pieceToCharTable", quote(v.pieceToCharTable));
    field(protocol, b, "pocketSize", std::to_string(v.pocketSize));
    protocol << '}'; field(out, first, "protocol", protocol.str());

    out << '}';
    return out.str();
}

} // namespace Stockfish
