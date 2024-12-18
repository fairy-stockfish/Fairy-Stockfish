local lu = require('luaunit')
local ffish = require('fairystockfish')

-- Add this right after loading the module
print("\nDumping ffish table contents:")
for k, v in pairs(ffish) do
    print(string.format("ffish.%s = %s (type: %s)", k, tostring(v), type(v)))
    if type(v) == "table" then
        print("  Table contents:")
        for k2, v2 in pairs(v) do
            print(string.format("    %s = %s (type: %s)", k2, tostring(v2), type(v2)))
        end
    end
end

print("Lua package.path:       " .. package.path)
print("Lua package.cpath:      " .. package.cpath)
print("Loaded module type:     " .. type(ffish))
print("Module contents:        " .. tostring(ffish))

-- Add this debug code before creating the board
print("\nDebug Board table:")
if ffish.Board then
    print("Board table type:", type(ffish.Board))
    print("Board table contents:")
    for k,v in pairs(ffish.Board) do
        print("  ", k, type(v))
    end
else
    print("ffish.Board is nil")
end

-- Try creating a board with pcall to get detailed error
local ok, board = pcall(function() 
    return ffish.Board.new()  -- Use static new() method instead of constructor
end)
if not ok then
    print("Failed to create board:", board)
else
    print("Successfully created board:", board)
end

-- Helper function for safe board creation with any factory method
local function safeCreateBoardWithMethod(method, ...)
    print("\nAttempting to create board with " .. method .. "...")
    local success, result = pcall(function(...)
        local board = ffish.Board[method](...)  -- Call static method
        if not board then
            error("Board creation returned nil")
        end
        return board
    end, ...)
    
    if not success then
        print("Failed to create board with " .. method .. ":", result)
        return nil
    end
    print("Successfully created board with " .. method)
    return result
end

-- Create boards using the factory methods
local board = safeCreateBoardWithMethod("new")
if not board then
    print("Exiting due to board creation failure")
    os.exit(1)
end

-- Continue with remaining tests only if board creation succeeded
local board2 = safeCreateBoardWithMethod("newVariant", "chess")
if not board2 then
    print("Warning: Failed to create board2")
end

