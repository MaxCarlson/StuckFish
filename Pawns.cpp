#include "Pawns.h"
#include "bitboards.h"
#include "externs.h"
#include "psqTables.h"

//for flipping squares if black, place this somewhere else as an extern
static const int bSQ[64] = {
	56,57,58,59,60,61,62,63,
	48,49,50,51,52,53,54,55,
	40,41,42,43,44,45,46,47,
	32,33,34,35,36,37,38,39,
	24,25,26,27,28,29,30,31,
	16,17,18,19,20,21,22,23,
	8, 9, 10,11,12,13,14,15,
	0, 1,  2, 3, 4, 5, 6, 7
};

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


//masks used for pawn eval, possibly other things too
U64 forwardBB[COLOR][SQ_ALL]; //line in front of square relative side to move
U64 PassedPawnMask[COLOR][SQ_ALL]; //line in front & pawnAttackSpan
U64 PawnAttackSpan[COLOR][SQ_ALL]; //all tiles that can be attacked by a pawn as it moves forward
U64 BetweenSquares[SQ_ALL][SQ_ALL];

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


// Connected pawn bonus by file and rank from blacks pov
// when looking as if it were a chess board. Whites pov by index
const Scores Connected[8][8] = {
	{ S(0 ,0 ), S(1 , 1), S(1 , 1), S(1 , 1), S(1 , 1), S(1 , 1), S(1 , 1), S(0 , 0) },
	{ S(0 ,0 ), S(1 , 1), S(1 , 1), S(1 , 1), S(1 , 1), S(1 , 1), S(1 , 1), S(0 , 0) },
	{ S(0 ,0 ), S(2 , 2), S(2 , 2), S(3 , 3), S(3 , 3), S(2 , 2), S(2 , 2), S(0 , 0) },
	{ S(3 ,3 ), S(5 , 5), S(5 , 5), S(6 , 6), S(6 , 6), S(5 , 5), S(5 , 5), S(3 , 3) },
	{ S(11,11), S(14,14), S(14,14), S(15,15), S(15,15), S(14,14), S(14,14), S(11,11) },
	{ S(27,27), S(30,30), S(30,30), S(31,31), S(31,31), S(30,30), S(30,30), S(27,27) },
	{ S(53,53), S(57,57), S(57,57), S(59,59), S(59,59), S(57,57), S(57,57), S(53,53) }
};

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
	
	Scores val;

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

	e->pieceSqTabScores[color]      = make_scores(0, 0);
	e->passedPawns[color]			= e->candidatePawns[color] = 0LL;
	e->kingSquares[color]			= SQ_NONE; //not used ATM, add to pawn eval so we can hash king safety?
	e->semiOpenFiles[color]			= 0xFF;
	e->pawnAttacks[color]			= shift_bb<Right>(ourPawns) | shift_bb<Left>(ourPawns);
	e->pawnsOnSquares[color][BLACK] = bit_count(ourPawns & DarkSquares);
	e->pawnsOnSquares[color][WHITE] = boards.pieceCount[color][PAWN] - e->pawnsOnSquares[color][BLACK];


	///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//DELETE OR COMMENT OUT WHEN NOT TESTING
	//BitBoards a = boards;

	//location of pawn, 
	int square;

	//draw from the list of our color pawns until there are no more to evaluate
	while ((square = *list++) != SQ_NONE) {

		int f = file_of(square);

		//piece square table index change if black
		int psq = color == WHITE ? square : bSQ[square];
		e->pieceSqTabScores[color] += make_scores(pieceSqTab[PAWN][mg][psq], pieceSqTab[PAWN][eg][psq]);

		//file of this pawn cannot be semi open
		e->semiOpenFiles[color] &= ~(1LL << f);

		//previous rank, we use a reverse pawn push from our color to get previous
		U64 pr = RankMasks8[rank_of(square + pawn_push(them))];

		//previous rank plus current rank
		bb = pr | RankMasks8[rank_of(square)];

		//flag pawn as passed, isolated, doauble,
		//unsupported or connected
		connected   =   ourPawns    & adjacentFiles[f] & bb;
		unsupported = !(ourPawns    & adjacentFiles[f] & pr);
		isolated    = !(ourPawns    & adjacentFiles[f]);
		doubled	    =   ourPawns    & forward_bb(color, square);
		opposed     =   enemyPawns  & forward_bb(color, square);
		passed      = !(enemyPawns  & passed_pawn_mask(color, square));
		lever       =   enemyPawns  & pawnAttacksBB[square];


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

			bb = pawn_attack_span(color, square) & (ourPawns | enemyPawns); //Pretty sure everything here works pefectly, Haven't looked at black but rank tested it 

			bb = pawn_attack_span(color, square) & RankMasks8[rank_of(backmost_sq(color, bb))];
			
			// If we have an enemy pawn in the same or next rank, the pawn is
			// backward because it cannot advance without being captured.
			backward = (bb | shift_bb<Up>(bb)) & enemyPawns;
		}
		
		if (!(opposed | passed | (pawn_attack_span(color, square) & enemyPawns))) continue; //need to TESTTESTTEST

		// A not-passed pawn is a candidate to become passed, if it is free to
		// advance and if the number of friendly pawns beside or behind this
		// pawn on adjacent files is higher than or equal to the number of
		// enemy pawns in the forward direction on the adjacent files.
			
		candidate = !(opposed | passed | backward | isolated)
				 && (bb = pawn_attack_span(them, square + pawn_push(color)) & ourPawns) != 0 //need to test the & to make sure it works
				 && bit_count(bb) >= bit_count(pawn_attack_span(color, square) & enemyPawns);


		// Passed pawns will be properly scored in evaluation because we need
		// full attack info to evaluate passed pawns. Only the frontmost passed
		// pawn on each file is considered a true passed pawn.


		if (passed && !doubled) 
			e->passedPawns[color] |= boards.square_bb(square);
		
		
		if (isolated)
			val -= Isolated[opposed][f]; 

		if (unsupported && !isolated)
			val -= UnsupportedPawnPenalty;

		if (doubled)
			val -= Doubled[f] / rank_distance(square, lsb(doubled));

		if (backward)
			val -= Backward[opposed][f];

		if (connected)
			val += Connected[f][relative_rankSq(color, square)];

		if (lever)
			val += Lever[relative_rankSq(color, square)]; 

		if (candidate)
		{
			val += CandidatePassed[relative_rankSq(color, square)];

			if (!doubled)
				e->candidatePawns[color] |= square;
		}
		
		
	}
	
	bb = e->semiOpenFiles[color] ^ 0xFF; 

	//length across board that pawns span from left to right
	e->pawnSpan[color] = bb ? (msb(bb) - lsb(bb)) : 0; 

	// In endgame it's better to have pawns on both wings. So give a bonus according
	// to file distance between left and right outermost pawns.
	val += PawnsFileSpan * e->pawnSpan[color];
	
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

	entry->score = evalPawns<WHITE>(boards, entry) - evalPawns<BLACK>(boards, entry);

	return entry;
}


}