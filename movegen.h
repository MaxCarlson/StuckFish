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
    MoveGen();

    //array of move objects by ply then number of moves
    Move moveAr[256];

    bool isWhite;
    int moveCount = 0;

    void generatePsMoves(bool capturesOnly);
    void clearMove(int ply, int numMoves);
    void constructBoards();
    void grab_boards(const BitBoards &BBBoard, bool wOrB);
        void updateBoards(const Move &move, const BitBoards &board);

    bool isAttacked(U64 pieceLoc, bool wOrB, bool isSearchKingCheck);

	//static exhange eval
	int SEE(const U64 &sq, int piece, bool isWhite);

	Move movegen_sort(int ply);

    void reorderMoves(int ply, const HashEntry &entry);

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

        int trailingZeros(U64 i);
        U64 ReverseBits(U64 input);
        U64 noWeOne(U64 b);
        U64 soWeOne(U64 b);
        U64 westOne(U64 b);
        U64 soEaOne(U64 b);
        U64 noEaOne(U64 b);
        U64 eastOne(U64 b);
        U64 southOne(U64 b);
        U64 northOne(U64 b);

        //Bitboard of all king movements that can then be shifted
        const U64 KING_SPAN=460039L;
        //board for knight moves that can be shifted
        const U64 KNIGHT_SPAN=43234889994L;
        const U64 FileABB = 0x0101010101010101ULL;
        const U64 FileBBB = FileABB << 1;
        const U64 FileCBB = FileABB << 2;
        const U64 FileDBB = FileABB << 3;
        const U64 FileEBB = FileABB << 4;
        const U64 FileFBB = FileABB << 5;
        const U64 FileGBB = FileABB << 6;
        const U64 FileHBB = FileABB << 7;
        //files for keeping knight moves from wrapping
        const U64 FILE_AB=FileABB + FileBBB;
        const U64 FILE_GH=FileGBB + FileHBB;


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


        //psuedo legal move gen
        void possibleWP(const U64 &wpawns, const U64 &blackking, bool capturesOnly);
        void possibleBP(const U64 &bpawns, const U64 &whiteking, bool capturesOnly);
        void possibleN(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleB(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleR(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleQ(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);
        void possibleK(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly);


        //void undoCapture(U64 location, char piece, char whiteOrBlack);

};

#endif // MOVEGEN_H