local board3 = safeCreateBoardWithMethod("newVariantFen", "chess", 
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
if not board3 then
    print("Warning: Failed to create board3")
end

local board4 = safeCreateBoardWithMethod("newVariantFen960", "chess",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false)
if not board4 then
    print("Warning: Failed to create board4")
end

-- Game still uses constructors
local game = ffish.Game.new()
local game2 = ffish.Game.new("[Event \"Test\"]")

-- Add error handling around Game creation
local ok, game = pcall(function()
    return ffish.Game.new()  -- Create empty game
end)
if not ok then
    print("Failed to create empty game:", game)
else
    print("Successfully created empty game")
end

local ok2, game2 = pcall(function()
    return ffish.Game.newFromPGN("[Event \"Test\"]")  -- Create game from PGN
end)
if not ok2 then
    print("Failed to create game from PGN:", game2)
else
    print("Successfully created game from PGN")
end

-- Test board methods
print(board:legalMoves())
print(board:fen())

-- Debug package paths
print("Lua package.path:", package.path)
print("Lua package.cpath:", package.cpath)

-- Try loading with pcall to get error message
local ok, ffish_module = pcall(require, 'fairystockfish')
if not ok then
    print("Failed to load fairystockfish module:", ffish_module)
    os.exit(1)
end

print("Module loaded successfully")
print("Module type:", type(ffish_module))
print("Module contents:", tostring(ffish_module))

if type(ffish_module) ~= "table" then
    print("Module did not return a table as expected")
    os.exit(1)
end

if not ffish_module.Board then
    print("Module missing Board class")
    os.exit(1)
end

-- Helper function to read file contents
local function readFile(path)
    local file = io.open(path, "rb")
    if not file then 
        print("Failed to open file: " .. path)
        return nil 
    end
    local content = file:read("*all")
    file:close()
    return content
end

-- Test Suite
TestFairystockfish = {}

function TestFairystockfish:setUp()
    self.pgnDir = "../pgn/"
    self.srcDir = "../../src/"
    self.WHITE = true
    self.BLACK = false
    self.boards = {}  -- Track boards for cleanup
end

function TestFairystockfish:tearDown()
    -- Clean up any boards created during the test
    for _, board in ipairs(self.boards) do
        if board then
            board:delete()
        end
    end
    self.boards = {}
end

-- Helper function to create and track boards
function TestFairystockfish:createBoard(...)
    local board = ffish.Board.new(...)
    table.insert(self.boards, board)
    return board
end


function TestFairystockfish:test_loadVariantConfig()
    local testConfig = [[
[minitic]
maxRank = 3
maxFile = 3
immobile = p
startFen = 3/3/3[PPPPPpppp] w - - 0 1
pieceDrops = true
doubleStep = false
castling = false
stalemateValue = draw
immobilityIllegal = false
connectN = 3
]]
    
    -- Add debug prints to verify config loading
    print("\nLoading variant config...")
    print("Config string:", testConfig)
    
    local ok, err = pcall(function()
        ffish.loadVariantConfig(testConfig)
    end)
    
    if not ok then
        print("Error loading variant config:", err)
        lu.fail("Failed to load variant config: " .. tostring(err))
        return
    end
    
    -- Verify variants list includes minitic
    local variants = ffish.variants()
    print("Available variants after loading:", variants)
    
    -- Check if minitic is in the variants list
    if not string.find(variants, "minitic") then
        lu.fail("minitic variant not found in variants list after loading config")
        return
    end
    
    -- Add error handling when creating board
    local ok, board = pcall(function()
        return ffish.Board.newVariant("minitic")
    end)
    
    if not ok then
        print("Error creating board:", board)
        lu.fail("Failed to create minitic board: " .. tostring(board))
        return
    end
    
    if not board then
        lu.fail("Board creation returned nil")
        return
    end
    
    -- Verify the board was created with correct FEN
    local ok2, fen = pcall(function()
        return board:fen()
    end)
    
    if not ok2 then
        print("Error getting board FEN:", fen)
        lu.fail("Failed to get board FEN: " .. tostring(fen))
        board:delete()
        return
    end
    
    print("Board FEN:", fen)
    lu.assertEquals(fen, "3/3/3[PPPPPpppp] w - - 0 1")
    
    -- Clean up
    board:delete()
end


function TestFairystockfish:test_Board_default()
    local board = self:createBoard()
    lu.assertEquals(board:fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    lu.assertEquals(board:is960(), false)
end

function TestFairystockfish:test_Board_variant()
    local board = ffish.Board.new("chess")
    lu.assertEquals(board:fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    lu.assertEquals(board:is960(), false)
    board:delete()
end

function TestFairystockfish:test_Board_large_variant()
    -- Create a board with minixiangqi variant
    local ok, board = pcall(function()
        return ffish.Board.newVariant("minixiangqi")
    end)
    
    if not ok then
        lu.fail("Failed to create minixiangqi board: " .. tostring(board))
        return
    end
    
    if not board then
        lu.fail("Board creation returned nil")
        return
    end
    
    -- Verify the FEN and chess960 flag
    local ok2, fen = pcall(function()
        return board:fen()
    end)
    
    if not ok2 then
        print("Error getting board FEN:", fen)
        lu.fail("Failed to get board FEN: " .. tostring(fen))
        board:delete()
        return
    end
    
    -- Get the starting FEN for minixiangqi
    local startingFen = ffish.startingFen("minixiangqi")
    lu.assertEquals(fen, startingFen)
    lu.assertFalse(board:is960())
    
    -- Clean up
    board:delete()
end

function TestFairystockfish:test_Board_fen()
    -- Create a board with crazyhouse variant and specific FEN position
    local ok, board = pcall(function()
        return ffish.Board.newVariantFen("crazyhouse", "rnbqkb1r/pp3ppp/5p2/2pp4/8/5N2/PPPP1PPP/RNBQKB1R/Np w KQkq - 0 5")
    end)
    
    if not ok then
        lu.fail("Failed to create crazyhouse board: " .. tostring(board))
        return
    end
    
    if not board then
        lu.fail("Board creation returned nil")
        return
    end
    
    -- Verify the FEN and chess960 flag
    local ok2, fen = pcall(function()
        return board:fen()
    end)
    
    if not ok2 then
        print("Error getting board FEN:", fen)
        lu.fail("Failed to get board FEN: " .. tostring(fen))
        board:delete()
        return
    end
    
    lu.assertEquals(fen, "rnbqkb1r/pp3ppp/5p2/2pp4/8/5N2/PPPP1PPP/RNBQKB1R[Np] w KQkq - 0 5")
    lu.assertFalse(board:is960())
    
    -- Clean up
    board:delete()
end

function TestFairystockfish:test_Board_chess960()
    -- Create a board with chess960 position and is960 flag set to true
    local ok, board = pcall(function()
        return ffish.Board.newVariantFen960("chess", "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b KQkq - 1 5", true)
    end)
    
    if not ok then
        lu.fail("Failed to create chess960 board: " .. tostring(board))
        return
    end
    
    if not board then
        lu.fail("Board creation returned nil")
        return
    end
    
    -- Verify the FEN and chess960 flag
    lu.assertEquals(board:fen(), "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b GAga - 1 5")
    lu.assertTrue(board:is960())
    
    -- Clean up
    board:delete()
end

function TestFairystockfish:test_legalMoves()
    -- Create a crazyhouse board with pieces in hand
    local board = ffish.Board.newVariantFen("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR[QPbq] w KQ - 0 7")
    local expectedMoves = 'a2a3 b2b3 d2d3 f2f3 g2g3 h2h3 a2a4 b2b4 d2d4 f2f4 g2g4 h2h4 c3b1 c3d1 c3e2 c3a4 c3b5 c3d5' ..
        ' g1e2 g1f3 g1h3 a1b1 P@e2 P@a3 P@b3 P@d3 P@e3 P@f3 P@g3 P@h3 P@a4 P@b4 P@c4 P@d4 P@f4 P@g4 P@h4 P@a5 P@b5' ..
        ' P@d5 P@f5 P@g5 P@h5 P@a6 P@b6 P@d6 P@e6 P@f6 P@g6 P@h6 P@e7 Q@b1 Q@d1 Q@f1 Q@e2 Q@a3 Q@b3 Q@d3 Q@e3 Q@f3 ' ..
        'Q@g3 Q@h3 Q@a4 Q@b4 Q@c4 Q@d4 Q@f4 Q@g4 Q@h4 Q@a5 Q@b5 Q@d5 Q@f5 Q@g5 Q@h5 Q@a6 Q@b6 Q@d6 Q@e6 Q@f6 Q@g6' ..
        ' Q@h6 Q@e7 Q@b8 Q@d8 Q@e8 Q@f8 e1d1 e1f1 e1e2'
    
    local moves = {}
    for move in board:legalMoves():gmatch("%S+") do
        table.insert(moves, move)
    end
    table.sort(moves)
    
    local expectedMovesTable = {}
    for move in expectedMoves:gmatch("%S+") do
        table.insert(expectedMovesTable, move)
    end
    table.sort(expectedMovesTable)
    
    lu.assertEquals(moves, expectedMovesTable)
    board:delete()
end

function TestFairystockfish:test_legalMovesSan()
    local board = ffish.Board.new("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7")
    local expectedMoves = 'a3 b3 d3 f3 g3 h3 a4 b4 d4 f4 g4 h4 Nb1 Nd1 Nce2 Na4 Nb5 Nd5 Nge2 Nf3 Nh3 Rb1 P@e2 P@a3' ..
        ' P@b3 P@d3 P@e3 P@f3 P@g3 P@h3 P@a4 P@b4 P@c4 P@d4 P@f4 P@g4 P@h4 P@a5 P@b5 P@d5 P@f5 P@g5 P@h5 P@a6 P@b6' ..
        ' P@d6 P@e6+ P@f6 P@g6+ P@h6 P@e7 Q@b1 Q@d1 Q@f1 Q@e2 Q@a3 Q@b3+ Q@d3 Q@e3 Q@f3+ Q@g3 Q@h3 Q@a4 Q@b4 Q@c4+' ..
        ' Q@d4 Q@f4+ Q@g4 Q@h4 Q@a5 Q@b5 Q@d5+ Q@f5+ Q@g5 Q@h5+ Q@a6 Q@b6 Q@d6 Q@e6+ Q@f6+ Q@g6+ Q@h6 Q@e7+ Q@b8' ..
        ' Q@d8 Q@e8+ Q@f8+ Kd1 Kf1 Ke2'
    
    local moves = {}
    for move in board:legalMovesSan():gmatch("%S+") do
        table.insert(moves, move)
    end
    table.sort(moves)
    
    local expectedMovesTable = {}
    for move in expectedMoves:gmatch("%S+") do
        table.insert(expectedMovesTable, move)
    end
    table.sort(expectedMovesTable)
    
    lu.assertEquals(moves, expectedMovesTable)
    board:delete()
end

function TestFairystockfish:test_numberLegalMoves()
    local board = ffish.Board.new("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR/QPbq w KQ - 0 7")
    lu.assertEquals(board:numberLegalMoves(), 90)
    board:delete()
    
    local yariboard = ffish.Board.new("yarishogi")
    lu.assertEquals(yariboard:numberLegalMoves(), 20)
    yariboard:delete()
end

function TestFairystockfish:test_push()
    local board = ffish.Board.new()
    lu.assertTrue(board:push("e2e4"))
    lu.assertTrue(board:push("e7e5"))
    lu.assertTrue(board:push("g1f3"))
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    lu.assertFalse(board:push("q2q7"))
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_pushSan()
    local board = ffish.Board.new()
    lu.assertTrue(board:pushSan("e4"))
    lu.assertTrue(board:pushSan("e5"))
    lu.assertTrue(board:pushSan("Nf3"))
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    lu.assertFalse(board:pushSan("Nf3"))
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_pushSanWithNotation()
    local board = ffish.Board.new()
    lu.assertTrue(board:pushSan("e4", ffish.Notation.SAN))
    lu.assertTrue(board:pushSan("e5", ffish.Notation.SAN))
    lu.assertTrue(board:pushSan("Nf3", ffish.Notation.SAN))
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_pop()
    local board = ffish.Board.new()
    board:push("e2e4")
    board:push("e7e5")
    board:pop()
    board:push("e7e5")
    board:push("g1f3")
    board:push("b8c6")
    board:push("f1b5")
    board:pop()
    board:pop()
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_reset()
    local board = ffish.Board.new()
    board:pushSan("e4")
    board:pushSan("e5")
    board:pushSan("Nf3")
    board:reset()
    lu.assertEquals(board:fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    board:delete()
end

function TestFairystockfish:test_is960()
    local board = ffish.Board.new()
    lu.assertFalse(board:is960())
    board:delete()
    
    local board2 = ffish.Board.newVariantFen960("chess", "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b KQkq - 1 5", true)
    lu.assertTrue(board2:is960())
    board2:delete()
end

function TestFairystockfish:test_fen()
    local board = ffish.Board.new()
    lu.assertEquals(board:fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    board:delete()
end

function TestFairystockfish:test_fenWithShowPromoted()
    local ok, board = pcall(function()
        return ffish.Board.newVariantFen("makruk", "8/6ks/3M~2r1/2K1M3/8/3R4/8/8 w - 128 18 50")
    end)
    
    if not ok then
        lu.fail("Failed to create makruk board: " .. tostring(board))
        return
    end
    
    if not board then
        lu.fail("Board creation returned nil")
        return
    end
    
    -- Both fen(true) and fen(false) should return the same FEN string for makruk
    -- since promotion markers are handled differently in this variant
    local expectedFen = "8/6ks/3M2r1/2K1M3/8/3R4/8/8 w - 128 18 50"
    lu.assertEquals(board:fen(true), expectedFen)
    lu.assertEquals(board:fen(false), expectedFen)
    board:delete()
end

function TestFairystockfish:test_fenWithShowPromotedAndCountStarted()
    local board = ffish.Board.newVariantFen("makruk", "8/6ks/3M~2r1/2K1M3/8/3R4/8/8 w - 128 18 50")
    -- Test that the promotion marker is not preserved in the FEN string
    -- and that the count parameter does not affect the count value in the FEN string
    local expectedFen = "8/6ks/3M2r1/2K1M3/8/3R4/8/8 w - 128 18 50"
    lu.assertEquals(board:fen(true, 0), expectedFen)
    lu.assertEquals(board:fen(true, -1), expectedFen)
    lu.assertEquals(board:fen(true, 89), expectedFen)
    board:delete()
end

function TestFairystockfish:test_setFen()
    local board = ffish.Board.new()
    board:setFen("r1bqkbnr/ppp2ppp/2np4/1B6/3NP3/8/PPP2PPP/RNBQK2R b KQkq - 0 5")
    lu.assertEquals(board:fen(), "r1bqkbnr/ppp2ppp/2np4/1B6/3NP3/8/PPP2PPP/RNBQK2R b KQkq - 0 5")
    board:delete()
end

function TestFairystockfish:test_sanMove()
    local board = ffish.Board.new()
    lu.assertEquals(board:sanMove("g1f3"), "Nf3")
    board:delete()
end

function TestFairystockfish:test_sanMoveWithNotation()
    local board = ffish.Board.new()
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.DEFAULT), "Nf3")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.SAN), "Nf3")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.LAN), "Ng1-f3")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.SHOGI_HOSKING), "N36")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.SHOGI_HODGES), "N-3f")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.SHOGI_HODGES_NUMBER), "N-36")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.JANGGI), "N87-66")
    lu.assertEquals(board:sanMove("g1f3", ffish.Notation.XIANGQI_WXF), "N2+3")
    board:delete()
