#ifndef BITBOARDS_H
#define BITBOARDS_H


typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <string>
#include <algorithm>
#include "move.h"
#include "zobristh.h"
#include "defines.h"

class ZobristH;
class Move;
class MoveGen;

struct BoardInfo { //possibly remove this and move this info to bitboards object? //DELETE THIS, REPLACED BY st
	int sideMaterial[COLOR]; //updated on make/unmake moves	
	int nonPawnMaterial[COLOR];

	//keeps internal track of which side 
	//is going to make the next move. Access through function
	int sideToMove; 

};

struct StateInfo {

	U64 Key;
	U64 PawnKey;
	U64 MaterialKey;

	int epSquare;

	int castlingRights;

	StateInfo* previous;
};

class BitBoards
{

public:
	//initialize some bit masks, as well as other values.
	//this is only called once at program startup
	void initBoards();

	//debug method
	bool posOkay();

	//holds zobrist key and arrays neccasary for modifying key
	ZobristH zobrist;

    //builds boards through reading an array
    void constructBoards(const std::string* FEN);

	//constructs boards from FEN string
	void readFenString(const std::string& FEN);

	//make the move
	void makeMove(const Move& m, StateInfo& newSt, int color);
	//unamke move
	void unmakeMove(const Move& m, int color);

	void makeNullMove(StateInfo& newSt);
	void undoNullMove();

	//Holds board information, struct above
	BoardInfo bInfo;

//bitboards
    U64 FullTiles;
    U64 EmptyTiles;

	//holds all bitboards of all pieces, [i][0] is blank at 0LL and used for non captures that still |
	U64 byColorPiecesBB[COLOR][PIECES]; //0 white, 1 black ; 0 = no piece, 1 pawn, 2 knight, 3 bishop, 4 rook, 5 queen, 6 king
	U64 allPiecesColorBB[COLOR]; //0 white, 1 black
	U64 byPieceType[PIECES]; //holds both black and white pieces of a type
	U64 squareBB[SQ_ALL]; //holds a U64 of each square on gameboard, indexed by 0 = top left, 63 = bottom right

	//pieces and their square locations. So we don't have to pop_lsb a board to find a piece
	int pieceLoc[COLOR][PIECES][16]; //color, piece type
	int pieceIndex[SQ_ALL];    //a lookup index so we can extract the index of the piece in the list above from just it's location
	int pieceCount[COLOR][PIECES]; //count of number of pieces; color, piece type. 0 = no piece, 1 pawn, etc.
	int pieceOn[SQ_ALL]; // allows us to lookup which piece is on a square
	

	//helper functions for ease of access to data structures holding info
	U64 pieces(int color)const;
	U64 pieces(int color, int pt)const;
	U64 pieces(int color, int pt, int pt1)const;
	U64 piecesByType(int p1) const;
	U64 piecesByType(int p1, int p2) const; 
	template<int pt> int count(int color) const;
	bool empty(int sq) const;
	int pieceOnSq(int sq) const;
	//returns the square the king is on
	int king_square(int color) const;
	//is this a pawn move on the 5th or higher relative rank?
	bool isPawnPush(const Move& m, int color);
	//is there a pawn on the 7 th rank relative to stm
	bool pawnOn7th(int color);
	//returns true if side color has any pieces aside from pawn/s or king
	bool non_pawn_material(int color) const;
	U64 square_bb(int sq) const;


	bool can_enpassant() const;
	int  ep_square() const;


	//Board Info Functions
	
	int stm() const; //returns side to move

    //return the key for the select hash table
	U64 TTKey() const;
	U64 pawn_key() const;
	U64 material_key() const;

	//used for interpolating between mid and end game scores, in TESTING
	int game_phase() const;


	//full of attacks all possible attacks for all squares and pieces
	//on a completely empty board
	U64 PseudoAttacks[PIECES][SQ_ALL]; //zero index is white pawns, 1 black pawns, pieces after are index by their number ~~ Move Outside Object

	U64 psuedoAttacks(int piece, int color, int sq) const;


	//castling
	void set_castling_rights(int color, int rfrom);

	int castlingRightsMasks[SQ_ALL];
	U64 castlingPath[CASTLING_RIGHTS];

	int castling_rights() const;

	int can_castle(int color) const;

	bool castling_impeded(int castlingRights) const;




//helper funtction to draw out bitboards like chess boards
	void drawBB(U64 board);
	//draw out bitboards like a full chessboard array
	void drawBBA();

	// holds the state info for the very start
	// st eventually points to this at the end
	StateInfo startState;

private:

	void set_state(StateInfo * si);
	
	StateInfo* st;

	template<int make>
	void do_castling(const Move & m, int color);

	void movePiece(int piece, int color, int from, int to);

	void addPiece(int piece, int color, int sq);
	void removePiece(int piece, int color, int sq);
};
/*
inline int BitBoards::stm() const
{
	return bInfo.sideToMove;
}
*/

//these function return a board of particular pieces/combination of pieces
inline U64 BitBoards::pieces(int color) const{
	return allPiecesColorBB[color];
}

inline U64 BitBoards::pieces(int color, int pt) const{
	return byColorPiecesBB[color][pt];
}

inline U64 BitBoards::pieces(int color, int pt, int pt1) const{
	return (byColorPiecesBB[color][pt] | byColorPiecesBB[color][pt1]);
}

inline U64 BitBoards::piecesByType(int p1) const
{
	return byPieceType[p1];
}

