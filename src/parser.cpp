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

#include <string>
#include <sstream>

#include "apiutil.h"
#include "parser.h"
#include "piece.h"
#include "types.h"

namespace Stockfish {

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
                : value == "cambodian" ? CAMBODIAN_COUNTING
                : value == "asean" ? ASEAN_COUNTING
                : NO_COUNTING;
        return value == "makruk" || value == "asean" || value == "none";
    }

    template <> bool set(const std::string& value, ChasingRule& target) {
        target =  value == "axf"  ? AXF_CHASING
                : NO_CHASING;
        return value == "axf" || value == "none";
    }

    template <> bool set(const std::string& value, EnclosingRule& target) {
        target =  value == "reversi"  ? REVERSI
                : value == "ataxx" ? ATAXX
                : value == "quadwrangle" ? QUADWRANGLE
                : value == "snort" ? SNORT
                : value == "anyside" ? ANYSIDE
                : value == "top" ? TOP
                : NO_ENCLOSING;
        return value == "reversi" || value == "ataxx" || value == "quadwrangle" || value =="snort" || value =="anyside" || value =="top" || value == "none";
    }

    template <> bool set(const std::string& value, WallingRule& target) {
        target =  value == "arrow"  ? ARROW
                : value == "duck" ? DUCK
                : value == "edge" ? EDGE
                : value == "past" ? PAST
                : value == "static" ? STATIC
                : NO_WALLING;
        return value == "arrow" || value == "duck" || value == "edge" || value =="past" || value == "static" || value == "none";
    }

    template <> bool set(const std::string& value, Bitboard& target) {
        std::string symbol;
        std::stringstream ss(value);
        target = 0;
        while (!ss.eof() && ss >> symbol && symbol != "-")
        {
            if (symbol.back() == '*') {
                if (isalpha(symbol[0]) && symbol.length() == 2) {
                    char file = tolower(symbol[0]);
                    if (File(file - 'a') > FILE_MAX) return false;
                    target |= file_bb(File(file - 'a'));
                } else {
                    return false;
                }
            } else if (symbol[0] == '*') {
                int rank = std::stoi(symbol.substr(1));
                if (Rank(rank - 1) > RANK_MAX) return false;
                target |= rank_bb(Rank(rank - 1));
            } else if (isalpha(symbol[0]) && symbol.length() > 1) {
                char file = tolower(symbol[0]);
                int rank = std::stoi(symbol.substr(1));
                if (Rank(rank - 1) > RANK_MAX || File(file - 'a') > FILE_MAX) return false;
                target |= square_bb(make_square(File(file - 'a'), Rank(rank - 1)));
            } else {
                return false;
            }
        }
        return !ss.fail();
    }

    template <> bool set(const std::string& value, PieceTypeBitboardGroup& target) {
        size_t i;
        int ParserState = -1;
        int RankNum = 0;
        int FileNum = 0;
        char PieceChar = 0;
        Bitboard board = 0x00;
        // String parser using state machine
        for (i = 0; i < value.length(); i++)
        {
            const char ch = value.at(i);
            if (ch == ' ')
            {
                continue;
            }
            if (ParserState == -1)  // Initial state, if "-" exists here then it means a null value. e.g. promotionRegion = - means no promotion region
            {
                if (ch == '-')
                {
                    return true;
                }
                ParserState = 0;
            }
            if (ParserState == 0)  // Find piece type character
            {
                if (ch >= 'A' && ch <= 'Z')
                {
                    PieceChar = ch;
                    ParserState = 1;
                }
                else
                {
                    std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Illegal piece type character: " << ch << std::endl;
                    return false;
                }
            }
            else if (ParserState == 1)  // Find "("
            {
                if (ch == '(')
                {
                    ParserState = 2;
                }
                else
                {
                    std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Expect \"(\". Actual: " << ch << std::endl;
                    return false;
                }
            }
            else if (ParserState == 2)  //Find file
            {
                if (ch >= 'a' && ch <= 'z')
                {
                    FileNum = ch - 'a';
                    ParserState = 3;
                }
                else if (ch == '*')
                {
                    FileNum = -1;
                    ParserState = 3;
                }
                else
                {
                    std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Illegal file character: " << ch << std::endl;
                    return false;
                }
            }
            else if (ParserState == 3)  //Find rank and terminator "," or ")"
            {
                if (ch == '*')
                {
                    RankNum = -1;
                }
                else if (ch >= '0' && ch <= '9' && RankNum >= 0)
                {
                    RankNum = RankNum * 10 + (ch - '0');
                }
                else if (ch == ',' || ch == ')')
                {
                    if (RankNum == 0)  // Here if RankNum==0 then it means either user delcared a 0 as rank, or no rank number delcared at all
                    {
                        std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Illegal rank number: " << RankNum << std::endl;
                        return false;
                    }
                    if (RankNum > 0)  //When RankNum==-1, it means a whole File.
                    {
                        RankNum--;
                    }
                    if (RankNum < -1 || RankNum > RANK_MAX)
                    {
                        std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Max rank number exceeds. Max: " << RANK_MAX << "; Actual: " << RankNum << std::endl;
                        return false;
                    }
                    else if (FileNum < -1 || FileNum > FILE_MAX)
                    {
                        std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Max file number exceeds. Max: " << FILE_MAX << "; Actual: " << FileNum << std::endl;
                        return false;
                    }
                    if (RankNum == -1 && FileNum == -1)
                    {
                        board ^= ~board;
                    }
                    else if (FileNum == -1)
                    {
                        board |= rank_bb(Rank(RankNum));
                    }
                    else if (RankNum == -1)
                    {
                        board |= file_bb(File(FileNum));
                    }
                    else
                    {
                        board |= square_bb(make_square(File(FileNum), Rank(RankNum)));
                    }
                    if (ch == ')')
                    {
                        target.set(PieceChar, board);
                        ParserState = 4;
                    }
                    else
                    {
                        RankNum = 0;
                        FileNum = 0;
                        ParserState = 2;
                    }
                }
                else
                {
                    std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Illegal rank character: " << ch << std::endl;
                    return false;
                }
            }
            else if (ParserState == 4)  // Find ";"
            {
                if (ch == ';')
                {
                    ParserState = 0;
                    RankNum = 0;
                    FileNum = 0;
                    PieceChar = 0;
                    board = 0x00;
                }
                else
                {
                    std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Expects \";\"." << std::endl;
                    return false;
                }
            }
        }
        if (ParserState != 0)
        {
            std::cerr << "At char " << i << " of PieceTypeBitboardGroup declaration: Unterminated expression." << std::endl;
            return false;
        }
        return true;
    }


    template <> bool set(const std::string& value, CastlingRights& target) {
        char c;
        CastlingRights castlingRight;
        std::stringstream ss(value);
        target = NO_CASTLING;
        bool valid = true;
        while (ss >> c && c != '-')
        {
            castlingRight =  c == 'K' ? WHITE_OO
                           : c == 'Q' ? WHITE_OOO
                           : c == 'k' ? BLACK_OO
                           : c == 'q' ? BLACK_OOO
                           : NO_CASTLING;
            if (castlingRight)
                target = CastlingRights(target | castlingRight);
            else
                valid = false;
        }
        return valid;
    }

    template <typename T> void set(PieceType pt, T& target) {
        target.insert(pt);
    }

    template <> void set(PieceType pt, PieceType& target) {
        target = pt;
    }

    template <> void set(PieceType pt, PieceSet& target) {
        target |= pt;
    }

} // namespace

