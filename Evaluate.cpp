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
U64 KingSide = FileMasks8[5] | FileMasks8[6] | FileMasks8[7];
U64 QueenSide = FileMasks8[2] | FileMasks8[1] | FileMasks8[0];

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

const int threatenedByPawn[STAGE][6] = //midgame/endgame - piece type threatened
{ { 0, 0, 24, 25, 35, 40 }, 
  { 0, 0, 32, 32, 57, 65 } };

const int rookHalfOpen[STAGE] = {9, 4};
const int rookOpen[STAGE] = {15, 9};

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
	int gameScore[COLOR][STAGE]; //white, black - mid game = 0, end game = 1

	U64 kingZone[COLOR]; //zone around king, by color
	int kingAttackers[COLOR]; //number of pieces attacking opponent king
	int kingAttWeights[COLOR]; //added weight of king attackers
	int adjacentKingAttckers[COLOR]; //num pieces attacking sqs adjacent to king

	U64 attackedBy[COLOR][7]; //bitboards of attacked squares by color and piece type, 0 for second index is all pieces of that color

	int mobility[COLOR][STAGE]; //color, midgame/endgame scoring

	int blockages[COLOR];
};

void Evaluate::generateKingZones(const BitBoards & boards, EvalInfo & ev)
{
	//draw zone around king all 8 tiles around him, plus three in front -- north = front for white, south black
	
	for (int side = 0; side < 2; ++side) {
		U64 kZone;

		if (side) kZone = boards.byColorPiecesBB[BLACK][KING];
		else kZone = boards.byColorPiecesBB[WHITE][KING];

		int location = boards.pieceLoc[side][KING][0];

		if (location > 9) {
			kZone |= KING_SPAN << (location - 9);
		}
		else {
			kZone |= KING_SPAN >> (9 - location);
		}

		//add a zone three tiles across in front of the 8 squares surrounding the king
		if (side)  kZone |= kZone << 8;
		else kZone |= kZone >> 8;

		if (location % 8 < 4) {
			kZone &= ~FILE_GH;

		}
		else {
			kZone &= ~FILE_AB;
		}

		if (side) ev.kingZone[BLACK] = kZone & ~boards.byColorPiecesBB[BLACK][KING];
		else ev.kingZone[WHITE] = kZone & ~boards.byColorPiecesBB[WHITE][KING];
	}
}

