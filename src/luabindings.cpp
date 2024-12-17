// Include Lua headers first
#include <lua.hpp>

// Then include LuaBridge
#include <LuaBridge/LuaBridge.h>

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
#include "movegen.h"
#include "apiutil.h"
#include <iostream>
#include <cstdio>
#include <string>

using namespace Stockfish;

// Forward declare LuaBoard class
class LuaBoard;

// Add lua_State forward declaration
struct lua_State;

// Add using directive at the top
using namespace luabridge;

// Forward declarations at the top
class LuaBoard;
class Game;

// Add at the top after includes
#define DEBUG_LOG(msg) do { printf("DEBUG [C++]: %s\n", msg); fflush(stdout); } while(0)
#define DEBUG_LOGF(fmt, ...) do { printf("DEBUG [C++]: " fmt "\n", __VA_ARGS__); fflush(stdout); } while(0)

// Add at the top after includes
static bool stockfishInitialized = false;
static std::mutex threadMutex;
static bool threadsInitialized = false;

// Add thread initialization function
void initializeThreads() {
    std::lock_guard<std::mutex> lock(threadMutex);
    if (!threadsInitialized) {
        DEBUG_LOG("Initializing threads");
        try {
            // Initialize UCI options first
            DEBUG_LOG("Setting up UCI options");
            UCI::init(Stockfish::Options);
            
            // Set threads option
            DEBUG_LOG("Setting threads option");
            Stockfish::Options["Threads"] = std::string("1");
            
            // Verify thread creation
            DEBUG_LOG("Verifying main thread");
            if (!Threads.main()) {
                throw std::runtime_error("Failed to create main thread after setting option");
            }
            
            threadsInitialized = true;
            DEBUG_LOG("Threads initialized successfully");
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception during thread initialization: %s", e.what());
            throw;
        }
        catch (...) {
            DEBUG_LOG("Unknown exception during thread initialization");
            throw;
        }
    }
}

// Modify initialize_stockfish to use fully qualified names
void initialize_stockfish() {
    static std::mutex initMutex;
    std::lock_guard<std::mutex> lock(initMutex);
    
    if (!stockfishInitialized) {
        try {
            DEBUG_LOG("Initializing Stockfish components");
            
            DEBUG_LOG("Initializing piece map");
            pieceMap.init();
            
            DEBUG_LOG("Initializing variants");
            Stockfish::variants.init();
            
            DEBUG_LOG("Initializing UCI");
            UCI::init(Stockfish::Options);
            
            DEBUG_LOG("Initializing bitboards");
            Bitboards::init();
            
            DEBUG_LOG("Initializing position");
            Position::init();
            
            DEBUG_LOG("Initializing bitbases");
            Bitbases::init();
            
            DEBUG_LOG("Initializing threads");
            initializeThreads();
            
            stockfishInitialized = true;
            DEBUG_LOG("Stockfish initialization complete");
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception during Stockfish initialization: %s", e.what());
            throw;
        }
        catch (...) {
            DEBUG_LOG("Unknown exception during Stockfish initialization");
            throw;
        }
    }
}

namespace ffish {
    std::string info() {
        return engine_info();
    }

    std::string variants() {
        std::string availableVariants;
        for (const std::string& variant : Stockfish::variants.get_keys()) {
            availableVariants += variant;
            availableVariants += " ";
        }
        if (!availableVariants.empty())
            availableVariants.pop_back();
        return availableVariants;
    }

    void loadVariantConfig(const std::string& config) {
        DEBUG_LOG("Loading variant config");
        DEBUG_LOGF("Config length: %zu", config.length());
        try {
            std::stringstream ss(config);
            DEBUG_LOG("Created stringstream");
            Stockfish::variants.parse_istream<false>(ss);
            DEBUG_LOG("Successfully parsed variant config");
            Stockfish::Options["UCI_Variant"].set_combo(Stockfish::variants.get_keys());
            DEBUG_LOG("Updated UCI variant options");
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception in loadVariantConfig: %s", e.what());
            throw;
        }
        catch (...) {
            DEBUG_LOG("Unknown exception in loadVariantConfig");
            throw;
        }
    }

    int validateFen(const std::string& fen, const std::string& variant, bool chess960) {
        const Variant* v = Stockfish::variants.find(variant)->second;
        return FEN::validate_fen(fen, v, chess960);
    }

    int validateFen(const std::string& fen, const std::string& variant) {
        return validateFen(fen, variant, false);
    }

    int validateFen(const std::string& fen) {
        return validateFen(fen, "chess", false);
    }

    void setOption(const std::string& name, const std::string& value);

    std::string startingFen(const std::string& uciVariant) {
        const Variant* v = Stockfish::variants.find(uciVariant)->second;
        return v->startFen;
    }

    bool capturesToHand(const std::string& uciVariant) {
        const Variant* v = Stockfish::variants.find(uciVariant)->second;
        return v->capturesToHand;
    }

    bool twoBoards(const std::string& uciVariant) {
        const Variant* v = Stockfish::variants.find(uciVariant)->second;
        return v->twoBoards;
    }
}

