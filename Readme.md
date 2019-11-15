# Fairy-Stockfish

## Overview

[![Build Status](https://travis-ci.org/ianfab/Fairy-Stockfish.svg?branch=master)](https://travis-ci.org/ianfab/Fairy-Stockfish)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/ianfab/Fairy-Stockfish?branch=master&svg=true)](https://ci.appveyor.com/project/ianfab/Fairy-Stockfish/branch/master)
[![PyPI version](https://badge.fury.io/py/pyffish.svg)](https://badge.fury.io/py/pyffish)

Fairy-Stockfish is a UCI/UCCI/USI/XBoard chess variant engine derived from [Stockfish](https://github.com/official-stockfish/Stockfish/) designed for the support of fairy chess variants and easy extensibility with more games. It can play various historical, regional, and modern chess variants as well [games with user-defined rules](https://github.com/ianfab/Fairy-Stockfish/wiki/Variant-configuration).

The goal of the project is to create an engine supporting a large variety of chess-like games, equipped with the powerful search of Stockfish. It is complementary to Stockfish forks more specialized for certain chess variants, such as [multi-variant Stockfish](https://github.com/ddugovic/Stockfish), [Seirawan-Stockfish](https://github.com/ianfab/Seirawan-Stockfish), [Makruk-Stockfish](https://github.com/ianfab/Makruk-Stockfish), etc., supporting many more variants with the tradeoff of slightly lower performance compared to a specialized implementation.

## Supported games

The games currently supported besides chess are listed below. Fairy-Stockfish can also play user-defined variants loaded via a variant configuration file, see the file `src/variants.ini`.

### Regional and historical games
- [Xiangqi](https://en.wikipedia.org/wiki/Xiangqi), [Minixiangqi](http://mlwi.magix.net/bg/minixiangqi.htm)
- [Shogi](https://en.wikipedia.org/wiki/Shogi)
- [Makruk](https://en.wikipedia.org/wiki/Makruk), [Ouk Chatrang](https://en.wikipedia.org/wiki/Makruk#Cambodian_chess), [Kar Ouk](https://en.wikipedia.org/wiki/Makruk#Cambodian_chess), [ASEAN](http://hgm.nubati.net/rules/ASEAN.html), Ai-Wok
- [Sittuyin](https://en.wikipedia.org/wiki/Sittuyin)
- [Shatar](https://en.wikipedia.org/wiki/Shatar), [Jeson Mor](https://en.wikipedia.org/wiki/Jeson_Mor)
- [Shatranj](https://en.wikipedia.org/wiki/Shatranj), [Courier](https://en.wikipedia.org/wiki/Courier_chess)

### Chess variants
- [Capablanca](https://en.wikipedia.org/wiki/Capablanca_Chess), [Janus](https://en.wikipedia.org/wiki/Janus_Chess), [Modern](https://en.wikipedia.org/wiki/Modern_Chess_(chess_variant)), [Chancellor](https://en.wikipedia.org/wiki/Chancellor_Chess), [Embassy](https://en.wikipedia.org/wiki/Embassy_Chess), [Gothic](https://www.chessvariants.com/large.dir/gothicchess.html), [Capablanca random chess](https://en.wikipedia.org/wiki/Capablanca_Random_Chess)
- [Grand](https://en.wikipedia.org/wiki/Grand_Chess), [Shako](https://www.chessvariants.com/large.dir/shako.html), [Centaur](https://www.chessvariants.com/large.dir/contest/royalcourt.html)
- [Chess960](https://en.wikipedia.org/wiki/Chess960), [Placement/Pre-Chess](https://www.chessvariants.com/link/placement-chess)
- [Crazyhouse](https://en.wikipedia.org/wiki/Crazyhouse), [Loop](https://en.wikipedia.org/wiki/Crazyhouse#Variations), [Chessgi](https://en.wikipedia.org/wiki/Crazyhouse#Variations), [Pocket Knight](http://www.chessvariants.com/other.dir/pocket.html), Capablanca-Crazyhouse
- [Seirawan](https://en.wikipedia.org/wiki/Seirawan_chess), Seirawan-Crazyhouse
- [Amazon](https://en.wikipedia.org/wiki/Amazon_(chess)), [Chigorin](https://en.wikipedia.org/wiki/Chigorin_Chess), [Almost chess](https://en.wikipedia.org/wiki/Almost_Chess)
- [Hoppel-Poppel](http://www.chessvariants.com/diffmove.dir/hoppel-poppel.html), New Zealand
- [Antichess](https://lichess.org/variant/antichess), [Giveaway](http://www.chessvariants.com/diffobjective.dir/giveaway.old.html), [Losers](https://www.chessclub.com/help/Wild17), [Codrus](http://www.binnewirtz.com/Schlagschach1.htm)
- [Extinction](https://en.wikipedia.org/wiki/Extinction_chess), [Kinglet](https://en.wikipedia.org/wiki/V._R._Parton#Kinglet_Chess)
- [King of the Hill](https://en.wikipedia.org/wiki/King_of_the_Hill_(chess)), [Racing Kings](https://en.wikipedia.org/wiki/V._R._Parton#Racing_Kings)
- [Three-check](https://en.wikipedia.org/wiki/Three-check_chess), Five-check
- [Los Alamos](https://en.wikipedia.org/wiki/Los_Alamos_chess)
- [Horde](https://en.wikipedia.org/wiki/Dunsany%27s_Chess#Horde_Chess)

### Shogi variants
- [Minishogi](https://en.wikipedia.org/wiki/Minishogi), [EuroShogi](https://en.wikipedia.org/wiki/EuroShogi), [Judkins shogi](https://en.wikipedia.org/wiki/Judkins_shogi)
- [Kyoto shogi](https://en.wikipedia.org/wiki/Kyoto_shogi), [Microshogi](https://en.wikipedia.org/wiki/Micro_shogi)
- [Dobutsu shogi](https://en.wikipedia.org/wiki/Dōbutsu_shōgi), [Goro goro shogi](https://en.wikipedia.org/wiki/D%C5%8Dbutsu_sh%C5%8Dgi#Variation)

### Related games
- [Breakthrough](https://en.wikipedia.org/wiki/Breakthrough_(board_game))
- [Clobber](https://en.wikipedia.org/wiki/Clobber)

## UCI parameters

The following UCI options are added or changed compared to official Stockfish:

  * #### UCI_Variant
    The most important option, since it allows to set the variant that is going to be played.
    Most GUIs for chess variants set this option automatically if you select a variant in the GUI,
    but in case of usage via CLI or via Shogi GUIs the option needs to be set manually.

  * #### UCI_Chess960
    Used as a universal flag to switch between Chess960-style shuffling variants (such as Crazyhouse960,
    S-Chess960/2880, Capablanca Random Chess, etc.) and the respective normal variant.
    For variants without castling, this option does not have any effect.

  * #### Protocol
    Can be used to switch between the supported protocols, namely UCI, UCCI (Xiangqi), USI (Shogi) and XBoard/CECP.
    This option is automatically set to the respective protocol if a GUI starts communication
    with one of the `uci`/`ucci`/`usi`/`xboard` commands (as required by the protocols).

  * #### VariantPath
    The path to the configuration file for user-defined variants.
    Alternatively, the "load" command can be used.

## Help

See the [Fairy-Stockfish Wiki](https://github.com/ianfab/Fairy-Stockfish/wiki) for more info, or if the required information is not available, open an [issue](https://github.com/ianfab/Fairy-Stockfish/issues).

# Stockfish
## Overview

[![Build Status](https://travis-ci.org/official-stockfish/Stockfish.svg?branch=master)](https://travis-ci.org/official-stockfish/Stockfish)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/official-stockfish/Stockfish?branch=master&svg=true)](https://ci.appveyor.com/project/mcostalba/stockfish/branch/master)

[Stockfish](https://stockfishchess.org) is a free, powerful UCI chess engine
derived from Glaurung 2.1. It is not a complete chess program and requires a
UCI-compatible GUI (e.g. XBoard with PolyGlot, Scid, Cute Chess, eboard, Arena,
Sigma Chess, Shredder, Chess Partner or Fritz) in order to be used comfortably.
Read the documentation for your GUI of choice for information about how to use
Stockfish with it.


## Files

This distribution of Stockfish consists of the following files:

  * Readme.md, the file you are currently reading.

  * Copying.txt, a text file containing the GNU General Public License version 3.

  * src, a subdirectory containing the full source code, including a Makefile
    that can be used to compile Stockfish on Unix-like systems.


## UCI parameters

Currently, Stockfish has the following UCI options:

  * #### Debug Log File
    Write all communication to and from the engine into a text file.

  * #### Contempt
    A positive value for contempt favors middle game positions and avoids draws.

  * #### Analysis Contempt
    By default, contempt is set to prefer the side to move. Set this option to "White"
    or "Black" to analyse with contempt for that side, or "Off" to disable contempt.

  * #### Threads
    The number of CPU threads used for searching a position. For best performance, set
    this equal to the number of CPU cores available.

  * #### Hash
    The size of the hash table in MB.

  * #### Clear Hash
    Clear the hash table.

  * #### Ponder
    Let Stockfish ponder its next move while the opponent is thinking.

  * #### MultiPV
    Output the N best lines (principal variations, PVs) when searching.
    Leave at 1 for best performance.

  * #### Skill Level
    Lower the Skill Level in order to make Stockfish play weaker (see also UCI_LimitStrength).
    Internally, MultiPV is enabled, and with a certain probability depending on the Skill Level a
    weaker move will be played.

  * #### UCI_LimitStrength
    Enable weaker play aiming for an Elo rating as set by UCI_Elo. This option overrides Skill Level.

  * #### UCI_Elo
    If enabled by UCI_LimitStrength, aim for an engine strength of the given Elo.
    This Elo rating has been calibrated at a time control of 60s+0.6s and anchored to CCRL 40/4.

  * #### Move Overhead
    Assume a time delay of x ms due to network and GUI overheads. This is useful to
    avoid losses on time in those cases.

  * #### Minimum Thinking Time
    Search for at least x ms per move.

  * #### Slow Mover
    Lower values will make Stockfish take less time in games, higher values will
    make it think longer.

  * #### nodestime
    Tells the engine to use nodes searched instead of wall time to account for
    elapsed time. Useful for engine testing.

  * #### UCI_Chess960
    An option handled by your GUI. If true, Stockfish will play Chess960.

  * #### UCI_AnalyseMode
    An option handled by your GUI.

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
    to a higher value to probe less agressively if you experience too much slowdown
    (in terms of nps) due to TB probing.

  * #### Syzygy50MoveRule
    Disable to let fifty-move rule draws detected by Syzygy tablebase probes count
    as wins or losses. This is useful for ICCF correspondence games.

  * #### SyzygyProbeLimit
    Limit Syzygy tablebase probing to positions with at most this many pieces left
    (including kings and pawns).


## What to expect from Syzygybases?

If the engine is searching a position that is not in the tablebases (e.g.
a position with 8 pieces), it will access the tablebases during the search.
If the engine reports a very large score (typically 153.xx), this means
that it has found a winning line into a tablebase position.

If the engine is given a position to search that is in the tablebases, it
will use the tablebases at the beginning of the search to preselect all
good moves, i.e. all moves that preserve the win or preserve the draw while
taking into account the 50-move rule.
It will then perform a search only on those moves. **The engine will not move
immediately**, unless there is only a single good move. **The engine likely
will not report a mate score even if the position is known to be won.**

It is therefore clear that this behaviour is not identical to what one might
be used to with Nalimov tablebases. There are technical reasons for this
difference, the main technical reason being that Nalimov tablebases use the
DTM metric (distance-to-mate), while Syzygybases use a variation of the
DTZ metric (distance-to-zero, zero meaning any move that resets the 50-move
counter). This special metric is one of the reasons that Syzygybases are
more compact than Nalimov tablebases, while still storing all information
needed for optimal play and in addition being able to take into account
the 50-move rule.


## Compiling Stockfish yourself from the sources

On Unix-like systems, it should be possible to compile Stockfish
directly from the source code with the included Makefile.

Stockfish has support for 32 or 64-bit CPUs, the hardware POPCNT
instruction, big-endian machines such as Power PC, and other platforms.

In general it is recommended to run `make help` to see a list of make
targets with corresponding descriptions. When not using the Makefile to
compile (for instance with Microsoft MSVC) you need to manually
set/unset some switches in the compiler command line; see file *types.h*
for a quick reference.


## Understanding the code base and participating in the project

Stockfish's improvement over the last couple of years has been a great
community effort. There are a few ways to help contribute to its growth.

### Donating hardware

Improving Stockfish requires a massive amount of testing. You can donate
your hardware resources by installing the [Fishtest Worker](https://github.com/glinscott/fishtest/wiki/Running-the-worker)
and view the current tests on [Fishtest](http://tests.stockfishchess.org/tests).

### Improving the code

If you want to help improve the code, there are several valuable ressources:

* [In this wiki,](https://www.chessprogramming.org) many techniques used in
Stockfish are explained with a lot of background information.

* [The section on Stockfish](https://www.chessprogramming.org/Stockfish)
describes many features and techniques used by Stockfish. However, it is
generic rather than being focused on Stockfish's precise implementation.
Nevertheless, a helpful resource.

* The latest source can always be found on [GitHub](https://github.com/official-stockfish/Stockfish).
Discussions about Stockfish take place in the [FishCooking](https://groups.google.com/forum/#!forum/fishcooking)
group and engine testing is done on [Fishtest](http://tests.stockfishchess.org/tests).
If you want to help improve Stockfish, please read this [guideline](https://github.com/glinscott/fishtest/wiki/Creating-my-first-test)
first, where the basics of Stockfish development are explained.


## Terms of use

Stockfish is free, and distributed under the **GNU General Public License version 3**
(GPL v3). Essentially, this means that you are free to do almost exactly
what you want with the program, including distributing it among your
friends, making it available for download from your web site, selling
it (either by itself or as part of some bigger software package), or
using it as the starting point for a software project of your own.

The only real limitation is that whenever you distribute Stockfish in
some way, you must always include the full source code, or a pointer
to where the source code can be found. If you make any changes to the
source code, these changes must also be made available under the GPL.

For full details, read the copy of the GPL v3 found in the file named
*Copying.txt*.