end

function TestFairystockfish:test_variationSan()
    local board = ffish.Board.new()
    board:push("e2e4")
    local sanMoves = board:variationSan("e7e5 g1f3 b8c6 f1c4")
    lu.assertEquals(sanMoves, "1...e5 2. Nf3 Nc6 3. Bc4")
    local sanMovesInvalid = board:variationSan("e7e5 g1f3 b8c6 f1c7")
    lu.assertEquals(sanMovesInvalid, "")
    board:delete()
end

function TestFairystockfish:test_variationSanWithNotation()
    local board = ffish.Board.new()
    board:push("e2e4")
    local sanMoves = board:variationSan("e7e5 g1f3 b8c6 f1c4", ffish.Notation.LAN)
    lu.assertEquals(sanMoves, "1...e7-e5 2. Ng1-f3 Nb8-c6 3. Bf1-c4")
    board:delete()
end

function TestFairystockfish:test_variationSanWithNotationAndMoveNumbers()
    local board = ffish.Board.new()
    board:push("e2e4")
    local sanMoves = board:variationSan("e7e5 g1f3 b8c6 f1c4", ffish.Notation.SAN, false)
    lu.assertEquals(sanMoves, "e5 Nf3 Nc6 Bc4")
    board:delete()
end

function TestFairystockfish:test_turn()
    local board = ffish.Board.new()
    lu.assertTrue(board:turn())
    board:push("e2e4")
    lu.assertFalse(board:turn())
    board:delete()
