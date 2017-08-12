#ifndef HASHENTRY_H
#define HASHENTRY_H
#include <string>
typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL
#include "move.h"


class HashEntry
{
public:
    HashEntry();
    //Z key for move
    U64 zobrist;
    //depth of move
    int depth;
    //move rating
    int eval;
    Move move;
    //flag denoting what the hash represents, i.e. alpha 1, beta 2, exact evaluation (between alpha beta) 3
    int flag;
    //how old move is maybe make a bool so move can be replaced
    int ancient;


    //color ~~ to be removed later when search is switched to PV or negamax, false for white
    //bool color;

private:



};

#endif // HASHENTRY_H
