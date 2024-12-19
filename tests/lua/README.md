# Fairy-Stockfish Lua Bindings

The Lua bindings for Fairy-Stockfish provide a powerful interface to the chess variant engine. The bindings are implemented using LuaBridge3 and support all features available in the Python and JavaScript bindings.

## Building with Lua Support

1. First, ensure you have Lua 5.1 development files installed:
   - On Ubuntu/Debian: `sudo apt-get install liblua5.1-0-dev`
   - On macOS: `brew install lua@5.1`
   - On Windows: Download from http://luabinaries.sourceforge.net/

2. Initialize and update the LuaBridge3 submodule:
   ```bash
   git submodule init
   git submodule update
   ```

3. Build Fairy-Stockfish with Lua support:
   ```bash
   cd src
   make build ARCH=x86-64 lua=yes
   ```

   Note: Replace `x86-64` with your target architecture (see `make help` for options).

## Installation

The Lua bindings are compiled as part of the Fairy-Stockfish build process when enabled with `lua=yes`. The resulting module is named `fairystockfish`.

To use the module in your Lua projects, ensure that:
1. The compiled module is in your Lua package path
2. The Fairy-Stockfish library is in your system's library path

## Usage

```lua
local ffish = require('fairystockfish')

-- Initialize a standard chess board
local board = ffish.Board.new()

-- Or initialize with a specific variant
local crazyhouse = ffish.Board.newVariant("crazyhouse")

-- Initialize with a specific FEN
local custom = ffish.Board.newVariantFen("chess", "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1")

-- Get available variants
print(ffish.variants())  -- prints list of all supported variants

-- Get legal moves
print(board:legalMoves())  -- prints all legal moves in UCI format
print(board:legalMovesSan())  -- prints all legal moves in SAN format

-- Make moves
board:push("e2e4")  -- UCI format
board:pushSan("e4")  -- SAN format

-- Get current position
print(board:fen())  -- prints current FEN

-- Check game status
print(board:isGameOver())  -- checks if game is over
print(board:isCheck())  -- checks if king is in check

-- Clean up when done
board:delete()
```

## API Reference

### Module Functions

- `ffish.info()` - Returns engine information
- `ffish.variants()` - Returns list of supported variants
- `ffish.loadVariantConfig(config)` - Loads custom variant configuration
- `ffish.validateFen(fen, [variant], [chess960])` - Validates FEN string
- `ffish.startingFen(variant)` - Returns starting FEN for variant
- `ffish.setOption(name, value)` - Sets engine option

### Board Class

#### Constructors

- `Board.new()` - Creates new standard chess board
- `Board.newVariant(variant)` - Creates board for specific variant
- `Board.newVariantFen(variant, fen)` - Creates board with specific variant and FEN
- `Board.newVariantFen960(variant, fen, is960)` - Creates board with Chess960 support

#### Methods

- `board:legalMoves()` - Returns legal moves in UCI format
- `board:legalMovesSan()` - Returns legal moves in SAN format
- `board:push(move)` - Makes move in UCI format
- `board:pushSan(move)` - Makes move in SAN format
- `board:pop()` - Takes back last move
- `board:reset()` - Resets board to starting position
- `board:fen()` - Returns current FEN
- `board:isCheck()` - Returns if king is in check
- `board:isGameOver()` - Returns if game is over
- `board:is960()` - Returns if board is Chess960
- `board:turn()` - Returns side to move (true=White, false=Black)
- `board:fullmoveNumber()` - Returns full move number
- `board:halfmoveClock()` - Returns halfmove clock
- `board:gamePly()` - Returns game ply
- `board:hasInsufficientMaterial(color)` - Checks for insufficient material
- `board:delete()` - Cleans up board resources

## Examples

See [test.lua](test.lua) for comprehensive examples of using the bindings.

## License

The Lua bindings are released under the same license as Fairy-Stockfish. 