#ifndef BITBOARDS_H
#define BITBOARDS_H


typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <string>
#include <algorithm>
#include <memory> 
#include <deque>

#include "zobristh.h"


class ZobristH;
class MoveGen;
class Thread;

struct BoardInfo { //possibly remove this and move this info to bitboards object? //DELETE THIS, REPLACED BY st
	int sideMaterial[COLOR]; //updated on make/unmake moves	
	int nonPawnMaterial[COLOR];

	//keeps internal track of which side 
	//is going to make the next move. Access through function
	int sideToMove; 
};

struct CheckInfo{
	CheckInfo(const BitBoards& board);

	//direct check candidates
	U64 dcCandidates;
	U64 pinned;
	U64 checkSq[PIECES];
	int ksq;
};

struct StateInfo {

	U64 Key;
	U64 PawnKey;
	U64 MaterialKey;

	//int sideMaterial[COLOR]; // need to use these and delete struct above
	//int nonPawnMaterial[COLOR];

	int capturedPiece; 

	int epSquare;
	int castlingRights;

	int rule50;
	int plysFromNull;

	U64 checkers;

	StateInfo* previous;
};

typedef std::unique_ptr<std::deque<StateInfo>> StateListPtr;

class BitBoards
{

public:
	//initialize some bit masks, as well as other values.
	//this is only called once at program startup
	void initBoards();

	//debug method ~~ needs additions and to be redone
	bool posOkay();

	//holds zobrist key and arrays neccasary for modifying key
	ZobristH zobrist; //REMOVE THIS FROM THE OBJECT WHEN YOU CAN

	U64 nextKey(Move m);

    //builds boards through reading an array
    void constructBoards(const std::string* FEN, Thread * th, StateInfo * si);

	// Seperate from construct, set states sets the board info and state pointer
	void set_state(StateInfo * si, Thread * th);

	//constructs boards from FEN string
	void readFenString(const std::string& FEN);

	bool isLegal(const Move & m, U64 pinned) const;
	bool pseudoLegal(Move m) const;

	//helper function for isLegal + for finding if in check
	bool isSquareAttacked(const int square, const int color) const;

	//make the move
	void makeMove(const Move& m, StateInfo& newSt, int color);
	//unamke move
	void unmakeMove(const Move & m, int color);

	void makeNullMove(StateInfo& newSt);
	void undoNullMove();

	bool isDraw(int ply);

	//Holds board information, struct above
	BoardInfo bInfo;

	//static exhange eval
	int SEE(const Move& m, int color, bool isCapture) const;
	bool seeGe(Move m, int Threshold = 0);
	//finds the smallest attacker for side to move, out of the stm attackers board,
	//removes the attacker from the attackers and occuied board, then finds any x-ray attackers behind that piece
	//and returns the int representing the piece
	template<int Pt>
	int min_attacker(int color, int to, const U64 & stmAttackers, U64 & occupied, U64 & attackers) const;



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
	int color_of_pc(int sq) const;

	bool capture(Move m) const;
	bool capture_or_promotion(Move m) const;
	int captured_piece() const;
	int moved_piece(Move m) const;

	//returns the square the king is on
	int king_square(int color) const;
	//is this a pawn move on the 5th or higher relative rank?
	bool isPawnPush( Move m, int color);
	bool isMadePawnPush(Move m, int color);
	//is there a pawn on the 7 th rank relative to stm
	bool pawnOn7th(int color);
	//returns true if side color has any pieces aside from pawn/s or king
	int non_pawn_material(int color) const;
	int non_pawn_material() const;
	U64 square_bb(int sq) const;
	
	int stm() const; //returns side to move

    //return the key for the select hash table
	U64 TTKey() const;
	U64 pawn_key() const;
	U64 material_key() const;

	//used for interpolating between mid and end game scores, in TESTING
	int game_phase() const;

	//attack info
	//full of attacks all possible attacks for all squares and pieces
	//on a completely empty board
	template<int Pt>
	U64 attacks_from(int from) const;
	template<int Pt>
	U64 attacks_from(int from, int color) const;
	template<int Pt>
	U64 attacks_from(int sq, U64 occ) const;

	U64 attackers_to(int square, U64 occupied) const;


	U64 PseudoAttacks[PIECES][SQ_ALL]; //zero index is white pawns, 1 black pawns, pieces after are index by their number ~~ Move Outside Object

	U64 psuedoAttacks(int piece, int color, int sq) const; // move outside object!!


	//checks
	U64 checkers() const;
	U64 check_candidates() const;
	U64 pinned_pieces(int color) const;
	bool gives_check(Move m, const CheckInfo& ci);
	U64 check_blockers(int color, int kingColor) const;

	//castling
	void set_castling_rights(int color, int rfrom);

	int castlingRightsMasks[SQ_ALL];
	U64 castlingPath[CASTLING_RIGHTS];

	int castling_rights() const;

	int can_castle(int color) const;

	bool castling_impeded(int castlingRights) const;


	//en passants
	bool can_enpassant() const;
	int  ep_square() const;


//helper funtction to draw out bitboards like chess boards
	void drawBB(U64 board) const;
	//draw out bitboards like a full chessboard array
	void drawBBA() const;


