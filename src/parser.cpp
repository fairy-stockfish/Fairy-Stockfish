/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2021 Fabian Fichter

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

#include "apiutil.h"
#include "parser.h"
#include "piece.h"
#include "types.h"

namespace {

    template <typename T> bool set(const std::string& value, T& target)
    {
        std::stringstream ss(value);
        ss >> target;
        return !ss.fail();
    }

    template <> bool set(const std::string& value, Rank& target) {
        std::stringstream ss(value);
        int i;
        ss >> i;
        target = Rank(i - 1);
        return !ss.fail() && target >= RANK_1 && target <= RANK_MAX;
    }

    template <> bool set(const std::string& value, File& target) {
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
        return !ss.fail() && target >= FILE_A && target <= FILE_MAX;
    }

    template <> bool set(const std::string& value, std::string& target) {
        target = value;
        return true;
    }

    template <> bool set(const std::string& value, bool& target) {
        target = value == "true";
        return value == "true" || value == "false";
    }

    template <> bool set(const std::string& value, Value& target) {
        target =  value == "win"  ? VALUE_MATE
                : value == "loss" ? -VALUE_MATE
                : value == "draw" ? VALUE_DRAW
                : VALUE_NONE;
        return value == "win" || value == "loss" || value == "draw" || value == "none";
    }

    template <> bool set(const std::string& value, MaterialCounting& target) {
        target =  value == "janggi"  ? JANGGI_MATERIAL
                : value == "unweighted" ? UNWEIGHTED_MATERIAL
                : value == "whitedrawodds" ? WHITE_DRAW_ODDS
                : value == "blackdrawodds" ? BLACK_DRAW_ODDS
                : NO_MATERIAL_COUNTING;
        return   value == "janggi" || value == "unweighted"
              || value == "whitedrawodds" || value == "blackdrawodds" || value == "none";
    }

    template <> bool set(const std::string& value, CountingRule& target) {
        target =  value == "makruk"  ? MAKRUK_COUNTING
                : value == "asean" ? ASEAN_COUNTING
                : NO_COUNTING;
        return value == "makruk" || value == "asean" || value == "none";
    }

    template <> bool set(const std::string& value, EnclosingRule& target) {
        target =  value == "reversi"  ? REVERSI
                : value == "ataxx" ? ATAXX
                : NO_ENCLOSING;
        return value == "reversi" || value == "ataxx" || value == "none";
    }

    template <> bool set(const std::string& value, Bitboard& target) {
        char file;
        int rank;
        std::stringstream ss(value);
        target = 0;
        while (!ss.eof() && ss >> file && ss >> rank)
            target |= file == '*' ? rank_bb(Rank(rank - 1)) : square_bb(make_square(File(tolower(file) - 'a'), Rank(rank - 1)));
        return !ss.fail();
    }

} // namespace

template <bool DoCheck>
template <class T> void VariantParser<DoCheck>::parse_attribute(const std::string& key, T& target) {
    const auto& it = config.find(key);
    if (it != config.end())
    {
        bool valid = set(it->second, target);
        if (DoCheck && !valid)
        {
            std::string typeName =  std::is_same<T, int>() ? "int"
                                  : std::is_same<T, Rank>() ? "Rank"
                                  : std::is_same<T, File>() ? "File"
                                  : std::is_same<T, bool>() ? "bool"
                                  : std::is_same<T, Value>() ? "Value"
                                  : std::is_same<T, MaterialCounting>() ? "MaterialCounting"
                                  : std::is_same<T, CountingRule>() ? "CountingRule"
                                  : std::is_same<T, Bitboard>() ? "Bitboard"
                                  : typeid(T).name();
            std::cerr << key << " - Invalid value " << it->second << " for type " << typeName << std::endl;
        }
    }
}

