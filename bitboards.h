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
    BitBoards();

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

    const U64 FileABB = 0x0101010101010101ULL;
    const U64 FileBBB = FileABB << 1;
    const U64 FileCBB = FileABB << 2;
    const U64 FileDBB = FileABB << 3;
    const U64 FileEBB = FileABB << 4;
    const U64 FileFBB = FileABB << 5;
    const U64 FileGBB = FileABB << 6;
    const U64 FileHBB = FileABB << 7;

    //totally full bitboard
    const U64 full  = 0xffffffffffffffffULL;
    const U64 Totallyempty = 0LL;

    //files to keep pieces from moving left or right off board
    const U64 notAFile = 0x7f7f7f7f7f7f7f7f; // ~0x8080808080808080
    const U64 notHFile = 0xfefefefefefefefe; // ~0x0101010101010101


    const U64 rank4 = 1095216660480L;
    const U64 rank5 = 4278190080L;
    const U64 rank6 = rank5 >> 8;
    const U64 rank7 = rank6 >> 8;
    const U64 rank8 = rank7 >> 8;
    //ugh
    const U64 rank3 = rank4 << 8;
    const U64 rank2 = rank3 << 8;
    const U64 rank1 = rank2 << 8;


    //board for knight moves that can be shifted
    const U64 KNIGHT_SPAN=43234889994L;

        //files for keeping knight moves from wrapping
    const U64 FILE_AB=FileABB + FileBBB;
    const U64 FILE_GH=FileGBB + FileHBB;

    //Bitboard of all king movements that can then be shifted
    const U64 KING_SPAN=460039L;

private:
    //removes cacptured piece from BB's
    void removeCapturedPiece(char captured, U64 location);

    //rolls back a capture on move unmake
    void undoCapture(U64 location, char piece, bool isNotWhite);
};

#endif // BITBOARDS_H
