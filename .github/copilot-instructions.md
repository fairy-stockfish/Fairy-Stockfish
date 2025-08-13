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

# Large board variants (supports boards up to 12x10)  
make -j2 ARCH=x86-64 largeboards=yes build

# All variants including ones with large branching factor
make -j2 ARCH=x86-64 largeboards=yes all=yes build
```

#### Architecture Options
Common architectures (use `make help` for full list):
- `x86-64-modern`: Modern 64-bit CPUs (recommended)
- `x86-64-avx2`: CPUs with AVX2 support
- `x86-64`: Generic 64-bit (portable but slower)
- `armv8`: ARM64 CPUs
- `apple-silicon`: Apple M1/M2 processors

#### Build Troubleshooting
- **Compilation errors:** Ensure C++17 compiler (GCC 7+, Clang 5+)
- **Linking errors on Linux:** Install `g++-multilib` for 32-bit builds

### Python Bindings (pyffish)
```bash
# Build Python bindings (from repository root)
python3 setup.py install

# Alternative: Install from PyPI
pip install pyffish
```

### JavaScript Bindings (ffish.js)
See the tests/js directory.
```bash
npm install ffishjs # Installs emscripten-built bindings
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
├── src/                    # Core C++ engine source
│   ├── Makefile           # Primary build configuration
│   ├── main.cpp           # Engine entry point
│   ├── uci.cpp           # UCI protocol implementation  
│   ├── xboard.cpp        # XBoard protocol implementation
│   ├── position.h        # Position representation
│   ├── position.cpp      # Board logic
│   ├── movegen.cpp       # Move generation
│   ├── search.cpp        # Search algorithm
│   ├── evaluate.cpp      # Position evaluation
│   ├── variant.h         # Variant properties
│   ├── variant.cpp       # Variant definitions (built-in variants)
│   ├── variants.ini      # Variant definitions (runtime configurable variants)
│   ├── nnue/             # Neural network evaluation
│   ├── syzygy/           # Endgame tablebase support
│   ├── pyffish.cpp       # Python bindings
│   └── ffishjs.cpp       # JavaScript bindings
├── tests/                 # Test scripts and data
│   ├── perft.sh          # Move generation tests
│   ├── protocol.sh       # Protocol compliance tests  
│   ├── js/               # JavaScript binding tests
│   └── pgn/              # Test game files
├── .github/workflows/     # CI/CD configurations
├── setup.py              # Python package configuration
└── README.md             # Comprehensive documentation
```

### Configuration Files
- **`src/variants.ini`**: Defines examples for configuration of chess variants
- **`setup.py`**: Python package build configuration
- **`tests/js/package.json`**: JavaScript bindings configuration

### Key Source Files
- **`src/variant.h`**: Variant rule properties
- **`src/variant.cpp`**: Variant-specific game rules
- **`src/variant.ini`**: Variant rule configuration examples and documentation of variant properties
- **`src/position.h`**: Position representation
- **`src/position.cpp`**: Board logic
- **`src/movegen.cpp`**: Move generation logic
- **`src/parser.cpp`**: Variant rule configuration parsing
- **`src/piece.cpp`**: Piece type definitions and behavior

## Continuous Integration

### GitHub Actions Workflows
- **`fairy.yml`**: Core engine testing (perft, protocols, variants)
- **`stockfish.yml`**: Standard Stockfish compatibility tests
- **`release.yml`**: Binary releases for multiple platforms
- **`wheels.yml`**: Python package builds
- **`ffishjs.yml`**: JavaScript binding builds

### Validation Pipeline
1. **Code Standards:** C++17 compliance, `-Werror` warnings
2. **Protocol Tests:** UCI, UCCI, USI, XBoard compatibility
3. **Variant Tests:** Move generation verification for all variants
4. **Performance Tests:** Benchmark consistency across builds
5. **Multi-platform:** Linux, Windows, macOS builds

## Common Development Patterns

### Making Engine Changes
1. **Always test basic functionality:** `./stockfish bench` after changes
2. **Validate variant compatibility:** `./stockfish check variants.ini`  
3. **Run relevant tests:** `../tests/perft.sh all` for move generation changes
4. **Check protocol compliance:** `../tests/protocol.sh` for interface changes

### Adding New Configurable Variants
1. **Edit `src/variants.ini`**: Add variant configuration
2. **Test parsing:** `./stockfish check variants.ini`

### Performance Considerations
- **Large board variants:** Use `largeboards=yes` build option
- **NNUE networks:** Required for optimal strength in most variants
- **Debug builds:** significantly slower than release builds

## Troubleshooting Guide

### Build Failures
- **C++ version:** Requires C++17-compatible compiler
- **32-bit builds:** May need `g++-multilib` package
- **Linking errors:** Ensure pthread library availability

### Test Failures  
- **Perft mismatches:** Usually indicates move generation bugs
- **Benchmark variations:** Expected 1-5% variance across builds
- **Missing expect:** Install expect utility for test scripts

This documentation covers the essential information needed for effective development in Fairy-Stockfish.