// Add these before the class definitions
namespace luabridge {
    template <>
    struct ContainerTraits<Game> {
        using Type = Game;
        static Game* get(Game* v) { return v; }
        static Game* get(Game& v) { return &v; }
        static const Game* get(const Game* v) { return v; }
        static const Game* get(const Game& v) { return &v; }
    };
}

// At the top, just declare the function
namespace ffish {
    void setOption(const std::string& name, const std::string& value);
}

// Define LuaBoard class first
class LuaBoard {
public:
    static bool sfInitialized;
    static lua_State* L;

    LuaBoard() : 
        v(nullptr),
        states(nullptr),  // Initialize to nullptr first
        thread(nullptr),
        chess960(false)
    {
        DEBUG_LOG("LuaBoard constructor start");
        
        try {
            // Initialize Stockfish if needed
            if (!stockfishInitialized) {
                DEBUG_LOG("Initializing Stockfish");
                initialize_stockfish();
            }

            // Create states
            DEBUG_LOG("Creating states");
            states = StateListPtr(new std::deque<StateInfo>(1));
            if (!states || states->empty()) {
                throw std::runtime_error("Failed to create states");
            }

            // Get thread with safety check
            DEBUG_LOG("Getting main thread");
            {
                std::lock_guard<std::mutex> lock(threadMutex);
                if (!threadsInitialized) {
                    DEBUG_LOG("Threads not initialized, initializing now");
                    initializeThreads();
                }
                thread = Threads.main();
                if (!thread) {
                    DEBUG_LOG("Failed to get main thread");
                    Stockfish::Options["Threads"] = std::string("1");
                    thread = Threads.main();
                    if (!thread) {
                        throw std::runtime_error("Failed to get main thread");
                    }
                }
            }
            DEBUG_LOG("Successfully got main thread");

            DEBUG_LOG("About to call init()");
            init("chess", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false);
            DEBUG_LOG("LuaBoard constructor complete");
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception in constructor: %s", e.what());
            cleanup();  // Clean up any partially initialized resources
            throw;  // Re-throw the exception
        }
        catch (...) {
            DEBUG_LOG("Unknown exception in constructor");
            cleanup();
            throw;
        }
    }

    LuaBoard(const std::string& uciVariant) :
        v(nullptr),
        states(new std::deque<StateInfo>(1)),
        thread(Threads.main()),
        chess960(false)
    {
        DEBUG_LOG("LuaBoard variant constructor start");
        
        if (!states) {
            DEBUG_LOG("Failed to create states");
            throw std::runtime_error("Failed to create states");
        }
        
        if (!thread) {
            DEBUG_LOG("Failed to get main thread");
            Stockfish::Options["Threads"] = std::string("1");
            thread = Threads.main();
            if (!thread) {
                throw std::runtime_error("Failed to get main thread");
            }
        }
        
        DEBUG_LOG("About to call init()");
        init(uciVariant, "", false);
        DEBUG_LOG("LuaBoard variant constructor complete");
    }

    LuaBoard(const std::string& uciVariant, const std::string& fen) :
        v(nullptr),
        states(new std::deque<StateInfo>(1)),
        thread(Threads.main()),
        chess960(false)
    {
        DEBUG_LOG("LuaBoard variant+fen constructor start");
        
        if (!states) {
            DEBUG_LOG("Failed to create states");
            throw std::runtime_error("Failed to create states");
        }
        
        if (!thread) {
            DEBUG_LOG("Failed to get main thread");
            Stockfish::Options["Threads"] = std::string("1");
            thread = Threads.main();
            if (!thread) {
                throw std::runtime_error("Failed to get main thread");
            }
        }
        
        DEBUG_LOG("About to call init()");
        init(uciVariant, fen, false);
        DEBUG_LOG("LuaBoard variant+fen constructor complete");
    }

    LuaBoard(const std::string& uciVariant, const std::string& fen, bool is960) :
        v(nullptr),
        states(new std::deque<StateInfo>(1)),
        thread(Threads.main()),
        chess960(is960)
    {
        DEBUG_LOG("LuaBoard variant+fen+960 constructor start");
        
        if (!states) {
            DEBUG_LOG("Failed to create states");
            throw std::runtime_error("Failed to create states");
        }
        
        if (!thread) {
            DEBUG_LOG("Failed to get main thread");
            Stockfish::Options["Threads"] = std::string("1");
            thread = Threads.main();
            if (!thread) {
                throw std::runtime_error("Failed to get main thread");
            }
        }
        
        DEBUG_LOG("About to call init()");
        init(uciVariant, fen, is960);
        DEBUG_LOG("LuaBoard variant+fen+960 constructor complete");
    }

    LuaBoard(LuaBoard&& other) noexcept
        : v(other.v)
        , states(std::move(other.states))
        , pos()
        , thread(other.thread)
        , moves(std::move(other.moves))
        , chess960(other.chess960)
    {
        pos.set(v, other.pos.fen(), other.chess960, &states->back(), other.thread);
        
        other.v = nullptr;
        other.thread = nullptr;
    }

    LuaBoard& operator=(LuaBoard&& other) noexcept {
        if (this != &other) {
            v = other.v;
            states = std::move(other.states);
            pos.set(v, other.pos.fen(), other.chess960, &states->back(), other.thread);
            thread = other.thread;
            moves = std::move(other.moves);
            chess960 = other.chess960;
            
            other.v = nullptr;
            other.thread = nullptr;
        }
        return *this;
    }