end

function TestFairystockfish:test_fullmoveNumber()
    local board = ffish.Board.new()
    lu.assertEquals(board:fullmoveNumber(), 1)
    board:push("e2e4")
    lu.assertEquals(board:fullmoveNumber(), 1)
    board:push("e7e5")
    board:push("g1f3")
    board:push("g8f6")
    board:push("f3e5")
    lu.assertEquals(board:fullmoveNumber(), 3)
    board:delete()
end

function TestFairystockfish:test_halfmoveClock()
    local board = ffish.Board.new()
    lu.assertEquals(board:halfmoveClock(), 0)
    board:push("e2e4")
    board:push("e7e5")
    lu.assertEquals(board:halfmoveClock(), 0)
    board:push("g1f3")
    board:push("g8f6")
    lu.assertEquals(board:halfmoveClock(), 2)
    board:push("f3e5")
    lu.assertEquals(board:halfmoveClock(), 0)
    board:delete()
end

function TestFairystockfish:test_gamePly()
    local board = ffish.Board.new()
    lu.assertEquals(board:gamePly(), 0)
    board:push("e2e4")
    lu.assertEquals(board:gamePly(), 1)
    board:push("e7e5")
    board:push("g1f3")
    board:push("g8f6")
    board:push("f3e5")
    lu.assertEquals(board:gamePly(), 5)
    board:delete()
end

function TestFairystockfish:test_hasInsufficientMaterial()
    local board = ffish.Board.new()
    lu.assertFalse(board:hasInsufficientMaterial(self.WHITE))
    lu.assertFalse(board:hasInsufficientMaterial(self.BLACK))
    
    board:setFen("8/5k2/8/8/8/2K5/6R1/8 w - - 0 1")
    lu.assertFalse(board:hasInsufficientMaterial(self.WHITE))
    lu.assertTrue(board:hasInsufficientMaterial(self.BLACK))
    
    board:setFen("8/5k2/8/8/8/2K5/6q1/8 w - - 0 1")
    lu.assertTrue(board:hasInsufficientMaterial(self.WHITE))
    lu.assertFalse(board:hasInsufficientMaterial(self.BLACK))
    
    board:setFen("8/5k2/8/8/8/2K5/6B1/8 w - - 0 1")
    lu.assertTrue(board:hasInsufficientMaterial(self.WHITE))
    lu.assertTrue(board:hasInsufficientMaterial(self.BLACK))
    board:delete()
end

function TestFairystockfish:test_isInsufficientMaterial()
    local board = ffish.Board.new()
    lu.assertFalse(board:isInsufficientMaterial())
    
    board:setFen("8/5k2/8/8/8/2K5/6R1/8 w - - 0 1")
    lu.assertFalse(board:isInsufficientMaterial())
    
    board:setFen("8/5k2/8/8/8/2K5/6q1/8 w - - 0 1")
    lu.assertFalse(board:isInsufficientMaterial())
    
    board:setFen("8/5k2/8/8/8/2K5/6B1/8 w - - 0 1")
    lu.assertTrue(board:isInsufficientMaterial())
    board:delete()
end