template <bool DoCheck>
template <bool Current, class T> bool VariantParser<DoCheck>::parse_attribute(const std::string& key, T& target) {
    const auto& it = config.find(key);
    if (it != config.end())
    {
        bool valid = set(it->second, target);
        if (DoCheck && !Current)
            std::cerr << key << " - Deprecated option might be removed in future version." << std::endl;
        if (DoCheck && !valid)
        {
            std::string typeName =  std::is_same<T, int>() ? "int"
                                  : std::is_same<T, Rank>() ? "Rank"
                                  : std::is_same<T, File>() ? "File"
                                  : std::is_same<T, bool>() ? "bool"
                                  : std::is_same<T, Value>() ? "Value"
                                  : std::is_same<T, MaterialCounting>() ? "MaterialCounting"
                                  : std::is_same<T, CountingRule>() ? "CountingRule"
                                  : std::is_same<T, ChasingRule>() ? "ChasingRule"
                                  : std::is_same<T, EnclosingRule>() ? "EnclosingRule"
                                  : std::is_same<T, Bitboard>() ? "Bitboard"
                                  : std::is_same<T, PieceTypeBitboardGroup>() ? "PieceTypeBitboardGroup"
                                  : std::is_same<T, CastlingRights>() ? "CastlingRights"
                                  : std::is_same<T, WallingRule>() ? "WallingRule"
                                  : typeid(T).name();
            std::cerr << key << " - Invalid value " << it->second << " for type " << typeName << std::endl;
        }
        return valid;
    }
    return false;
}