//returns a U64 of both color piece1 | piece 2
inline U64 BitBoards::piecesByType(int p1, int p2) const
{
	return byPieceType[p1] | byPieceType[p2];
}

inline bool BitBoards::empty(int sq) const
{
	return pieceOn[sq] == PIECE_EMPTY;
}

inline int BitBoards::pieceOnSq(int sq) const
{
	return pieceOn[sq];
}

inline int BitBoards::stm() const
{
	return bInfo.sideToMove;
}

//castling helpers
inline int BitBoards::castling_rights() const
{
	return st->castlingRights;
}

inline int BitBoards::can_castle(int color) const {
	return st->castlingRights & ((WHITE_OO | WHITE_OOO) << (2 * color));
}

// & all pieces board by the castling path indexed by
// castiling rights
inline bool BitBoards::castling_impeded(int cr) const {
	return FullTiles & castlingPath[cr];
}

inline int BitBoards::game_phase() const
{
	int npm = bInfo.nonPawnMaterial[WHITE] + bInfo.nonPawnMaterial[BLACK];

	npm = std::max(ENDGAME_LIMIT, std::min(npm, MIDGAME_LIMIT));

	return (((npm - ENDGAME_LIMIT) * 64) / (MIDGAME_LIMIT - ENDGAME_LIMIT));
}

//returns the key to the transposition
inline U64 BitBoards::TTKey() const {
	return st->Key;
}

//returns the incrementaly updated pawn hash key
inline U64 BitBoards::pawn_key() const {
	return st->PawnKey;
}

inline U64 BitBoards::material_key() const
{
	return st->MaterialKey;
}

inline int BitBoards::king_square(int color) const
{
	return pieceLoc[color][KING][0];
}

//return an attack set for a piece on any square, attacks are generated
//as if there are no pieces on the boards
inline U64 BitBoards::psuedoAttacks(int piece, int color, int sq) const
{
	return piece == PAWN ? color == WHITE
		? PseudoAttacks[PAWN - 1][sq] : PseudoAttacks[PAWN][sq]
		: PseudoAttacks[piece][sq];
}

//is there a pawn past their relative 4th rank?
inline bool BitBoards::isPawnPush(const Move &m, int color) 
{	
	return (m.piece == PAWN && relative_rankSq(color, m.from) > 4);
}

//is there a pawn on the 7th rank for side to move?
inline bool BitBoards::pawnOn7th(int color)
{	
	return (byColorPiecesBB[color][PAWN] & (color ? 0xFF000000000000L : 0xFF00L));
}

//returns true if side color has any pieces aside from pawn/s or king
inline bool BitBoards::non_pawn_material(int color) const
{
	return (pieceCount[color][KNIGHT] != 0 || pieceCount[color][BISHOP] != 0
		&& pieceCount[color][ROOK] != 0 || pieceCount[color][QUEEN] != 0) ? true : false;
}

inline U64 BitBoards::square_bb(int sq) const
{
	return squareBB[sq];
}
inline bool BitBoards::can_enpassant() const
{
	return (st->epSquare > 0 && st->epSquare < 64);
}
inline int BitBoards::ep_square() const
{
	return st->epSquare;
}

//can only be used if there is no piece on landing spot
inline void BitBoards::movePiece(int piece, int color, int from, int to) 
{
	U64 from_to = square_bb(from) ^ square_bb(to);

	byColorPiecesBB[color][piece] ^= from_to;
	allPiecesColorBB[color] ^= from_to;

	byPieceType[piece] ^= from_to;
	FullTiles ^= from_to;
	EmptyTiles ^= from_to;

	pieceOn[from] = PIECE_EMPTY;
	pieceOn[to]   = piece;

	//update the pieces location
	pieceIndex[to] = pieceIndex[from];
	pieceLoc[color][piece][pieceIndex[to]] = to;
}
//also can only be used to add piece if no piece is on destination sq
inline void BitBoards::addPiece(int piece, int color, int sq)
{
	byColorPiecesBB[color][piece] ^= square_bb(sq); 
	allPiecesColorBB[color] ^= square_bb(sq);
	byPieceType[piece] ^= square_bb(sq);
	FullTiles ^= square_bb(sq);
	EmptyTiles ^= square_bb(sq);

	pieceOn[sq] = piece;
	//increment piece count and add piece to location and index for location
	pieceIndex[sq] = pieceCount[color][piece]++;
	pieceLoc[color][piece][pieceIndex[sq]] = sq;
}
//will add piece to bitboards if there is no piece to be removed on square!!
inline void BitBoards::removePiece(int piece, int color, int sq)
{
	byColorPiecesBB[color][piece] ^= square_bb(sq);
	allPiecesColorBB[color] ^= square_bb(sq);
	byPieceType[piece] ^= square_bb(sq);
	FullTiles ^= square_bb(sq);
	EmptyTiles ^= square_bb(sq);

	pieceOn[sq] = PIECE_EMPTY;

	int lSq = pieceLoc[color][piece][--pieceCount[color][piece]];
	pieceIndex[lSq] = pieceIndex[sq];
	pieceLoc[color][piece][pieceIndex[lSq]] = lSq;
	pieceLoc[color][piece][pieceCount[color][piece]] = SQ_NONE;
}

//returns a count of the number of pieces
//of a particular color/piece
template<int pt>
inline int BitBoards::count(int color) const
{
	return pieceCount[color][pt];
}

#endif // BITBOARDS_H