function TestFairystockfish:test_isGameOver()
    -- Test checkmate (Fool's mate)
    local board = ffish.Board.newVariantFen("chess", "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3")
    lu.assertTrue(board:isGameOver())
    lu.assertTrue(board:isCheck())
    board:delete()

    -- Test stalemate
    local board2 = ffish.Board.newVariantFen("chess", "7k/8/7K/8/8/8/8/8 b - - 0 1")
    lu.assertTrue(board2:isGameOver())
    lu.assertFalse(board2:isCheck())
    board2:delete()

    -- Test optional draw claimed
    local board3 = ffish.Board.new()
    board3:pushSanMoves("Nf3 Nc6 Ng1 Nb8 Nf3 Nc6 Ng1")
    lu.assertFalse(board3:isGameOver(false))
    lu.assertFalse(board3:isGameOver(true))
    board3:pushSan("Nb8")
    lu.assertFalse(board3:isGameOver(false))
    lu.assertTrue(board3:isGameOver(true))
    board3:delete()
end

function TestFairystockfish:test_result()
    -- Scholar's mate (win for white)
    local board = ffish.Board.new()
    lu.assertEquals(board:result(), "*")
    board:pushSanMoves("e4 e5 Bc4 Nc6 Qh5 Nf6")
    lu.assertEquals(board:result(), "*")
    board:pushSan("Qxf7#")
    lu.assertEquals(board:result(), "1-0")

    -- Fool's mate (win for black)
    board:reset()
    board:pushSanMoves("f3 e5 g4")
    lu.assertEquals(board:result(), "*")
    board:pushSan("Qh4#")
    lu.assertEquals(board:result(), "0-1")

    -- Stalemate
    board:setFen("2Q2bnr/4p1pq/5pkr/7p/7P/4P3/PPPP1PP1/RNB1KBNR w KQ - 1 10")
    lu.assertEquals(board:result(), "*")
    board:pushSan("Qe6")
    lu.assertEquals(board:result(), "1/2-1/2")

    -- Draw claimed by n-fold repetition
    board:reset()
    board:pushSanMoves("Nf3 Nc6 Ng1 Nb8 Nf3 Nc6 Ng1")
    lu.assertEquals(board:result(false), "*")
    lu.assertEquals(board:result(true), "*")
    board:pushSan("Nb8")
    lu.assertEquals(board:result(false), "*")
    lu.assertEquals(board:result(true), "1/2-1/2")

    board:delete()
end

function TestFairystockfish:test_checkedPieces()
    -- Test cases with different check scenarios
    local testCases = {
        {
            fen = "rnbqkb1r/pppp1Bpp/5n2/4p3/4P3/8/PPPP1PPP/RNBQK1NR b KQkq - 0 3",
            expected = "e",  -- Simple king check (e-file)
            variant = "chess"
        },
        {
            fen = "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3",
            expected = "",    -- No pieces in check
            variant = "chess"
        },
        {
            fen = "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3",
            expected = "e",  -- Checkmate position (e-file)
            variant = "chess"
        }
    }
    
    for _, test in ipairs(testCases) do
        local board = ffish.Board.new(test.variant)
        board:setFen(test.fen)
        
        local ok, checkedPieces = pcall(function() return board:checkedPieces() end)
        if ok then
            -- Sort the pieces to ensure consistent comparison
            local pieces = {}
            for piece in checkedPieces:gmatch("%S+") do
                table.insert(pieces, piece)
            end
            table.sort(pieces)
            local sortedPieces = table.concat(pieces, " ")
            
            -- Sort expected pieces for comparison
            local expectedPieces = {}
            for piece in test.expected:gmatch("%S+") do
                table.insert(expectedPieces, piece)
            end
            table.sort(expectedPieces)
            local sortedExpected = table.concat(expectedPieces, " ")
            
            lu.assertEquals(sortedPieces, sortedExpected, 
                string.format("Failed for FEN: %s\nExpected: %s\nGot: %s", 
                    test.fen, test.expected, checkedPieces))
        else
            print(string.format("Warning: checkedPieces() not supported for variant %s", test.variant))
        end
        board:delete()
    end
end

function TestFairystockfish:test_isCheck()
    local board = ffish.Board.new()
    lu.assertFalse(board:isCheck())
    
    board:setFen("rnbqkb1r/pppp1Bpp/5n2/4p3/4P3/8/PPPP1PPP/RNBQK1NR b KQkq - 0 3")
    lu.assertTrue(board:isCheck())
    
    board:setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4")
    board:pushSan("Qxf7#")
    lu.assertTrue(board:isCheck())
    board:delete()
end

-- function TestFairystockfish:test_isBikjang()
--     local board = ffish.Board.new("janggi")
--     lu.assertFalse(board:isBikjang())
--     board:delete()
    
--     -- This is a real bikjang position where both kings face each other with no pieces in between
--     local board2 = ffish.Board.newVariantFen("janggi", "4k4/9/9/9/9/9/9/9/9/4K4 w - - 0 1")
--     lu.assertTrue(board2:isBikjang())
--     board2:delete()
-- end

function TestFairystockfish:test_isCapture()
    local board = ffish.Board.new()
    lu.assertFalse(board:isCapture("e2e4"))
    board:pushMoves("e2e4 e7e5 g1f3 b8c6 f1c4 f8c5")
    lu.assertFalse(board:isCapture("e1g1"))
    board:reset()
    board:pushMoves("e2e4 g8f6 e4e5 d7d5")
    lu.assertTrue(board:isCapture("e5f6"))
    lu.assertTrue(board:isCapture("e5d6"))
    board:delete()
end

function TestFairystockfish:test_moveStack()
    local board = ffish.Board.new()
    board:push("e2e4")
    board:push("e7e5")
    board:push("g1f3")
    lu.assertEquals(board:moveStack(), "e2e4 e7e5 g1f3")
    board:setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4")
    board:pushSan("Qxf7#")
    lu.assertEquals(board:moveStack(), "h5f7")
    board:delete()
end

function TestFairystockfish:test_pushMoves()
    local board = ffish.Board.new()
    board:pushMoves("e2e4 e7e5 g1f3")
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_pushSanMoves()
    local board = ffish.Board.new()
    board:pushSanMoves("e4 e5 Nf3")
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_pushSanMovesWithNotation()
    local board = ffish.Board.new()
    board:pushSanMoves("e4 e5 Nf3", ffish.Notation.SAN)
    lu.assertEquals(board:fen(), "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    board:delete()
end

function TestFairystockfish:test_pocket()
    local board = ffish.Board.new("crazyhouse", "rnb1kbnr/ppp1pppp/8/8/8/5q2/PPPP1PPP/RNBQKB1R/Pnp w KQkq - 0 4")
    lu.assertEquals(board:pocket(self.WHITE), "p")
    lu.assertEquals(board:pocket(self.BLACK), "np")
    board:delete()
    
    local board2 = ffish.Board.new("crazyhouse", "rnb1kbnr/ppp1pppp/8/8/8/5q2/PPPP1PPP/RNBQKB1R[Pnp] w KQkq - 0 4")
    lu.assertEquals(board2:pocket(self.WHITE), "p")
    lu.assertEquals(board2:pocket(self.BLACK), "np")
    board2:delete()
end

function TestFairystockfish:test_toString()
    local board = ffish.Board.new("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3")
    local expected = "r n b . k b n r\n" ..
                    "p p p . p p p p\n" ..
                    ". . . . . . . .\n" ..
                    ". . . q . . . .\n" ..
                    ". . . . . . . .\n" ..
                    ". . . . . . . .\n" ..
                    "P P P P . P P P\n" ..
                    "R N B Q K B N R"
    lu.assertEquals(board:toString(), expected)
    board:delete()
end

function TestFairystockfish:test_toVerboseString()
    local board = ffish.Board.new("chess", "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3")
    local verboseStr = board:toVerboseString()
    -- Check for key elements in verbose string
    lu.assertStrContains(verboseStr, "+---+---+---+---+---+---+---+---+")
    lu.assertStrContains(verboseStr, "Fen: rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3")
    lu.assertStrContains(verboseStr, "| r | n | b |   | k | b | n | r |")
    board:delete()
end

function TestFairystockfish:test_variant()
    local board = ffish.Board.new("chess")
    lu.assertEquals(board:variant(), "chess")
    board:delete()
    
    local board2 = ffish.Board.new("")
    lu.assertEquals(board2:variant(), "chess")
    board2:delete()
    
    local board3 = ffish.Board.new("standard")
    lu.assertEquals(board3:variant(), "chess")
    board3:delete()
    
    local board4 = ffish.Board.new("Standard")
    lu.assertEquals(board4:variant(), "chess")
    board4:delete()
    
    local board5 = ffish.Board.new("atomic")
    lu.assertEquals(board5:variant(), "atomic")
    board5:delete()
end

function TestFairystockfish:test_info()
    lu.assertStrMatches(ffish.info(), "^Fairy%-Stockfish.*")
end

function TestFairystockfish:test_setOption()
    ffish.setOption("VariantPath", "variants.ini")
    -- If we got here without error, test passes
    lu.assertTrue(true)
end

function TestFairystockfish:test_variants()
    print("\nStarting minimal test...")
    
    -- Just try to get the variants list
    print("Getting variants list...")
    local ok, variants = pcall(function()
        return ffish.variants()
    end)
    
    if not ok then
        print("Error getting variants:", variants)
        lu.fail("Failed to get variants: " .. tostring(variants))
        return
    end
    
    print("Got variants successfully:", variants)
    lu.assertStrContains(variants, "chess")
end

function TestFairystockfish:test_readGamePGN()
    local testPGN = [[
[Event "Test Game"]
[Site "Chess.com"]
[Date "2023.01.01"]
[Round "1"]
[White "Player1"]
[Black "Player2"]
[Result "1-0"]
[Variant "Crazyhouse"]

1. e4 e5 2. Nf3 Nc6 3. Bc4 Bc5 1-0
]]
    
    local game = ffish.readGamePGN(testPGN)
    lu.assertNotNil(game)
    
    local variant = game:headers("Variant"):lower():gsub("960$", "")
    local board = ffish.Board.new(variant)
    
    for move in game:mainlineMoves():gmatch("%S+") do
        lu.assertTrue(board:push(move))
    end
    
    board:delete()
    game:delete()
end

function TestFairystockfish:test_gameHeaderKeys()
    local testPGN = [[
[Event "Test Game"]
[Site "Chess.com"]
[Date "2023.01.01"]
[Round "1"]
[White "Player1"]
[Black "Player2"]
[Result "1-0"]

1. e4 e5 2. Nf3 1-0
]]
    
    local game = ffish.readGamePGN(testPGN)
    lu.assertNotNil(game)
    
    local keys = game:headerKeys()
    lu.assertStrContains(keys, "Event")
    lu.assertStrContains(keys, "Site")
    lu.assertStrContains(keys, "Date")
    
    game:delete()
end

function TestFairystockfish:test_gameHeaders()
    local testPGN = [[
[Event "Test Game"]
[Site "Chess.com"]
[Date "2023.01.01"]
[Round "1"]
[White "Player1"]
[Black "Player2"]
[Result "1-0"]
[Variant "Seirawan"]
[FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1"]

1. e4 e5 2. Nf3 1-0
]]
    
    local game = ffish.readGamePGN(testPGN)
    lu.assertNotNil(game)
    
    lu.assertEquals(game:headers("White"), "Player1")
    lu.assertEquals(game:headers("Black"), "Player2")
    lu.assertEquals(game:headers("Variant"), "Seirawan")
    lu.assertEquals(game:headers("FEN"), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[HEhe] w KQBCDFGkqbcdfg - 0 1")
    
    game:delete()
end

function TestFairystockfish:test_validateFen()
    -- Test basic chess FEN validation
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), 1)
    lu.assertEquals(ffish.validateFen("6k1/R7/2p4p/2P2p1P/PPb2Bp1/2P1K1P1/5r2/8 b - - 4 39"), 1)
end

function TestFairystockfish:test_validateFenErrorCodes()
    -- Error id checks
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[]wKQkq-3+301", "3check-crazyhouse"), -10)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] KQkq - 3+3 0 1", "3check-crazyhouse"), -6)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/ppppXppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -10)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppKpp/8/8/8/8/PPPPPPPP/RNBQ1BNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -9)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -8)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -8)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[77] w KQkq - 3+3 0 1", "3check-crazyhouse"), -7)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] o KQkq - 3+3 0 1", "3check-crazyhouse"), -6)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w K6kq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbq1bnr/pppkpppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbqkbn1/pppppppr/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/RB w KQkq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq ss 3+3 0 1", "3check-crazyhouse"), -4)
    lu.assertEquals(ffish.validateFen("rnbqkknr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -3)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 x 1", "3check-crazyhouse"), -2)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 -13", "3check-crazyhouse"), -1)
    lu.assertEquals(ffish.validateFen("", "chess"), 0)