template <bool DoCheck>
template <bool Current, class T> bool VariantParser<DoCheck>::parse_attribute(const std::string& key, T& target, std::string pieceToChar) {
    const auto& it = config.find(key);
    if (it != config.end())
    {
        target = T();
        char token;
        size_t idx = std::string::npos;
        std::stringstream ss(it->second);
        while (ss >> token && (idx = token == '*' ? size_t(ALL_PIECES) : pieceToChar.find(toupper(token))) != std::string::npos)
            set(PieceType(idx), target);
        if (DoCheck && idx == std::string::npos && token != '-')
            std::cerr << key << " - Invalid piece type: " << token << std::endl;
        return idx != std::string::npos || token == '-';
    }
    return false;
}

template <bool DoCheck>
Variant* VariantParser<DoCheck>::parse() {
    Variant* v = new Variant();
    v->reset_pieces();
    return parse(v);
}

template <bool DoCheck>
Variant* VariantParser<DoCheck>::parse(Variant* v) {
    parse_attribute("maxRank", v->maxRank);
    parse_attribute("maxFile", v->maxFile);
    // piece types
    for (PieceType pt = PAWN; pt <= KING; ++pt)
    {
        if (pt == CUSTOM_PIECES_ROYAL)
            // reserved custom royal/king slot
            continue;

        // piece char
        std::string name = piece_name(pt);

        const auto& keyValue = config.find(name);
        if (keyValue != config.end() && !keyValue->second.empty())
        {
            if (isalpha(keyValue->second.at(0)))
                v->add_piece(pt, keyValue->second.at(0));
            else
            {
                if (DoCheck && keyValue->second.at(0) != '-')
                    std::cerr << name << " - Invalid letter: " << keyValue->second.at(0) << std::endl;
                v->remove_piece(pt);
            }
            // betza
            if (is_custom(pt))
            {
                if (keyValue->second.size() > 1)
                {
                    v->customPiece[pt - CUSTOM_PIECES] = keyValue->second.substr(2);
                    // Is there an en passant flag in the Betza notation?
                    if (v->customPiece[pt - CUSTOM_PIECES].find('e') != std::string::npos)
                    {
                        v->enPassantTypes[WHITE] |= piece_set(pt);
                        v->enPassantTypes[BLACK] |= piece_set(pt);
                    }
                }
                else if (DoCheck)
                    std::cerr << name << " - Missing Betza move notation" << std::endl;
            }
            else if (pt == KING)
            {
                if (keyValue->second.size() > 1)
                {
                    // custom royal piece
                    v->customPiece[CUSTOM_PIECES_ROYAL - CUSTOM_PIECES] = keyValue->second.substr(2);
                    v->kingType = CUSTOM_PIECES_ROYAL;
                }
                else
                    v->kingType = KING;
            }
        }
        // mobility region
        std::string capitalizedPiece = name;
        capitalizedPiece[0] = toupper(capitalizedPiece[0]);
        for (Color c : {WHITE, BLACK})
        {
            std::string color = c == WHITE ? "White" : "Black";
            parse_attribute("mobilityRegion" + color + capitalizedPiece, v->mobilityRegion[c][pt]);
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

    // Parse deprecate values for backwards compatibility
    Rank promotionRank = RANK_8;
    if (parse_attribute<false>("promotionRank", promotionRank))
    {
        for (Color c : {WHITE, BLACK})
            v->promotionRegion[c] = zone_bb(c, promotionRank, v->maxRank);
    }
    Rank doubleStepRank = RANK_2;
    Rank doubleStepRankMin = RANK_2;
    if (   parse_attribute<false>("doubleStepRank", doubleStepRank)
        || parse_attribute<false>("doubleStepRankMin", doubleStepRankMin))
    {
        for (Color c : {WHITE, BLACK})
            v->doubleStepRegion[c] =   zone_bb(c, doubleStepRankMin, v->maxRank)
                                    & ~forward_ranks_bb(c, relative_rank(c, doubleStepRank, v->maxRank));
    }
    parse_attribute<false>("whiteFlag", v->flagRegion[WHITE]);
    parse_attribute<false>("blackFlag", v->flagRegion[BLACK]);
    parse_attribute<false>("castlingRookPiece", v->castlingRookPieces[WHITE], v->pieceToChar);
    parse_attribute<false>("castlingRookPiece", v->castlingRookPieces[BLACK], v->pieceToChar);

    bool dropOnTop = false;
    parse_attribute<false>("dropOnTop", dropOnTop);
    if (dropOnTop) v->enclosingDrop=TOP;

    // Parse aliases
    parse_attribute("pawnTypes", v->promotionPawnType[WHITE], v->pieceToChar);
    parse_attribute("pawnTypes", v->promotionPawnType[BLACK], v->pieceToChar);
    parse_attribute("pawnTypes", v->promotionPawnTypes[WHITE], v->pieceToChar);
    parse_attribute("pawnTypes", v->promotionPawnTypes[BLACK], v->pieceToChar);
    parse_attribute("pawnTypes", v->enPassantTypes[WHITE], v->pieceToChar);
    parse_attribute("pawnTypes", v->enPassantTypes[BLACK], v->pieceToChar);
    parse_attribute("pawnTypes", v->nMoveRuleTypes[WHITE], v->pieceToChar);
    parse_attribute("pawnTypes", v->nMoveRuleTypes[BLACK], v->pieceToChar);

    // Parse the official config options
    parse_attribute("variantTemplate", v->variantTemplate);
    parse_attribute("pieceToCharTable", v->pieceToCharTable);
    parse_attribute("pocketSize", v->pocketSize);
    parse_attribute("chess960", v->chess960);
    parse_attribute("twoBoards", v->twoBoards);
    parse_attribute("startFen", v->startFen);
    parse_attribute("promotionRegionWhite", v->promotionRegion[WHITE]);
    parse_attribute("promotionRegionBlack", v->promotionRegion[BLACK]);
    parse_attribute("pieceSpecificPromotionRegion", v->pieceSpecificPromotionRegion);
    if (v->pieceSpecificPromotionRegion && !parse_attribute("whitePiecePromotionRegion", v->whitePiecePromotionRegion))
    {
        std::cerr << "Syntax error in whitePiecePromotionRegion or missing whitePiecePromotionRegion definition." << std::endl;
    }
    if (v->pieceSpecificPromotionRegion && !parse_attribute("blackPiecePromotionRegion", v->blackPiecePromotionRegion))
    {
        std::cerr << "Syntax error in blackPiecePromotionRegion or missing blackPiecePromotionRegion definition." << std::endl;
    }
    // Take the first promotionPawnTypes as the main promotionPawnType
    parse_attribute("promotionPawnTypes", v->promotionPawnType[WHITE], v->pieceToChar);
    parse_attribute("promotionPawnTypes", v->promotionPawnType[BLACK], v->pieceToChar);
    parse_attribute("promotionPawnTypes", v->promotionPawnTypes[WHITE], v->pieceToChar);
    parse_attribute("promotionPawnTypes", v->promotionPawnTypes[BLACK], v->pieceToChar);
    parse_attribute("promotionPawnTypesWhite", v->promotionPawnType[WHITE], v->pieceToChar);
    parse_attribute("promotionPawnTypesBlack", v->promotionPawnType[BLACK], v->pieceToChar);
    parse_attribute("promotionPawnTypesWhite", v->promotionPawnTypes[WHITE], v->pieceToChar);
    parse_attribute("promotionPawnTypesBlack", v->promotionPawnTypes[BLACK], v->pieceToChar);
    parse_attribute("promotionPieceTypes", v->promotionPieceTypes[WHITE], v->pieceToChar);
    parse_attribute("promotionPieceTypes", v->promotionPieceTypes[BLACK], v->pieceToChar);
    parse_attribute("promotionPieceTypesWhite", v->promotionPieceTypes[WHITE], v->pieceToChar);
    parse_attribute("promotionPieceTypesBlack", v->promotionPieceTypes[BLACK], v->pieceToChar);
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
    parse_attribute("blastImmuneTypes", v->blastImmuneTypes, v->pieceToChar);
    parse_attribute("mutuallyImmuneTypes", v->mutuallyImmuneTypes, v->pieceToChar);
    parse_attribute("petrifyOnCaptureTypes", v->petrifyOnCaptureTypes, v->pieceToChar);
    parse_attribute("petrifyBlastPieces", v->petrifyBlastPieces);
    parse_attribute("doubleStep", v->doubleStep);
    parse_attribute("doubleStepRegionWhite", v->doubleStepRegion[WHITE]);
    parse_attribute("doubleStepRegionBlack", v->doubleStepRegion[BLACK]);
    parse_attribute("pieceSpecificDoubleStepRegion", v->pieceSpecificDoubleStepRegion);
    if (v->pieceSpecificDoubleStepRegion && !parse_attribute("whitePieceDoubleStepRegion", v->whitePieceDoubleStepRegion))
    {
        std::cerr << "Syntax error in whitePieceDoubleStepRegion or missing whitePieceDoubleStepRegion definition." << std::endl;
    }
    if (v->pieceSpecificDoubleStepRegion && !parse_attribute("blackPieceDoubleStepRegion", v->blackPieceDoubleStepRegion))
    {
        std::cerr << "Syntax error in blackPieceDoubleStepRegion or missing blackPieceDoubleStepRegion definition." << std::endl;
    }
    parse_attribute("pieceSpecificTripleStepRegion", v->pieceSpecificTripleStepRegion);
    if (v->pieceSpecificTripleStepRegion && !parse_attribute("whitePieceTripleStepRegion", v->whitePieceTripleStepRegion))
    {
        std::cerr << "Syntax error in whitePieceTripleStepRegion or missing whitePieceTripleStepRegion definition." << std::endl;
    }
    if (v->pieceSpecificTripleStepRegion && !parse_attribute("blackPieceTripleStepRegion", v->blackPieceTripleStepRegion))
    {
        std::cerr << "Syntax error in blackPieceTripleStepRegion or missing blackPieceTripleStepRegion definition." << std::endl;
    }
    parse_attribute("tripleStepRegionWhite", v->tripleStepRegion[WHITE]);
    parse_attribute("tripleStepRegionBlack", v->tripleStepRegion[BLACK]);
    parse_attribute("enPassantRegion", v->enPassantRegion);
    parse_attribute("enPassantTypes", v->enPassantTypes[WHITE], v->pieceToChar);
    parse_attribute("enPassantTypes", v->enPassantTypes[BLACK], v->pieceToChar);
    parse_attribute("enPassantTypesWhite", v->enPassantTypes[WHITE], v->pieceToChar);
    parse_attribute("enPassantTypesBlack", v->enPassantTypes[BLACK], v->pieceToChar);
    parse_attribute("castling", v->castling);
    parse_attribute("castlingDroppedPiece", v->castlingDroppedPiece);
    parse_attribute("castlingKingsideFile", v->castlingKingsideFile);
    parse_attribute("castlingQueensideFile", v->castlingQueensideFile);
    parse_attribute("castlingRank", v->castlingRank);
    parse_attribute("castlingKingFile", v->castlingKingFile);
    parse_attribute("castlingKingPiece", v->castlingKingPiece[WHITE], v->pieceToChar);
    parse_attribute("castlingKingPiece", v->castlingKingPiece[BLACK], v->pieceToChar);
    parse_attribute("castlingKingPieceWhite", v->castlingKingPiece[WHITE], v->pieceToChar);
    parse_attribute("castlingKingPieceBlack", v->castlingKingPiece[BLACK], v->pieceToChar);
    parse_attribute("castlingRookKingsideFile", v->castlingRookKingsideFile);
    parse_attribute("castlingRookQueensideFile", v->castlingRookQueensideFile);
    parse_attribute("castlingRookPieces", v->castlingRookPieces[WHITE], v->pieceToChar);
    parse_attribute("castlingRookPieces", v->castlingRookPieces[BLACK], v->pieceToChar);
    parse_attribute("castlingRookPiecesWhite", v->castlingRookPieces[WHITE], v->pieceToChar);
    parse_attribute("castlingRookPiecesBlack", v->castlingRookPieces[BLACK], v->pieceToChar);
    parse_attribute("oppositeCastling", v->oppositeCastling);
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
    parse_attribute("enclosingDrop", v->enclosingDrop);
    parse_attribute("enclosingDropStart", v->enclosingDropStart);
    parse_attribute("whiteDropRegion", v->whiteDropRegion);
    parse_attribute("blackDropRegion", v->blackDropRegion);
    parse_attribute("pieceSpecificDropRegion", v->pieceSpecificDropRegion);
    if (v->pieceSpecificDropRegion && !parse_attribute("whitePieceDropRegion", v->whitePieceDropRegion))
    {
        std::cerr << "Syntax error in whitePieceDropRegion or missing whitePieceDropRegion definition." << std::endl;
    }
    if (v->pieceSpecificDropRegion && !parse_attribute("blackPieceDropRegion", v->blackPieceDropRegion))
    {
        std::cerr << "Syntax error in blackPieceDropRegion or missing blackPieceDropRegion definition." << std::endl;
    }
    parse_attribute("sittuyinRookDrop", v->sittuyinRookDrop);
    parse_attribute("dropOppositeColoredBishop", v->dropOppositeColoredBishop);
    parse_attribute("dropPromoted", v->dropPromoted);
    parse_attribute("dropNoDoubled", v->dropNoDoubled, v->pieceToChar);
    parse_attribute("dropNoDoubledCount", v->dropNoDoubledCount);
    parse_attribute("immobilityIllegal", v->immobilityIllegal);
    parse_attribute("gating", v->gating);
    parse_attribute("wallingRule", v->wallingRule);
    parse_attribute("wallingRegionWhite", v->wallingRegion[WHITE]);
    parse_attribute("wallingRegionBlack", v->wallingRegion[BLACK]);
    parse_attribute("wallingRegion", v->wallingRegion[WHITE]);
    parse_attribute("wallingRegion", v->wallingRegion[BLACK]);
    parse_attribute("wallOrMove", v->wallOrMove);
    parse_attribute("seirawanGating", v->seirawanGating);
    parse_attribute("cambodianMoves", v->cambodianMoves);
    parse_attribute("diagonalLines", v->diagonalLines);
    parse_attribute("pass", v->pass[WHITE]);
    parse_attribute("pass", v->pass[BLACK]);
    parse_attribute("passWhite", v->pass[WHITE]);
    parse_attribute("passBlack", v->pass[BLACK]);
    parse_attribute("passOnStalemate", v->passOnStalemate[WHITE]);
    parse_attribute("passOnStalemate", v->passOnStalemate[BLACK]);
    parse_attribute("passOnStalemateWhite", v->passOnStalemate[WHITE]);
    parse_attribute("passOnStalemateBlack", v->passOnStalemate[BLACK]);
    parse_attribute("makpongRule", v->makpongRule);
    parse_attribute("flyingGeneral", v->flyingGeneral);
    parse_attribute("soldierPromotionRank", v->soldierPromotionRank);
    parse_attribute("flipEnclosedPieces", v->flipEnclosedPieces);
    // game end
    parse_attribute("nMoveRuleTypes", v->nMoveRuleTypes[WHITE], v->pieceToChar);
    parse_attribute("nMoveRuleTypes", v->nMoveRuleTypes[BLACK], v->pieceToChar);
    parse_attribute("nMoveRuleTypesWhite", v->nMoveRuleTypes[WHITE], v->pieceToChar);
    parse_attribute("nMoveRuleTypesBlack", v->nMoveRuleTypes[BLACK], v->pieceToChar);
    parse_attribute("nMoveRule", v->nMoveRule);
    parse_attribute("nFoldRule", v->nFoldRule);
    parse_attribute("nFoldValue", v->nFoldValue);
    parse_attribute("nFoldValueAbsolute", v->nFoldValueAbsolute);
    parse_attribute("perpetualCheckIllegal", v->perpetualCheckIllegal);
    parse_attribute("moveRepetitionIllegal", v->moveRepetitionIllegal);
    parse_attribute("chasingRule", v->chasingRule);
    parse_attribute("stalemateValue", v->stalemateValue);
    parse_attribute("stalematePieceCount", v->stalematePieceCount);
    parse_attribute("checkmateValue", v->checkmateValue);
    parse_attribute("shogiPawnDropMateIllegal", v->shogiPawnDropMateIllegal);
    parse_attribute("shatarMateRule", v->shatarMateRule);
    parse_attribute("bikjangRule", v->bikjangRule);
    parse_attribute("extinctionValue", v->extinctionValue);
    parse_attribute("extinctionClaim", v->extinctionClaim);
    parse_attribute("extinctionPseudoRoyal", v->extinctionPseudoRoyal);
    parse_attribute("dupleCheck", v->dupleCheck);
    // extinction piece types
    parse_attribute("extinctionPieceTypes", v->extinctionPieceTypes, v->pieceToChar);
    parse_attribute("extinctionPieceCount", v->extinctionPieceCount);
    parse_attribute("extinctionOpponentPieceCount", v->extinctionOpponentPieceCount);
    parse_attribute("flagPiece", v->flagPiece[WHITE], v->pieceToChar);
    parse_attribute("flagPiece", v->flagPiece[BLACK], v->pieceToChar);
    parse_attribute("flagPieceWhite", v->flagPiece[WHITE], v->pieceToChar);
    parse_attribute("flagPieceBlack", v->flagPiece[BLACK], v->pieceToChar);
    parse_attribute("flagRegion", v->flagRegion[WHITE]);
    parse_attribute("flagRegion", v->flagRegion[BLACK]);
    parse_attribute("flagRegionWhite", v->flagRegion[WHITE]);
    parse_attribute("flagRegionBlack", v->flagRegion[BLACK]);
    parse_attribute("flagPieceCount", v->flagPieceCount);
    parse_attribute("flagPieceBlockedWin", v->flagPieceBlockedWin);
    parse_attribute("flagMove", v->flagMove);
    parse_attribute("flagPieceSafe", v->flagPieceSafe);
    parse_attribute("checkCounting", v->checkCounting);
    parse_attribute("connectN", v->connectN);
    parse_attribute("connectPieceTypes", v->connectPieceTypes, v->pieceToChar);
    parse_attribute("connectHorizontal", v->connectHorizontal);
    parse_attribute("connectVertical", v->connectVertical);
    parse_attribute("connectDiagonal", v->connectDiagonal);
    parse_attribute("connectRegion1White", v->connectRegion1[WHITE]);
    parse_attribute("connectRegion2White", v->connectRegion2[WHITE]);
    parse_attribute("connectRegion1Black", v->connectRegion1[BLACK]);
    parse_attribute("connectRegion2Black", v->connectRegion2[BLACK]);
    parse_attribute("connectNxN", v->connectNxN);
    parse_attribute("collinearN", v->collinearN);
    parse_attribute("connectValue", v->connectValue);
    parse_attribute("materialCounting", v->materialCounting);
    parse_attribute("adjudicateFullBoard", v->adjudicateFullBoard);
    parse_attribute("countingRule", v->countingRule);
    parse_attribute("castlingWins", v->castlingWins);
    
    // Report invalid options
    if (DoCheck)
    {
        const std::set<std::string>& parsedKeys = config.get_consumed_keys();
        for (const auto& it : config)
            if (parsedKeys.find(it.first) == parsedKeys.end())
                std::cerr << "Invalid option: " << it.first << std::endl;
    }
    // Check consistency
    if (DoCheck)
    {
        // pieces
        for (PieceSet ps = v->pieceTypes; ps;)
        {
            PieceType pt = pop_lsb(ps);
            for (Color c : {WHITE, BLACK})
                if (std::count(v->pieceToChar.begin(), v->pieceToChar.end(), v->pieceToChar[make_piece(c, pt)]) != 1)
                    std::cerr << piece_name(pt) << " - Ambiguous piece character: " << v->pieceToChar[make_piece(c, pt)] << std::endl;
        }

        v->conclude(); // In preparation for the consistency checks below

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
            for (PieceSet ps = v->pieceTypes; ps;)
            {
                PieceType pt = pop_lsb(ps);
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
        if (v->castling && v->castlingRank > v->maxRank)
            std::cerr << "Inconsistent settings: castlingRank > maxRank." << std::endl;
        if (v->castling && v->castlingQueensideFile > v->castlingKingsideFile)
            std::cerr << "Inconsistent settings: castlingQueensideFile > castlingKingsideFile." << std::endl;

        // Check for limitations
        if (v->pieceDrops && v->wallingRule)
            std::cerr << "pieceDrops and any walling are incompatible." << std::endl;

        // Options incompatible with royal kings
        if (v->pieceTypes & KING)
        {
            if (v->blastOnCapture)
                std::cerr << "Can not use kings with blastOnCapture." << std::endl;
            if (v->flipEnclosedPieces)
                std::cerr << "Can not use kings with flipEnclosedPieces." << std::endl;
            if (v->wallingRule==DUCK)
                std::cerr << "Can not use kings with wallingRule = duck." << std::endl;
            // We can not fully check support for custom king movements at this point,
            // since custom pieces are only initialized on loading of the variant.
            // We will assume this is valid, but it might cause problems later if it's not.
            if (!is_custom(v->kingType))
            {
                const PieceInfo* pi = pieceMap.find(v->kingType)->second;
                if (   pi->hopper[0][MODALITY_QUIET].size()
                    || pi->hopper[0][MODALITY_CAPTURE].size()
                    || std::any_of(pi->steps[0][MODALITY_CAPTURE].begin(),
                                   pi->steps[0][MODALITY_CAPTURE].end(),
                                   [](const std::pair<const Direction, int>& d) { return d.second; }))
                    std::cerr << piece_name(v->kingType) << " is not supported as kingType." << std::endl;
            }
        }
        // Options incompatible with royal kings OR pseudo-royal kings. Possible in theory though:
        // 1. In blast variants, moving a (pseudo-)royal blastImmuneType into another piece is legal.
        // 2. In blast variants, capturing a piece next to a (pseudo-)royal blastImmuneType is legal.
        // 3. Moving a (pseudo-)royal mutuallyImmuneType into a square threatened by the same type is legal.
        if ((v->extinctionPseudoRoyal) || (v->pieceTypes & KING))
        {
            if (v->blastImmuneTypes)
                std::cerr << "Can not use kings or pseudo-royal with blastImmuneTypes." << std::endl;
            if (v->mutuallyImmuneTypes)
                std::cerr << "Can not use kings or pseudo-royal with mutuallyImmuneTypes." << std::endl;
        }
        if (v->flagPieceSafe && v->blastOnCapture)
            std::cerr << "Can not use flagPieceSafe with blastOnCapture (flagPieceSafe uses simple assessment that does not see blast)." << std::endl;
    }
    return v;
}

template Variant* VariantParser<true>::parse();
template Variant* VariantParser<false>::parse();
template Variant* VariantParser<true>::parse(Variant* v);
template Variant* VariantParser<false>::parse(Variant* v);

} // namespace Stockfish
