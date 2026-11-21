# fairy-stockfish.wasm port for the Stockfish 19 merge

Port of the wasm patches of fairy-stockfish/fairy-stockfish.wasm (branch `nnue` @ 718ebc68,
= Fairy-Stockfish master 52838835 + wasm patches) to the SF19 merge (aa9f69b8, PR #1045).
The upstream-sync merge conflicts in 9 files, so it has to be applied by hand.

Apply in the wasm repo (the result is identical to the verified port commit):

    git checkout nnue
    git merge aa9f69b8      # 9 conflicts
    git diff --name-only --diff-filter=U | xargs git checkout --theirs -- && git add -A
    git apply part1.patch   # engine sources, relative to aa9f69b8
    git apply part2.patch   # src/emscripten, relative to 718ebc68
    git commit              # commit message needs "bench: 2813825"

Follow-ups in the wasm repo:
- drop emscripten/wasm_simd.cpp from SRCS (no longer used)
- CI step `make -C .. net | grep "Default net"` (net.sh prints other text now)
- CI size limit: no-embedded build is 2,102,530 B (> 2,000,000)
- recommended: `EM_CXXFLAGS += -DUSE_SLOPPY_ATOMICS` in emscripten/Makefile
  (ucinewgame 200 ms -> 23 ms, startup back to master level)

Verified: bench equal to native (2813825), SIMD affine layers bit-identical to scalar,
functional tests in Node 16 and Chromium. The port needs a rebase onto the final merge commit.
