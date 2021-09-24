# Fairy-Stockfish

## Overview

[![Build Status](https://github.com/ianfab/Fairy-Stockfish/workflows/build/badge.svg?branch=master)](https://github.com/ianfab/Fairy-Stockfish/actions?query=workflow%3Abuild)
[![Build Status](https://github.com/ianfab/Fairy-Stockfish/workflows/fairy/badge.svg?branch=master)](https://github.com/ianfab/Fairy-Stockfish/actions?query=workflow%3Afairy)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/ianfab/Fairy-Stockfish?branch=master&svg=true)](https://ci.appveyor.com/project/ianfab/Fairy-Stockfish/branch/master)
[![PyPI version](https://badge.fury.io/py/pyffish.svg)](https://badge.fury.io/py/pyffish)
[![NPM version](https://img.shields.io/npm/v/ffish.svg?sanitize=true)](https://www.npmjs.com/package/ffish)

Fairy-Stockfish is a chess variant engine derived from [Stockfish](https://github.com/official-stockfish/Stockfish/) designed for the support of fairy chess variants and easy extensibility with more games. It can play various regional, historical, and modern chess variants as well as [games with user-defined rules](https://github.com/ianfab/Fairy-Stockfish/wiki/Variant-configuration). For [compatibility with graphical user interfaces](https://github.com/ianfab/Fairy-Stockfish/wiki/Graphical-user-interfaces) it supports the UCI, UCCI, USI, UCI-cyclone, and CECP/XBoard protocols.

The goal of the project is to create an engine supporting a large variety of chess-like games, equipped with the powerful search of Stockfish. Despite its generality the [playing strength](https://github.com/ianfab/Fairy-Stockfish/wiki/Playing-strength) is on a very high level in almost all supported variants. Due to its multi-protocol support Fairy-Stockfish works with almost any chess variant GUI.

## Installation
You can download the [Windows executable](https://github.com/ianfab/Fairy-Stockfish/releases/latest/download/fairy-stockfish-largeboard_x86-64.exe) or [Linux binary](https://github.com/ianfab/Fairy-Stockfish/releases/latest/download/fairy-stockfish-largeboard_x86-64) from the [latest release](https://github.com/ianfab/Fairy-Stockfish/releases/latest) or [compile the program from source](https://github.com/ianfab/Fairy-Stockfish#compiling-stockfish-yourself-from-the-sources). The program comes without a graphical user interface, so you perhaps want to use it together with a [compatible GUI](https://github.com/ianfab/Fairy-Stockfish/wiki/Graphical-user-interfaces), or [play against it online](https://github.com/ianfab/Fairy-Stockfish/wiki/Online) at [pychess](https://www.pychess.org/), [lishogi](https://lishogi.org/), [xichess](http://www.xichess.com/), or [lichess](https://lichess.org/@/Fairy-Stockfish). Read more about [how to use](https://github.com/ianfab/Fairy-Stockfish/wiki/Usage) Fairy-Stockfish in the wiki.

Optional NNUE evaluation parameter files to improve playing strength for many variants can be obtained via [patreon](https://www.patreon.com/ianfab), also see the [variant NNUE overview](https://docs.google.com/spreadsheets/d/1fgGvBKleUOI1wZZbiNhpT98qhQ7mYuQ5kar1lTQ9g3w/edit?usp=sharing).
See the [wiki](https://github.com/ianfab/Fairy-Stockfish/wiki/NNUE) for more details on NNUE.

## Contributing

If you like this project, please support its development via [patreon](https://www.patreon.com/ianfab) or [paypal](https://paypal.me/FairyStockfish), by [contributing CPU time](https://github.com/ianfab/fishtest/wiki) to the framework for testing of code improvements, or by [contributing to the code](https://github.com/ianfab/Fairy-Stockfish/wiki/Contributing) or documentation. An [introduction to the code base](https://github.com/ianfab/Fairy-Stockfish/wiki/Understanding-the-code) can be found in the wiki.

## Supported games

The games currently supported besides chess are listed below. Fairy-Stockfish can also play user-defined variants loaded via a variant configuration file, see the file [`src/variants.ini`](https://github.com/ianfab/Fairy-Stockfish/blob/master/src/variants.ini) and the [wiki](https://github.com/ianfab/Fairy-Stockfish/wiki/Variant-configuration).

### Regional and historical games
- [Xiangqi](https://en.wikipedia.org/wiki/Xiangqi), [Manchu](https://en.wikipedia.org/wiki/Manchu_chess), [Minixiangqi](http://mlwi.magix.net/bg/minixiangqi.htm), [Supply chess](https://en.wikipedia.org/wiki/Xiangqi#Variations)
- [Shogi](https://en.wikipedia.org/wiki/Shogi), [Shogi variants](https://github.com/ianfab/Fairy-Stockfish#shogi-variants)
- [Janggi](https://en.wikipedia.org/wiki/Janggi)
- [Makruk](https://en.wikipedia.org/wiki/Makruk), [ASEAN](http://hgm.nubati.net/rules/ASEAN.html), Makpong, Ai-Wok
- [Ouk Chatrang](https://en.wikipedia.org/wiki/Makruk#Cambodian_chess), [Kar Ouk](https://en.wikipedia.org/wiki/Makruk#Cambodian_chess)
- [Sittuyin](https://en.wikipedia.org/wiki/Sittuyin)
- [Shatar](https://en.wikipedia.org/wiki/Shatar), [Jeson Mor](https://en.wikipedia.org/wiki/Jeson_Mor)
- [Shatranj](https://en.wikipedia.org/wiki/Shatranj), [Courier](https://en.wikipedia.org/wiki/Courier_chess)

### Chess variants
- [Capablanca](https://en.wikipedia.org/wiki/Capablanca_Chess), [Janus](https://en.wikipedia.org/wiki/Janus_Chess), [Modern](https://en.wikipedia.org/wiki/Modern_Chess_(chess_variant)), [Chancellor](https://en.wikipedia.org/wiki/Chancellor_Chess), [Embassy](https://en.wikipedia.org/wiki/Embassy_Chess), [Gothic](https://www.chessvariants.com/large.dir/gothicchess.html), [Capablanca random chess](https://en.wikipedia.org/wiki/Capablanca_Random_Chess)
- [Grand](https://en.wikipedia.org/wiki/Grand_Chess), [Shako](https://www.chessvariants.com/large.dir/shako.html), [Centaur](https://www.chessvariants.com/large.dir/contest/royalcourt.html), [Tencubed](https://www.chessvariants.com/contests/10/tencubedchess.html), [Opulent](https://www.chessvariants.com/rules/opulent-chess)
- [Chess960](https://en.wikipedia.org/wiki/Chess960), [Placement/Pre-Chess](https://www.chessvariants.com/link/placement-chess)
- [Crazyhouse](https://en.wikipedia.org/wiki/Crazyhouse), [Loop](https://en.wikipedia.org/wiki/Crazyhouse#Variations), [Chessgi](https://en.wikipedia.org/wiki/Crazyhouse#Variations), [Pocket Knight](http://www.chessvariants.com/other.dir/pocket.html), Capablanca-Crazyhouse
- [Bughouse](https://en.wikipedia.org/wiki/Bughouse_chess), [Koedem](http://schachclub-oetigheim.de/wp-content/uploads/2016/04/Koedem-rules.pdf)
- [Seirawan](https://en.wikipedia.org/wiki/Seirawan_chess), Seirawan-Crazyhouse
- [Amazon](https://www.chessvariants.com/diffmove.dir/amazone.html), [Chigorin](https://www.chessvariants.com/diffsetup.dir/chigorin.html), [Almost chess](https://en.wikipedia.org/wiki/Almost_Chess)
- [Hoppel-Poppel](http://www.chessvariants.com/diffmove.dir/hoppel-poppel.html), New Zealand
- [Antichess](https://lichess.org/variant/antichess), [Giveaway](http://www.chessvariants.com/diffobjective.dir/giveaway.old.html), [Suicide](https://www.freechess.org/Help/HelpFiles/suicide_chess.html), [Losers](https://www.chessclub.com/help/Wild17), [Codrus](http://www.binnewirtz.com/Schlagschach1.htm)
- [Extinction](https://en.wikipedia.org/wiki/Extinction_chess), [Kinglet](https://en.wikipedia.org/wiki/V._R._Parton#Kinglet_chess), Three Kings, [Coregal](https://www.chessvariants.com/winning.dir/coregal.html)
- [King of the Hill](https://en.wikipedia.org/wiki/King_of_the_Hill_(chess)), [Racing Kings](https://en.wikipedia.org/wiki/V._R._Parton#Racing_Kings)
- [Three-check](https://en.wikipedia.org/wiki/Three-check_chess), Five-check
- [Los Alamos](https://en.wikipedia.org/wiki/Los_Alamos_chess), [Gardner's Minichess](https://en.wikipedia.org/wiki/Minichess#5%C3%975_chess)
- [Atomic](https://en.wikipedia.org/wiki/Atomic_chess)
- [Horde](https://en.wikipedia.org/wiki/Dunsany%27s_Chess#Horde_Chess), [Maharajah and the Sepoys](https://en.wikipedia.org/wiki/Maharajah_and_the_Sepoys)
- [Knightmate](https://www.chessvariants.com/diffobjective.dir/knightmate.html), [Nightrider](https://en.wikipedia.org/wiki/Nightrider_(chess)), [Grasshopper](https://en.wikipedia.org/wiki/Grasshopper_chess)

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
- [Flipersi](https://en.wikipedia.org/wiki/Reversi), [Flipello](https://en.wikipedia.org/wiki/Reversi#Othello)

## Help

See the [Fairy-Stockfish Wiki](https://github.com/ianfab/Fairy-Stockfish/wiki) for more info, or if the required information is not available, open an [issue](https://github.com/ianfab/Fairy-Stockfish/issues) or join our [discord server](https://discord.gg/FYUGgmCFB4).

## Bindings

Besides the C++ engine, this project also includes bindings for other programming languages in order to be able to use it as a library for chess variants. They support move, SAN, and FEN generation, as well as checking of game end conditions for all variants supported by Fairy-Stockfish. Since the bindings are using the C++ code, they are very performant compared to libraries directly written in the respective target language.  

### Python

The python binding [pyffish](https://pypi.org/project/pyffish/) contributed by [@gbtami](https://github.com/gbtami) is implemented in [pyffish.cpp](https://github.com/ianfab/Fairy-Stockfish/blob/master/src/pyffish.cpp). It is e.g. used in the backend for the [pychess server](https://github.com/gbtami/pychess-variants).

### Javascript

The javascript binding ffish.js contributed by [@QueensGambit](https://github.com/QueensGambit) is implemented in [ffishjs.cpp](https://github.com/ianfab/Fairy-Stockfish/blob/master/src/ffishjs.cpp). The compilation/binding to javascript is done using emscripten, see the [readme](https://github.com/ianfab/Fairy-Stockfish/tree/master/tests/js).

## Ports

### WASM

A port of Fairy-Stockfish to WebAssembly is maintained at https://github.com/ianfab/stockfish.wasm.
