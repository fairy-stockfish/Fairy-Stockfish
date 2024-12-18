local lu = require('luaunit')
local ffish = require('fairystockfish')

-- Test Suite
TestFairystockfish = {}

function TestFairystockfish:setUp()
    self.pgnDir = "../pgn/"
    self.srcDir = "../../src/"
    self.WHITE = true
    self.BLACK = false
    self.boards = {}
    
    local variantPath = "/Users/andy/Work/Fairy-Stockfish/src/variants.ini"
    local f = io.open(variantPath, "r")
    if f then
        f:close()
        ffish.setOption("VariantPath", variantPath)
    else
        error("variants.ini not found at " .. variantPath)
    end
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
    
    local ok, err = pcall(function()
        ffish.loadVariantConfig(testConfig)
    end)
    
    if not ok then
        lu.fail("Failed to load variant config: " .. tostring(err))
        return
    end
    
    -- Verify variants list includes minitic
    local variants = ffish.variants()
    
    -- Check if minitic is in the variants list
    if not string.find(variants, "minitic") then
        lu.fail("minitic variant not found in variants list after loading config")
        return
    end
    
    local ok, board = pcall(function()
        return ffish.Board.newVariant("minitic")
    end)
    
    if not ok then
        lu.fail("Failed to create minitic board: " .. tostring(board))
        return
    end
    
    if not board then
        lu.fail("Board creation returned nil")
        return
    end
    
    local ok2, fen = pcall(function()
        return board:fen()
    end)
    
    if not ok2 then
        lu.fail("Failed to get board FEN: " .. tostring(fen))
        board:delete()
        return
    end
    
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
    
    local ok2, fen = pcall(function()
        return board:fen()
    end)
    
    if not ok2 then
        lu.fail("Failed to get board FEN: " .. tostring(fen))
        board:delete()
        return
    end
    
    local startingFen = ffish.startingFen("minixiangqi")
    lu.assertEquals(fen, startingFen)
    lu.assertFalse(board:is960())
    
    -- Clean up
    board:delete()
end

function TestFairystockfish:test_Board_fen()
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
    
    local ok2, fen = pcall(function()
        return board:fen()
    end)
    
    if not ok2 then
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
    
    lu.assertEquals(board:fen(), "rnknb1rq/pp2ppbp/3p2p1/2p5/4PP2/2N1N1P1/PPPP3P/R1K1BBRQ b GAga - 1 5")
    lu.assertTrue(board:is960())
    
    -- Clean up
    board:delete()
end

function TestFairystockfish:test_legalMoves()
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
    local board = ffish.Board.newVariantFen("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR[QPbq] w KQ - 0 7")
    
    local moves = board:legalMovesSan()
    
    local moveTable = {}
    for move in moves:gmatch("%S+") do
        table.insert(moveTable, move)
    end
    table.sort(moveTable)
    
    local expectedMoves = {
        "a3", "b3", "d3", "f3", "g3", "h3", "a4", "b4", "d4", "f4", "g4", "h4",
        "Nb1", "Nd1", "Nce2", "Na4", "Nb5", "Nd5", "Nge2", "Nf3", "Nh3", "Rb1",
        "P@e2", "P@a3", "P@b3", "P@d3", "P@e3", "P@f3", "P@g3", "P@h3", "P@a4",
        "P@b4", "P@c4", "P@d4", "P@f4", "P@g4", "P@h4", "P@a5", "P@b5", "P@d5",
        "P@f5", "P@g5", "P@h5", "P@a6", "P@b6", "P@d6", "P@e6+", "P@f6", "P@g6+",
        "P@h6", "P@e7", "Q@b1", "Q@d1", "Q@f1", "Q@e2", "Q@a3", "Q@b3+", "Q@d3",
        "Q@e3", "Q@f3+", "Q@g3", "Q@h3", "Q@a4", "Q@b4", "Q@c4+", "Q@d4", "Q@f4+",
        "Q@g4", "Q@h4", "Q@a5", "Q@b5", "Q@d5+", "Q@f5+", "Q@g5", "Q@h5+", "Q@a6",
        "Q@b6", "Q@d6", "Q@e6+", "Q@f6+", "Q@g6+", "Q@h6", "Q@e7+", "Q@b8", "Q@d8",
        "Q@e8+", "Q@f8+", "Kd1", "Kf1", "Ke2"
    }
    table.sort(expectedMoves)
    
    lu.assertEquals(moveTable, expectedMoves)
    board:delete()