    LuaBoard(const LuaBoard&) = delete;
    LuaBoard& operator=(const LuaBoard&) = delete;

    ~LuaBoard() {
        std::cerr << "DEBUG: LuaBoard destructor called" << std::endl;
        states.reset();
        v = nullptr;
        thread = nullptr;
        moves.clear();
    }

    std::string legalMoves() {
        std::string moves;
        for (const ExtMove& move : MoveList<LEGAL>(this->pos)) {
            moves += UCI::move(this->pos, move);
            moves += " ";
        }
        if (!moves.empty())
            moves.pop_back();
        return moves;
    }

    bool push(const std::string& uciMove) {
        try {
            std::string moveStr = uciMove;
            const Move move = UCI::to_move(this->pos, moveStr);
            if (move == MOVE_NONE)
                return false;
            states->emplace_back();
            pos.do_move(move, states->back());
            moves.push_back(move);
            return true;
        }
        catch (const std::exception& e) {
            luaL_error(L, "Failed to push move: %s", e.what());
            return false;
        }
    }

    void pop() {
        if (!moves.empty()) {
            pos.undo_move(moves.back());
            moves.pop_back();
            states->pop_back();
        }
    }

    std::string fen() const {
        return pos.fen();
    }

    bool isCheck() const {
        return Stockfish::checked(pos);
    }

    bool isGameOver() const {
        return MoveList<LEGAL>(pos).size() == 0 || is_insufficient_material();
    }

    bool is_insufficient_material() const {
        return has_insufficient_material(WHITE, pos) && has_insufficient_material(BLACK, pos);
    }

    std::string legalMovesSan() {
        std::string movesSan;
        for (const ExtMove& move : MoveList<LEGAL>(this->pos)) {
            movesSan += SAN::move_to_san(this->pos, move, NOTATION_SAN);
            movesSan += " ";
        }
        if (!movesSan.empty())
            movesSan.pop_back();
        return movesSan;
    }

    int numberLegalMoves() const {
        return MoveList<LEGAL>(pos).size();
    }

    bool pushSan(const std::string& sanMove) {
        return pushSan(sanMove, NOTATION_SAN);
    }

    bool pushSan(const std::string& sanMove, Notation notation) {
        Move foundMove = MOVE_NONE;
        for (const ExtMove& move : MoveList<LEGAL>(pos)) {
            if (sanMove == SAN::move_to_san(this->pos, move, notation)) {
                foundMove = move;
                break;
            }
        }
        if (foundMove == MOVE_NONE)
            return false;
        states->emplace_back();
        pos.do_move(foundMove, states->back());
        moves.push_back(foundMove);
        return true;
    }

    void reset() {
        set_fen(v->startFen);
    }

    bool is960() const {
        return chess960;
    }

    std::string fen(bool showPromoted) const {
        return pos.fen(false, showPromoted);
    }

    std::string fen(bool showPromoted, int countStarted) const {
        return pos.fen(false, showPromoted, countStarted);
    }

    void set_fen(const std::string& fen) {
        try {
            states = StateListPtr(new std::deque<StateInfo>(1));
            moves.clear();
            
            // Skip FEN validation for variants with custom pieces
            if (v->variantTemplate != "spartan" && 
                v->variantTemplate != "janggi" && 
                v->variantTemplate != "xiangqi" && 
                v->variantTemplate != "shogi" && 
                v->variantTemplate != "makruk") {
                if (FEN::validate_fen(fen, v, chess960) != 1) {
                    throw std::runtime_error("Invalid FEN: " + fen);
                }
            }
            
            try {
                pos.set(v, fen, chess960, &states->back(), thread);
            }
            catch (const std::exception& e) {
                throw std::runtime_error(std::string("Failed to set position: ") + e.what());
            }
            
            if (!pos.pos_is_ok()) {
                throw std::runtime_error("Invalid position after setting FEN");
            }
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception in set_fen: %s", e.what());
            throw;
        }
    }

    std::string sanMove(const std::string& uciMove) {
        return sanMove(uciMove, NOTATION_SAN);
    }

    std::string sanMove(const std::string& uciMove, Notation notation) {
        std::string moveStr = uciMove;
        const Move move = UCI::to_move(this->pos, moveStr);
        if (move == MOVE_NONE)
            return "";
        return SAN::move_to_san(this->pos, move, notation);
    }

    bool turn() const {
        return !pos.side_to_move();
    }

    int fullmoveNumber() const {
        return pos.game_ply() / 2 + 1;
    }

    int halfmoveClock() const {
        return pos.rule50_count();
    }

    int gamePly() const {
        return pos.game_ply();
    }

    bool hasInsufficientMaterial(bool turn) const {
        return has_insufficient_material(turn ? WHITE : BLACK, pos);
    }

    std::string result(bool claimDraw = false) const {
        Value result;
        bool gameEnd = pos.is_immediate_game_end(result);
        
        if (!gameEnd) {
            if (is_insufficient_material()) {
                gameEnd = true;
                result = VALUE_DRAW;
            }
        }
        
        if (!gameEnd && MoveList<LEGAL>(pos).size() == 0) {
            gameEnd = true;
            result = pos.checkmate_value();
        }
        
        if (!gameEnd && claimDraw)
            gameEnd = pos.is_optional_game_end(result);

        if (!gameEnd)
            return "*";
            
        if (result == VALUE_DRAW)
            return "1/2-1/2";
            
        if (pos.side_to_move() == BLACK)
            result = -result;
            
        return result > VALUE_DRAW ? "1-0" : "0-1";
    }

