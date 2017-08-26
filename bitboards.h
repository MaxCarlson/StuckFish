#ifndef BITBOARDS_H
#define BITBOARDS_H


typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <string>
#include "move.h"
#include "zobristh.h"
class ZobristH;
class Move;
class MoveGen;

struct BoardInfo {

};

class BitBoards
{

public:
	//holds zobrist key and arrays neccasary for modifying key
	ZobristH zobrist;

    //builds boards through reading an array
    void constructBoards();

    void makeMove(const Move &move, bool isWhite);
    void unmakeMove(const Move &moveKey, bool isWhite);

    //helper funtction to draw out bitboards like chess boards
    void drawBB(U64 board);
    //draw out bitboards like a full chessboard array
    void drawBBA();

	//piece material arrays for sides, using piece values
	int sideMaterial[2]; //updated on make/unmake moves //MOVE TO STRUCT ALONG WITH OTHER INFO?

	bool isPawnPush(const Move& m, bool isWhite);

	

	int relativeRank(int sq, bool isWhite);

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
/*
	//master zobrist key for boards
	U64 Key;
	U64 fetchKey(const Move & m, bool isWhite);
	U64 getZobristHash();
	void updateKeyColor();
	//construct zobrist arrays
	void initializeZobrist(ZobristH zobrist);
*/
	//array used to denote if castling has occured
	bool castled[4]; //wqs, wks, bqs, bks
	bool rookMoved[4]; //wqR, wkR, bqr, bkr

private:
	//updates zobrist key for boards
	void UpdateKey(int start, int end, const Move & moveKey, bool isWhite);

    //removes cacptured piece from BB's
    void removeCapturedPiece(bool isWhite, char captured, U64 location);

    //rolls back a capture on move unmake
    void undoCapture(U64 location, char piece, bool isNotWhite);
};

inline bool BitBoards::isPawnPush(const Move &m, bool isWhite) 
{	//is there a pawn past their relative 4th rank?
	return (m.piece == PAWN && relativeRank(m.from, isWhite) > 4);
}




#endif // BITBOARDS_H
