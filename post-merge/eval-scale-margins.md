# Search margins scaled by the evaluation volatility (candidate A')

Not applied. Notes for a follow-up after the Stockfish 19 merge (PR #1045).

Idea: razoring, the small ProbCut and the qsearch futility base are tuned to the
evaluation scale of chess. In variants with a much more volatile evaluation they
prune far more aggressively. Candidate A' widens these three margins by the
running mean evaluation swing between plies (per thread, since ucinewgame), with a
deadband so that chess (mean swing 120-360 per game) stays unchanged, without any
variant name check.

Result at 5s+0.05 (classical, 200 games each, vs 5d8239b3): crazyhouse -12,
shogi +23, antichess +19, 3check +41, atomic +17, pooled +17 +-20 (with the
similar candidate A: +17 +-14). Not significant per variant; needs a longer
confirmation (e.g. SPRT) and a comparison with a static per-variant scale.

The full reports of the audit session follow. The patch is the A diff of the
final report with the two changes described in the addendum.

## Final report

[eval-scale margins] FINAL REPORT (base 5d8239b3; ~6.5h; nothing pushed)

## Measurements
See the interim message for the full tables.
- Elevated wrong-prune rates vs chess appear only in razoring, qsearch futility / SEE-futility and small ProbCut, plus capture SEE in antichess/losers/racingkings.
- RFP, NMP, ProbCut, capture futility and quiet futility are at chess level, so they should NOT be scaled.

## Scale source
- Random-playout calibration at variant load did not track real volatility (shogi 1.1x vs 4.2x), so I dropped it.
- I used the runtime mean of min(|e(ply)+e(ply-1)|, 2000) over all nodes since ucinewgame. Scale = clamp(mean/300, 1, 4), activated after 65536 samples and recomputed per iteration.
- Chess game means were 122-341, so the scale is explicitly gated off for the chess variant. Chess bench is exactly unchanged: 3595323 / largeboards 3576658.
- xiangqi, makruk and janggi stay below 300, so the patch is a no-op there (no games spent).

## Games
5s+0.05, conc 3, 1 thread, classical, 200 games each, vs 5d8239b3, W-L-D and Elo +-95%.

A = scale only razoring 482*d*d, qsearch futilityBase +306 (and thus the SEE-futility threshold), small ProbCut +428. Max 4x.
  crazyhouse 98-95-7 +5 | atomic 71-65-64 +10 | 3check 96-93-11 +5 | antichess 91-70-39 +37 | losers 94-94-12 0 | shogi 107-88-5 +33 | ataxx 99-101-0 -3 | flipello 85-99-16 -24
  pooled main6 (zh, atomic, 3check, antichess, losers, shogi) +15 +-19; all 8: +8 +-16

B = A + hindsight depth-- threshold 166*scale + LMR term clamp((alpha-eval)/scale, -64, 96).
  zh -5 | atomic +17 | 3check +45 | antichess +19 | losers +38 | shogi +10 | ataxx -74 (79-121, significant)
  pooled main6 +21 +-18

C = B with those two extra terms capped at scale 2x. On the main variants it is almost identical to B, which suggests the ataxx loss came from the 4x scale on those two terms.
  ataxx -21 | flipello +5 | 3check +4 | zh +17 | losers -9 | antichess +19 | atomic -2 | shogi +9
  pooled main6 +6 +-18; B and C together on main6 about +14 +-13

D = C with the whole scale capped at 2x.
  flipello -22 | ataxx -7

## Conclusion
- The direction is consistently mildly positive in the volatile variants: every candidate pools +6..+21 on the main six. But no single variant result is significant at 200 games, and flipello/ataxx are neutral to slightly negative.
- Recommend A: the simplest patch (3 margins + 1 counter), it changes exactly the steps the instrumentation flagged, and it is exactly neutral for chess and chess-like variants.
- Not recommended: scaling the LMR alpha-eval term or the hindsight threshold by large factors (B lost -74 in ataxx).
- I suggest a longer verification (e.g. 1000 games in antichess, shogi and zh, or an SPRT) before merging. Treat it as low-risk / likely small gain, not a proven gain.

## Recommended patch (A), unified diff against 5d8239b3
diff --git a/src/search.cpp b/src/search.cpp
index 795317d..fb59f26 100644
--- a/src/search.cpp
+++ b/src/search.cpp
@@ -49,6 +49,7 @@
 #include "types.h"
 #include "uci.h"
 #include "uci.h"
+#include "variant.h"
 #include "xboard.h"
 #include "syzygy/tbprobe.h"
 #include "nnue/network.h"
@@ -455,6 +456,14 @@ bool Search::Worker::iterative_deepening() {
     {
         rootDepth++;
 
+        // Some margins tuned for chess are too tight for variants with a more
+        // volatile evaluation. Widen them according to the mean evaluation swing
+        // between plies since the start of the game, if it is well above that of
+        // chess (about 150). Chess itself always keeps its tuned margins.
+        if (evalSwingCnt >= 65536 && rootPos.variant() != variants.find("chess")->second)
+            evalScale =
+              std::clamp(int(evalSwingSum * 1024 / (300 * i64(evalSwingCnt))), 1024, 4096);
+
         // Age out PV variability metric and signal the start of a new iteration
         if (mainThread)
         {
@@ -931,6 +940,10 @@ void Search::Worker::clear() {
 
     ttMoveHistory = 0;
 
+    evalSwingSum = 0;
+    evalSwingCnt = 0;
+    evalScale    = 1024;
+
     for (auto& to : continuationCorrectionHistory)
         for (auto& h : to)
             h.fill(5);
@@ -1094,6 +1107,13 @@ Value Search::Worker::search(
     improving         = ss->staticEval > (ss - 2)->staticEval;
     opponentWorsening = ss->staticEval > -(ss - 1)->staticEval;
 
+    // Track the evaluation swing for the scaling of search margins
+    if (!ss->inCheck && !excludedMove && ((ss - 1)->currentMove).is_ok() && !(ss - 1)->inCheck)
+    {
+        evalSwingSum += std::min(std::abs(ss->staticEval + (ss - 1)->staticEval), 2000);
+        evalSwingCnt++;
+    }
+
     // Hindsight adjustment of reductions based on static evaluation difference
     if (priorReduction >= 3 && !opponentWorsening)
         depth++;
@@ -1225,7 +1245,7 @@ Value Search::Worker::search(
     // Step 8. Razoring
     // If eval is really low, skip search entirely and return the qsearch value
     if (!PvNode && !pos.blast_on_capture() && pos.walling_rule() != DUCK
-        && eval < alpha - 482 * depth * depth)
+        && eval < alpha - 482 * depth * depth * evalScale / 1024)
         return qsearch<NonPV>(pos, ss, alpha, beta);
 
     // Step 9. Futility pruning: child node
@@ -1354,7 +1374,7 @@ Value Search::Worker::search(
 moves_loop:  // When in check, search starts here
 
     // Step 13. A small ProbCut idea
-    probCutBeta = beta + 428;
+    probCutBeta = beta + 428 * evalScale / 1024;
     if ((ttData.bound & BOUND_LOWER) && ttData.depth >= depth - 4 && ttData.value >= probCutBeta
         && !is_decisive(beta) && is_valid(ttData.value) && !is_decisive(ttData.value))
         return probCutBeta;
@@ -2063,7 +2083,7 @@ Value Search::Worker::qsearch(Position& pos, Stack* ss, Value alpha, Value beta,
         if (bestValue > alpha)
             alpha = bestValue;
 
-        futilityBase = ss->staticEval + 306;
+        futilityBase = ss->staticEval + 306 * evalScale / 1024;
     }
 
     const PieceToHistory* contHist[] = {(ss - 1)->continuationHistory};
diff --git a/src/search.h b/src/search.h
index 48eb975..a82bfc8 100644
--- a/src/search.h
+++ b/src/search.h
@@ -383,6 +383,12 @@ class Worker {
     RelaxedAtomic<u64> nodes, tbHits, bestMoveChanges;
     int                selDepth, nmpMinPly;
 
+    // Evaluation swing between plies, used to scale search margins
+    // to the evaluation scale of the variant (1024 = chess scale)
+    i64 evalSwingSum;
+    u64 evalSwingCnt;
+    int evalScale;
+
 
     Position  rootPos;
     StateInfo rootState;

## Commit message
Scale razoring and qsearch/ProbCut margins to the variant's eval volatility

Several search margins are tuned to the scale of chess evaluations. In
variants with a much more volatile (classical) evaluation, e.g. drop,
check counting, antichess-like or reversi-like variants, they prune
relatively far more aggressively. Measured with a verification search on
sampled prunings (local instrumentation, self-play), the share of wrong
decisions vs chess is:
- razoring: chess 3.3%, crazyhouse 7.5%, shogi 9.3%, 3check 12.8%,
  flipello 26.5%, ataxx 56%
- qsearch futility: chess 1.0%, crazyhouse 4.2%, atomic 8.0%,
  antichess 25%, flipello 50%
- small ProbCut: chess 1.1%, atomic 5.3%, losers 6.4%
while RFP, null move, ProbCut and quiet move futility stay at chess level.

Track the mean evaluation swing |eval(ply) + eval(ply-1)| over all nodes
searched since the start of the game (about 150 in chess, 550-750 in
shogi, atomic, 3check, crazyhouse) and use it to widen the razoring,
small ProbCut and qsearch futility margins by up to 4x if it exceeds
twice that of chess. Chess itself always keeps the tuned margins.

5s+0.05, 1 thread, classical eval, vs previous (W-L-D, Elo +-95%):
crazyhouse  98- 95-  7   +5 +-48
atomic      71- 65- 64  +10 +-40
3check      96- 93- 11   +5 +-47
antichess   91- 70- 39  +37 +-44
losers      94- 94- 12    0 +-47
shogi      107- 88-  5  +33 +-48
ataxx       99-101-  0   -3 +-49
flipello    85- 99- 16  -24 +-47
pooled main six variants: +15 +-19, all eight: +8 +-16
xiangqi, makruk, janggi: evaluation swing below threshold, no change.

Bench: 3595323

## Other findings (not eval-scale; possible follow-ups; not fixed)
1. antichess/losers: capture SEE pruning is wrong 49% / 16% (chess 0.5%), and qsearch SEE-futility 45% / 45%. SEE and "staticEval + captured value" are meaningless when captures are forced and material is bad. These steps likely need a must_capture / extinction_value()==VALUE_MATE gate (pre-existing on master in similar form).
2. racingkings: razoring is 61% wrong at depth 3-4, and capture SEE 12%. janggi razoring is 30% wrong at depth 3-4 (chess 9%). The eval scale is not the cause there; a depth limit (razor only at depth <= 2) may help.
3. amazons and isolation: RFP is wrong 12-14% (chess 4%) and small ProbCut 37% (amazons). The classical eval is nearly always 0 at 16cp granularity, so margin-based pruning is noise there. Possibly gate RFP when eval granularity is too coarse. Unscaled (my scale is never below 1).
4. Flipello and ataxx remain volatile at depth 3-9 RFP (6-7.5% wrong) even with the existing depth-9 limit.
5. NNUE: not measured (no variant nets in this environment). The mechanism measures actual static-eval swings, so it adapts to NNUE evals automatically. Did not touch evaluate.cpp or correction history.

Local artifacts: worktrees base, instr, candA..D, final, and match logs (games/*.log) in my scratchpad.

## Addendum

[eval-scale margins] ADDENDUM to the final report, answering your 4 concerns. I only saw your two messages after sending the final report; apologies. Still nothing pushed.

## 1. Name gate removed → revised candidate A′
- No variant check any more. The deadband itself keeps chess unchanged: scale = clamp((mean - 200) * 1024 / 200, 1024, 4096).
- mean = cumulative mean of min(|e(ply)+e(ply-1)|, 2000) since ucinewgame, used once there are ≥ 65536 samples. This gives 1x up to a mean of 400 and 2x at 600.
- Measured per-game cumulative mean in chess (22 self-play games at 100-150ms/move, max over the game after 65536 / 1M / 4M samples): typically 200-280, worst game 357 (ending at 340). Converged variant values: shogi 450-500, 3check 500-635, crazyhouse 570-740, atomic 450-820, antichess/losers 500-850. flipello and ataxx are >1000, so they hit the 4x cap.
- Bench: 3595323 (normal) and 3576658 (largeboards), unchanged.
- Chess and chess-like variants (incl. chess960 and ini variants) are unchanged unless a game's mean swing exceeds 400. I never observed that in chess, but it is not a hard guarantee.
- xiangqi, makruk and janggi are well below 400.

Games, 5s+0.05, conc 3, 200 each vs 5d8239b3:
crazyhouse 92-99-9 -12 | shogi 106-93-1 +23 | antichess 91-80-29 +19 | 3check 108-85-7 +41 | atomic 74-66-60 +17
pooled: 471-423-106, +16.7 +-20.4
- Pooled with candidate A on the same 5 variants (nearly the same mechanism, A at mean/300): 934-834-232, +17.4 +-14.3 (~2.4 sigma).
- crazyhouse sits around 0 over all four candidates. shogi, antichess, 3check and atomic are consistently positive.
- Not re-run with A′: losers, ataxx, flipello (A: 0 / -3 / -24).

## 2. Determinism and threads
- The accumulator is per Worker (plain members, written only by the owning thread), so there are no data races. Each thread computes its own scale. There is no shared state, by design.
- clear() resets it, so ucinewgame + the same command sequence is reproducible. Checked in atomic with `go depth 16`: fresh twice → 1441701 nodes both times.
- Searching P1 after a depth-17 search of P0 (with ucinewgame before each pair) is also reproducible: 3742819 nodes twice.
- History-dependent nodes (fresh vs after another search) differ, as they also do in base (base 1927765 vs 2450336) because of TT/histories. With A′ the scale is one more such history input (1441701 vs 3742819).
- reprosearch.sh: `expect` is not installed in this container, so it could not run. It is chess-only anyway, where evalScale stays 1024 because chess stays below the deadband.

PRE-EXISTING ISSUE FOUND (unrelated to this patch): crazyhouse is not reproducible even in base 5d8239b3 and in master 52838835.
- Steps: ucinewgame → isready/readyok → position startpos moves e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 → go depth 16, repeated.
- base gives 239295 vs 226675 nodes with different bestmoves; master gives 645770 vs 1106757.
- atomic and shogi are reproducible this way. So something drop/hand-specific is apparently not reset by ucinewgame, or memory is uninitialized. Worth a separate look.

## 3. Static alternative
Not tested in games, for lack of time, so I can't say whether A beats a static version.
- A random-playout calibration at variant load does not work: it doesn't track real search volatility (shogi 1.1x vs 4.2x, crazyhouse 2.1x vs 5.7x).
- Piece values cannot work either: crazyhouse has chess piece values but 5x the swing.
- So a static per-variant scale means a variant property (e.g. `evalScale` in variants.ini) with hand-set values for the built-ins. From the measured converged A′ scales:
  - crazyhouse ~2.2
  - 3check ~2.0
  - atomic ~2.0
  - antichess/losers ~1.7
  - shogi ~1.4
  - koth/seirawan/duck ~1.0-1.6
  - flipello/ataxx 4
  - default 1 for all others
- That is deterministic and simple, but user-defined ini variants would get nothing unless configured.
- If you prefer it for the merge, the search changes are identical to A′ with evalScale taken from the variant instead of the counter. I'd expect similar results on the tested variants, since the runtime scale converges early in each game.

## 4. flipello / ataxx and d2678209
- No double counting. d2678209 limits the child-futility (RFP) depth for REVERSI and singular extensions for drops. A/A′ never touch RFP or singular extensions, only razoring, the small ProbCut and the qsearch futility base.
- All flipello/ataxx results are on top of d2678209 (base 5d8239b3), on both sides of the match. I had no time for a run with d2678209 reverted.
- Since A/A′ changes flipello (scale 4x), with A flipello -24 +-47 and ataxx -3: if you want zero risk there, cap the scale at 2x (I tested this as D, which also scaled the hindsight threshold and LMR term: flipello -22, ataxx -7, so no clear difference).

## Updated recommendation
- Use A′ instead of A: same search changes, no name gate.
- Patch = the A diff from my final report with two changes:
  (a) drop the `#include "variant.h"` hunk;
  (b) replace the scale computation with
```
        // Some margins tuned for chess are too tight for variants with a more
        // volatile evaluation. Widen them according to the mean evaluation swing
        // between plies since the start of the game if it is clearly above the
        // range of chess (about 120-360 per game).
        if (evalSwingCnt >= 65536)
            evalScale = std::clamp(
              int((evalSwingSum / i64(evalSwingCnt) - 200) * 1024 / 200), 1024, 4096);
```
- All other hunks are identical: evalSwingSum/Cnt/evalScale members, clear(), the swing tracking after opponentWorsening, and the three margins (razoring 482*d*d, small ProbCut 428, qsearch futilityBase 306 all *evalScale/1024).
- Bench: 3595323.
- Commit message as in the final report, with the results replaced by: crazyhouse -12, shogi +23, antichess +19, 3check +41, atomic +17 (200 games each, pooled +17 +-20), and the threshold sentence replaced by "if the mean swing exceeds 400 (chess games stay at 120-360)".
- Still recommend a longer confirmation before merging.
