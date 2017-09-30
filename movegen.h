#ifndef MOVEGEN_H
#define MOVEGEN_H

typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <iostream>
#include <string>
#include <vector>

#include "move.h"

class BitBoards;
class Historys;
class HashEntry;
class searchStack;



class MoveGen
{
public:

	//array of move objects 
	Move moveAr[256];

	SMoves mList[256];

	//number of moves generated this node
    int moveCount;

	template<int genType>
	void generate(const BitBoards& board);

	//grab the best scoring move and return it
	Move movegen_sort(int ply, Move * moveAr) const;

    void reorderMoves(searchStack *ss, const HashEntry *entry);

private:

    //assigns a score to moves and adds them to the move array
    void movegen_push(const BitBoards & board, int color, int piece, int captured, char flag, int from, int to);

    bool blind(const BitBoards & board, int to, int color, int pieceVal, int captureVal);

	template<int color, int Pt> 
	void generateMoves(const BitBoards& board, const U64 &target);


	template<int color, int genType> FORCE_INLINE
	void generateAll(const BitBoards& board, const U64 & target);

	//psuedo legal move gen for indvidual pieces
	template<int color, int genType>
	void pawnMoves(const BitBoards & boards, U64 target);

	template<int color, int cs>
	void castling(const BitBoards & boards);
};



inline Move MoveGen::movegen_sort(int ply, Move * moveAr) const
{
	int best = -INF;
	int high;
	//find best scoring move
	for (int i = 0; i < moveCount; ++i) {
		if (moveAr[i].score > best && !moveAr[i].tried) {
			high = i;
			best = moveAr[i].score;
		}
	}
	//mark best scoring move tried since we're about to try it
	//~~~ change later if we don't always try move on return
	moveAr[high].tried = true;

	return moveAr[high];
}


#endif // MOVEGEN_H