    std::string checkedPieces() const {
        Bitboard checked = Stockfish::checked(pos);
        std::string squares;
        while (checked) {
            Square sr = pop_lsb(checked);
            squares += UCI::square(pos, sr);
            squares += " ";
        }
        if (!squares.empty())
            squares.pop_back();
        return squares;
    }

    bool isBikjang() const {
        return pos.bikjang();
    }

    bool isCapture(const std::string& uciMove) const {
        try {
            std::string moveStr = uciMove;
            Move m = UCI::to_move(this->pos, moveStr);
            if (m == MOVE_NONE) {
                return false;
            }
            // Check if the move is legal before checking if it's a capture
            bool isLegal = false;
            for (const ExtMove& move : MoveList<LEGAL>(pos)) {
                if (move == m) {
                    isLegal = true;
                    break;
                }
            }
            if (!isLegal) {
                return false;
            }
            return pos.capture(m);
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception in isCapture: %s", e.what());
            return false;
        }
    }

    std::string getMoveStack() const {
        std::string moveStr;
        for (const auto& move : moves) {
            moveStr += UCI::move(pos, move);
            moveStr += " ";
        }
        if (!moveStr.empty())
            moveStr.pop_back();
        return moveStr;
    }

    void pushMoves(const std::string& uciMoves) {
        std::istringstream ss(uciMoves);
        std::string move;
        while (ss >> move)
            push(move);
    }

    void pushSanMoves(const std::string& sanMoves) {
        pushSanMoves(sanMoves, NOTATION_SAN);
    }

    void pushSanMoves(const std::string& sanMoves, Notation notation) {
        std::istringstream ss(sanMoves);
        std::string move;
        while (ss >> move)
            pushSan(move, notation);
    }

    std::string pocket(bool color) {
        const Color c = Color(!color);
        std::string pocket;
        for (PieceType pt = KING; pt >= PAWN; --pt) {
            for (int i = 0; i < pos.count_in_hand(c, pt); ++i) {
                pocket += std::string(1, pos.piece_to_char()[make_piece(BLACK, pt)]);
            }
        }
        return pocket;
    }

    std::string toString() {
        std::string str;
        for (Rank r = pos.max_rank(); r >= RANK_1; --r) {
            for (File f = FILE_A; f <= pos.max_file(); ++f) {
                if (f != FILE_A)
                    str += " ";
                Piece p = pos.piece_on(make_square(f, r));
                str += p == NO_PIECE ? "." : std::string(1, pos.piece_to_char()[p]);
            }
            if (r != RANK_1)
                str += "\n";
        }
        return str;
    }

    std::string toVerboseString() {
        std::stringstream ss;
        ss << pos;
        return ss.str();
    }

    std::string variant() {
        for (const auto& v : variants)
            if (v.second == this->v)
                return v.first;
        return "unknown";
    }

    std::string variationSan(const std::string& uciMoves) {
        return variationSan(uciMoves, NOTATION_SAN, true);
    }

    std::string variationSan(const std::string& uciMoves, Notation notation) {
        return variationSan(uciMoves, notation, true);
    }

    std::string variationSan(const std::string& uciMoves, Notation notation, bool moveNumbers) {
        StateListPtr tempStates(new std::deque<StateInfo>());
        std::istringstream ss(uciMoves);
        std::string uciMove, variationSan;
        bool first = true;

        while (ss >> uciMove) {
            const Move move = UCI::to_move(this->pos, uciMove);
            if (move == MOVE_NONE)
                return "";

            if (first) {
                first = false;
                if (moveNumbers) {
                    variationSan = std::to_string(fullmoveNumber());
                    if (pos.side_to_move() == WHITE)
                        variationSan += ". ";
                    else
                        variationSan += "...";
                }
                variationSan += SAN::move_to_san(this->pos, move, notation);
            }
            else {
                if (moveNumbers && pos.side_to_move() == WHITE) {
                    variationSan += " ";
                    variationSan += std::to_string(fullmoveNumber());
                    variationSan += ".";
                }
                variationSan += " ";
                variationSan += SAN::move_to_san(this->pos, move, notation);
            }

            tempStates->emplace_back();
            pos.do_move(move, tempStates->back());
        }

        // Undo all moves
        while (!tempStates->empty()) {
            pos.undo_move(UCI::to_move(pos, uciMove));
            tempStates->pop_back();
        }

        return variationSan;
    }

    static void init_lua(lua_State* state) {
        L = state;
    }

private:
    const Variant* v;
    StateListPtr states;
    Position pos;
    Thread* thread;
    std::vector<Move> moves;
    bool chess960;

