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

-- Game now uses static factory methods like Board
local game = ffish.Game.new()
if not game then
    print("Warning: Failed to create game")
end

local game2 = ffish.Game.fromPGN("[Event \"Test\"]")
if not game2 then
    print("Warning: Failed to create game from PGN")
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
    -- Use a minimal test config string instead of reading from file
    local testConfig = [[
[variant]
name = tictactoe
variant = tictactoe
pieceToChar = .PPPPPppppp
startFen = 3/3/3[PPPPPpppp] w - - 0 1
]]
    ffish.loadVariantConfig(testConfig)
    
    local board = ffish.Board.newVariant("tictactoe")
    lu.assertEquals(board:fen(), "3/3/3[PPPPPpppp] w - - 0 1")
    board:delete()
end

-- Run the tests
os.exit(lu.LuaUnit.run())

-- Clean up boards
if board then board:delete() end
if board2 then board2:delete() end
if board3 then board3:delete() end
if board4 then board4:delete() end

-- Clean up games at the end
if game then game:delete() end
if game2 then game2:delete() end