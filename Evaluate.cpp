#include "Evaluate.h"
#include "defines.h"
#include "bitboards.h"
#include "externs.h"

const U64 RankMasks8[8] =/*from rank8 to rank1 ?*/
{
	0xFFL, 0xFF00L, 0xFF0000L, 0xFF000000L, 0xFF00000000L, 0xFF0000000000L, 0xFF000000000000L, 0xFF00000000000000L
};
const U64 FileMasks8[8] =/*from fileA to FileH*/
{
	0x101010101010101L, 0x202020202020202L, 0x404040404040404L, 0x808080808080808L,
	0x1010101010101010L, 0x2020202020202020L, 0x4040404040404040L, 0x8080808080808080L
};
//for flipping squares in blockers if black
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

//masks for determining which side king is on
//U64 wKingSide = FileMasks8[5] | FileMasks8[6] | FileMasks8[7];
//U64 wQueenSide = FileMasks8[2] | FileMasks8[1] | FileMasks8[0];

const U64 full = 0xffffffffffffffffULL;

//adjustments of piece value based on our color pawn count
const int knight_adj[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 10 };
const int rook_adj[9] = { 15,  12,   9,  6,  3,  0, -3, -6, -9 };

//values definitely need to be adjusted and file/rank/side variable
const int rookOpenFile = 10;
const int rookHalfOpenFile = 5;

//positive value
const int BISHOP_PAIR = 30;
//used as negatives to incourage bishop pair
const int KNIGHT_PAIR = 8;
const int ROOK_PAIR = 16;

const int threatenedByPawn[2][6] = 
{ { 0, 0, 24, 25, 35, 40 }, 
  { 0, 0, 32, 32, 57, 65 } };


static const int SafetyTable[100] = {
	0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
	18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
	68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

//mid game mobility bonus per square of mobiltiy
const int midGmMobilityBonus[6][28]{
	{},{}, //no piece and pawn indexs
	{ -20, -16, -2, 3, 6, 10, 15, 17, 19 }, //knights 

	{ -17, -10, 1, 4, 7, 11, 14, //bishops
	17, 19, 20, 21, 22, 22, 23 },

	{ -12, -8, -3, 1, 4, 6, 9, 11, //rooks
	12, 13, 14, 15, 15, 15, 16 },

	{ -14, -9, -3, 0, 2, 4, 5, 6, 7, 7,//queens
	7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9 }

};

//end game mobility bonus
const int endGMobilityBonus[6][28]{
	{},{},
	{ -17, -14, -3, 0, 3, 6, 9, 10, 11 }, //knights 

	{ -15, -7, 1, 6, 9, 12, 14, //bishops
	15, 17, 17, 17, 19, 19, 19 },

	{ -14, -7, 0, 4, 9, 13, 17, 21, //rooks 
	24, 27, 29, 29, 30, 31, 31 },

	{ -14, -8, -4, 0, 3, 5, 10, 15, //queens
	16, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17, 17, 17, 17, 17,
	17, 17, 17, 17 }
};

const int KingAttackerWeights[6] = { 0, 0, 2, 2, 3, 5 };


struct EvalInfo {
	U64 kingZone[2]; //zone around king, by color
	int kingAttackers[2];
	int kingAttWeights[2];
	int adjacentKingAttckers[2];

	U64 attackedBy[2][7]; //bitboards of attacked squares by color and piece type, 0 for second index is all pieces of that color
	
	int mobility[2][2]; //color, midgame/endgame scoring
};


template<int pT, int color>
int evaluatePieces(const BitBoards & boards, EvalInfo & ev) {

	const int* piece = boards.pieceLoc[color][pT];
	const int them = (color == WHITE ? BLACK : WHITE);

	int square;
	
	ev.attackedBy[color][pT] = 0LL;
	U64 bb; 

	while (square = *piece++ != SQ_NONE) {
		
		bb = pT == BISHOP ? slider_attacks.BishopAttacks(boards.FullTiles, square)
			: pT == ROOK ? slider_attacks.RookAttacks(boards.FullTiles, square)
			: pT == QUEEN ? slider_attacks.QueenAttacks(boards.FullTiles, square); // need to add knight and king attacks, pawns evaled sepperatly

		//add pinned piece code, not moving pieces out of line from their pin

		//add attack squares to both piece attacks by color, and all piece attacks by color
		ev.attackedBy[color][pT] |= ev.attackedBy[color][0] |= bb;

		if (bb & ev.kingZone[them]) {

			ev.kingAttackers[color]++;
			ev.kingAttWeights[color] += KingAttackerWeights[pT];		
			U64 adjacent = bb & ev.kingZone[them]; //NEEDS TO BE KING ATTACKS INSTEAD OF KING ZONE

			if (adjacent) ev.adjacentKingAttckers[color] += bit_count(adjacent);
		}

		//if piece is queen, don't count squares that are attacked by 
		//knight, bishop, rook towards mobility
		if (pT == QUEEN)
			bb &= ~(ev.attackedBy[them][KNIGHT]
				| ev.attackedBy[them][BISHOP]
				| ev.attackedBy[them][ROOK]);


		//gather mobility count and record mob scores for mid and end game
		int mobility = bit_count(bb);

		ev.mobility[color][0] = midGmMobilityBonus[pT][mobility];
		ev.mobility[color][1] = endGMobilityBonus[pT][mobility];
	}

}

int Evaluate::evaluate(const BitBoards & boards, bool isWhite)
{
	EvalInfo ev;
	int score = 0;

	score = evaluatePieces<KNIGHT, WHITE>(boards, ev);



	return score;
}
