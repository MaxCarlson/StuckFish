#ifndef ZOBRISTH_H
#define ZOBRISTH_H
#include <string>


typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

class BitBoards;
class MoveGen;
class Move;

class ZobristH
{
public:
    ZobristH();

    //zobrist key for object
    U64 zobristKey;
    //U64 zEnPassasnt[8]; ~~restore once implemented
    //U64 zCastle[4];

    //generate unsigned 64 bit ints for hash mapping
    U64 random64();

    //fill zorbist array with random 64bit numbers
    void zobristFill();

    //get zorbist key by XOR ing all pieces random numbers with Zkey
    U64 getZobristHash(const BitBoards& BBBoard);

	//used for prefetching a transposition entry
	U64 fetchKey(const Move& m, bool isWhite);

    //Update zobrist key by XOR ing rand numbers in zArray
    void UpdateKey(int start, int end, const Move& moveKey, bool isWhite);
    void UpdateColor();
    void UpdateNull();

    //print out how many times a number in the array is created
    //use excel to graph results
    void testDistibution();

    //used for checking if keys match when generated from scratch ~~ without changing master key
    U64 debugKey(bool isWhite, const BitBoards& BBBoard);

};

#endif // ZOBRISTH_H
