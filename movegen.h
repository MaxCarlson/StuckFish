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

    bool isWhite;
	//number of moves generated this node
    int moveCount;

    void generatePsMoves(const BitBoards& boards, bool isWhite, bool capturesOnly);

	//grab bitboard changes after a move, or any change in search. Need to get rid of eventually and just pass constRef to everything
    void grab_boards(const BitBoards &BBBoard, bool wOrB); //remove this function and just pass const ref BitBoards to all functions that need it

	bool isLegal(const BitBoards & b, const Move & m, bool isWhite);

    bool isAttacked(U64 pieceLoc, bool wOrB, bool isSearchKingCheck);
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
    bool blind(const BitBoards & board, const Move &move, int color, int pieceVal, int captureVal);

	//used in static exchange eval
	U64 attackersTo(int sq, const BitBoards& b, const U64 occ) const;

	//finds the smallest attacker for side to move, out of the stm attackers board,
	//removes the attacker from the attackers and occuied board, then finds any x-ray attackers behind that piece
	//and returns the int representing the piece
	inline int min_attacker(bool isWhite, const BitBoards & b, const int &to, const U64 &stmAttackers, U64 &occupied, U64 &attackers);

    int whichPieceCaptured(U64 landing);


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
