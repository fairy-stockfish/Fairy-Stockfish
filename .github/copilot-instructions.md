# Fairy-Stockfish Development Guide

## Repository Overview

**Fairy-Stockfish** is a chess variant engine derived from Stockfish, designed to support numerous chess variants and protocols. Written primarily in C++17, it includes Python and JavaScript bindings for library use.

**Repository Statistics:**
- **Languages:** C++17 (primary), Python, JavaScript, Shell scripts
- **Size:** ~55MB with neural networks, ~15MB source code  
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

**ALWAYS run commands from the `src/` directory unless specified otherwise.**

#### 1. Download Neural Network (Required First Step)
```bash
cd src
make net  # Downloads default NNUE network (~75MB)
```
**Note:** This step MUST be completed before building, or the engine will fail to start.

#### 2. Basic Build Commands
```bash
# Standard release build (recommended for most users)
make -j2 ARCH=x86-64 build

# Debug build (for development)
make -j2 ARCH=x86-64 debug=yes build

# Large board variants (supports boards up to 12x12)  
make -j2 ARCH=x86-64 largeboards=yes build

# All variants including Game of the Amazons
make -j2 ARCH=x86-64 largeboards=yes all=yes build

# With NNUE evaluation (stronger play)
make -j2 ARCH=x86-64 nnue=yes build
```

#### 3. Architecture Options
Common architectures (use `make help` for full list):
- `x86-64-modern`: Modern 64-bit CPUs (recommended)
- `x86-64-avx2`: CPUs with AVX2 support
- `x86-64`: Generic 64-bit (portable but slower)
- `armv8`: ARM64 CPUs
- `apple-silicon`: Apple M1/M2 processors

#### 4. Build Troubleshooting
- **"Network not found" error:** Run `make net` first
- **Compilation errors:** Ensure C++17 compiler (GCC 7+, Clang 5+)
- **Linking errors on Linux:** Install `g++-multilib` for 32-bit builds
- **Build takes >5 minutes:** Normal for release builds; use `-j4` for faster compilation

### Python Bindings (pyffish)
```bash
# Build Python bindings (from repository root)
python3 setup.py build_ext --inplace
python3 setup.py install

# Alternative: Install from PyPI
pip install pyffish
```

### JavaScript Bindings (ffish.js)
```bash
cd tests/js
npm install  # Installs emscripten-built bindings
```

## Testing & Validation

### Core Engine Tests
```bash
cd src

# Basic functionality test (3-5 minutes)
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
cd src  # Note: Run from src/ directory (same as build commands)

# Protocol compliance tests (1-2 minutes)
../tests/protocol.sh

# Move generation validation (10-15 minutes for all variants)
../tests/perft.sh all
../tests/perft.sh chess      # Chess only (2-3 minutes)  
../tests/perft.sh largeboard # Large board variants only

# Regression testing
../tests/regression.sh
```

### Performance Testing
```bash
cd src  # Note: Run from src/ directory

# Reproducible search test
../tests/reprosearch.sh

# Build signature verification  
../tests/signature.sh
```

**Test Prerequisites:** Install `expect` package (`sudo apt install expect` on Ubuntu).

## Project Architecture

### Directory Structure
```
├── src/                    # Core C++ engine source
│   ├── Makefile           # Primary build configuration
│   ├── main.cpp           # Engine entry point
│   ├── uci.cpp           # UCI protocol implementation  
│   ├── xboard.cpp        # XBoard protocol implementation
│   ├── position.cpp      # Game position management
│   ├── movegen.cpp       # Move generation
│   ├── search.cpp        # Search algorithm
│   ├── evaluate.cpp      # Position evaluation
│   ├── variants.ini      # Variant definitions (200+ variants)
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
- **`src/variants.ini`**: Defines all supported chess variants
- **`setup.py`**: Python package build configuration  
- **`appveyor.yml`**: Windows CI configuration
- **`tests/js/package.json`**: JavaScript bindings configuration

### Key Source Files
- **`src/variant.cpp`**: Variant-specific game rules
- **`src/parser.cpp`**: FEN/move parsing for all variants
- **`src/piece.cpp`**: Piece type definitions and behavior
- **`src/partner.cpp`**: Bughouse partnership logic
- **`src/tune.cpp`**: Parameter tuning functionality

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
3. **Run relevant tests:** `../tests/perft.sh <variant>` for move generation changes
4. **Check protocol compliance:** `../tests/protocol.sh` for interface changes

### Adding New Variants
1. **Edit `src/variants.ini`**: Add variant configuration
2. **Test parsing:** `./stockfish check variants.ini`
3. **Add perft test:** Update `tests/perft.sh` with known-good positions
4. **Validate gameplay:** `./stockfish bench <variant>`

### Performance Considerations
- **Large board variants:** Use `largeboards=yes` build option
- **NNUE networks:** Required for optimal strength in most variants
- **Debug builds:** 3-10x slower than release builds
- **Memory usage:** ~100MB+ for large hash tables and networks

## Troubleshooting Guide

### Build Failures
- **Missing network:** Download with `make net`  
- **C++ version:** Requires C++17-compatible compiler
- **32-bit builds:** May need `g++-multilib` package
- **Linking errors:** Ensure pthread library availability

### Runtime Issues
- **"Variant not found":** Check `variants.ini` for typos
- **Slow performance:** Use release build without `debug=yes`
- **Memory errors:** Large board variants need more RAM
- **Protocol errors:** Check UCI/XBoard command compatibility

### Test Failures  
- **Perft mismatches:** Usually indicates move generation bugs
- **Protocol timeouts:** Increase timeout in test scripts
- **Benchmark variations:** Expected 1-5% variance across builds
- **Missing expect:** Install expect utility for test scripts

## Development Workflow

**Trust these instructions.** Only search for additional information if these instructions are incomplete or incorrect. The build and test procedures documented here are validated and comprehensive.

### Typical Development Cycle
1. **Setup:** `cd src && make net && make -j2 ARCH=x86-64 debug=yes build`
2. **Test:** `./stockfish bench` (quick validation)  
3. **Develop:** Make focused changes to relevant source files
4. **Validate:** Run appropriate tests from src/ (`../tests/perft.sh`, `../tests/protocol.sh`)
5. **Release build:** `make clean && make -j2 ARCH=x86-64 build`
6. **Final test:** `./stockfish bench` and relevant test suite

### Quick Verification Commands
```bash
# Verify build works
./stockfish bench 2>/dev/null >/dev/null && echo "Build OK"

# Check variant parsing
./stockfish check variants.ini 2>&1 | grep -c "Invalid" | grep -q "^0$" && echo "Variants OK"

# Protocol basic test
echo -e "uci\nquit" | ./stockfish | grep -q "uciok" && echo "UCI OK"
```

This documentation covers the essential information needed for effective development in Fairy-Stockfish. The build system is robust but requires following the documented steps precisely, especially the initial `make net` command.