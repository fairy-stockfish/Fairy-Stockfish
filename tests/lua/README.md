# Fairy-Stockfish Lua Bindings

The Lua bindings for Fairy-Stockfish provide a powerful interface to the chess variant engine. The bindings are implemented using LuaBridge3 and support all features available in the Python and JavaScript bindings.

## Prerequisites

1. Install pkg-config (required for finding Lua):
   ```bash
   # On macOS
   brew install pkg-config

   # On Ubuntu/Debian
   sudo apt-get install pkg-config
   ```

2. Install Lua 5.1 (required for LuaJIT compatibility):
   ```bash
   # On macOS
   brew install lua@5.1

   # On Ubuntu/Debian
   sudo apt-get install lua5.1 liblua5.1-dev
   ```

3. Initialize and update the LuaBridge3 submodule:
   ```bash
   git submodule init
   git submodule update
   ```

## Building with Lua Support

1. Navigate to the source directory:
   ```bash
   cd src
   ```

2. Build Fairy-Stockfish with Lua support:
   ```bash
   # For Apple Silicon Macs
   make build ARCH=apple-silicon lua=yes

   # For Intel Macs/Linux
   make build ARCH=x86-64-modern lua=yes
   ```

   Note: Choose the appropriate ARCH value for your system:
   - `apple-silicon` for Apple M1/M2 Macs
   - `x86-64-modern` for modern Intel/AMD processors
   - See `make help` for other architecture options

## Installation

The Lua bindings are compiled as part of the Fairy-Stockfish build process when enabled with `lua=yes`. The resulting module is named `fairystockfish.so` and is created in the `src` directory.

To use the module in your Lua projects, you have several options:

1. Copy `src/fairystockfish.so` to one of your existing Lua package directories:
   ```bash
   # Find your Lua package paths
   lua -e "print(package.cpath)"
   
   # Example paths:
   # macOS (Homebrew): /opt/homebrew/lib/lua/5.4/?.so
   # Linux: /usr/local/lib/lua/5.4/?.so
   ```

2. Add the Fairy-Stockfish `src` directory to your `LUA_CPATH` environment variable:
   ```bash
   export LUA_CPATH="/path/to/Fairy-Stockfish/src/?.so;;"
   ```

3. Modify `package.cpath` in your Lua script before requiring the module:
   ```lua
   package.cpath = "/path/to/Fairy-Stockfish/src/?.so;" .. package.cpath
   local ffish = require('fairystockfish')
   ```

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