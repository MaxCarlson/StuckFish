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

inline Scores operator-=(Scores s1, const Scores s2) {
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

inline Scores make_scores(int m, int e) {
	Scores x;
	x.mg = m; x.eg = e;
	return x;
}

//masks used for pawn eval, possibly other things too
U64 forwardBB[COLOR][64]; //line in front of square relative side to move
U64 PassedPawnMask[COLOR][64]; //line in front & pawnAttackSpan
U64 PawnAttackSpan[COLOR][64]; //all tiles that can be attacked by a pawn as it moves forward

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

//returns a mask of all tiles a pawn could attack
//as it moves forward along it's path
inline U64 pawn_attack_span(int color, int sq) {
	return PawnAttackSpan[color][sq];
}

#define S(mg, eg) make_scores(mg, eg)

// Doubled pawn penalty by file
const Scores Doubled[8] = {
	S(5, 20), S(8, 22), S(9, 22), S(9, 22),
	S(9, 22), S(9, 22), S(8, 22), S(5, 20)
};

// Isolated pawn penalty by opposed flag and file
const Scores Isolated[2][8]{
  { S(18, 22), S(25, 23), S(28, 24), S(28, 24),
    S(28, 24), S(28, 24), S(25, 23), S(18, 22) },

  { S(11, 14), S(18, 17), S(20, 18), S(20, 18),
    S(20, 18), S(20, 18), S(18, 17), S(11, 14) }
};

// Backward pawn penalty by opposed flag and file
const Scores Backward[2][8] = {
  { S(14, 20), S(21, 22), S(23, 22), S(23, 22),
    S(23, 22), S(23, 22), S(21, 22), S(14, 20) },

  { S(9,  13), S(13, 15), S(16, 15), S(16, 15),
    S(16, 15), S(16, 15), S(13, 15), S(9,  13) } 
};


// Connected pawn bonus by file and rank (initialized by formula)
//Scores Connected[FILE_NB][RANK_NB]; !! NEED TO ADD TO SCORE CONNECTED

// Candidate passed pawn bonus by rank
const Scores CandidatePassed[8] = {
	S(0,   0), S(2,   6), S(2, 6), S(6, 14),
	S(16, 33), S(39, 77), S(0, 0), S(0,  0) 
};

// Levers bonus by rank
const Scores Lever[8] = {
	S(0,   0), S(0,  0), S(0, 0), S(0, 0),
	S(10, 10), S(20,20), S(0, 0), S(0, 0) 
};

// Bonus for file distance of the two outermost pawns
const Scores PawnsFileSpan = S(0, 6);

// Unsupported pawn penalty
const Scores UnsupportedPawnPenalty = S(10, 5);

//global pawn hash table
Pawns::Table pawnsTable;

namespace Pawns {

template<int color>
Scores evalPawns(const BitBoards & boards, PawnsEntry *e) {
	
	Scores val; val.mg = 0; val.eg = 0;

	//set color and movement info per our side
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
	U64 bb;

	//hold a pointer to an array of pawn attacks for our color per square
	const U64* pawnAttacksBB = boards.PseudoAttacks[PAWN];

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
	//used for increasing square count bu one relative stm
	int deltaN = color == WHITE ? 8 : -8;

	while ((square = *list++) != SQ_NONE) {

		int f = file_of(square);

		//file of this pawn cannot be semi open
		e->semiOpenFiles[color] &= ~(1LL << f);

		//previous rank
		U64 pr = RankMasks8[square / 8 + sqfx]; //!@!@@@!@@!@!@!@!!@!@!@!@!!@@!@ NEED to test and make sure this grabs the right rank, CTRL F to find others if it doesn't

		//previous rank plus current rank
		bb = pr | RankMasks8[square / 8 - 1];

		//flag pawn as passed, isolated, doauble,
		//unsupported or connected
		connected   =  ourPawns    & adjacentFiles[f] & bb;
		unsupported = !(ourPawns   & adjacentFiles[f] & pr);
		isolated    = !(ourPawns   & adjacentFiles[f]);
		doubled	    =  ourPawns    & forward_bb(color, square);
		opposed     =  enemyPawns  & forward_bb(color, square);
		passed      = !(enemyPawns & passed_pawn_mask(color, square));
		lever       =  enemyPawns  & pawnAttacksBB[square];

		
		// If the pawn is passed, isolated, or connected it cannot be
		// backward. If there are friendly pawns behind on adjacent files
		// or if it can capture an enemy pawn it cannot be backward either.
		if((passed | isolated | connected)
			|| (ourPawns & pawn_attack_span(them, square))
			|| (boards.psuedoAttacks(PAWN, color, square) & enemyPawns)) {

			backward = false;
		}
		else{
			// We now know that there are no friendly pawns beside or behind this
			// pawn on adjacent files. We now check whether the pawn is
			// backward by looking in the forward direction on the adjacent
			// files, and picking the closest pawn there.

			bb = pawn_attack_span(color, square) & (ourPawns | enemyPawns);
			bb = pawn_attack_span(color, square) & RankMasks8[backmost_sq(color, bb) / 8 + sqfx];

			// If we have an enemy pawn in the same or next rank, the pawn is
			// backward because it cannot advance without being captured.
			backward = (bb | shift_bb<Up>(bb)) & enemyPawns;

		}
		
		//if (!(opposed | passed | (pawn_attack_span(color, square) & enemyPawns)) continue; //need to TESTTESTTEST

		// A not-passed pawn is a candidate to become passed, if it is free to
		// advance and if the number of friendly pawns beside or behind this
		// pawn on adjacent files is higher than or equal to the number of
		// enemy pawns in the forward direction on the adjacent files.
			
		candidate = !(opposed | passed | backward | isolated)
				 && (bb = pawn_attack_span(them, square + deltaN) & ourPawns) != 0 //need to test the & to make sure it works
				 && bit_count(bb) >= bit_count(pawn_attack_span(color, square) & enemyPawns);

		if (passed && !doubled) 
			e->passedPawns[color] |= square; 

		if (isolated)
			val -= Isolated[opposed][f]; //NEED TO TEST IF OPERATOR OVERLOADS WORK OR IF THEY NEED CHAGNING

		if (unsupported && !isolated)
			val -= UnsupportedPawnPenalty;

		if (doubled)
			val -= Doubled[f]; // NEED TO ADD ~~~ / rank_distance(square, lsb(doubled));

		if (backward)
			val -= Backward[opposed][f];

		//if (connected)
			//val += Connected[f][relative_rank(color, square)];

		if (lever)
			val += Lever[relative_rank(color, square)]; // NEED TO TEST AND MAKE SURE ALL RELATIVE RANK FUNCTIOSN WORKS

		if (candidate)
		{
			val += CandidatePassed[relative_rank(color, square)];

			if (!doubled)
				e->candidatePawns[color] |= square;
		}
		
	}

	bb = e->semiOpenFiles[color] ^ 0xFF; //TEST?????????????????????????????????????????????????

	e->pawnSpan[color] = bb ? (msb(bb) - lsb(bb)) : 0; //ALSO TESTTTTTTTTTTTTTTTTTTTT

	// In endgame it's better to have pawns on both wings. So give a bonus according
	// to file distance between left and right outermost pawns.
	//val += PawnsFileSpan * e->pawnSpan[color];
	
	return val;
}



PawnsEntry * Pawns::probe(const BitBoards & boards, Table & entries)
{
	U64 key = boards.pawn_key();

	PawnsEntry* entry = entries[key];

	if (entry->Key == key) {
		return entry;
	}

	entry->Key = key;

	Scores s = evalPawns<WHITE>(boards, entry) - evalPawns<BLACK>(boards, entry);

	entry->score[mg] = s.mg;
	entry->score[eg] = s.eg;

	return entry;
}


}