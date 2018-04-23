#ifndef ZOBRISTH_H
#define ZOBRISTH_H
#include <string>
#include "defines.h"


class BitBoards;
class MoveGen;

class ZobristH
{
public:
    //zobrist key for object
    U64 zobristKey;
	//array for random numbers used to gen zobrist key
	U64 zArray[2][7][64];
	//denotes a castling has taken place for zobrist key
	U64 zCastle[CASTLING_RIGHTS];
	//used to change color of move
	U64 zBlackMove;

    U64 zEnPassant[8]; 

    //generate unsigned 64 bit ints for hash mapping
    U64 random64();

    //fill zorbist array with random 64bit numbers
    void zobristFill();

    //get zorbist key by XOR ing all pieces random numbers with Zkey
    U64 getZobristHash(const BitBoards& BBBoard);

    //Update zobrist key by XOR ing rand numbers in zArray
    void UpdateColor();

    //print out how many times a number in the array is created
    //use excel to graph results
    void testDistibution();

    //used for checking if keys match when generated from scratch ~~ without changing master key
    U64 debugKey(bool isWhite, const BitBoards& BBBoard);
	U64 debugPawnKey(const BitBoards& BBBoard);
	U64 debugMaterialKey(const BitBoards& BBBoard);

};

#endif // ZOBRISTH_H