    void init(const std::string& uciVariant, const std::string& fen, bool is960) {
        DEBUG_LOGF("init() called with variant: %s, fen: %s, is960: %d", 
                   uciVariant.c_str(), fen.c_str(), is960);

        if (!L) {
            DEBUG_LOG("Lua state is null");
            throw std::runtime_error("Lua state not initialized");
        }

        try {
            if (!sfInitialized) {
                DEBUG_LOG("Initializing Stockfish");
                initialize_stockfish();
                sfInitialized = true;
                DEBUG_LOG("Stockfish initialized");
            }

            DEBUG_LOG("Looking up variant");
            auto variantIter = variants.find(uciVariant.empty() || uciVariant == "standard" || 
                                           uciVariant == "Standard" ? "chess" : uciVariant);
            if (variantIter == variants.end()) {
                DEBUG_LOGF("Invalid variant: %s", uciVariant.c_str());
                throw std::runtime_error("Invalid variant: " + uciVariant);
            }
            
            v = variantIter->second;
            if (!v) {
                DEBUG_LOG("Null variant pointer");
                throw std::runtime_error("Null variant pointer");
            }

            DEBUG_LOG("Initializing UCI variant");
            UCI::init_variant(v);
            
            std::string actualFen = fen.empty() ? v->startFen : fen;
            
            // Skip FEN validation for spartan chess
            if (v->variantTemplate != "spartan") {
                if (FEN::validate_fen(actualFen, v, is960) != 1) {
                    throw std::runtime_error("Invalid FEN: " + actualFen);
                }
            }
            
            this->chess960 = is960;
            pos.set(v, actualFen, is960, &states->back(), thread);
            
            if (!pos.pos_is_ok()) {
                throw std::runtime_error("Invalid position after initialization");
            }
            
            DEBUG_LOG("Board initialization complete");
        }
        catch (const std::exception& e) {
            DEBUG_LOGF("Exception in init: %s", e.what());
            throw;
        }
        catch (...) {
            DEBUG_LOG("Unknown exception in init");
            throw;
        }
    }

    void cleanup() {
        DEBUG_LOG("Cleaning up board");
        states.reset();
        v = nullptr;
        thread = nullptr;
        moves.clear();
    }
};

// Define LuaBoard static members
bool LuaBoard::sfInitialized = false;
lua_State* LuaBoard::L = nullptr;

// Now define setOption after LuaBoard is fully defined
namespace ffish {
    void setOption(const std::string& name, const std::string& value) {
        Stockfish::Options[name] = value;
        LuaBoard::sfInitialized = false;
    }
}

// Then define Game class
class Game {
private:
    static lua_State* L;
    std::unique_ptr<LuaBoard> board;
    std::map<std::string, std::string> headerMap;
    std::string variant = "chess";
    std::string fen = "";
    bool is960 = false;

public:
    // Basic constructor
    Game() {}
    
    // Add constructor that takes both L and pgn
    Game(const std::string& pgn) {
        if (!L) {
            throw std::runtime_error("Lua state not initialized");
        }
        size_t lineStart = 0;
        bool headersParsed = false;
        
        while (lineStart < pgn.size()) {
            size_t lineEnd = pgn.find('\n', lineStart);
            if (lineEnd == std::string::npos)
                lineEnd = pgn.size();

            // Skip empty lines
            if (lineStart == lineEnd) {
                lineStart = lineEnd + 1;
                continue;
            }

            // Parse header
            if (pgn[lineStart] == '[') {
                size_t headerKeyStart = lineStart + 1;
                size_t headerKeyEnd = pgn.find(' ', lineStart);
                size_t headerItemStart = pgn.find('"', headerKeyEnd) + 1;
                size_t headerItemEnd = pgn.find('"', headerItemStart);

                headerMap[pgn.substr(headerKeyStart, headerKeyEnd-headerKeyStart)] = 
                    pgn.substr(headerItemStart, headerItemEnd-headerItemStart);
            }
            else {
                if (!headersParsed) {
                    headersParsed = true;
                    auto it = headerMap.find("Variant");
                    if (it != headerMap.end()) {
                        is960 = it->second.find("960", it->second.size() - 3) != std::string::npos;
                        variant = is960 ? 
                            it->second.substr(0, it->second.size() - 4) : it->second;
                        std::transform(variant.begin(), variant.end(), 
                            variant.begin(), ::tolower);
                    }

                    it = headerMap.find("FEN");
                    if (it != headerMap.end())
                        fen = it->second;

                    board = std::make_unique<LuaBoard>(variant, fen, is960);
                }

                // Parse moves (rest of the existing move parsing code)
                // ...
            }
            lineStart = lineEnd + 1;
        }
    }

    // Move operations
    Game(Game&& other) noexcept = default;
    Game& operator=(Game&& other) noexcept = default;

    // Delete copy operations
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    ~Game() {
        DEBUG_LOG("Game destructor called");
        if (board) {
            board.reset();
        }
    }

    std::string headerKeys() {
        std::string keys;
        for (const auto& pair : headerMap) {
            keys += pair.first;
            keys += " ";
        }
        if (!keys.empty())
            keys.pop_back();
        return keys;
    }

    std::string headers(const std::string& key) {
        auto it = headerMap.find(key);
        if (it == headerMap.end())
            return "";
        return it->second;
    }

    std::string mainlineMoves() {
        if (!board)
            return "";
        return board->getMoveStack();
    }

    bool isEnd() {
        return board ? board->isGameOver() : false;
    }

