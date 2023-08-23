# Fairy-Stockfish

## Overview

[![Build Status](https://github.com/fairy-stockfish/Fairy-Stockfish/workflows/Release/badge.svg?branch=master)](https://github.com/fairy-stockfish/Fairy-Stockfish/actions?query=workflow%3ARelease)
[![Build Status](https://github.com/fairy-stockfish/Fairy-Stockfish/workflows/fairy/badge.svg?branch=master)](https://github.com/fairy-stockfish/Fairy-Stockfish/actions?query=workflow%3Afairy)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/fairy-stockfish/Fairy-Stockfish?branch=master&svg=true)](https://ci.appveyor.com/project/ianfab/Fairy-Stockfish/branch/master)
[![PyPI version](https://badge.fury.io/py/pyffish.svg)](https://badge.fury.io/py/pyffish)
[![NPM version](https://img.shields.io/npm/v/ffish.svg?sanitize=true)](https://www.npmjs.com/package/ffish)

Fairy-Stockfish is a chess variant engine derived from [Stockfish](https://github.com/official-stockfish/Stockfish/) designed for the support of fairy chess variants and easy extensibility with more games. It can play various regional, historical, and modern chess variants as well as [games with user-defined rules](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Variant-configuration). For [compatibility with graphical user interfaces](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Graphical-user-interfaces) it supports the UCI, UCCI, USI, UCI-cyclone, and CECP/XBoard protocols.

The goal of the project is to create an engine supporting a large variety of chess-like games, equipped with the powerful search of Stockfish. Despite its generality the [playing strength](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Playing-strength) is on a very high level in almost all supported variants. Due to its multi-protocol support Fairy-Stockfish works with almost any chess variant GUI.

## Installation
You can download the [Windows executable](https://github.com/fairy-stockfish/Fairy-Stockfish/releases/latest/download/fairy-stockfish-largeboard_x86-64.exe) or [Linux binary](https://github.com/fairy-stockfish/Fairy-Stockfish/releases/latest/download/fairy-stockfish-largeboard_x86-64) from the [latest release](https://github.com/fairy-stockfish/Fairy-Stockfish/releases/latest) or [compile the program from source](https://github.com/fairy-stockfish/Fairy-Stockfish#compiling-stockfish-yourself-from-the-sources). The program comes without a graphical user interface, so you perhaps want to use it together with a [compatible GUI](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Graphical-user-interfaces), or [play against it online](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Online) at [pychess](https://www.pychess.org/), [lishogi](https://lishogi.org/@/Fairy-Stockfish), or [lichess](https://lichess.org/@/Fairy-Stockfish). Read more about [how to use](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Usage) Fairy-Stockfish in the wiki.

If you want to preview the functionality of Fairy-Stockfish before downloading, you can try it out on the [Fairy-Stockfish playground](https://fairyground.vercel.app/) in the browser.

Optional NNUE evaluation parameter files to improve playing strength for many variants are in the [list of NNUE networks](https://fairy-stockfish.github.io/nnue/#current-best-nnue-networks).
For the regional variants Xiangqi, Janggi, and Makruk [dedicated releases with built-in NNUE networks](https://github.com/fairy-stockfish/Fairy-Stockfish-NNUE) are available. See the [wiki](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/NNUE) for more details on NNUE.

## Contributing

If you like this project, please support its development via [patreon](https://www.patreon.com/ianfab) or [paypal](https://paypal.me/FairyStockfish), by [contributing CPU time](https://github.com/ianfab/fishtest/wiki) to the framework for testing of code improvements, or by [contributing to the code](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Contributing) or documentation. An [introduction to the code base](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Understanding-the-code) can be found in the wiki.

## Supported games

The games currently supported besides chess are listed below. Fairy-Stockfish can also play user-defined variants loaded via a variant configuration file, see the file [`src/variants.ini`](https://github.com/ianfab/Fairy-Stockfish/blob/master/src/variants.ini) and the [wiki](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki/Variant-configuration).

### Regional and historical games
- [Xiangqi](https://en.wikipedia.org/wiki/Xiangqi), [Manchu](https://en.wikipedia.org/wiki/Manchu_chess), [Minixiangqi](http://mlwi.magix.net/bg/minixiangqi.htm), [Supply chess](https://en.wikipedia.org/wiki/Xiangqi#Variations)
- [Shogi](https://en.wikipedia.org/wiki/Shogi), [Shogi variants](https://github.com/fairy-stockfish/Fairy-Stockfish#shogi-variants)
- [Janggi](https://en.wikipedia.org/wiki/Janggi)
- [Makruk](https://en.wikipedia.org/wiki/Makruk), [ASEAN](http://hgm.nubati.net/rules/ASEAN.html), Makpong, Ai-Wok
- [Ouk Chatrang](https://en.wikipedia.org/wiki/Makruk#Cambodian_chess), [Kar Ouk](https://en.wikipedia.org/wiki/Makruk#Cambodian_chess)
- [Sittuyin](https://en.wikipedia.org/wiki/Sittuyin)
- [Shatar](https://en.wikipedia.org/wiki/Shatar), [Jeson Mor](https://en.wikipedia.org/wiki/Jeson_Mor)
- [Shatranj](https://en.wikipedia.org/wiki/Shatranj), [Courier](https://en.wikipedia.org/wiki/Courier_chess)
- [Raazuvaa](https://www.reddit.com/r/chess/comments/sj7y3n/raazuvaa_the_chess_of_the_maldives)

### Chess variants
- [Capablanca](https://en.wikipedia.org/wiki/Capablanca_Chess), [Janus](https://en.wikipedia.org/wiki/Janus_Chess), [Modern](https://en.wikipedia.org/wiki/Modern_Chess_(chess_variant)), [Chancellor](https://en.wikipedia.org/wiki/Chancellor_Chess), [Embassy](https://en.wikipedia.org/wiki/Embassy_Chess), [Gothic](https://www.chessvariants.com/large.dir/gothicchess.html), [Capablanca random chess](https://en.wikipedia.org/wiki/Capablanca_Random_Chess)
- [Grand](https://en.wikipedia.org/wiki/Grand_Chess), [Shako](https://www.chessvariants.com/large.dir/shako.html), [Centaur](https://www.chessvariants.com/large.dir/contest/royalcourt.html), [Tencubed](https://www.chessvariants.com/contests/10/tencubedchess.html), [Opulent](https://www.chessvariants.com/rules/opulent-chess)
- [Chess960](https://en.wikipedia.org/wiki/Chess960), [Placement/Pre-Chess](https://www.chessvariants.com/link/placement-chess)
- [Crazyhouse](https://en.wikipedia.org/wiki/Crazyhouse), [Loop](https://en.wikipedia.org/wiki/Crazyhouse#Variations), [Chessgi](https://en.wikipedia.org/wiki/Crazyhouse#Variations), [Pocket Knight](http://www.chessvariants.com/other.dir/pocket.html), Capablanca-Crazyhouse
- [Bughouse](https://en.wikipedia.org/wiki/Bughouse_chess), [Koedem](http://schachclub-oetigheim.de/wp-content/uploads/2016/04/Koedem-rules.pdf)
- [Seirawan](https://en.wikipedia.org/wiki/Seirawan_chess), Seirawan-Crazyhouse, [Dragon Chess](https://www.edami.com/dragonchess/)
- [Amazon](https://www.chessvariants.com/diffmove.dir/amazone.html), [Chigorin](https://www.chessvariants.com/diffsetup.dir/chigorin.html), [Almost chess](https://en.wikipedia.org/wiki/Almost_Chess)
- [Hoppel-Poppel](http://www.chessvariants.com/diffmove.dir/hoppel-poppel.html), New Zealand
- [Antichess](https://lichess.org/variant/antichess), [Giveaway](http://www.chessvariants.com/diffobjective.dir/giveaway.old.html), [Suicide](https://www.freechess.org/Help/HelpFiles/suicide_chess.html), [Losers](https://www.chessclub.com/help/Wild17), [Codrus](http://www.binnewirtz.com/Schlagschach1.htm)
- [Extinction](https://en.wikipedia.org/wiki/Extinction_chess), [Kinglet](https://en.wikipedia.org/wiki/V._R._Parton#Kinglet_chess), Three Kings, [Coregal](https://www.chessvariants.com/winning.dir/coregal.html)
- [King of the Hill](https://en.wikipedia.org/wiki/King_of_the_Hill_(chess)), [Racing Kings](https://en.wikipedia.org/wiki/V._R._Parton#Racing_Kings), [Dodo Chess](https://en.wikipedia.org/wiki/V._R._Parton#Dodo_chess)
- [Three-check](https://en.wikipedia.org/wiki/Three-check_chess), Five-check
- [Los Alamos](https://en.wikipedia.org/wiki/Los_Alamos_chess), [Gardner's Minichess](https://en.wikipedia.org/wiki/Minichess#5%C3%975_chess)
- [Atomic](https://en.wikipedia.org/wiki/Atomic_chess)
- [Horde](https://en.wikipedia.org/wiki/Dunsany%27s_Chess#Horde_Chess), [Maharajah and the Sepoys](https://en.wikipedia.org/wiki/Maharajah_and_the_Sepoys)
- [Knightmate](https://www.chessvariants.com/diffobjective.dir/knightmate.html), [Nightrider](https://en.wikipedia.org/wiki/Nightrider_(chess)), [Grasshopper](https://en.wikipedia.org/wiki/Grasshopper_chess)
- [Duck chess](https://duckchess.com/), [Omicron](http://www.eglebbk.dds.nl/program/chess-omicron.html), [Gustav III](https://www.chessvariants.com/play/gustav-iiis-chess)
- [Berolina Chess](https://en.wikipedia.org/wiki/Berolina_pawn#Berolina_chess), [Pawn-Sideways](https://arxiv.org/abs/2009.04374), [Pawn-Back](https://arxiv.org/abs/2009.04374), [Torpedo](https://arxiv.org/abs/2009.04374), [Legan Chess](https://en.wikipedia.org/wiki/Legan_chess)
- [Spartan Chess](https://www.chessvariants.com/rules/spartan-chess)
- [Wolf Chess](https://en.wikipedia.org/wiki/Wolf_chess)
- [Troitzky Chess](https://www.chessvariants.com/play/troitzky-chess)

### Shogi variants
- [Minishogi](https://en.wikipedia.org/wiki/Minishogi), [EuroShogi](https://en.wikipedia.org/wiki/EuroShogi), [Judkins shogi](https://en.wikipedia.org/wiki/Judkins_shogi)
- [Kyoto shogi](https://en.wikipedia.org/wiki/Kyoto_shogi), [Microshogi](https://en.wikipedia.org/wiki/Micro_shogi)
- [Dobutsu shogi](https://en.wikipedia.org/wiki/Dōbutsu_shōgi), [Goro goro shogi](https://en.wikipedia.org/wiki/D%C5%8Dbutsu_sh%C5%8Dgi#Variation)
- [Tori shogi](https://en.wikipedia.org/wiki/Tori_shogi)
- [Yari shogi](https://en.wikipedia.org/wiki/Yari_shogi)
- [Okisaki shogi](https://en.wikipedia.org/wiki/Okisaki_shogi)
- [Sho shogi](https://en.wikipedia.org/wiki/Sho_shogi)

### Related games
- [Amazons](https://en.wikipedia.org/wiki/Game_of_the_Amazons)
- [Ataxx](https://en.wikipedia.org/wiki/Ataxx)
- [Breakthrough](https://en.wikipedia.org/wiki/Breakthrough_(board_game))
- [Clobber](https://en.wikipedia.org/wiki/Clobber)
- [Cfour](https://en.wikipedia.org/wiki/Connect_Four), [Tic-tac-toe](https://en.wikipedia.org/wiki/Tic-tac-toe)
- [Five Field Kono](https://en.wikipedia.org/wiki/Five_Field_Kono)
- [Flipersi](https://en.wikipedia.org/wiki/Reversi), [Flipello](https://en.wikipedia.org/wiki/Reversi#Othello)
- [Fox and Hounds](https://boardgamegeek.com/boardgame/148180/fox-and-hounds)
- [Isolation](https://boardgamegeek.com/boardgame/1875/isolation)
- [Joust](https://www.chessvariants.com/programs.dir/joust.html)
- [Snailtrail](https://boardgamegeek.com/boardgame/37135/snailtrail)


## Help

See the [Fairy-Stockfish Wiki](https://github.com/fairy-stockfish/Fairy-Stockfish/wiki) for more info, or if the required information is not available, open an [issue](https://github.com/fairy-stockfish/Fairy-Stockfish/issues) or join our [discord server](https://discord.gg/FYUGgmCFB4).

## Bindings

Besides the C++ engine, this project also includes bindings for other programming languages in order to be able to use it as a library for chess variants. They support move, SAN, and FEN generation, as well as checking of game end conditions for all variants supported by Fairy-Stockfish. Since the bindings are using the C++ code, they are very performant compared to libraries directly written in the respective target language.

### Python

The python binding [pyffish](https://pypi.org/project/pyffish/) contributed by [@gbtami](https://github.com/gbtami) is implemented in [pyffish.cpp](https://github.com/fairy-stockfish/Fairy-Stockfish/blob/master/src/pyffish.cpp). It is e.g. used in the backend for the [pychess server](https://github.com/gbtami/pychess-variants).

### Javascript

The javascript binding [ffish.js](https://www.npmjs.com/package/ffish) contributed by [@QueensGambit](https://github.com/QueensGambit) is implemented in [ffishjs.cpp](https://github.com/fairy-stockfish/Fairy-Stockfish/blob/master/src/ffishjs.cpp). The compilation/binding to javascript is done using emscripten, see the [readme](https://github.com/fairy-stockfish/Fairy-Stockfish/tree/master/tests/js).

## Ports

### WebAssembly

For in-browser use a [port of Fairy-Stockfish to WebAssembly](https://github.com/fairy-stockfish/fairy-stockfish.wasm) is available at [npm](https://www.npmjs.com/package/fairy-stockfish-nnue.wasm). It is e.g. used for local analysis on [pychess.org](https://www.pychess.org/analysis/chess). Also see the [Fairy-Stockfish WASM demo](https://github.com/ianfab/fairy-stockfish-nnue-wasm-demo) available at https://fairy-stockfish-nnue-wasm.vercel.app/.

# Stockfish
## Overview

[![Build Status](https://github.com/official-stockfish/Stockfish/actions/workflows/stockfish.yml/badge.svg)](https://github.com/official-stockfish/Stockfish/actions)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/official-stockfish/Stockfish?branch=master&svg=true)](https://ci.appveyor.com/project/mcostalba/stockfish/branch/master)

[Stockfish](https://stockfishchess.org) is a free, powerful UCI chess engine
derived from Glaurung 2.1. Stockfish is not a complete chess program and requires a
UCI-compatible graphical user interface (GUI) (e.g. XBoard with PolyGlot, Scid,
Cute Chess, eboard, Arena, Sigma Chess, Shredder, Chess Partner or Fritz) in order
to be used comfortably. Read the documentation for your GUI of choice for information
about how to use Stockfish with it.

The Stockfish engine features two evaluation functions for chess, the classical
evaluation based on handcrafted terms, and the NNUE evaluation based on efficiently
updatable neural networks. The classical evaluation runs efficiently on almost all
CPU architectures, while the NNUE evaluation benefits from the vector
intrinsics available on most CPUs (sse2, avx2, neon, or similar).


## Files

This distribution of Stockfish consists of the following files:

  * [Readme.md](https://github.com/official-stockfish/Stockfish/blob/master/README.md), the file you are currently reading.

  * [Copying.txt](https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt), a text file containing the GNU General Public License version 3.

  * [AUTHORS](https://github.com/official-stockfish/Stockfish/blob/master/AUTHORS), a text file with the list of authors for the project

  * [src](https://github.com/official-stockfish/Stockfish/tree/master/src), a subdirectory containing the full source code, including a Makefile
    that can be used to compile Stockfish on Unix-like systems.

  * a file with the .nnue extension, storing the neural network for the NNUE
    evaluation. Binary distributions will have this file embedded.

## The UCI protocol and available options

The Universal Chess Interface (UCI) is a standard protocol used to communicate with
a chess engine, and is the recommended way to do so for typical graphical user interfaces
(GUI) or chess tools. Stockfish implements the majority of it options as described
in [the UCI protocol](https://www.shredderchess.com/download/div/uci.zip).

Developers can see the default values for UCI options available in Stockfish by typing
`./stockfish uci` in a terminal, but the majority of users will typically see them and
change them via a chess GUI. This is a list of available UCI options in Stockfish:

  * #### Threads
    The number of CPU threads used for searching a position. For best performance, set
    this equal to the number of CPU cores available.

  * #### Hash
    The size of the hash table in MB. It is recommended to set Hash after setting Threads.

  * #### Clear Hash
    Clear the hash table.

  * #### Ponder
    Let Stockfish ponder its next move while the opponent is thinking.

  * #### MultiPV
    Output the N best lines (principal variations, PVs) when searching.
    Leave at 1 for best performance.

  * #### Use NNUE
    Toggle between the NNUE and classical evaluation functions. If set to "true",
    the network parameters must be available to load from file (see also EvalFile),
    if they are not embedded in the binary.

  * #### EvalFile
    The name of the file of the NNUE evaluation parameters. Depending on the GUI the
    filename might have to include the full path to the folder/directory that contains the file.
    Other locations, such as the directory that contains the binary and the working directory,
    are also searched.

  * #### UCI_AnalyseMode
    An option handled by your GUI.

  * #### UCI_Chess960
    An option handled by your GUI. If true, Stockfish will play Chess960.

  * #### UCI_ShowWDL
    If enabled, show approximate WDL statistics as part of the engine output.
    These WDL numbers model expected game outcomes for a given evaluation and
    game ply for engine self-play at fishtest LTC conditions (60+0.6s per game).

  * #### UCI_LimitStrength
    Enable weaker play aiming for an Elo rating as set by UCI_Elo. This option overrides Skill Level.

  * #### UCI_Elo
    If enabled by UCI_LimitStrength, aim for an engine strength of the given Elo.
    This Elo rating has been calibrated at a time control of 60s+0.6s and anchored to CCRL 40/4.

  * #### Skill Level
    Lower the Skill Level in order to make Stockfish play weaker (see also UCI_LimitStrength).
    Internally, MultiPV is enabled, and with a certain probability depending on the Skill Level a
    weaker move will be played.

  * #### SyzygyPath
    Path to the folders/directories storing the Syzygy tablebase files. Multiple
    directories are to be separated by ";" on Windows and by ":" on Unix-based
    operating systems. Do not use spaces around the ";" or ":".

    Example: `C:\tablebases\wdl345;C:\tablebases\wdl6;D:\tablebases\dtz345;D:\tablebases\dtz6`

    It is recommended to store .rtbw files on an SSD. There is no loss in storing
    the .rtbz files on a regular HD. It is recommended to verify all md5 checksums
    of the downloaded tablebase files (`md5sum -c checksum.md5`) as corruption will
    lead to engine crashes.

  * #### SyzygyProbeDepth
    Minimum remaining search depth for which a position is probed. Set this option
    to a higher value to probe less aggressively if you experience too much slowdown
    (in terms of nps) due to tablebase probing.

  * #### Syzygy50MoveRule
    Disable to let fifty-move rule draws detected by Syzygy tablebase probes count
    as wins or losses. This is useful for ICCF correspondence games.

  * #### SyzygyProbeLimit
    Limit Syzygy tablebase probing to positions with at most this many pieces left
    (including kings and pawns).

  * #### Move Overhead
    Assume a time delay of x ms due to network and GUI overheads. This is useful to
    avoid losses on time in those cases.

  * #### Slow Mover
    Lower values will make Stockfish take less time in games, higher values will
    make it think longer.

  * #### nodestime
    Tells the engine to use nodes searched instead of wall time to account for
    elapsed time. Useful for engine testing.

  * #### Debug Log File
    Write all communication to and from the engine into a text file.

For developers the following non-standard commands might be of interest, mainly useful for debugging:

  * #### bench *ttSize threads limit fenFile limitType evalType*
    Performs a standard benchmark using various options. The signature of a version (standard node
    count) is obtained using all defaults. `bench` is currently `bench 16 1 13 default depth mixed`.

  * #### compiler
    Give information about the compiler and environment used for building a binary.

  * #### d
    Display the current position, with ascii art and fen.

  * #### eval
    Return the evaluation of the current position.

  * #### export_net [filename]
    Exports the currently loaded network to a file.
    If the currently loaded network is the embedded network and the filename
    is not specified then the network is saved to the file matching the name
    of the embedded network, as defined in evaluate.h.
    If the currently loaded network is not the embedded network (some net set
    through the UCI setoption) then the filename parameter is required and the
    network is saved into that file.

  * #### flip
    Flips the side to move.


## A note on classical evaluation versus NNUE evaluation

Both approaches assign a value to a position that is used in alpha-beta (PVS) search
to find the best move. The classical evaluation computes this value as a function
of various chess concepts, handcrafted by experts, tested and tuned using fishtest.
The NNUE evaluation computes this value with a neural network based on basic
inputs (e.g. piece positions only). The network is optimized and trained
on the evaluations of millions of positions at moderate search depth.

The NNUE evaluation was first introduced in shogi, and ported to Stockfish afterward.
It can be evaluated efficiently on CPUs, and exploits the fact that only parts
of the neural network need to be updated after a typical chess move.
[The nodchip repository](https://github.com/nodchip/Stockfish) provides additional
tools to train and develop the NNUE networks. On CPUs supporting modern vector instructions
(avx2 and similar), the NNUE evaluation results in much stronger playing strength, even
if the nodes per second computed by the engine is somewhat lower (roughly 80% of nps
is typical).

Notes:

1) the NNUE evaluation depends on the Stockfish binary and the network parameter
file (see the EvalFile UCI option). Not every parameter file is compatible with a given
Stockfish binary, but the default value of the EvalFile UCI option is the name of a network
that is guaranteed to be compatible with that binary.

2) to use the NNUE evaluation, the additional data file with neural network parameters
needs to be available. Normally, this file is already embedded in the binary or it
can be downloaded. The filename for the default (recommended) net can be found as the default
value of the `EvalFile` UCI option, with the format `nn-[SHA256 first 12 digits].nnue`
(for instance, `nn-c157e0a5755b.nnue`). This file can be downloaded from
```
https://tests.stockfishchess.org/api/nn/[filename]
```
replacing `[filename]` as needed.

## What to expect from the Syzygy tablebases?

If the engine is searching a position that is not in the tablebases (e.g.
a position with 8 pieces), it will access the tablebases during the search.
If the engine reports a very large score (typically 153.xx), this means
it has found a winning line into a tablebase position.

If the engine is given a position to search that is in the tablebases, it
will use the tablebases at the beginning of the search to preselect all
good moves, i.e. all moves that preserve the win or preserve the draw while
taking into account the 50-move rule.
It will then perform a search only on those moves. **The engine will not move
immediately**, unless there is only a single good move. **The engine likely
will not report a mate score, even if the position is known to be won.**

It is therefore clear that this behaviour is not identical to what one might
be used to with Nalimov tablebases. There are technical reasons for this
difference, the main technical reason being that Nalimov tablebases use the
DTM metric (distance-to-mate), while the Syzygy tablebases use a variation of the
DTZ metric (distance-to-zero, zero meaning any move that resets the 50-move
counter). This special metric is one of the reasons that the Syzygy tablebases are
more compact than Nalimov tablebases, while still storing all information
needed for optimal play and in addition being able to take into account
the 50-move rule.

## Large Pages

Stockfish supports large pages on Linux and Windows. Large pages make
the hash access more efficient, improving the engine speed, especially
on large hash sizes. Typical increases are 5..10% in terms of nodes per
second, but speed increases up to 30% have been measured. The support is
automatic. Stockfish attempts to use large pages when available and
will fall back to regular memory allocation when this is not the case.

### Support on Linux

Large page support on Linux is obtained by the Linux kernel
transparent huge pages functionality. Typically, transparent huge pages
are already enabled, and no configuration is needed.

### Support on Windows

The use of large pages requires "Lock Pages in Memory" privilege. See
[Enable the Lock Pages in Memory Option (Windows)](https://docs.microsoft.com/en-us/sql/database-engine/configure-windows/enable-the-lock-pages-in-memory-option-windows)
on how to enable this privilege, then run [RAMMap](https://docs.microsoft.com/en-us/sysinternals/downloads/rammap)
to double-check that large pages are used. We suggest that you reboot
your computer after you have enabled large pages, because long Windows
sessions suffer from memory fragmentation, which may prevent Stockfish
from getting large pages: a fresh session is better in this regard.

## Compiling Stockfish yourself from the sources

Stockfish has support for 32 or 64-bit CPUs, certain hardware
instructions, big-endian machines such as Power PC, and other platforms.

On Unix-like systems, it should be easy to compile Stockfish
directly from the source code with the included Makefile in the folder
`src`. In general it is recommended to run `make help` to see a list of make
targets with corresponding descriptions.

```
    cd src
    make help
    make net
    make build ARCH=x86-64-modern
```

When not using the Makefile to compile (for instance, with Microsoft MSVC) you
need to manually set/unset some switches in the compiler command line; see
file *types.h* for a quick reference.

When reporting an issue or a bug, please tell us which Stockfish version
and which compiler you used to create your executable. This information
can be found by typing the following command in a console:

```
    ./stockfish compiler
```

## Understanding the code base and participating in the project

Stockfish's improvement over the last decade has been a great community
effort. There are a few ways to help contribute to its growth.

### Donating hardware

Improving Stockfish requires a massive amount of testing. You can donate
your hardware resources by installing the [Fishtest Worker](https://github.com/glinscott/fishtest/wiki/Running-the-worker:-overview)
and view the current tests on [Fishtest](https://tests.stockfishchess.org/tests).

### Improving the code

If you want to help improve the code, there are several valuable resources:

* [In this wiki,](https://www.chessprogramming.org) many techniques used in
Stockfish are explained with a lot of background information.

* [The section on Stockfish](https://www.chessprogramming.org/Stockfish)
describes many features and techniques used by Stockfish. However, it is
generic rather than being focused on Stockfish's precise implementation.
Nevertheless, a helpful resource.

* The latest source can always be found on [GitHub](https://github.com/official-stockfish/Stockfish).
Discussions about Stockfish take place these days mainly in the [FishCooking](https://groups.google.com/forum/#!forum/fishcooking)
group and on the [Stockfish Discord channel](https://discord.gg/nv8gDtt).
The engine testing is done on [Fishtest](https://tests.stockfishchess.org/tests).
If you want to help improve Stockfish, please read this [guideline](https://github.com/glinscott/fishtest/wiki/Creating-my-first-test)
first, where the basics of Stockfish development are explained.


## Terms of use

Stockfish is free, and distributed under the **GNU General Public License version 3**
(GPL v3). Essentially, this means you are free to do almost exactly
what you want with the program, including distributing it among your
friends, making it available for download from your website, selling
it (either by itself or as part of some bigger software package), or
using it as the starting point for a software project of your own.

The only real limitation is that whenever you distribute Stockfish in
some way, you MUST always include the full source code, or a pointer
to where the source code can be found, to generate the exact binary
you are distributing. If you make any changes to the source code,
these changes must also be made available under the GPL.

For full details, read the copy of the GPL v3 found in the file named
[*Copying.txt*](https://github.com/official-stockfish/Stockfish/blob/master/Copying.txt).
