/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2019 Fabian Fichter

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

#include <string>
#include <sstream>

#include "parser.h"
#include "piece.h"
#include "types.h"

namespace {

    template <typename T> void set(const std::string& value, T& target)
    {
        std::stringstream ss(value);
        ss >> target;
    }

    template <> void set(const std::string& value, Rank& target) {
        std::stringstream ss(value);
        int i;
        ss >> i;
        target = Rank(i - 1);
    }

    template <> void set(const std::string& value, File& target) {
        std::stringstream ss(value);
        if (isdigit(ss.peek()))
        {
            int i;
            ss >> i;
            target = File(i - 1);
        }
        else
        {
            char c;
            ss >> c;
            target = File(c - 'a');
        }
    }

    template <> void set(const std::string& value, std::string& target) {
        target = value;
    }

    template <> void set(const std::string& value, bool& target) {
        target = value == "true";
    }

    template <> void set(const std::string& value, Value& target) {
        target =  value == "win"  ? VALUE_MATE
                : value == "loss" ? -VALUE_MATE
                : VALUE_DRAW;
    }

    template <> void set(const std::string& value, CountingRule& target) {
        target =  value == "makruk"  ? MAKRUK_COUNTING
                : value == "asean" ? ASEAN_COUNTING
                : NO_COUNTING;
    }

    template <> void set(const std::string& value, Bitboard& target) {
        char token, token2;
        std::stringstream ss(value);
        target = 0;
        while (ss >> token && ss >> token2)
            target |= make_square(File(tolower(token) - 'a'), Rank(token2 - '1'));
    }

} // namespace

template <class T> void VariantParser::parse_attribute(const std::string& key, T& target) {
    const auto& it = config.find(key);
    if (it != config.end())
        set(it->second, target);
}

Variant* VariantParser::parse() {
    Variant* v = new Variant();
    v->reset_pieces();
    v->promotionPieceTypes = {};
    return parse(v);
}

