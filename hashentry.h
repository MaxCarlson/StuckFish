#ifndef HASHENTRY_H
#define HASHENTRY_H
#include <string>

#include "move.h"


class HashEntry
{
public:
    HashEntry();
    //Z key for move
    U64 zobrist;
    //depth of move
    U8 depth;
    //move rating
    S16 eval;
	//move itself
    Move move;
    //flag denoting what the hash represents, i.e. alpha 1, beta 2, exact evaluation (between alpha beta) 3
    U8 flag;
};

#endif // HASHENTRY_H