    std::string result() {
        return board ? board->result() : "*";
    }

    // Make readGamePGN a friend so it can access private members
    friend Game* readGamePGN(lua_State* L, const std::string& pgn);

    // Add static initialization function
    static void init_lua(lua_State* state) {
        L = state;
    }
};

// Define Game static member after its class definition
lua_State* Game::L = nullptr;

// PGN parsing helper functions
bool skipComment(const std::string& pgn, size_t& curIdx, size_t& lineEnd) {
    curIdx = pgn.find('}', curIdx);
    if (curIdx == std::string::npos) {
        return false;
    }
    if (curIdx > lineEnd)
        lineEnd = pgn.find('\n', curIdx);
    return true;
}

Game* readGamePGN(lua_State* L, const std::string& pgn) {
    Game* game = new Game(pgn);
    size_t lineStart = 0;
    bool headersParsed = false;

    while(true) {
        size_t lineEnd = pgn.find('\n', lineStart);
        if (lineEnd == std::string::npos)
            lineEnd = pgn.size();

        if (!headersParsed && pgn[lineStart] == '[') {
            size_t headerKeyStart = lineStart + 1;
            size_t headerKeyEnd = pgn.find(' ', lineStart);
            size_t headerItemStart = pgn.find('"', headerKeyEnd) + 1;
            size_t headerItemEnd = pgn.find('"', headerItemStart);

            game->headerMap[pgn.substr(headerKeyStart, headerKeyEnd-headerKeyStart)] = 
                pgn.substr(headerItemStart, headerItemEnd-headerItemStart);
        }
        else {
            if (!headersParsed) {
                headersParsed = true;
                auto it = game->headerMap.find("Variant");
                if (it != game->headerMap.end()) {
                    game->is960 = it->second.find("960", it->second.size() - 3) != std::string::npos;
                    game->variant = game->is960 ? 
                        it->second.substr(0, it->second.size() - 4) : it->second;
                    std::transform(game->variant.begin(), game->variant.end(), 
                        game->variant.begin(), ::tolower);
                }

                it = game->headerMap.find("FEN");
                if (it != game->headerMap.end())
                    game->fen = it->second;

                game->board = std::make_unique<LuaBoard>(game->variant, game->fen, game->is960);
            }

            // Parse moves
            size_t curIdx = lineStart;
            while (curIdx <= lineEnd) {
                if (pgn[curIdx] == '*')
                    return game;

                if (pgn[curIdx] == '{') {
                    if (!skipComment(pgn, curIdx, lineEnd))
                        return game;
                    ++curIdx;
                }

                size_t openedRAV = 0;
                if (pgn[curIdx] == '(') {
                    openedRAV = 1;
                    ++curIdx;
                }
                while (openedRAV != 0) {
                    switch (pgn[curIdx]) {
                        case '(':
                            ++openedRAV;
                            break;
                        case ')':
                            --openedRAV;
                            break;
                        case '{':
                            if (!skipComment(pgn, curIdx, lineEnd))
                                return game;
                        default: ;
                    }
                    ++curIdx;
                    if (curIdx > lineEnd)
                        lineEnd = pgn.find('\n', curIdx);
                }

                if (pgn[curIdx] == '$') {
                    curIdx = pgn.find(' ', curIdx);
                }

                if (pgn[curIdx] >= '0' && pgn[curIdx] <= '9') {
                    curIdx = pgn.find('.', curIdx);
                    if (curIdx == std::string::npos)
                        break;
                    ++curIdx;
                    while (curIdx < pgn.size() && pgn[curIdx] == ' ')
                        ++curIdx;
                    while (curIdx < pgn.size() && pgn[curIdx] == '.')
                        ++curIdx;
                }

                size_t sanMoveEnd = std::min(pgn.find(' ', curIdx), lineEnd);
                if (sanMoveEnd > curIdx) {
                    std::string sanMove = pgn.substr(curIdx, sanMoveEnd-curIdx);
                    size_t annotationChar1 = sanMove.find('?');
                    size_t annotationChar2 = sanMove.find('!');
                    if (annotationChar1 != std::string::npos || annotationChar2 != std::string::npos)
                        sanMove = sanMove.substr(0, std::min(annotationChar1, annotationChar2));
                    game->board->pushSan(sanMove);
                }
                curIdx = sanMoveEnd + 1;
            }
        }
        lineStart = lineEnd + 1;
        if (lineStart >= pgn.size())
            return game;
    }
    return game;
}

// Add class factory functions before luaopen_fairystockfish
static LuaBoard* createBoard() {
    DEBUG_LOG("createBoard() called");
    try {
        DEBUG_LOG("About to create new LuaBoard");
        std::unique_ptr<LuaBoard> board(new LuaBoard());  // Use unique_ptr for exception safety
        DEBUG_LOG("Board created successfully");
        return board.release();  // Release ownership to caller
    } catch (const std::exception& e) {
        DEBUG_LOGF("Exception in createBoard: %s", e.what());
        return nullptr;
    } catch (...) {
        DEBUG_LOG("Unknown exception in createBoard");
        return nullptr;
    }
}