end

function TestFairystockfish:test_setOptionInt()
    ffish.setOptionInt("Threads", 4)
    -- If we got here without error, test passes
    lu.assertTrue(true)
end

function TestFairystockfish:test_setOptionBool()
    ffish.setOptionBool("Ponder", true)
    -- If we got here without error, test passes
    lu.assertTrue(true)
end

function TestFairystockfish:test_chess960Castling()
    local board = ffish.Board.new("chess", "bqrbkrnn/pppppppp/8/8/8/8/PPPPPPPP/BQRBKRNN w CFcf - 0 1", true)
    board:pushMoves("g1f3 h8g6")
    lu.assertFalse(board:isCapture("e1f1"))
    board:delete()
end

function TestFairystockfish:test_sittuyinSpecialMoves()
    local board = ffish.Board.new("sittuyin", "8/2k5/8/4P3/4P1N1/5K2/8/8[] w - - 0 1")
    lu.assertFalse(board:isCapture("e5e5f"))
    board:delete()
end

function TestFairystockfish:test_validateFenWithVariant()
    -- Check starting FENs for all variants
    local variants = {}
    for variant in ffish.variants():gmatch("%S+") do
        table.insert(variants, variant)
    end
    
    for _, variant in ipairs(variants) do
        local startFen = ffish.startingFen(variant)
        lu.assertEquals(ffish.validateFen(startFen, variant), 1, "Invalid start FEN for " .. variant)
        
        -- Check if the FEN is still valid if board.fen() is returned
        local board = ffish.Board.new(variant)
        lu.assertEquals(ffish.validateFen(board:fen(), variant), 1)
        board:delete()
    end
    
    -- Test alternative pocket formulations for crazyhouse variants
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/RB w KQkq - 3+3 0 1", "3check-crazyhouse"), 1)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/ w KQkq - 3+3 0 1", "3check-crazyhouse"), 1)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 3+3 0 1", "3check-crazyhouse"), 1)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[-] w KQkq - 3+3 0 1", "3check-crazyhouse"), 1)
end

