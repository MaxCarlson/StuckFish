#ifndef MOVEGEN_H
#define MOVEGEN_H

typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <iostream>
#include <string>
#include "defines.h"


class BitBoards;
class Historys;
class HashEntry;
class searchStack;

template<int genType>
SMove* generate(const BitBoards & board, SMove *mlist);

class MoveGen // defunct class, delete once everything else is working
{
public:


private:

};



#endif // MOVEGEN_H
