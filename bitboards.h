#ifndef BITBOARDS_H
#define BITBOARDS_H


typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <string>

class ZobristH;
class Move;
class MoveGen;

class BitBoards
{

public:

    //builds boards through reading an array
    void constructBoards();

    void makeMove(const Move &move, ZobristH &zobrist, bool isWhite);
    void unmakeMove(const Move &moveKey, ZobristH &zobrist, bool isWhite);

    //helper funtction to draw out bitboards like chess boards
    void drawBB(U64 board);
    //draw out bitboards like a full chessboard array
    void drawBBA();

	//piece material arrays for sides, using piece values
	int sideMaterial[2]; //updated on make/unmake moves

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

private:
    //removes cacptured piece from BB's
    void removeCapturedPiece(bool isWhite, char captured, U64 location);

    //rolls back a capture on move unmake
    void undoCapture(U64 location, char piece, bool isNotWhite);
};

#endif // BITBOARDS_H
