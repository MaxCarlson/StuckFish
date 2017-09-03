#include "Pawns.h"
#include "bitboards.h"
#include "externs.h"

static const U64 RankMasks8[8] = //from rank 1 - 8 
{
	0xFF00000000000000L, 0xFF000000000000L,  0xFF0000000000L, 0xFF00000000L, 0xFF000000L, 0xFF0000L, 0xFF00L, 0xFFL,
};


static const U64 FileMasks8[8] =/*from fileA to FileH*/
{
	0x101010101010101L, 0x202020202020202L, 0x404040404040404L, 0x808080808080808L,
	0x1010101010101010L, 0x2020202020202020L, 0x4040404040404040L, 0x8080808080808080L
};

//hold all files adjacent to the index input,
//0 == file A adjacent file (fileB)
static const U64 adjacentFiles[8] = {
	0x202020202020202L, 0x505050505050505L , 0xa0a0a0a0a0a0a0aL, 0x1414141414141414L, 
	0x2828282828282828L, 0x5050505050505050L, 0xa0a0a0a0a0a0a0a0L, 0x4040404040404040L
};

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

//masks used for pawn eval, possibly other things too
U64 forwardBB[COLOR][64];
U64 PassedPawnMask[COLOR][64];

//function returns a bitboard of all squares ahead of the sq
//input, realtive to side to move.
inline U64 forward_bb(int color, int sq) {
	return forwardBB[color][sq];
}

//returns a mask of all tiles in front of a pawn,
//as well as all tiles forward one tile and to the left and right
//relative to side to move
inline U64 passed_pawn_mask(int color, int sq) {
	return PassedPawnMask[color][sq];
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

	//initialize values
	U64 ourPawns = boards.pieces(color, PAWN);
	U64 enemyPawns = boards.pieces(them, PAWN);
	U64 doubled;

	bool passed, isolated, opposed, connected, backward, candidate, unsupported, lever;

	e->passedPawns[color]			= e->candidatePawns[color] = 0;
	e->kingSquares[color]			= SQ_NONE;
	e->semiOpenFiles[color]			= 0xFF;
	e->pawnAttacks[color]			= shift_bb<Right>(ourPawns) | shift_bb<Left>(ourPawns);
	e->pawnsOnSquares[color][BLACK] = bit_count(ourPawns & DarkSquares);
	e->pawnsOnSquares[color][WHITE] = boards.pieceCount[color][PAWN] - e->pawnsOnSquares[color][BLACK];

	int square;
	//used for indexing files without if in loop
	int sqfx = color == WHITE ? 1 : -2;

	while ((square = *list++) != SQ_NONE) {

		int f = file_of(square);

		//file of this pawn cannot be semi open
		e->semiOpenFiles[color] &= ~(1LL << f);

		//previous rank
		U64 pr = RankMasks8[square / 8 + sqfx];

		//previous rank plus current rank
		U64 prc = pr | RankMasks8[square / 8 - 1];

		//flag pawn as passed, isolated, doauble,
		//unsupported or connected
		connected   =  ourPawns    & adjacentFiles[f] & prc;
		unsupported = !(ourPawns   & adjacentFiles[f] & pr);
		isolated    = !(ourPawns   & adjacentFiles[f]);
		doubled	    =  ourPawns    & forward_bb(color, square);
		opposed     =  enemyPawns  & forward_bb(color, square);
		//passed      = !(enemyPawns & passed_pawn_mask(color, square));



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
