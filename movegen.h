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

    void grab_boards(const BitBoards &BBBoard, bool wOrB);
        void updateBoards(const Move &move, const BitBoards &board);

    bool isAttacked(U64 pieceLoc, bool wOrB, bool isSearchKingCheck);
	

	//static exhange eval
	int SEE(const U64 &fromSQ, const U64 &captureSQ, int piece, int captured, bool isWhite);

	Move movegen_sort(int ply);

    void reorderMoves(int ply, const HashEntry *entry);

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

        int trailingZeros(const U64 i) const;
        U64 ReverseBits(U64 input);
        U64 noWeOne(U64 b);
        U64 soWeOne(U64 b);
        U64 westOne(U64 b);
        U64 soEaOne(U64 b);
        U64 noEaOne(U64 b);
        U64 eastOne(U64 b);
        U64 southOne(U64 b);
        U64 northOne(U64 b);

        void drawBBA();
        void drawBB(U64 board);		

private:

        //assigns a score to moves and adds them to the move array
        void movegen_push(char piece, char captured, char flag, U8 from, U8 to);
        bool blind(const Move &move, int pieceVal, int captureVal);	

		//used in static exchange eval
		std::vector<U64> getSmallestAttacker(const U64 & sq, bool isWhite);
		Move makeCaptureSEE(const U64 & from, const U64 & to, int captured, bool isWhite);
		void unmakeCaptureSEE(const Move &m, bool isWhite);

        char whichPieceCaptured(U64 landing);


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

inline int MoveGen::trailingZeros(const U64 i) const {
	
	//find the first one and number of zeros after it
	if (i == 0) return 64;
	U64 x = i;
	U64 y;
	int n = 63;
	y = x << 32; if (y != 0) { n -= 32; x = y; }
	y = x << 16; if (y != 0) { n -= 16; x = y; }
	y = x << 8; if (y != 0) { n -= 8; x = y; }
	y = x << 4; if (y != 0) { n -= 4; x = y; }
	y = x << 2; if (y != 0) { n -= 2; x = y; }
	return (int)(n - ((x << 1) >> 63));
	
	//return msb(i); replace all with this ?
}

#endif // MOVEGEN_H