	Thread * this_thread() const;

private:
	
	StateInfo* st;

	// Holds a pointer to the thread
	// this object is currently in
	Thread * thisThread;

	template<int make>
	void do_castling(int from, int to, int color);

	void movePiece(int piece, int color, int from, int to);

	void addPiece(int piece, int color, int sq);
	void removePiece(int piece, int color, int sq);
};

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

// Returns 3 if the square is empty
inline int BitBoards::color_of_pc(int sq) const
{
	return empty(sq) ? 3 : !(squareBB[sq] & allPiecesColorBB[WHITE]);
}

inline int BitBoards::pieceOnSq(int sq) const
{
	return pieceOn[sq];
}
// Is a square empty?
inline bool BitBoards::empty(int sq) const
{
	return pieceOn[sq] == PIECE_EMPTY;
}

inline bool BitBoards::capture(Move m) const
{
	return (!empty(to_sq(m)) || move_type(m) == ENPASSANT);
}

inline bool BitBoards::capture_or_promotion(Move m) const
{
	return move_type(m) != NORMAL ? move_type(m) != CASTLING : !empty(to_sq(m));
}

inline int BitBoards::captured_piece() const
{
	return st->capturedPiece;
}

// Can only be used if the move hasn't been made!!
inline int BitBoards::moved_piece(Move m) const
{
	return pieceOnSq(from_sq(m));
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

inline int BitBoards::can_castle(int color) const 
{
	return st->castlingRights & ((WHITE_OO | WHITE_OOO) << (2 * color));
}

// & all pieces board by the castling path indexed by
// castiling rights
inline bool BitBoards::castling_impeded(int cr) const 
{
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

inline Thread * BitBoards::this_thread() const {
	return thisThread;
}

inline int BitBoards::king_square(int color) const
{
	return pieceLoc[color][KING][0];
}
// Does Not Support Pawns.
// Color is unimportant where mentioned.
template<int Pt>
inline U64 BitBoards::attacks_from(int from) const
{
	return Pt == KNIGHT  ? psuedoAttacks(KNIGHT, WHITE, from)
		 : Pt == BISHOP  ? slider_attacks.BishopAttacks( FullTiles, from)
		 : Pt == ROOK    ? slider_attacks.RookAttacks(   FullTiles, from)
		 : Pt == QUEEN   ? slider_attacks.QueenAttacks(  FullTiles, from)
		 : Pt == KING    ? psuedoAttacks(KING, WHITE, from)
		 : 0LL;
}
//Used for Pawns with color info.
template<>
inline U64 BitBoards::attacks_from<PAWN>(int from, int color) const
{
	return psuedoAttacks(PAWN, color, from);
}
// template for rooks, bishops, and queens
// with occlusion bitboard
template<int Pt>
inline U64 BitBoards::attacks_from(int sq, U64 occ) const
{
	return   Pt == BISHOP ? slider_attacks.BishopAttacks(occ, sq)
		   : Pt == ROOK   ? slider_attacks.RookAttacks(  occ, sq)
		   :			    slider_attacks.QueenAttacks( occ, sq);
}


//return an attack set for a piece on any square, attacks are generated
//as if there are no pieces on the boards
inline U64 BitBoards::psuedoAttacks(int piece, int color, int sq) const
{
	return piece == PAWN ? color == WHITE
		? PseudoAttacks[PAWN - 1][sq] : PseudoAttacks[PAWN][sq]
		: PseudoAttacks[piece][sq];
}

// Returns a bitboard of pieces checking
// our king
inline U64 BitBoards::checkers() const
{
	return st->checkers;
}

// Returns BB of all of our pieces that 
// are blocking checks against opponent king
inline U64 BitBoards::check_candidates() const
{
	return check_blockers(stm(), !stm());
}

// Returns BB of all our pieces pinned down
// blocking check against our king
inline U64 BitBoards::pinned_pieces(int color) const
{
	return check_blockers(color, color);
}


// Is this move pushing a pawn past the reletive 4th rank?
// Can only be used before move is made!!
inline bool BitBoards::isPawnPush(Move m, int color) 
{	
	return (pieceOnSq(from_sq(m)) == PAWN && relative_rankSq(color, from_sq(m)) > 4);
}
// Only to be used after the move has been made!!
inline bool BitBoards::isMadePawnPush(Move m, int color) {
	return (pieceOnSq(to_sq(m)) == PAWN && relative_rankSq(color, from_sq(m)) > 4);
}

//is there a pawn on the 7th rank for side to move?
inline bool BitBoards::pawnOn7th(int color)
{	
	return (byColorPiecesBB[color][PAWN] & (color ? 0xFF000000000000L : 0xFF00L));
}

// Returns the amount of material a side has, exlcuding king and pawns
inline int BitBoards::non_pawn_material(int color) const
{
	return bInfo.nonPawnMaterial[color];
}
// Returns npm for both sides
inline int BitBoards::non_pawn_material() const
{
	return bInfo.nonPawnMaterial[WHITE] + bInfo.nonPawnMaterial[BLACK];
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