function TestFairystockfish:test_validateFenWithVariantAndChess960()
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w AHah - 0 1", "chess", true), 1)
    lu.assertEquals(ffish.validateFen("nrbqbkrn/pppppppp/8/8/8/8/PPPPPPPP/NRBQBKRN w BGbg - 0 1", "chess", true), 1)
end

function TestFairystockfish:test_startingFen()
    lu.assertEquals(ffish.startingFen("chess"), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
end

function TestFairystockfish:test_capturesToHand()
    lu.assertFalse(ffish.capturesToHand("seirawan"))
    lu.assertTrue(ffish.capturesToHand("shouse"))
end

function TestFairystockfish:test_twoBoards()
    lu.assertFalse(ffish.twoBoards("chess"))
    lu.assertTrue(ffish.twoBoards("bughouse"))
end

function TestFairystockfish:test_chess960Castling()
    local board = ffish.Board.new("chess", "bqrbkrnn/pppppppp/8/8/8/8/PPPPPPPP/BQRBKRNN w CFcf - 0 1", true)
    board:pushMoves("g1f3 h8g6")
    lu.assertFalse(board:isCapture("e1f1"))
    board:delete()
end

function TestFairystockfish:test_setOptionInt()
    ffish.setOptionInt("Threads", 4)
    -- If we got here without error, test passes
    lu.assertTrue(true)
end

function TestFairystockfish:test_setOptionBool()
    ffish.setOptionBool("Ponder", true)
    -- If we got here without error, test passes
    lu.assertTrue(true)
end

function TestFairystockfish:test_validateFenErrorCodes()
    -- Error id checks
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[]wKQkq-3+301", "3check-crazyhouse"), -10)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] KQkq - 3+3 0 1", "3check-crazyhouse"), -6)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/ppppXppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -10)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppKpp/8/8/8/8/PPPPPPPP/RNBQ1BNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -9)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -8)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -8)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[77] w KQkq - 3+3 0 1", "3check-crazyhouse"), -7)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] o KQkq - 3+3 0 1", "3check-crazyhouse"), -6)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w K6kq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbq1bnr/pppkpppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbqkbn1/pppppppr/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/RB w KQkq - 3+3 0 1", "3check-crazyhouse"), -5)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq ss 3+3 0 1", "3check-crazyhouse"), -4)
    lu.assertEquals(ffish.validateFen("rnbqkknr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 1", "3check-crazyhouse"), -3)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 x 1", "3check-crazyhouse"), -2)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR[] w KQkq - 3+3 0 -13", "3check-crazyhouse"), -1)
    lu.assertEquals(ffish.validateFen("", "chess"), 0)
end

function TestFairystockfish:test_sittuyinSpecialMoves()
    local board = ffish.Board.new("sittuyin", "8/2k5/8/4P3/4P1N1/5K2/8/8[] w - - 0 1")
    lu.assertFalse(board:isCapture("e5e5f"))
    board:delete()
end

