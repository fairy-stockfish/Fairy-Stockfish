# Fairy-Stockfish Development Guide

## Repository Overview

**Fairy-Stockfish** is a chess variant engine derived from Stockfish, designed to support numerous chess variants and protocols. Written primarily in C++17, it includes Python and JavaScript bindings for library use.

**Repository Statistics:**
- **Languages:** C++17 (primary), Python, JavaScript, Shell scripts
- **Architecture:** Multi-protocol chess engine (UCI, UCCI, USI, XBoard/CECP)
- **Target Platforms:** Windows, Linux, macOS, Android, WebAssembly
- **Supported Variants:** 90+ chess variants including regional, historical, and modern variants

## Build System & Requirements

### Prerequisites
- **Compiler:** GCC, Clang, or MSVC with C++17 support
- **Build Tool:** GNU Make (required for C++ engine)
- **Python:** 3.7+ (for Python bindings)
- **Node.js:** (for JavaScript bindings)
- **Additional Tools:** expect utility (for testing)

### Core Build Process

Note: Run engine and test commands from the `src/` directory unless specified otherwise.

#### Basic Build Commands
```bash
# Standard release build (recommended for most users)
make -j2 ARCH=x86-64 build

# Debug build (for development)
make -j2 ARCH=x86-64 debug=yes build

# All variants including ones with large boards (up to 12x10) and large branching factor (all)
make -j2 ARCH=x86-64 largeboards=yes all=yes build
```

### Python Bindings (pyffish)
```bash
# Build Python bindings (from repository root)
python3 setup.py install

# Alternative: Install from PyPI
pip install pyffish
```

### JavaScript Bindings (ffish.js)
Also see the `tests/js/README.md`.
```bash
cd src/

# Build JavaScript bindings (requires emscripten)
make -f Makefile_js build

# Alternative: Install from npm
npm install ffish
```

## Testing & Validation

All test commands below assume the current directory is `src/`.

### Core Engine Tests
```bash
# Basic functionality test
./stockfish bench

# Variant-specific benchmarks
./stockfish bench xiangqi
./stockfish bench shogi
./stockfish bench capablanca

# Validate variants configuration
./stockfish check variants.ini
```

### Comprehensive Test Suite
```bash
# Protocol compliance tests
../tests/protocol.sh

# Move generation validation
../tests/perft.sh all
../tests/perft.sh chess      # Chess only
../tests/perft.sh largeboard # Large board variants only

# Regression testing
../tests/regression.sh

# Reproducible search test
../tests/reprosearch.sh

# Build signature verification  
../tests/signature.sh
```


## Project Architecture

### Directory Structure
```
src/                  # Core C++ engine source
tests/                # Test scripts and data
.github/workflows/    # CI/CD configurations
```

### Configuration Files
- **`src/variants.ini`**: Defines examples for configuration of chess variants
- **`setup.py`**: Python package build configuration
- **`tests/js/package.json`**: JavaScript bindings configuration

### Key Source Files in `src/`
- **`variant.h`**: Variant rule properties
- **`variant.cpp`**: Variant-specific game rules
- **`variant.ini`**: Variant rule configuration examples and documentation of variant properties
- **`position.h`**: Position representation
- **`position.cpp`**: Board logic
- **`movegen.cpp`**: Move generation logic
- **`parser.cpp`**: Variant rule configuration parsing
- **`piece.cpp`**: Piece type definitions and behavior
- **`pyffish.cpp`**: Python bindings
- **`ffishjs.cpp`**: JavaScript bindings

## Continuous Integration

### GitHub Actions Workflows
- **`fairy.yml`**: Core engine testing (perft, protocols, variants)
- **`stockfish.yml`**: Standard Stockfish compatibility tests
- **`release.yml`**: Binary releases for multiple platforms
- **`wheels.yml`**: Python package builds
- **`ffishjs.yml`**: JavaScript binding builds

## Common Development Patterns

### Making Engine Changes
1. **Always test basic functionality:** `./stockfish bench` after changes
2. **Validate variant compatibility:** `./stockfish check variants.ini`  
3. **Run relevant tests:** `../tests/perft.sh all` for move generation changes

### Adding New Configurable Variants
1. **Edit `src/variants.ini`**: Add variant configuration
2. **Test parsing:** `./stockfish check variants.ini`

### Relevant websites for researching chess variant rules
- [Chess Variants on Wikipedia](https://en.wikipedia.org/wiki/List_of_chess_variants)
- [Chess Variants Wiki](https://www.chessvariants.com/)
- [Variant Chess on BoardGameGeek](https://boardgamegeek.com/boardgamefamily/4024/traditional-games-chess)
- [PyChess Variants](https://www.pychess.org/variants)
- [Ludii](https://ludii.games/library.php)
- [Lichess.org Variants](https://lichess.org/variant)
- [Greenchess](https://greenchess.net/variants.php)
- [Chess Variants on Chess.com](https://www.chess.com/variants)

### Development Best Practices
* Make sure to only stage and commit changes that are intended to be part of the task.
* Keep changes minimal and focused on the task at hand.
* After applying changes make sure that all places related to the task have been identified.
* Stay consistent with the existing code style and conventions.
