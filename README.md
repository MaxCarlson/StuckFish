# StuckFish
A UCI chess engine and learning expereince. Based on CPW Engine as well as StockFish. Converted from my Advanced-Qt-Chess repo, and before that my chess repo.


### Demo
60 seconds per player match between StuckFish and 
![Video of Match](https://raw.githubusercontent.com/MaxCarlson/StuckFish/master/Images/StuckFish.webm)


### Search specific features include:

* Principal Variation search
* Iterative deepening

* Null moves search

* Lazy SMP

* Futility pruning

* Late move reductions

* History heuristics with butterfly boards

* Counter move historys

* Mate distance pruning

* and many more things!


### UCI specific features include:

* Setting hashtable size

* Clearing the hashtable

* Setting up a position from a FEN string

* Setting the number of threads the engine will use


### Misc Features

"Perfect" multi-threaded perft and divide functions tested to (mostly) max depth of positions found [here](https://chessprogramming.wikispaces.com/Perft+Results#Initial%20Position-Perft%2013)

Also includes a divide utility (like perft) which prints the number of positions found for each root move of the current position up to a given input depth.


**StuckFish is still very much a work in progress!**



Lot's of credit due to Stockfish, CPW-engine and Chess Programing Wiki which you can find below.

Stockfish:              https://github.com/official-stockfish

CPW-Engine:             https://github.com/nescitus/cpw-engine

Chess Programming Wiki: https://chessprogramming.wikispaces.com/

