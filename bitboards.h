#ifndef BITBOARDS_H
#define BITBOARDS_H


typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <string>
#include <deque>
#include "move.h"
#include "zobristh.h"
class ZobristH;
class Move;
class MoveGen;

struct BoardInfo {
	int sideMaterial[2]; //updated on make/unmake moves
	//int pieceSqVals[2][2]; //piece square table bonuses and reductions
};

class BitBoards
{

public:
	//holds zobrist key and arrays neccasary for modifying key
	ZobristH zobrist;

    //builds boards through reading an array
    void constructBoards();

	//make the move
	void makeMove(const Move& m, bool isWhite);
	//unamke move
	void unmakeMove(const Move& m, bool isWhite);

	//is this a pawn move on the 5th or higher relative rank?
	bool isPawnPush(const Move& m, bool isWhite);
	//is there a pawn on the 7 th rank relative to stm
	bool pawnOn7th(bool isWhite);

	//Holds board information, struct above
	BoardInfo bInfo;

//bitboards
    U64 FullTiles;
    U64 EmptyTiles;

	//holds all bitboards of all pieces, [i][0] is blank at 0LL and used for non captures that still |
	U64 byColorPiecesBB[2][7]; //0 white, 1 black ; 0 = no piece, 1 pawn, 2 knight, 3 bishop, 4 rook, 5 queen, 6 king
	U64 allPiecesColorBB[2]; //0 white, 1 black
	U64 byPieceType[7]; //holds both black and white pieces of a type
	U64 squareBB[64]; //holds a U64 of each square on gameboard, indexed by 0 = top left, 63 = bottom right

	
	//pieces and their square locations. So we don't have to pop_lsb a board to find a piece
	std::deque<int>pieceLoc[2][7]; //color, piece type
	int pieceCount[2][7]; //count of number of pieces; color, piece type
	int pieceIndex[64]; //a lookup index so we can extract the index of the piece in the list above from just it's location

//state info
	//piece material arrays for sides, using piece values
 //MOVE TO STRUCT ALONG WITH OTHER INFO?

	//array used to denote if castling has occured
	bool castled[4]; //wqs, wks, bqs, bks
	bool rookMoved[4]; //wqR, wkR, bqr, bkr


//helper funtction to draw out bitboards like chess boards
	void drawBB(U64 board);
	//draw out bitboards like a full chessboard array
	void drawBBA();

private:

	void movePiece(int piece, int color, int from, int to);

	void addPiece(int piece, int color, int sq);
	void removePiece(int piece, int color, int sq);

	//returns relative rank for side to move
	int relativeRank(int sq, bool isWhite);
};

//can only be used if there is no piece on landing spot
inline void BitBoards::movePiece(int piece, int color, int from, int to)
{
	U64 from_to = squareBB[from] ^ squareBB[to];

	byColorPiecesBB[color][piece] ^= from_to;
	allPiecesColorBB[color] ^= from_to;

	byPieceType[piece] ^= from_to;
	FullTiles ^= from_to;
	EmptyTiles ^= from_to;

	//change the location of the piece to where it's moving 
	pieceLoc[color][piece][pieceIndex[from]] = to;

}
//also can only be used to add piece if no piece is on destination sq
inline void BitBoards::addPiece(int piece, int color, int sq)
{
	byColorPiecesBB[color][piece] ^= squareBB[sq];
	allPiecesColorBB[color] ^= squareBB[sq];
	byPieceType[piece] ^= squareBB[sq];
	FullTiles ^= squareBB[sq];
	EmptyTiles ^= squareBB[sq];

	
	pieceLoc[color][piece].push_back(sq);
	pieceIndex[sq] = pieceCount[color][piece];
	pieceCount[color][piece]++;
}

inline void BitBoards::removePiece(int piece, int color, int sq)
{
	byColorPiecesBB[color][piece] ^= squareBB[sq];
	allPiecesColorBB[color] ^= squareBB[sq];
	byPieceType[piece] ^= squareBB[sq];
	FullTiles ^= squareBB[sq];
	EmptyTiles ^= squareBB[sq];

	pieceCount[color][piece]--;
	pieceLoc[color][piece].erase(pieceLoc[color][piece].begin() +pieceIndex[sq]);
	pieceIndex[sq] = INVALID;
}

//is there a pawn past their relative 4th rank?
inline bool BitBoards::isPawnPush(const Move &m, bool isWhite) 
{	
	return (m.piece == PAWN && relativeRank(m.from, isWhite) > 4);
}

//is there a pawn on the 7th rank for side to move?
inline bool BitBoards::pawnOn7th(bool isWhite)
{	//again color is messed up, !isWhite, if white, points to whites index for pieces
	return (byColorPiecesBB[!isWhite][PAWN] & (isWhite ? 0xFF00L : 0xFF000000000000L));
}






#endif // BITBOARDS_H