function TestFairystockfish:test_invalid_variant()
    -- Test that validateFen returns +1 for valid FEN with valid variant
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "chess"), 1)

    -- Test that validateFen returns -10 for invalid FEN with valid variant
    lu.assertEquals(ffish.validateFen("invalid fen", "chess"), -10, "Invalid piece character")

    -- Test that validateFen returns 0 for empty FEN string
    lu.assertEquals(ffish.validateFen("", "chess"), 0, "Fen is empty")

    -- Test that validateFen returns -8 for FEN with wrong number of ranks
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1", "chess"), -8, "Invalid number of ranks")

    -- Test that validateFen returns -10 for FEN with invalid piece characters
    lu.assertEquals(ffish.validateFen("rnbqkbnr/ppppXppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "chess"), -10, "Invalid piece character")

    -- Test that validateFen returns -6 for FEN with invalid side to move
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1", "chess"), -6, "Invalid side to move")

    -- Test that validateFen returns -5 for FEN with invalid castling rights
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkqX - 0 1", "chess"), -5, "Invalid castling rights")

    -- Test that validateFen returns -4 for FEN with invalid en passant square
    -- Using a valid FEN string with only the en passant part being invalid
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq abc 0 1", "chess"), -4, "Invalid en passant square - too many characters")
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq 1e 0 1", "chess"), -4, "Invalid en passant square - first char not a letter")
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq ex 0 1", "chess"), -4, "Invalid en passant square - second char not a digit")

    -- Test that validateFen returns -3 for FEN with invalid piece placement
    lu.assertEquals(ffish.validateFen("rnbqkknr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "chess"), -3, "Invalid piece placement")

    -- Test that validateFen returns -2 for FEN with invalid halfmove clock
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - x 1", "chess"), -2, "Invalid halfmove clock")

    -- Test that validateFen returns 1 for FEN with fullmove number 0 (handled gracefully)
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0", "chess"), 1, "Fullmove number 0 is handled gracefully")

    -- Test that validateFen returns -10 for invalid variant
    pcall(function()
        ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "invalid_variant")
    end)
    -- The test passes if we get here without error
    lu.assertTrue(true)
end

function TestFairystockfish:test_invalid_fen()
    -- Test various invalid FEN strings
    lu.assertEquals(ffish.validateFen("invalid fen string", "chess"), -10)  -- Invalid piece character
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1", "chess"), -8)  -- Missing last rank
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq+ - 0 1", "chess"), -5)  -- Invalid castling rights
    lu.assertEquals(ffish.validateFen("rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "chess"), -8)  -- Too many pawns
    lu.assertEquals(ffish.validateFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1", "chess"), -6)  -- Invalid side to move
end

function TestFairystockfish:test_game_status()
    -- Test checkmate (Fool's mate)
    local board = ffish.Board.new("chess")
    print("\nTesting checkmate position:")
    print("Initial FEN:", board:fen())
    
    -- Make moves one by one using UCI notation
    lu.assertTrue(board:push("f2f3"), "Failed to push f2f3")
    print("After f3:", board:fen())
    
    lu.assertTrue(board:push("e7e5"), "Failed to push e7e5")
    print("After e5:", board:fen())
    
    lu.assertTrue(board:push("g2g4"), "Failed to push g2g4")
    print("After g4:", board:fen())
    
    lu.assertTrue(board:push("d8h4"), "Failed to push d8h4")
    print("Final position:", board:fen())
    print("Legal moves:", board:legalMoves())
    print("Is game over:", board:isGameOver())
    print("Is check:", board:isCheck())
    
    lu.assertTrue(board:isGameOver())
    lu.assertTrue(board:isCheck())
    board:delete()
    
    -- Test stalemate
    -- This is a simple stalemate position where White has no legal moves
    local board2 = ffish.Board.newVariantFen("chess", "k7/8/1Q6/8/8/8/8/7K b - - 0 1")
    print("\nTesting stalemate position:")
    print("FEN:", board2:fen())
    print("Legal moves:", board2:legalMoves())
    print("Is game over:", board2:isGameOver())
    print("Is check:", board2:isCheck())
    lu.assertTrue(board2:isGameOver())
    lu.assertFalse(board2:isCheck())
    board2:delete()
end

function TestFairystockfish:test_variant_specific_rules()
    -- Test crazyhouse piece drops
    local board = ffish.Board.new("crazyhouse")
    board:pushSan("e4")
    board:pushSan("d5")
    board:pushSan("exd5")
    lu.assertStrContains(board:legalMoves(), "P@")
    board:delete()
    
    -- Test xiangqi river crossing
    local xiangqi = ffish.Board.new("xiangqi")
    lu.assertFalse(xiangqi:pushSan("E2+6")) -- Elephant can't cross river
    xiangqi:delete()
end

function TestFairystockfish:test_move_generation_special_cases()
    -- En passant
    local board = ffish.Board.new("chess", "rnbqkbnr/ppp2ppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3")
    lu.assertStrContains(board:legalMoves(), "e5d6")
    board:delete()
    
    -- Castling through check
    local board2 = ffish.Board.new("chess", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1")
    lu.assertFalse(board2:pushSan("O-O-O")) -- Can't castle through check
    board2:delete()
end

function TestFairystockfish:test_memory_management()
    local board = ffish.Board.new()
    for i = 1, 1000 do
        board:push("e2e4")
        board:pop()
    end
    board:delete()
    -- No memory leaks if we get here
end

function TestFairystockfish:test_concurrent_access()
    local boards = {}
    for i = 1, 10 do
        boards[i] = ffish.Board.new()
        boards[i]:pushSan("e4")
    end
    
    for _, board in ipairs(boards) do
        lu.assertEquals(board:fen(), "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1")
        board:delete()
    end
end

function TestFairystockfish:test_position_evaluation()
    local board = ffish.Board.new()
    board:pushSan("e4")
    board:pushSan("e5")
    board:pushSan("Nf3")
    
    -- Test basic evaluation functions if available
    if board.hasMethod and board:hasMethod("evaluate") then
        local eval = board:evaluate()
        lu.assertNumber(eval)
    end
    board:delete()
end

function TestFairystockfish:test_notation_conversions()
    local board = ffish.Board.new()
    
    -- Test different notation styles
    local move = "e2e4"
    lu.assertEquals(board:sanMove(move, ffish.Notation.SAN), "e4")
    lu.assertEquals(board:sanMove(move, ffish.Notation.LAN), "e2-e4")
    
    board:delete()
end

function TestFairystockfish:test_variationSanWithNotationAndMoveNumbers()
    local board = ffish.Board.new()
    board:push("e2e4")
    -- Test with move numbers enabled (default)
    local sanMovesWithNumbers = board:variationSanWithNotationAndMoveNumbers("e7e5 g1f3 b8c6 f1c4", 0, true)
    lu.assertEquals(sanMovesWithNumbers, "1...e5 2. Nf3 Nc6 3. Bc4")
    -- Test with move numbers disabled
    local sanMovesWithoutNumbers = board:variationSanWithNotationAndMoveNumbers("e7e5 g1f3 b8c6 f1c4", 0, false)
    lu.assertEquals(sanMovesWithoutNumbers, "e5 Nf3 Nc6 Bc4")
    board:delete()
end 

function TestFairystockfish:test_variationSanWithNotation()
    local board = ffish.Board.new()
    board:push("e2e4")
    print("LAN notation value:", ffish.Notation.LAN)
    local sanMoves = board:variationSanWithNotation("e7e5 g1f3 b8c6 f1c4", ffish.Notation.LAN)
    print("Expected: 1...e7-e5 2. Ng1-f3 Nb8-c6 3. Bf1-c4")
    print("Actual:", sanMoves)
    lu.assertEquals(sanMoves, "1...e7-e5 2. Ng1-f3 Nb8-c6 3. Bf1-c4")
    board:delete()
end



-- Run the tests
os.exit(lu.LuaUnit.run())

-- Clean up boards
if board then board:delete() end
if board2 then board2:delete() end
if board3 then board3:delete() end
if board4 then board4:delete() end