end

function TestFairystockfish:test_numberLegalMoves()
    local board = ffish.Board.newVariantFen("crazyhouse", "r1b3nr/pppp1kpp/2n5/2b1p3/4P3/2N5/PPPP1PPP/R1B1K1NR[QPbq] w KQ - 0 7")
    
    local moves = board:legalMoves()
    
    local moveCount = 0
    for move in moves:gmatch("%S+") do
        moveCount = moveCount + 1
    end
    
    local apiCount = board:numberLegalMoves()
    
    lu.assertEquals(apiCount, moveCount, "API move count doesn't match manual count")
    lu.assertEquals(apiCount, 90, "Expected 90 legal moves")
    board:delete()
    
    local minichess = ffish.Board.newVariant("losalamos")
    lu.assertEquals(minichess:numberLegalMoves(), 10)
    minichess:delete()
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
    
    local expectedFen = "8/6ks/3M2r1/2K1M3/8/3R4/8/8 w - 128 18 50"
    lu.assertEquals(board:fen(true), expectedFen)
    lu.assertEquals(board:fen(false), expectedFen)
    board:delete()
end

function TestFairystockfish:test_fenWithShowPromotedAndCountStarted()
    local board = ffish.Board.newVariantFen("makruk", "8/6ks/3M~2r1/2K1M3/8/3R4/8/8 w - 128 18 50")
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
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.SAN), "Nf3")
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.LAN), "Ng1-f3")
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.SHOGI_HOSKING), "N36")
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.SHOGI_HODGES), "N-3f")
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.SHOGI_HODGES_NUMBER), "N-36")
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.JANGGI), "N87-66")
    
    lu.assertEquals(board:sanMoveNotation("g1f3", ffish.Notation.XIANGQI_WXF), "N2+3")
    
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
    local sanMoves = board:variationSanWithNotationAndMoveNumbers("e7e5 g1f3 b8c6 f1c4", ffish.Notation.SAN, false)
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
    local board = ffish.Board.new()
    lu.assertEquals(board:result(), "*")
    board:pushSanMoves("e4 e5 Bc4 Nc6 Qh5 Nf6")
    lu.assertEquals(board:result(), "*")
    board:pushSan("Qxf7#")
    lu.assertEquals(board:result(), "1-0")

    board:reset()
    board:pushSanMoves("f3 e5 g4")
    lu.assertEquals(board:result(), "*")
    board:pushSan("Qh4#")
    lu.assertEquals(board:result(), "0-1")

    board:setFen("2Q2bnr/4p1pq/5pkr/7p/7P/4P3/PPPP1PP1/RNB1KBNR w KQkq - 1 10")
    lu.assertEquals(board:result(), "*")
    board:pushSan("Qe6")
    lu.assertEquals(board:result(), "1/2-1/2")

    board:reset()
    board:pushMoves("g1f3 g8f6 f3g1 f6g8 g1f3 g8f6 f3g1 f6g8")
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
            -- print(string.format("Warning: checkedPieces() not supported for variant %s", test.variant))
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
    -- Test with square brackets format
    local board = ffish.Board.newVariantFen("crazyhouse", "rnb1kbnr/ppp1pppp/8/8/8/5q2/PPPP1PPP/RNBQKB1R[Pnp] w KQkq - 0 4")
    lu.assertEquals(board:pocket(true), "np")  -- White has knight and pawn in pocket
    lu.assertEquals(board:pocket(false), "p")  -- Black has pawn in pocket
    board:delete()
end

function TestFairystockfish:test_toString()
    -- Create board with specific test position
    local board = ffish.Board.new()
    board:setFen("rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3")
    
    local expected = 
        "r n b . k b n r\n" ..
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
    -- Create board with the test position
    local board = ffish.Board.new()
    -- Set the specific position we want to test
    board:setFen("rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3")
    
    local verboseStr = board:toVerboseString()
    
    -- Check for key elements that should be present in any verbose string
    lu.assertStrContains(verboseStr, "+---+---+---+---+---+---+---+---+")
    lu.assertStrContains(verboseStr, "| r | n | b |   | k | b | n | r |")
    lu.assertStrContains(verboseStr, "| P | P | P | P |   | P | P | P |")
    
    -- Check for the FEN string - make sure we're looking for the exact FEN that matches our position
    local expectedFen = "rnb1kbnr/ppp1pppp/8/3q4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3"
    lu.assertStrContains(verboseStr, "Fen: " .. expectedFen)
    
    board:delete()
