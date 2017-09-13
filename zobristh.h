#ifndef ZOBRISTH_H
#define ZOBRISTH_H
#include <string>
#include "move.h"



class BitBoards;
class MoveGen;
class Move;

class ZobristH
{
public:
    //zobrist key for object
    U64 zobristKey;
	//array for random numbers used to gen zobrist key
	U64 zArray[2][7][64];
	//denotes a castling has taken place for zobrist key
	U64 zCastle[4];
	//used to change color of move
	U64 zBlackMove;
    //U64 zEnPassasnt[8]; ~~restore once implemented

    //generate unsigned 64 bit ints for hash mapping
    U64 random64();

    //fill zorbist array with random 64bit numbers
    void zobristFill();

    //get zorbist key by XOR ing all pieces random numbers with Zkey
    U64 getZobristHash(const BitBoards& BBBoard);

	//used for prefetching a very probable match to the next transposition entry
	U64 fetchKey(const Move& m, int color);

    //Update zobrist key by XOR ing rand numbers in zArray
    void UpdateKey(int start, int end, const Move& moveKey, bool isWhite);
    void UpdateColor();

    //print out how many times a number in the array is created
    //use excel to graph results
    void testDistibution();

    //used for checking if keys match when generated from scratch ~~ without changing master key
    U64 debugKey(bool isWhite, const BitBoards& BBBoard);
	U64 debugPawnKey(const BitBoards& BBBoard);
	U64 debugMaterialKey(const BitBoards& BBBoard);

};

inline void ZobristH::UpdateKey(int start, int end, const Move& moveKey, bool isWhite)
{
	//update the zobrist key after move or unmake move.
	//color is messed up, 0 = white. Hence we inverse the is white to get actual color
	int color = !isWhite;

	zobristKey ^= zArray[color][moveKey.piece][moveKey.from];
	zobristKey ^= zArray[color][moveKey.piece][moveKey.to];

	zobristKey ^= zArray[!color][moveKey.captured][moveKey.to];

	//pawn promotions to queen, other not implemented
	if (moveKey.flag == 'Q') {
		zobristKey ^= zArray[color][PAWN][moveKey.to];
		zobristKey ^= zArray[color][QUEEN][moveKey.to];
	}

	//need caslting code
}

#endif // ZOBRISTH_H
