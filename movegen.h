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
	//std::vector<Move> moveAr;

    bool isWhite;
	//number of moves generated this node
    int moveCount;

    void generatePsMoves(bool capturesOnly);

	//grab bitboard changes after a move, or any change in search. Need to get rid of eventually and just pass constRef to everything
    void grab_boards(const BitBoards &BBBoard, bool wOrB);

    bool isAttacked(U64 pieceLoc, bool wOrB, bool isSearchKingCheck);
	

	//static exhange eval
	int SEE(const Move& m, const BitBoards& b, bool isWhite, bool isCapture);

	//grab the best scoring move and return it
	inline Move movegen_sort(int ply, Move * moveAr) const;

    void reorderMoves(searchStack *ss, const HashEntry *entry);

    //bitboards
        U64 FullTiles;
        U64 EmptyTiles;
        U64 BBWhitePieces;
        U64 BBWhitePawns;
        U64 BBWhiteRooks;
        U64 BBWhiteKnights;
        U64 BBWhiteBishops;
        U64 BBWhiteQueens;
        U64 BBWhiteKing;
        U64 BBBlackPieces;
        U64 BBBlackPawns;
        U64 BBBlackRooks;
        U64 BBBlackKnights;
        U64 BBBlackBishops;
        U64 BBBlackQueens;
        U64 BBBlackKing;

        
        U64 noWeOne(U64 b);
        U64 soWeOne(U64 b);
        U64 westOne(U64 b);
        U64 soEaOne(U64 b);
        U64 noEaOne(U64 b);
        U64 eastOne(U64 b);
        U64 southOne(U64 b);
        U64 northOne(U64 b);

        void drawBBA() const;
        void drawBB(U64 board) const;		

private:

        //assigns a score to moves and adds them to the move array
        void movegen_push(char piece, char captured, char flag, U8 from, U8 to);
        bool blind(const Move &move, int pieceVal, int captureVal);	

		//used in static exchange eval
		U64 attackersTo(int sq, const BitBoards& b, const U64 occ) const;
		//finds the smallest attacker for side to move, out of the stm attackers board,
		//removes the attacker from the attackers and occuied board, then finds any x-ray attackers behind that piece
		//and returns the int representing the piece
		FORCE_INLINE int min_attacker(bool isWhite, const BitBoards & b, const int &to, const U64 &stmAttackers, U64 &occupied, U64 &attackers);

        int whichPieceCaptured(U64 landing);


        //psuedo legal move gen for indvidual pieces
        void possibleWP(const U64 &wpawns, const U64 &blackking, bool capturesOnly);
        void possibleBP(const U64 &bpawns, const U64 &whiteking, bool capturesOnly);
        void possibleN(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleB(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleR(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleQ(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleK(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);


        //void undoCapture(U64 location, char piece, char whiteOrBlack);

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