template<int pT, int color>
void evaluatePieces(const BitBoards & boards, EvalInfo & ev) {

	const int* piece = boards.pieceLoc[color][pT];
	const int them = (color == WHITE ? BLACK : WHITE);
	const int nextPiece = (color == WHITE ? pT : pT + 1);

	//if we're out of pieces stop evaluating
	if (pT == KING) return;

	int square;
	
	ev.attackedBy[color][pT] = 0LL;

	while (square = *piece++ != SQ_NONE) {
		
		U64 bb = pT == BISHOP ? slider_attacks.BishopAttacks(boards.FullTiles, square)
			: pT == ROOK ? slider_attacks.RookAttacks(boards.FullTiles, square)
			: pT == QUEEN ? slider_attacks.QueenAttacks(boards.FullTiles, square)
			 : 0LL; // need to add knight attacks, pawns evaled sepperatly

		//add pinned piece code, not moving pieces out of line from their pin

		//add attack squares to both piece attacks by color, and all piece attacks by color
		ev.attackedBy[color][pT] |= ev.attackedBy[color][0] |= bb;

		//if pieces is attacking zone around king..
		if (bb & ev.kingZone[them]) {

			ev.kingAttackers[color]++;
			ev.kingAttWeights[color] += KingAttackerWeights[pT];		
			U64 adjacent = bb & ev.kingZone[them]; //NEEDS TO BE KING ATTACKS INSTEAD OF KING ZONE!@@!@!@!@!@!@!@@!@@!@!@!@!@!@!@@@!@!@

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

		ev.mobility[color][mg] += midGmMobilityBonus[pT][mobility];
		ev.mobility[color][eg] += endGMobilityBonus[pT][mobility];

		//if piece is threatened by a pawn, apply penalty
		//increasing penalty in threatened piece value
		if (ev.attackedBy[them][PAWN] & square) {
			ev.gameScore[color][mg] -= threatenedByPawn[mg][pT];
			ev.gameScore[color][eg] -= threatenedByPawn[eg][pT];
		}

		//need knight and bishop evals

		if (pT == ROOK) {
			U64 currentFile = FileMasks8[square & 7];

			U64 opawns = boards.byColorPiecesBB[color][PAWN];
			U64 epawns = boards.byColorPiecesBB[them][PAWN];

			bool ownBlockingPawns = false, oppBlockingPawns = false;
			//open and half open file detection add bonus to score of side
			if (currentFile & opawns) {
				ownBlockingPawns = true;
			}
			if (currentFile & epawns) { //move inside if above???
				oppBlockingPawns = true;
			}

			if (!ownBlockingPawns) {
				if (!oppBlockingPawns) {
					ev.gameScore[color][mg] += rookOpen[mg];
					ev.gameScore[color][eg] += rookOpen[eg];
				}
				else {
					ev.gameScore[color][mg] += rookHalfOpen[mg];
					ev.gameScore[color][eg] += rookHalfOpen[eg];
				}
			}
		}
	}

	return evaluatePieces<nextPiece, !color>(boards, ev);
}

template<> //get rid of recursive function too complex error
void evaluatePieces<KING, WHITE>(const BitBoards & boards, EvalInfo & ev) { return; };

int Evaluate::evaluate(const BitBoards & boards, bool isWhite)
{
	EvalInfo ev;
	int score = 0;


	//evaluate knights - queens
	evaluatePieces<KNIGHT, WHITE>(boards, ev);



	return score;
}


void Evaluate::blockedPieces(int side, const BitBoards& boards, EvalInfo & ev)
{
	U64 pawn, epawn, knight, bishop, rook, king;
	U64 empty = boards.EmptyTiles;
	U64 emptyLoc = 1LL;
	int oppo = !side;


	pawn = boards.byColorPiecesBB[side][PAWN];
	knight = boards.byColorPiecesBB[side][KNIGHT];
	bishop = boards.byColorPiecesBB[side][BISHOP];
	rook = boards.byColorPiecesBB[side][ROOK];
	king = boards.byColorPiecesBB[side][KING];

	epawn = boards.byColorPiecesBB[oppo][PAWN];

	//central pawn block bishop blocked
	if (isPiece(bishop, flip(side, C1))
		&& isPiece(pawn, flip(side, D2))
		&& emptyLoc << flip(side, D3))
		ev.blockages[side] -= 24;

	if (isPiece(bishop, flip(side, F1))
		&& isPiece(pawn, flip(side, E2))
		&& emptyLoc << flip(side, E3))
		ev.blockages[side] -= 24;

	//trapped knights
	if (isPiece(knight, flip(side, A8))
		&& isPiece(epawn, flip(oppo, A7))
		|| isPiece(epawn, flip(oppo, C7)))
		ev.blockages[side] -= 150;

	//trapped knights
	if (isPiece(knight, flip(side, H8))
		&& isPiece(epawn, flip(oppo, H7))
		|| isPiece(epawn, flip(oppo, F7)))
		ev.blockages[side] -= 150;

	if (isPiece(knight, flip(side, A7))
		&& isPiece(epawn, flip(oppo, A6))
		&& isPiece(epawn, flip(oppo, B7)))
		ev.blockages[side] -= 150;

	if (isPiece(knight, flip(side, H7))
		&& isPiece(epawn, flip(oppo, H6))
		&& isPiece(epawn, flip(oppo, G7)))
		ev.blockages[side] -= 150;

	//knight blocking queenside pawn
	if (isPiece(knight, flip(side, C3))
		&& isPiece(knight, flip(side, C2))
		&& isPiece(knight, flip(side, D4))
		&& !isPiece(knight, flip(side, E4)))
		ev.blockages[side] -= 5;

	//trapped bishop
	if (isPiece(bishop, flip(side, A7))
		&& isPiece(epawn, flip(oppo, B6)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, flip(side, H7))
		&& isPiece(epawn, flip(oppo, G6)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, flip(side, B8))
		&& isPiece(epawn, flip(oppo, C7)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, flip(side, G8))
		&& isPiece(epawn, flip(oppo, F7)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, flip(side, A6))
		&& isPiece(epawn, flip(oppo, B5)))
		ev.blockages[side] -= 50;

	if (isPiece(bishop, flip(side, H6))
		&& isPiece(epawn, flip(oppo, G5)))
		ev.blockages[side] -= 50;

	// bishop on initial square supporting castled king
	//if(isPiece(bishop, flip(side, F1))
	//&& isPiece(king, flip(side, B1)))
	//need positional themes

	//uncastled king blocking own rook
	if ((isPiece(king, flip(side, F1)) || isPiece(king, flip(side, G1)))
		&& (isPiece(rook, flip(side, H1)) || isPiece(rook, flip(side, G1))))
		ev.blockages[side] -= 24;

	if ((isPiece(king, flip(side, C1)) || isPiece(king, flip(side, B1)))
		&& (isPiece(rook, flip(side, A1)) || isPiece(rook, flip(side, B1))))
		ev.blockages[side] -= 24;
}

bool Evaluate::isPiece(const U64 &piece, int sq)
{
	//is the piece on the square?
	U64 loc = 1LL << sq;
	if (loc & piece) return true;
	return false;
}

int Evaluate::flip(int side, int sq)
{
	return side == WHITE ? sq : bSQ[sq];
}
