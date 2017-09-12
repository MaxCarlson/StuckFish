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
    //MoveGen();

	//array of move objects 
	Move moveAr[256];

	//number of moves generated this node
    int moveCount;

	// called from search, calls individual move functions 
	// and fills array above with moves with each function
	// pushing moves to movegen_push which stores info and scores with BLIND
    void generatePsMoves(const BitBoards& boards, bool isWhite, bool capturesOnly);

	bool isLegal(const BitBoards & b, const Move & m, bool isWhite);

	//helper function for isLegal + for finding if in check
	bool isSquareAttacked(const BitBoards & b, const int square, const int color);
	
	//static exhange eval
	int SEE(const Move& m, const BitBoards& b, bool isWhite, bool isCapture);

	//grab the best scoring move and return it
	inline Move movegen_sort(int ply, Move * moveAr) const;

    void reorderMoves(searchStack *ss, const HashEntry *entry);

private:

	int whichPieceCaptured(const BitBoards& board, int location, int color);

    //assigns a score to moves and adds them to the move array
    void movegen_push(const BitBoards & board, int color, int piece, int captured, char flag, int from, int to);
    bool blind(const BitBoards & board, int to, int color, int pieceVal, int captureVal);

	//used in static exchange eval
	//U64 attackersTo(int sq, const BitBoards& b, const U64 occ) const;

	U64 attackersTo(const BitBoards & b, const U64 occ, int square);

	//finds the smallest attacker for side to move, out of the stm attackers board,
	//removes the attacker from the attackers and occuied board, then finds any x-ray attackers behind that piece
	//and returns the int representing the piece

	template<int Pt> FORCE_INLINE
	int min_attacker(const BitBoards & b, int color, const int & to, const U64 & stmAttackers, U64 & occupied, U64 & attackers);

    //psuedo legal move gen for indvidual pieces
	template<int color>
	void pawnMoves(const BitBoards & boards, bool capturesOnly);

    void possibleN(const BitBoards& board, int color, const U64 &capturesOnly);
    void possibleB(const BitBoards& board, int color, const U64 &capturesOnly);
    void possibleR(const BitBoards& board, int color, const U64 &capturesOnly);
    void possibleQ(const BitBoards& board, int color, const U64 &capturesOnly);
    void possibleK(const BitBoards& board, int color, const U64 &capturesOnly);

};



inline Move MoveGen::movegen_sort(int ply, Move * moveAr) const
{
	int best = -INF;
	int high = 0;
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
