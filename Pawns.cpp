#include "Pawns.h"
#include "bitboards.h"

//holds mid and end game values
struct Scores { int mg, eg; };

const U64 DarkSquares = 0xAA55AA55AA55AA55ULL;

inline Scores operator-(Scores s1, const Scores s2) { 
	s1.mg -= s2.mg;
	s1.eg -= s2.eg;
	return s1;
};

inline Scores operator+(Scores s1, const Scores s2) {
	s1.mg += s2.mg;
	s1.eg += s2.eg;
	return s1;
};

inline Scores operator+=(Scores& s1, const Scores s2) {
	s1.mg += s2.mg;
	s1.eg += s2.eg;
	return s1;
};

enum Directions{
	N,
	NE,
	NW,
	W,
	S,
	SW,
	SE,	
	E	
};

//shifts a bitboard in any of the enum directions above. Must use enum values in template
template<int sh> 
U64 shift_bb(const U64& b) {
	return  sh == N ? b << 8 : sh == S ? b >> 8
		: sh == NE ? (b & ~FileHBB) << 9 : sh == SE ? (b & ~FileHBB) >> 7
		: sh == NW ? (b & ~FileABB) << 7 : sh == SW ? (b & ~FileABB) >> 9
		: 0LL;
}

template<int color>
Scores evalPawns(const BitBoards & boards, Pawns *e) {
	
	Scores val; val.mg = 0; val.eg = 0;

	const int them  = color == WHITE  ? BLACK : WHITE;
	const int Up    = color == WHITE  ?	   N  : S;
	const int Right = color == WHITE  ?	   NE : SW;
	const int Left  = color == WHITE  ?	   NW : SE;

	//grab piece list of our pawns so we can iterate through and score them
	const int* list = boards.pieceLoc[color][PAWN];

	U64 ourPawns = boards.pieces(color, PAWN);
	U64 enemyPawns = boards.pieces(them, PAWN);

	//initialize values
	e->passedPawns[color]			= e->candidatePawns[color] = 0;
	e->kingSquares[color]			= SQ_NONE;
	e->semiOpenFiles[color]			= 0xFF;
	e->pawnAttacks[color]			= shift_bb<Right>(ourPawns) | shift_bb<Left>(ourPawns);
	e->pawnsOnSquares[color][BLACK] = bit_count(ourPawns & DarkSquares);
	e->pawnsOnSquares[color][WHITE] = boards.pieceCount[color][PAWN] - e->pawnsOnSquares[color][BLACK];

	int square;
	while ((square = *list++) != SQ_NONE) {




	}

	
	return val;
}

Pawns * probe(const BitBoards & boards, Table & entries)
{
	U64 key = boards.pawn_key();

	Pawns* entry = entries[key];

	if (entry->Key == key) {
		return entry;
	}

	entry->Key = key;

	Scores s = evalPawns<WHITE>(boards, entry) - evalPawns<BLACK>(boards, entry);

	entry->score[mg] = s.mg;
	entry->score[eg] = s.eg;

	return entry;
}