static LuaBoard* createBoardVariant(const std::string& variant) {
    DEBUG_LOGF("createBoardVariant() called with variant: %s", variant.c_str());
    try {
        DEBUG_LOG("About to create new LuaBoard with variant");
        LuaBoard* board = new LuaBoard(variant);
        if (!board) {
            DEBUG_LOG("Board creation returned nullptr");
            return nullptr;
        }
        DEBUG_LOG("Board created successfully");
        return board;
    } catch (const std::exception& e) {
        DEBUG_LOGF("Exception in createBoardVariant: %s", e.what());
        return nullptr;
    } catch (...) {
        DEBUG_LOG("Unknown exception in createBoardVariant");
        return nullptr;
    }
}

static LuaBoard* createBoardVariantFen(const std::string& variant, const std::string& fen) {
    std::cerr << "DEBUG: Creating new board with variant: " << variant << " and fen: " << fen << std::endl;
    try {
        LuaBoard* board = new LuaBoard(variant, fen);
        std::cerr << "DEBUG: Board created successfully" << std::endl;
        return board;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to create board: " << e.what() << std::endl;
        return nullptr;
    }
}

static LuaBoard* createBoardVariantFen960(const std::string& variant, const std::string& fen, bool is960) {
    std::cerr << "DEBUG: Creating new board with variant: " << variant << ", fen: " << fen << " and is960: " << is960 << std::endl;
    try {
        LuaBoard* board = new LuaBoard(variant, fen, is960);
        std::cerr << "DEBUG: Board created successfully" << std::endl;
        return board;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to create board: " << e.what() << std::endl;
        return nullptr;
    }
}

// Add destructor function with safety check
static void destroyBoard(LuaBoard* board) {
    if (board) {
        std::cerr << "DEBUG: Destroying board" << std::endl;
        delete board;
    }
}

// Add these helper functions at the top of the file after the includes
static Game* createGame() {
    DEBUG_LOG("Creating new empty Game");
    try {
        return new Game();
    } catch (const std::exception& e) {
        DEBUG_LOGF("Exception creating empty Game: %s", e.what());
        return nullptr;
    }
}

static Game* createGameFromPGN(const std::string& pgn) {
    DEBUG_LOGF("Creating Game from PGN: %s", pgn.c_str());
    try {
        return new Game(pgn);
    } catch (const std::exception& e) {
        DEBUG_LOGF("Exception creating Game from PGN: %s", e.what());
        return nullptr;
    }
}