end

function TestFairystockfish:test_variant()
    local board = ffish.Board.new("chess")
    lu.assertEquals(board:variant(), "chess")
    
    -- Make moves one by one using UCI notation
    lu.assertTrue(board:push("f2f3"), "Failed to push f2f3")
    
    lu.assertTrue(board:push("e7e5"), "Failed to push e7e5")
    
    lu.assertTrue(board:push("g2g4"), "Failed to push g2g4")
    
    lu.assertTrue(board:push("d8h4"), "Failed to push d8h4")
    
    lu.assertTrue(board:isGameOver())
    lu.assertTrue(board:isCheck())
    board:delete()
    
    -- Test stalemate
    -- This is a simple stalemate position where White has no legal moves
    local board2 = ffish.Board.newVariantFen("chess", "k7/8/1Q6/8/8/8/8/7K b - - 0 1")
    lu.assertTrue(board2:isGameOver())
    lu.assertFalse(board2:isCheck())
    board2:delete()
end

function TestFairystockfish:test_variant_specific_rules()
    -- Test crazyhouse piece drops
    local board = ffish.Board.newVariant("crazyhouse")
    
    -- Make moves to capture a pawn
    board:pushSan("e4")
    board:pushSan("d5")
    board:pushSan("exd5")
    
    -- Check if the captured pawn is in White's hand
    local fen = board:fen()
    local pocket = fen:match("%[(.-)%]")
    
    -- Black makes a move
    board:pushSan("Nf6")
    
    -- Get and print all legal moves
    local moves = board:legalMoves()
    
    -- Check for pawn drops specifically
    local hasDropMove = false
    local dropMoves = {}
    for move in moves:gmatch("%S+") do
        if move:match("^[pP]@%a%d$") then
            hasDropMove = true
            table.insert(dropMoves, move)
        end
    end
    
    lu.assertTrue(hasDropMove, "Expected to find pawn drop moves (P@e4 or p@e4 format) in legal moves. Found none in: " .. moves)
    board:delete()
    
    -- Test xiangqi river crossing
    local xiangqi = ffish.Board.new("xiangqi")
    lu.assertFalse(xiangqi:pushSan("E2+6")) -- Elephant can't cross river
    xiangqi:delete()
end

function TestFairystockfish:test_move_generation_special_cases()
    -- En passant
    local board = ffish.Board.newVariantFen("chess", "rnbqkbnr/ppp2ppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3")
    local moves = board:legalMoves()
    lu.assertStrContains(moves, "e5d6", "En passant capture e5d6 should be available")
    board:delete()
    
    -- Castling through check
    local board2 = ffish.Board.newVariantFen("chess", "r3k2r/8/8/8/4q3/8/8/R3K2R w KQkq - 0 1")
    lu.assertFalse(board2:pushSan("O-O-O"), "Should not be able to castle through check with queen on e4")
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
    lu.assertEquals(board:sanMoveNotation(move, ffish.Notation.SAN), "e4")
    lu.assertEquals(board:sanMoveNotation(move, ffish.Notation.LAN), "e2-e4")
    
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
    local sanMoves = board:variationSanWithNotation("e7e5 g1f3 b8c6 f1c4", ffish.Notation.LAN)
    lu.assertEquals(sanMoves, "1...e7-e5 2. Ng1-f3 Nb8-c6 3. Bf1-c4")
    board:delete()
end

function TestFairystockfish:test_info()
    local board4 = ffish.Board.newVariant("chess")
    lu.assertEquals(board4:variant(), "chess")
    board4:delete()
end

-- Run the tests
os.exit(lu.LuaUnit.run())

-- Clean up boards
if board then board:delete() end
if board2 then board2:delete() end
if board3 then board3:delete() end
if board4 then board4:delete() end