template <bool DoCheck>
void VariantParser<DoCheck>::parse_attribute(const std::string& key, PieceType& target, std::string pieceToChar) {
    const auto& it = config.find(key);
    if (it != config.end())
    {
        char token;
        size_t idx;
        std::stringstream ss(it->second);
        if (ss >> token && (idx = token == '-' ? 0 : pieceToChar.find(toupper(token))) != std::string::npos)
            target = PieceType(idx);
        else if (DoCheck)
            std::cerr << key << " - Invalid piece type: " << token << std::endl;
    }
}

template <bool DoCheck>
Variant* VariantParser<DoCheck>::parse() {
    Variant* v = new Variant();
    v->reset_pieces();
    v->promotionPieceTypes = {};
    return parse(v);
}

template <bool DoCheck>
Variant* VariantParser<DoCheck>::parse(Variant* v) {
    // piece types
    for (const auto& pieceInfo : pieceMap)
    {
        // piece char
        const auto& keyValue = config.find(pieceInfo.second->name);
        if (keyValue != config.end() && !keyValue->second.empty())
        {
            if (isalpha(keyValue->second.at(0)))
                v->add_piece(pieceInfo.first, keyValue->second.at(0));
            else
            {
                if (DoCheck && keyValue->second.at(0) != '-')
                    std::cerr << pieceInfo.second->name << " - Invalid letter: " << keyValue->second.at(0) << std::endl;
                v->remove_piece(pieceInfo.first);
            }
        }
        // mobility region
        std::string capitalizedPiece = pieceInfo.second->name;
        capitalizedPiece[0] = toupper(capitalizedPiece[0]);
        for (Color c : {WHITE, BLACK})
        {
            std::string color = c == WHITE ? "White" : "Black";
            parse_attribute("mobilityRegion" + color + capitalizedPiece, v->mobilityRegion[c][pieceInfo.first]);
        }
    }
    // custom piece types
    for (PieceType pt = CUSTOM_PIECES; pt <= CUSTOM_PIECES_END; ++pt)
    {
        std::string customPieceName = "customPiece" + std::to_string(pt - CUSTOM_PIECES + 1);
        const auto& itCustomPt = config.find(customPieceName);
        if (itCustomPt != config.end() && !itCustomPt->second.empty())
        {
            // piece char
            if (isalpha(itCustomPt->second.at(0)))
                v->add_piece(pt, itCustomPt->second.at(0));
            else
            {
                if (DoCheck && itCustomPt->second.at(0) != '-')
                    std::cerr << customPieceName << " - Invalid letter: " << itCustomPt->second.at(0) << std::endl;
                v->remove_piece(pt);
            }
            // betza
            if (itCustomPt->second.size() > 1)
                v->customPiece[pt - CUSTOM_PIECES] = itCustomPt->second.substr(2);
            else if (DoCheck)
                std::cerr << customPieceName << " - Missing Betza move notation" << std::endl;
        }
    }
    // piece values
    for (Phase phase : {MG, EG})
    {
        const std::string optionName = phase == MG ? "pieceValueMg" : "pieceValueEg";
        const auto& pv = config.find(optionName);
        if (pv != config.end())
        {
            char token;
            size_t idx = 0;
            std::stringstream ss(pv->second);
            while (!ss.eof() && ss >> token && (idx = v->pieceToChar.find(toupper(token))) != std::string::npos
                             && ss >> token && ss >> v->pieceValue[phase][idx]) {}
            if (DoCheck && idx == std::string::npos)
                std::cerr << optionName << " - Invalid piece type: " << token << std::endl;
            else if (DoCheck && !ss.eof())
                std::cerr << optionName << " - Invalid piece value for type: " << v->pieceToChar[idx] << std::endl;
        }
    }
    parse_attribute("variantTemplate", v->variantTemplate);
    parse_attribute("pieceToCharTable", v->pieceToCharTable);
    parse_attribute("pocketSize", v->pocketSize);
    parse_attribute("maxRank", v->maxRank);
    parse_attribute("maxFile", v->maxFile);
    parse_attribute("chess960", v->chess960);
    parse_attribute("twoBoards", v->twoBoards);
    parse_attribute("startFen", v->startFen);
    parse_attribute("promotionRank", v->promotionRank);
    // promotion piece types
    const auto& it_prom = config.find("promotionPieceTypes");
    if (it_prom != config.end())
    {
        v->promotionPieceTypes = {};
        char token;
        size_t idx = 0;
        std::stringstream ss(it_prom->second);
        while (ss >> token && ((idx = v->pieceToChar.find(toupper(token))) != std::string::npos))
            v->promotionPieceTypes.insert(PieceType(idx));
        if (DoCheck && idx == std::string::npos && token != '-')
            std::cerr << "promotionPieceTypes - Invalid piece type: " << token << std::endl;
    }
    parse_attribute("sittuyinPromotion", v->sittuyinPromotion);
    // promotion limit
    const auto& it_prom_limit = config.find("promotionLimit");
    if (it_prom_limit != config.end())
    {
        char token;
        size_t idx = 0;
        std::stringstream ss(it_prom_limit->second);
        while (!ss.eof() && ss >> token && (idx = v->pieceToChar.find(toupper(token))) != std::string::npos
                         && ss >> token && ss >> v->promotionLimit[idx]) {}
        if (DoCheck && idx == std::string::npos)
            std::cerr << "promotionLimit - Invalid piece type: " << token << std::endl;
        else if (DoCheck && !ss.eof())
            std::cerr << "promotionLimit - Invalid piece count for type: " << v->pieceToChar[idx] << std::endl;
    }
    // promoted piece types
    const auto& it_prom_pt = config.find("promotedPieceType");
    if (it_prom_pt != config.end())
    {
        char token;
        size_t idx = 0, idx2 = 0;
        std::stringstream ss(it_prom_pt->second);
        while (   ss >> token && (idx = v->pieceToChar.find(toupper(token))) != std::string::npos && ss >> token
               && ss >> token && (idx2 = (token == '-' ? 0 : v->pieceToChar.find(toupper(token)))) != std::string::npos)
            v->promotedPieceType[idx] = PieceType(idx2);
        if (DoCheck && (idx == std::string::npos || idx2 == std::string::npos))
            std::cerr << "promotedPieceType - Invalid piece type: " << token << std::endl;
    }
    parse_attribute("piecePromotionOnCapture", v->piecePromotionOnCapture);
    parse_attribute("mandatoryPawnPromotion", v->mandatoryPawnPromotion);
    parse_attribute("mandatoryPiecePromotion", v->mandatoryPiecePromotion);
    parse_attribute("pieceDemotion", v->pieceDemotion);
    parse_attribute("blastOnCapture", v->blastOnCapture);
    parse_attribute("doubleStep", v->doubleStep);
    parse_attribute("doubleStepRank", v->doubleStepRank);
    parse_attribute("doubleStepRankMin", v->doubleStepRankMin);
    parse_attribute("enPassantRegion", v->enPassantRegion);
    parse_attribute("castling", v->castling);
    parse_attribute("castlingDroppedPiece", v->castlingDroppedPiece);
    parse_attribute("castlingKingsideFile", v->castlingKingsideFile);
    parse_attribute("castlingQueensideFile", v->castlingQueensideFile);
    parse_attribute("castlingRank", v->castlingRank);
    parse_attribute("castlingKingFile", v->castlingKingFile);
    parse_attribute("castlingKingPiece", v->castlingKingPiece, v->pieceToChar);
    parse_attribute("castlingRookPiece", v->castlingRookPiece, v->pieceToChar);
    parse_attribute("kingType", v->kingType, v->pieceToChar);
    parse_attribute("checking", v->checking);
    parse_attribute("dropChecks", v->dropChecks);
    parse_attribute("mustCapture", v->mustCapture);
    parse_attribute("mustDrop", v->mustDrop);
    parse_attribute("mustDropType", v->mustDropType, v->pieceToChar);
    parse_attribute("pieceDrops", v->pieceDrops);
    parse_attribute("dropLoop", v->dropLoop);
    parse_attribute("capturesToHand", v->capturesToHand);
    parse_attribute("firstRankPawnDrops", v->firstRankPawnDrops);
    parse_attribute("promotionZonePawnDrops", v->promotionZonePawnDrops);
    parse_attribute("dropOnTop", v->dropOnTop);
    parse_attribute("enclosingDrop", v->enclosingDrop);
    parse_attribute("enclosingDropStart", v->enclosingDropStart);
    parse_attribute("whiteDropRegion", v->whiteDropRegion);
    parse_attribute("blackDropRegion", v->blackDropRegion);
    parse_attribute("sittuyinRookDrop", v->sittuyinRookDrop);
    parse_attribute("dropOppositeColoredBishop", v->dropOppositeColoredBishop);
    parse_attribute("dropPromoted", v->dropPromoted);
    parse_attribute("dropNoDoubled", v->dropNoDoubled, v->pieceToChar);
    parse_attribute("immobilityIllegal", v->immobilityIllegal);
    parse_attribute("gating", v->gating);
    parse_attribute("arrowGating", v->arrowGating);
    parse_attribute("seirawanGating", v->seirawanGating);
    parse_attribute("cambodianMoves", v->cambodianMoves);
    parse_attribute("diagonalLines", v->diagonalLines);
    parse_attribute("pass", v->pass);
    parse_attribute("passOnStalemate", v->passOnStalemate);
    parse_attribute("makpongRule", v->makpongRule);
    parse_attribute("flyingGeneral", v->flyingGeneral);
    parse_attribute("soldierPromotionRank", v->soldierPromotionRank);
    parse_attribute("flipEnclosedPieces", v->flipEnclosedPieces);
    // game end
    parse_attribute("nMoveRule", v->nMoveRule);
    parse_attribute("nFoldRule", v->nFoldRule);
    parse_attribute("nFoldValue", v->nFoldValue);
    parse_attribute("nFoldValueAbsolute", v->nFoldValueAbsolute);
    parse_attribute("perpetualCheckIllegal", v->perpetualCheckIllegal);
    parse_attribute("moveRepetitionIllegal", v->moveRepetitionIllegal);
    parse_attribute("stalemateValue", v->stalemateValue);
    parse_attribute("stalematePieceCount", v->stalematePieceCount);
    parse_attribute("checkmateValue", v->checkmateValue);
    parse_attribute("shogiPawnDropMateIllegal", v->shogiPawnDropMateIllegal);
    parse_attribute("shatarMateRule", v->shatarMateRule);
    parse_attribute("bikjangRule", v->bikjangRule);
    parse_attribute("extinctionValue", v->extinctionValue);
    parse_attribute("extinctionClaim", v->extinctionClaim);
    parse_attribute("extinctionPseudoRoyal", v->extinctionPseudoRoyal);
    // extinction piece types
    const auto& it_ext = config.find("extinctionPieceTypes");
    if (it_ext != config.end())
    {
        v->extinctionPieceTypes = {};
        char token;
        size_t idx = 0;
        std::stringstream ss(it_ext->second);
        while (ss >> token && (idx = token == '*' ? size_t(ALL_PIECES) : v->pieceToChar.find(toupper(token))) != std::string::npos)
            v->extinctionPieceTypes.insert(PieceType(idx));
        if (DoCheck && idx == std::string::npos)
            std::cerr << "extinctionPieceTypes - Invalid piece type: " << token << std::endl;
    }
    parse_attribute("extinctionPieceCount", v->extinctionPieceCount);
    parse_attribute("extinctionOpponentPieceCount", v->extinctionOpponentPieceCount);
    parse_attribute("flagPiece", v->flagPiece, v->pieceToChar);
    parse_attribute("whiteFlag", v->whiteFlag);
    parse_attribute("blackFlag", v->blackFlag);
    parse_attribute("flagMove", v->flagMove);
    parse_attribute("checkCounting", v->checkCounting);
    parse_attribute("connectN", v->connectN);
    parse_attribute("materialCounting", v->materialCounting);
    parse_attribute("countingRule", v->countingRule);
    // Report invalid options
    if (DoCheck)
    {
        const std::set<std::string>& parsedKeys = config.get_comsumed_keys();
        for (const auto& it : config)
            if (parsedKeys.find(it.first) == parsedKeys.end())
                std::cerr << "Invalid option: " << it.first << std::endl;
    }
    // Check consistency
    if (DoCheck)
    {
        // startFen
        if (FEN::validate_fen(v->startFen, v, v->chess960) != FEN::FEN_OK)
            std::cerr << "startFen - Invalid starting position: " << v->startFen << std::endl;

        // pieceToCharTable
        if (v->pieceToCharTable != "-")
        {
            const std::string fenBoard = v->startFen.substr(0, v->startFen.find(' '));
            std::stringstream ss(v->pieceToCharTable);
            char token;
            while (ss >> token)
                if (isalpha(token) && v->pieceToChar.find(toupper(token)) == std::string::npos)
                    std::cerr << "pieceToCharTable - Invalid piece type: " << token << std::endl;
            for (PieceType pt : v->pieceTypes)
            {
                char ptl = tolower(v->pieceToChar[pt]);
                if (v->pieceToCharTable.find(ptl) == std::string::npos && fenBoard.find(ptl) != std::string::npos)
                    std::cerr << "pieceToCharTable - Missing piece type: " << ptl << std::endl;
                char ptu = toupper(v->pieceToChar[pt]);
                if (v->pieceToCharTable.find(ptu) == std::string::npos && fenBoard.find(ptu) != std::string::npos)
                    std::cerr << "pieceToCharTable - Missing piece type: " << ptu << std::endl;
            }
        }

        // Contradictory options
        if (!v->checking && v->checkCounting)
            std::cerr << "checkCounting=true requires checking=true." << std::endl;
        if (v->doubleStep && v->doubleStepRankMin > v->doubleStepRank)
            std::cerr << "Inconsistent settings: doubleStepRankMin > doubleStepRank." << std::endl;
        if (v->castling && v->castlingRank > v->maxRank)
            std::cerr << "Inconsistent settings: castlingRank > maxRank." << std::endl;
        if (v->castling && v->castlingQueensideFile > v->castlingKingsideFile)
            std::cerr << "Inconsistent settings: castlingQueensideFile > castlingKingsideFile." << std::endl;

        // Check for limitations

        // Options incompatible with royal kings
        if (v->pieceTypes.find(KING) != v->pieceTypes.end())
        {
            if (v->blastOnCapture)
                std::cerr << "Can not use kings with blastOnCapture." << std::endl;
            if (v->flipEnclosedPieces)
                std::cerr << "Can not use kings with flipEnclosedPieces." << std::endl;
            const PieceInfo* pi = pieceMap.find(v->kingType)->second;
            if (   pi->hopper[MODALITY_QUIET].size()
                || pi->hopper[MODALITY_CAPTURE].size()
                || std::any_of(pi->steps[MODALITY_CAPTURE].begin(), pi->steps[MODALITY_CAPTURE].end(), [](const std::pair<const Direction, int>& d) { return d.second; }))
                std::cerr << pi->name << " is not supported as kingType." << std::endl;
        }
    }
    return v;
}

template Variant* VariantParser<true>::parse();
template Variant* VariantParser<false>::parse();
template Variant* VariantParser<true>::parse(Variant* v);
template Variant* VariantParser<false>::parse(Variant* v);