Variant* VariantParser::parse(Variant* v) {
    // piece types
    for (const auto& pieceInfo : pieceMap)
    {
        const auto& keyValue = config.find(pieceInfo.second->name);
        if (keyValue != config.end() && !keyValue->second.empty())
            v->add_piece(pieceInfo.first, keyValue->second.at(0));
    }
    parse_attribute("variantTemplate", v->variantTemplate);
    parse_attribute("pocketSize", v->pocketSize);
    parse_attribute("maxRank", v->maxRank);
    parse_attribute("maxFile", v->maxFile);
    parse_attribute("chess960", v->chess960);
    parse_attribute("startFen", v->startFen);
    parse_attribute("promotionRank", v->promotionRank);
    // promotion piece types
    const auto& it_prom = config.find("promotionPieceTypes");
    if (it_prom != config.end())
    {
        char token;
        size_t idx;
        std::stringstream ss(it_prom->second);
        while (ss >> token && ((idx = v->pieceToChar.find(token)) != std::string::npos))
            v->promotionPieceTypes.insert(PieceType(idx));
    }
    parse_attribute("sittuyinPromotion", v->sittuyinPromotion);
    // promotion limit
    const auto& it_prom_limit = config.find("promotionLimit");
    if (it_prom_limit != config.end())
    {
        char token;
        size_t idx;
        std::stringstream ss(it_prom_limit->second);
        while (ss >> token && (idx = v->pieceToChar.find(toupper(token))) != std::string::npos && ss >> token && ss >> v->promotionLimit[idx]) {}
    }
    // promoted piece types
    const auto& it_prom_pt = config.find("promotedPieceType");
    if (it_prom_pt != config.end())
    {
        char token;
        size_t idx, idx2;
        std::stringstream ss(it_prom_pt->second);
        while (   ss >> token && (idx = v->pieceToChar.find(toupper(token))) != std::string::npos && ss >> token
               && ss >> token && (idx2 = v->pieceToChar.find(toupper(token))) != std::string::npos)
            v->promotedPieceType[idx] = PieceType(idx2);
    }
    parse_attribute("piecePromotionOnCapture", v->piecePromotionOnCapture);
    parse_attribute("mandatoryPawnPromotion", v->mandatoryPawnPromotion);
    parse_attribute("mandatoryPiecePromotion", v->mandatoryPiecePromotion);
    parse_attribute("pieceDemotion", v->pieceDemotion);
    parse_attribute("endgameEval", v->endgameEval);
    parse_attribute("doubleStep", v->doubleStep);
    parse_attribute("doubleStepRank", v->doubleStepRank);
    parse_attribute("firstRankDoubleSteps", v->firstRankDoubleSteps);
    parse_attribute("castling", v->castling);
    parse_attribute("castlingDroppedPiece", v->castlingDroppedPiece);
    parse_attribute("castlingKingsideFile", v->castlingKingsideFile);
    parse_attribute("castlingQueensideFile", v->castlingQueensideFile);
    parse_attribute("castlingRank", v->castlingRank);
    parse_attribute("checking", v->checking);
    parse_attribute("mustCapture", v->mustCapture);
    parse_attribute("mustDrop", v->mustDrop);
    parse_attribute("pieceDrops", v->pieceDrops);
    parse_attribute("dropLoop", v->dropLoop);
    parse_attribute("capturesToHand", v->capturesToHand);
    parse_attribute("firstRankDrops", v->firstRankDrops);
    parse_attribute("dropOnTop", v->dropOnTop);
    parse_attribute("whiteDropRegion", v->whiteDropRegion);
    parse_attribute("blackDropRegion", v->blackDropRegion);
    parse_attribute("sittuyinRookDrop", v->sittuyinRookDrop);
    parse_attribute("dropOppositeColoredBishop", v->dropOppositeColoredBishop);
    parse_attribute("dropPromoted", v->dropPromoted);
    parse_attribute("shogiDoubledPawn", v->shogiDoubledPawn);
    parse_attribute("immobilityIllegal", v->immobilityIllegal);
    parse_attribute("gating", v->gating);
    parse_attribute("seirawanGating", v->seirawanGating);
    parse_attribute("cambodianMoves", v->cambodianMoves);
    // game end
    parse_attribute("nMoveRule", v->nMoveRule);
    parse_attribute("nFoldRule", v->nFoldRule);
    parse_attribute("nFoldValue", v->nFoldValue);
    parse_attribute("nFoldValueAbsolute", v->nFoldValueAbsolute);
    parse_attribute("perpetualCheckIllegal", v->perpetualCheckIllegal);
    parse_attribute("stalemateValue", v->stalemateValue);
    parse_attribute("checkmateValue", v->checkmateValue);
    parse_attribute("shogiPawnDropMateIllegal", v->shogiPawnDropMateIllegal);
    parse_attribute("shatarMateRule", v->shatarMateRule);
    parse_attribute("bareKingValue", v->bareKingValue);
    parse_attribute("extinctionValue", v->extinctionValue);
    parse_attribute("bareKingMove", v->bareKingMove);
    // extinction piece types
    const auto& it_ext = config.find("extinctionPieceTypes");
    if (it_ext != config.end())
    {
        char token;
        size_t idx;
        std::stringstream ss(it_ext->second);
        while (ss >> token && ((idx = v->pieceToChar.find(token)) != std::string::npos || token == '*'))
            v->extinctionPieceTypes.insert(PieceType(token == '*' ? 0 : idx));
    }
    // flag piece type
    const auto& it_flag_pt = config.find("flagPiece");
    if (it_flag_pt != config.end())
    {
        char token;
        size_t idx;
        std::stringstream ss(it_flag_pt->second);
        if (ss >> token && (idx = v->pieceToChar.find(token)) != std::string::npos)
            v->flagPiece = PieceType(idx);
    }
    parse_attribute("whiteFlag", v->whiteFlag);
    parse_attribute("blackFlag", v->blackFlag);
    parse_attribute("flagMove", v->flagMove);
    parse_attribute("checkCounting", v->checkCounting);
    parse_attribute("connectN", v->connectN);
    parse_attribute("countingRule", v->countingRule);
    return v;
}