// Lua module registration
extern "C" int luaopen_fairystockfish(lua_State* L) {
    DEBUG_LOG("Module initialization start");
    
    try {
        DEBUG_LOG("Setting up Lua state");
        LuaBoard::init_lua(L);
        Game::init_lua(L);

        DEBUG_LOG("Starting Stockfish initialization");
        initialize_stockfish();
        
        DEBUG_LOG("Creating module table");
        lua_newtable(L);
        
        DEBUG_LOG("About to register classes and functions");
        
        // Register everything into the module table
        luabridge::getGlobalNamespace(L)
            .beginNamespace("ffish")
                // Register free functions
                .addFunction("info", &ffish::info)
                .addFunction("variants", &ffish::variants)
                .addFunction("loadVariantConfig", &ffish::loadVariantConfig)
                .addFunction("validateFen", (int(*)(const std::string&))&ffish::validateFen)
                .addFunction("startingFen", &ffish::startingFen)
                .addFunction("capturesToHand", &ffish::capturesToHand)
                .addFunction("twoBoards", &ffish::twoBoards)
                .addFunction("readGamePGN", [L](const std::string& pgn) {
                    return readGamePGN(L, pgn);
                })
                .addFunction("setOption", [](const std::string& name, const std::string& value) {
                    Stockfish::Options[name] = value;
                    LuaBoard::sfInitialized = false;
                })

                // Create Notation namespace
                .beginNamespace("Notation")
                    .addFunction("DEFAULT", []() -> int { return static_cast<int>(NOTATION_DEFAULT); })
                    .addFunction("SAN", []() -> int { return static_cast<int>(NOTATION_SAN); })
                    .addFunction("LAN", []() -> int { return static_cast<int>(NOTATION_LAN); })
                    .addFunction("SHOGI_HOSKING", []() -> int { return static_cast<int>(NOTATION_SHOGI_HOSKING); })
                    .addFunction("SHOGI_HODGES", []() -> int { return static_cast<int>(NOTATION_SHOGI_HODGES); })
                    .addFunction("SHOGI_HODGES_NUMBER", []() -> int { return static_cast<int>(NOTATION_SHOGI_HODGES_NUMBER); })
                    .addFunction("JANGGI", []() -> int { return static_cast<int>(NOTATION_JANGGI); })
                    .addFunction("XIANGQI_WXF", []() -> int { return static_cast<int>(NOTATION_XIANGQI_WXF); })
                .endNamespace()

                // Register Board class
                .beginClass<LuaBoard>("Board")
                    // Use static functions instead of constructors
                    .addStaticFunction("new", &createBoard)
                    .addStaticFunction("newVariant", &createBoardVariant)
                    .addStaticFunction("newVariantFen", &createBoardVariantFen)
                    .addStaticFunction("newVariantFen960", [](const std::string& variant, const std::string& fen, bool is960) {
                        DEBUG_LOGF("Creating board with variant: %s, fen: %s, is960: %d", 
                                   variant.c_str(), fen.c_str(), is960);
                        return new LuaBoard(variant, fen, is960);
                    })
                    // Add delete method instead
                    .addFunction("delete", &destroyBoard)
                    // Add other methods
                    .addFunction("legalMoves", &LuaBoard::legalMoves)
                    .addFunction("push", &LuaBoard::push)
                    .addFunction("pop", &LuaBoard::pop)
                    .addFunction("reset", &LuaBoard::reset)
                    .addFunction("fen", (std::string(LuaBoard::*)() const)&LuaBoard::fen)
                    .addFunction("isCheck", &LuaBoard::isCheck)
                    .addFunction("isGameOver", &LuaBoard::isGameOver)
                    .addFunction("legalMovesSan", &LuaBoard::legalMovesSan)
                    .addFunction("pushSan", (bool(LuaBoard::*)(const std::string&))&LuaBoard::pushSan)
                    .addFunction("turn", &LuaBoard::turn)
                    .addFunction("fullmoveNumber", &LuaBoard::fullmoveNumber)
                    .addFunction("result", (std::string(LuaBoard::*)() const)&LuaBoard::result)
                    .addFunction("is960", &LuaBoard::is960)
                    .addFunction("checkedPieces", &LuaBoard::checkedPieces)
                    .addFunction("setFen", &LuaBoard::set_fen)
                    .addFunction("pushMoves", &LuaBoard::pushMoves)
                    .addFunction("isCapture", &LuaBoard::isCapture)
                    .addFunction("gamePly", &LuaBoard::gamePly)
                    .addFunction("halfmoveClock", &LuaBoard::halfmoveClock)
                    .addFunction("hasInsufficientMaterial", &LuaBoard::hasInsufficientMaterial)
                    .addFunction("isInsufficientMaterial", &LuaBoard::is_insufficient_material)
                    .addFunction("isBikjang", &LuaBoard::isBikjang)
                    .addFunction("pushSanMoves", (void(LuaBoard::*)(const std::string&))&LuaBoard::pushSanMoves)
                    .addFunction("pushSanMovesNotation", [](LuaBoard* board, const std::string& moves, int notation) {
                        board->pushSanMoves(moves, static_cast<Notation>(notation));
                    })
                    .addFunction("moveStack", &LuaBoard::getMoveStack)
                    .addFunction("numberLegalMoves", &LuaBoard::numberLegalMoves)
                    .addFunction("pocket", &LuaBoard::pocket)
                    .addFunction("sanMove", (std::string(LuaBoard::*)(const std::string&))&LuaBoard::sanMove)
                    .addFunction("sanMoveNotation", [](LuaBoard* board, const std::string& move, int notation) {
                        return board->sanMove(move, static_cast<Notation>(notation));
                    })
                .endClass()

                // Register Game class
                .beginClass<Game>("Game")
                    .addStaticFunction("new", &createGame)
                    .addStaticFunction("newFromPGN", &createGameFromPGN)
                    .addFunction("headerKeys", &Game::headerKeys)
                    .addFunction("headers", &Game::headers)
                    .addFunction("mainlineMoves", &Game::mainlineMoves)
                    .addFunction("isEnd", &Game::isEnd)
                    .addFunction("result", &Game::result)
                    .addFunction("delete", [](Game* game) { delete game; })
                .endClass()
            .endNamespace();

        DEBUG_LOG("Registration complete");
        
        // Get our registered namespace
        lua_getglobal(L, "ffish");
        if (!lua_istable(L, -1)) {
            DEBUG_LOG("Failed to find ffish table after registration");
            return luaL_error(L, "Failed to find ffish table after registration");
        }

        DEBUG_LOG("Module initialization complete");
        return 1;
    }
    catch (const std::exception& e) {
        DEBUG_LOGF("Exception in module initialization: %s", e.what());
        luaL_error(L, "Failed to initialize module: %s", e.what());
        return 0;
    }
    catch (...) {
        DEBUG_LOG("Unknown exception in module initialization");
        luaL_error(L, "Unknown error initializing module");
        return 0;
    }
}


// Add to your error handling
static int pushLuaError(lua_State* L, const char* msg) {
    lua_pushnil(L);
    lua_pushstring(L, msg);
    return 2;
}

// Add at the top of the file after includes
namespace {
    // Static values for Notation enum
    static const int NOTATION_DEFAULT_VAL = static_cast<int>(NOTATION_DEFAULT);
    static const int NOTATION_SAN_VAL = static_cast<int>(NOTATION_SAN);
    static const int NOTATION_LAN_VAL = static_cast<int>(NOTATION_LAN);
    static const int NOTATION_SHOGI_HOSKING_VAL = static_cast<int>(NOTATION_SHOGI_HOSKING);
    static const int NOTATION_SHOGI_HODGES_VAL = static_cast<int>(NOTATION_SHOGI_HODGES);
    static const int NOTATION_SHOGI_HODGES_NUMBER_VAL = static_cast<int>(NOTATION_SHOGI_HODGES_NUMBER);
    static const int NOTATION_JANGGI_VAL = static_cast<int>(NOTATION_JANGGI);
    static const int NOTATION_XIANGQI_WXF_VAL = static_cast<int>(NOTATION_XIANGQI_WXF);
}
 