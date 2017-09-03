#include "Evaluate.h"

#include <iostream>

#include "defines.h"
#include "bitboards.h"
#include "externs.h"
#include "psqTables.h"
#include "hashentry.h"
#include "Pawns.h"


const U64 RankMasks8[8] = //from rank 1 - 8 
{
	0xFF00000000000000L, 0xFF000000000000L,  0xFF0000000000L, 0xFF00000000L, 0xFF000000L, 0xFF0000L, 0xFF00L, 0xFFL,
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

U64 center[STAGE] = {
	(FileCBB | FileDBB | FileEBB | FileFBB) & (RankMasks8[1] | RankMasks8[2] | RankMasks8[3]),
	(FileCBB | FileDBB | FileEBB | FileFBB) & (RankMasks8[6] | RankMasks8[5] | RankMasks8[4])
};

const U64 full = 0xffffffffffffffffULL;

//adjustments of piece value based on our color pawn count
const int knight_adj[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 10 };
const int rook_adj[9] = { 15,  12,   9,  6,  3,  0, -3, -6, -9 };

//positive value
const int BISHOP_PAIR = 30;
//used as negatives to incourage bishop pair
const int KNIGHT_PAIR = 8;
const int ROOK_PAIR = 16;
const int Hanging[STAGE] = {9, 7};
const int kingOnPawn[STAGE] = {0, 30};
const int kingOnMPawn[STAGE] = {0, 64};


const int threatenedByPawn[STAGE][6] = //midgame/endgame - piece type threatened
{ { 0, 0, 24, 25, 35, 40 }, 
  { 0, 0, 32, 32, 57, 65 } };
/*
const int threatenedByPawn[STAGE][6] = //midgame/endgame - piece type threatened
{ { 0, 0, 10, 13, 23, 32 },
{ 0, 0, 12, 15, 35, 45 } };
*/
const int threats[2][2][6] = //minor/major - mid/endgame threats/ piece type
{   //mid game             //end game
	{{0, 2, 6, 6, 12, 12}, {0, 8, 10, 10, 20, 20},}, //minor
	{{0, 4, 4, 4,  4,  6}, {0, 8, 10, 10, 10, 11}}   //major
};

const int rookHalfOpen[STAGE] = {5, 3};
const int rookOpen[STAGE] = {10, 5};

const int QueenContactCheck = 7;
const int RookContactCheck = 5;
const int QueenCheck = 3;
const int RookCheck = 2;
const int BishopCheck = 1;
const int KnightCheck = 1;

const int KingAttackerWeights[6] = { 0, 0, 2, 2, 3, 5 };

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


struct EvalInfo {
	int gameScore[COLOR][STAGE] = { { 0 } }; //white, black - mid game = 0, end game = 1

	int psqScores[COLOR][STAGE] = { { 0 } }; //piece square table scores

	U64 kingZone[COLOR] = {0LL}; //zone around king, by color
	int kingAttackers[COLOR] = {0}; //number of pieces attacking opponent king
	int kingAttWeights[COLOR] = { 0 }; //added weight of king attackers
	int adjacentKingAttckers[COLOR] = { 0 }; //num pieces attacking sqs adjacent to king

	U64 attackedBy[COLOR][7] = { {0LL} }; //bitboards of attacked squares by color and piece type, 0 for second index is all pieces of that color

	int mobility[COLOR][STAGE] = { { 0 } }; //color, midgame/endgame scoring

	int blockages[COLOR] = { 0 };

	int adjustMaterial[COLOR] = { 0 };

	Pawns::PawnsEntry * pe;
};

void Evaluate::generateKingZones(const BitBoards & boards, EvalInfo & ev)
{
	//draw zone around king all 8 tiles around him, plus three in front -- north = front for white, south black
	
	for (int side = 0; side < 2; ++side) {
		U64 kZone, tkz;

		if (side) kZone = boards.byColorPiecesBB[BLACK][KING];
		else kZone = boards.byColorPiecesBB[WHITE][KING];

		int location = boards.pieceLoc[side][KING][0];

		//piece square table flip if black
		int psqLoc = side == WHITE ? location : bSQ[location];

		//psq table scoring for king
		ev.psqScores[side][mg] += pieceSqTab[KING][mg][psqLoc];
		ev.psqScores[side][eg] += pieceSqTab[KING][eg][psqLoc];

		//grab pseudo attacks for king on square
		kZone = boards.psuedoAttacks(KING, side, location);

		//add king attacks to attacked by and attacked by all
		ev.attackedBy[side][0] |= ev.attackedBy[side][KING] = kZone;

		//add a zone three tiles across in front of the 8 squares surrounding the king
		if (side)  kZone |= kZone << 8;
		else kZone |= kZone >> 8;

		if (location % 8 < 4) {
			kZone &= ~FILE_GH;

		}
		else {
			kZone &= ~FILE_AB;
		}
		//remove square king is on, maybe delete this in future?
		if (side) ev.kingZone[BLACK] = kZone & ~boards.byColorPiecesBB[BLACK][KING];
		else ev.kingZone[WHITE] = kZone & ~boards.byColorPiecesBB[WHITE][KING];
	}
}

template<int pT, int color>
void evaluatePieces(const BitBoards & boards, EvalInfo & ev, U64 * mobilityArea) {

	const int* piece = boards.pieceLoc[color][pT];
	const int them = (color == WHITE ? BLACK : WHITE);

	//increment piece type if we're on a black piece
	const int nextPiece = (color == WHITE ? pT : pT + 1);

	int square;

	while (( square = *piece++ ) != SQ_NONE) {

		//adjust square table lookup for black if need be
		int adjSq = color == WHITE ? square : bSQ[square];

		//add piece square table scores 
		ev.psqScores[color][mg] += pieceSqTab[pT][mg][adjSq];
		ev.psqScores[color][eg] += pieceSqTab[pT][eg][adjSq];		

		//if (pT == PAWN) { //remove later once we have sepperate pawn eval class
		//	ev.attackedBy[color][0] |= ev.attackedBy[color][PAWN] |= boards.psuedoAttacks(PAWN, color, square);
		//	continue;
		//}
		
		//find attack squares, including xray bishop and rook attacks blocked by queens / rooks/queens
		U64 bb = pT == BISHOP ? slider_attacks.BishopAttacks(boards.FullTiles ^ boards.pieces(color, QUEEN), square)
			: pT == ROOK ? slider_attacks.RookAttacks(boards.FullTiles ^ boards.pieces(color, QUEEN, ROOK), square)
			: pT == QUEEN ? slider_attacks.QueenAttacks(boards.FullTiles, square)
			 : boards.PseudoAttacks[KNIGHT][square]; 

		//add pinned piece code, not moving pieces out of line from their pin

		//add attack squares to both piece attacks by color, and all piece attacks by color
		//very important order, if all pieces comes first the pT will grab all attack boards generated.
		ev.attackedBy[color][0] |= ev.attackedBy[color][pT] |= bb;

		//if pieces is attacking zone around king..
		if (bb & ev.kingZone[them]) {

			ev.kingAttackers[color]++;
			ev.kingAttWeights[color] += KingAttackerWeights[pT];		
			U64 adjacent = bb & ev.attackedBy[them][KING]; 

			//if piece is attacking squares defended by the enemy king..
			if (adjacent) ev.adjacentKingAttckers[color] += bit_count(adjacent);
		}

		//if piece is queen, don't count squares that are attacked by 
		//knight, bishop, rook towards mobility
		if (pT == QUEEN)
			bb &= ~(ev.attackedBy[them][KNIGHT]
				| ev.attackedBy[them][BISHOP]
				| ev.attackedBy[them][ROOK]);

		//gather mobility count and record mob scores for mid and end game
		//don't count squares occupied by our pawns or king, or attacked by their pawns
		//towards mobility. Additional restrictions for queen mob.
		int mobility = bit_count(bb & mobilityArea[color]);

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

	if (pT == PAWN) { //remove this once we get sepperate pawn eval
		mobilityArea[them] = ~(ev.attackedBy[color][PAWN] | mobilityArea[them]);
	}

	return evaluatePieces<nextPiece, !color>(boards, ev, mobilityArea);
}

template<> //get rid of recursive function too complex error, also return when piece type hits king
void evaluatePieces<KING, WHITE>(const BitBoards & boards, EvalInfo & ev, U64 * mobility) { return; };

template<int color>
int centerControl(const BitBoards & boards, const EvalInfo & ev) {

	const int them = color == WHITE ? BLACK : WHITE;

	BitBoards a = boards;
	//find the safe tiles, safe if not attacked by enemy pawn,
	//or if it is attacked by an enemy piece and not defended
	U64 safe = center[color]
			& ~boards.pieces(color, PAWN)
			& ~ev.attackedBy[them][PAWN]
			& (ev.attackedBy[color][0] | ev.attackedBy[them][0]);
	//a.drawBBA();
	//a.drawBB(center[color]);
	//a.drawBB(safe);

	//find squares which are between one and three squares behind friendly pawns
	U64 behind = boards.pieces(color, PAWN);

	behind |= (color == WHITE ? behind <<  8 : behind >>  8);
	behind |= (color == WHITE ? behind << 16 : behind >> 16);

	//a.drawBB(boards.pieces(color, PAWN));
	//a.drawBB(behind);
	//a.drawBB(RankMasks8[1]);

	return bit_count((color == WHITE ? safe >> 32 : safe << 32) | (behind & safe));
}

template<int color>
int evaluateKing(const BitBoards & boards, EvalInfo & ev, int mgScore) {

	const int them = color == WHITE ? BLACK : WHITE;
	int attackUnits = 0;
	int score = 0;

	//our king location
	const int square = msb(boards.byColorPiecesBB[color][KING]);

	U64 undefended, safe;

	if (ev.kingAttackers[them]) {
				
		//find attacked squares around king which have no defenders apart from king
		undefended = ev.attackedBy[them][0] //attacked by squares for all pieces of color
				& ev.attackedBy[color][KING]
				& ~( ev.attackedBy[color][PAWN] | ev.attackedBy[color][KNIGHT]
				| ev.attackedBy[color][BISHOP] | ev.attackedBy[color][ROOK]
				| ev.attackedBy[color][QUEEN]);

		attackUnits = std::min(17, (ev.kingAttackers[them] * ev.kingAttWeights[them]) / 2)
					+ 2 * (ev.adjacentKingAttckers[them] + bit_count(undefended))
					- mgScore / 32
					- !boards.pieceCount[them][QUEEN] * 15;

		//queen enemy safe contact checks, where enemy queen is next to king 
		U64 bb = undefended & ev.attackedBy[them][QUEEN] & ~boards.pieces(them);

		if (bb) {
				//remove squares defended by enemy pieces
			bb &= (ev.attackedBy[them][PAWN] | ev.attackedBy[them][KNIGHT]
				| ev.attackedBy[them][BISHOP] | ev.attackedBy[them][ROOK]);

			if (bb) attackUnits += QueenContactCheck * bit_count(bb);
		}

		//rook safe enemy contact checks
		bb = undefended & ev.attackedBy[them][ROOK] & ~boards.pieces(them);

		//only look at checking squares, & with rook attacks from king location
		bb &= boards.psuedoAttacks(ROOK, color, square);

		if (bb) {
				//remove squares defended by enemy pieces
			bb &= (ev.attackedBy[them][PAWN] | ev.attackedBy[them][KNIGHT]
				| ev.attackedBy[them][BISHOP] | ev.attackedBy[them][ROOK]);

			if (bb) attackUnits += RookContactCheck * bit_count(bb);
		}

		//look at enemy safe checks from sliders and knights
		safe = ~(boards.pieces(them) | ev.attackedBy[color][0]);

					//color doesn't matter here, attacks are the same
		U64 b1 = boards.psuedoAttacks(ROOK, color, square) & safe;
		U64 b2 = boards.psuedoAttacks(BISHOP, color, square) & safe;

		bb = (b1 | b2) & ev.attackedBy[them][QUEEN];

		//enemy safe queen checks
		if (bb) attackUnits += QueenCheck * bit_count(bb);

		bb = b1 & ev.attackedBy[them][ROOK];

		if(bb) attackUnits += RookCheck * bit_count(bb);

		//enemy safe bishop checks
		bb = b2 & ev.attackedBy[them][BISHOP];

		if(bb) attackUnits += BishopCheck * bit_count(bb);

		//enemy safe knight checks

		bb = boards.psuedoAttacks(KNIGHT, them, square) & ev.attackedBy[them][KNIGHT] & safe;

		if(bb) attackUnits += KnightCheck * bit_count(bb);

		attackUnits = std::min(99, attackUnits);

		score -= SafetyTable[attackUnits];
	}

	return score;
}

template<int color>
void evalThreats(const BitBoards & boards, EvalInfo & ev) {

	int them = (color == WHITE ? BLACK : WHITE);

	U64 bb, weakEnemys, protectedEnemys;

	enum {Minor, Major};

	//enemys (that are not their pawns) which are protected by their knights or bishops
	protectedEnemys = (boards.pieces(them) ^ boards.pieces(them, PAWN))
					& ev.attackedBy[them][PAWN]
					& (ev.attackedBy[color][KNIGHT] | ev.attackedBy[color][BISHOP]);


	//if an enemy is protected by a pawn, and attacked by our bishop or knight
	if (protectedEnemys) {
		int p = boards.pieceOnSq(lsb(protectedEnemys));

		ev.gameScore[color][mg] += threats[Minor][mg][p];
		ev.gameScore[color][eg] += threats[Minor][eg][p];
	}

	weakEnemys = boards.pieces(them)
			   & ~ev.attackedBy[them][0] //not supported by their pieces
			   & ev.attackedBy[color][0]; //and attacked by any of our pieces

	//add bonus for attacking undefended pieces
	if (weakEnemys) {

		//enemy piece is attacked by our knight or bishop
		bb = weakEnemys & (ev.attackedBy[color][KNIGHT] | ev.attackedBy[color][BISHOP]);

		if (bb) {
			int p = boards.pieceOnSq(lsb(bb));

			ev.gameScore[color][mg] += threats[Minor][mg][p];
			ev.gameScore[color][eg] += threats[Minor][eg][p];
		}

		//enemy piece is attacked by our rook/queen
		bb = weakEnemys & (ev.attackedBy[color][ROOK] | ev.attackedBy[color][QUEEN]);

		if (bb) {
			int p = boards.pieceOnSq(lsb(bb));

			ev.gameScore[color][mg] += threats[Major][mg][p];
			ev.gameScore[color][eg] += threats[Major][eg][p];
		}

		//enemy piece/s aren't supported
		bb = weakEnemys & ~ev.attackedBy[them][0]; 

		if (bb) {
			int pop = bit_count(bb);

			ev.gameScore[color][mg] += pop * Hanging[mg];
			ev.gameScore[color][eg] += pop * Hanging[eg];
		}

		//enemy pawn/s are attacked by our king
		bb = weakEnemys & boards.pieces(them, PAWN) & ev.attackedBy[color][KING];

		if (bb) {
			pop_lsb(&bb);
			//one or many pawns?
			int score = bb ? kingOnMPawn[eg] : kingOnPawn[eg];

			//no mid game value for this threat
			ev.gameScore[color][eg] += score;
		}
	}
}

enum {Mobility,  PawnStructure,   Center, KingSafety};

const struct Weight { int mg, eg; } Weights[] = { 
	{ 289, 344 }, { 233, 201 }, { 50, 0 }, { 318, 0 }
};
/*
const struct Weight { int mg, eg; } Weights[] = { //Test Not Good
	{ 289, 320 },{ 0, 0 },{ 50, 0 },{ 290, 0 }
};
*/
//for applying non phase independant scoring.
//Weights are used to modify values in a particular section 
//compared to those of other sections in overall scoring
void applyWeights(int* score, const Weight & w, int val) {
	
	score[mg] += val * w.mg / 256;
	score[eg] += val * w.eg / 256;
}
//used when there are sepperate values from mid and end game.
void applyWeights(int* score, const Weight & w, int val, int val1) {

	score[mg] += val  * w.mg / 256;
	score[eg] += val1 * w.eg / 256;
}

int Evaluate::evaluate(const BitBoards & boards, bool isWhite)
{
	int hash = boards.zobrist.zobristKey & 5021982; //REPLACE THIS!!
	HashEntry entry = transpositionEval[hash];
	//if we get a hash-table hit, return the evaluation
	if (entry.zobrist == boards.zobrist.zobristKey) {
		if (isWhite) {
			//if eval was from blacks POV, return -eval
			if (entry.flag) return -entry.eval;
			else return entry.eval; //if eval was from our side, return normally
		}
		else {

			if (entry.flag) return entry.eval;
			else return -entry.eval;
		}

	}
	
	EvalInfo ev;
	int result = 0, score[STAGE] = { 0 };

	//initilize king zones and king attacks for both kings
	generateKingZones(boards, ev);

	///TEST PAWN CLASS
	ev.pe = Pawns::probe(boards, pawnsTable); //NEED TO CONVERT ALL SCORING IN THIS FILE TO SCORES STRUCT

	score[mg] = ev.pe->score[mg];
	score[eg] = ev.pe->score[eg];

	ev.attackedBy[WHITE][0] |= ev.attackedBy[WHITE][PAWN] = ev.pe->pawnAttacks[WHITE];
	ev.attackedBy[BLACK][0] |= ev.attackedBy[BLACK][PAWN] = ev.pe->pawnAttacks[BLACK];

	ev.psqScores[WHITE][mg] += ev.pe->pieceSqTabScores[WHITE].mg;
	ev.psqScores[BLACK][mg] += ev.pe->pieceSqTabScores[BLACK].mg;
	ev.psqScores[WHITE][eg] += ev.pe->pieceSqTabScores[WHITE].eg;
	ev.psqScores[BLACK][eg] += ev.pe->pieceSqTabScores[BLACK].eg;

	//evaluate all pieces, except for most pawn info and king. Start at knight once we have
	//a sepperate class for pawn info, holding a more relevent pawn TT than we have now
	//right now we're passing it a mobility area that's calcuated mostly inside, later 
	//we'll have to add exclusion of pawns && pawn attack squares outside the function.

	U64 mobilityArea[COLOR]; //remove our king+our pawns from our color mobility areas
	mobilityArea[WHITE] = boards.pieces(WHITE, PAWN, KING); //remove protected by pawns in evaluate pices function until we have 
	mobilityArea[BLACK] = boards.pieces(BLACK, PAWN, KING); //a sepperate pawn eval and advanced pawn hash table
															//mobilityArea[color] = ~(ev.attackedBy[them][PAWN] | mobilityArea[color]);
	evaluatePieces<KNIGHT, WHITE>(boards, ev, mobilityArea);

	//can only evaluate threats after piece attack boards have been generated
	evalThreats<WHITE>(boards, ev);
	evalThreats<BLACK>(boards, ev);

	score[mg] += boards.bInfo.sideMaterial[WHITE] + ev.psqScores[WHITE][mg] - boards.bInfo.sideMaterial[BLACK] - ev.psqScores[BLACK][mg];
	score[eg] += boards.bInfo.sideMaterial[WHITE] + ev.psqScores[WHITE][eg] - boards.bInfo.sideMaterial[BLACK] - ev.psqScores[BLACK][eg];

	int ctrl =  centerControl<WHITE>(boards, ev) - centerControl<BLACK>(boards, ev);
	applyWeights(score, Weights[Center], ctrl);

	int gamePhase = 0;;

	for (int color = 1; color < 2; color++) {
		gamePhase += boards.pieceCount[color][KNIGHT];
		gamePhase += boards.pieceCount[color][BISHOP];
		gamePhase += boards.pieceCount[color][ROOK] * 2;
		gamePhase += boards.pieceCount[color][QUEEN] * 4;
	}

	blockedPieces(WHITE, boards, ev);
	blockedPieces(BLACK, boards, ev);

	score[mg] += wKingShield(boards) - bKingShield(boards);

	//adjusting material value of pieces bonus for bishop, small penalty for others
	if (boards.pieceCount[WHITE][BISHOP] > 1) ev.adjustMaterial[WHITE] += BISHOP_PAIR;
	if (boards.pieceCount[BLACK][BISHOP] > 1) ev.adjustMaterial[BLACK] -= BISHOP_PAIR;
	if (boards.pieceCount[WHITE][KNIGHT] > 1) ev.adjustMaterial[WHITE] -= KNIGHT_PAIR;
	if (boards.pieceCount[BLACK][KNIGHT] > 1) ev.adjustMaterial[BLACK] += KNIGHT_PAIR;
	if (boards.pieceCount[WHITE][ROOK] > 1) ev.adjustMaterial[WHITE] -= ROOK_PAIR;
	if (boards.pieceCount[BLACK][ROOK] > 1) ev.adjustMaterial[BLACK] += ROOK_PAIR;


	ev.adjustMaterial[WHITE] += knight_adj[(boards.pieceCount[WHITE][PAWN])] * boards.pieceCount[WHITE][KNIGHT];
	ev.adjustMaterial[BLACK] -= knight_adj[boards.pieceCount[BLACK][PAWN]] * boards.pieceCount[BLACK][KNIGHT];
	ev.adjustMaterial[WHITE] += rook_adj[boards.pieceCount[WHITE][PAWN]] * boards.pieceCount[WHITE][ROOK];
	ev.adjustMaterial[BLACK] -= rook_adj[boards.pieceCount[BLACK][PAWN]] * boards.pieceCount[BLACK][ROOK];

	//probe pawn hash table for a hit, if we don't get a hit
	//proceed with pawnEval
	//result += getPawnScore(boards, ev);

	//evaluate both kings. Function returns a score taken from the king safety array
	int ksf = evaluateKing<WHITE>(boards, ev, score[mg]) - evaluateKing<BLACK>(boards, ev, score[mg]);
	applyWeights(score, Weights[KingSafety],  ksf);

	//add threat and some other scoring done in eval pieces, etc to scores
	score[mg] += ev.gameScore[WHITE][mg] - ev.gameScore[BLACK][mg];
	score[eg] += ev.gameScore[WHITE][eg] - ev.gameScore[BLACK][eg];

	//score[mg] += (ev.mobility[WHITE][mg] - ev.mobility[BLACK][mg]);
	//score[eg] += (ev.mobility[WHITE][eg] - ev.mobility[BLACK][eg]);
	int m1 = ev.mobility[WHITE][mg] - ev.mobility[BLACK][mg];
	int m2 = ev.mobility[WHITE][eg] - ev.mobility[BLACK][eg];

	applyWeights(score, Weights[Mobility], m1, m2);//TEST

	if (gamePhase > 24) gamePhase = 24;
	int mgWeight = gamePhase;
	int egWeight = 24 - gamePhase;

	result += ((score[mg] * mgWeight) + (score[eg] * egWeight)) / 24;

	result += (ev.blockages[WHITE] - ev.blockages[BLACK]);
	result += (ev.adjustMaterial[WHITE] - ev.adjustMaterial[BLACK]);

	//low material adjustment scoring here
	int strong = result > 0 ? WHITE : BLACK;
	int weak = !strong;

	if (boards.pieceCount[strong][PAWN] == 0) { // NEED more intensive filters for low material

		if (boards.bInfo.sideMaterial[strong] < 400) return 0;

		if (boards.pieceCount[weak][PAWN] == 0 && boards.bInfo.sideMaterial[strong] == 2 * SORT_VALUE[KNIGHT])  return 0;//two knights

		if (boards.bInfo.sideMaterial[strong] == SORT_VALUE[ROOK] && boards.bInfo.sideMaterial[weak] == SORT_VALUE[BISHOP]) result /= 2;

		if (boards.bInfo.sideMaterial[strong] == SORT_VALUE[ROOK] + SORT_VALUE[BISHOP]
			&& boards.bInfo.sideMaterial[weak] == SORT_VALUE[ROOK]) result /= 2;

		if (boards.bInfo.sideMaterial[strong] == SORT_VALUE[ROOK] + SORT_VALUE[KNIGHT]
			&& boards.bInfo.sideMaterial[weak] == SORT_VALUE[ROOK]) result /= 2;
	}


	//switch score for color
	result = isWhite ? result : -result;

	//save to TT eval table
	saveTT(isWhite, result, hash, boards);

	return result;
}

int Evaluate::wKingShield(const BitBoards & boards)
{
	//gather info on defending pawns
	int result = 0;
	U64 pawns = boards.byColorPiecesBB[WHITE][PAWN];
	U64 king = boards.byColorPiecesBB[WHITE][KING];

	U64 location = 1LL;

	//king on kingside
	if (KingSide & king) {
		if (pawns & (location << F2)) result += 10;
		else if (pawns & (location << F3)) result += 5;

		if (pawns & (location << G2)) result += 10;
		else if (pawns & (location << G3)) result += 5;

		if (pawns & (location << H2)) result += 10;
		else if (pawns & (location << H3)) result += 5;
	}
	else if (QueenSide & king) {
		if (pawns & (location << A2)) result += 10;
		else if (pawns & (location << A3)) result += 5;

		if (pawns & (location << B2)) result += 10;
		else if (pawns & (location << B3)) result += 5;

		if (pawns & (location << C2)) result += 10;
		else if (pawns & (location << C3)) result += 5;
	}
	return result;
}

int Evaluate::bKingShield(const BitBoards & boards)
{
	int result = 0;
	U64 pawns = boards.byColorPiecesBB[BLACK][PAWN];
	U64 king = boards.byColorPiecesBB[BLACK][KING];

	U64 location = 1LL;

	//king on kingside
	if (QueenSide & king) {
		if (pawns & (location << F7)) result += 10;
		else if (pawns & (location << F6)) result += 5;

		if (pawns & (location << G7)) result += 10;
		else if (pawns & (location << G6)) result += 5;

		if (pawns & (location << H7)) result += 10;
		else if (pawns & (location << H6)) result += 5;
	}
	//queen side
	else if (KingSide & king) {
		if (pawns & (location << A7)) result += 10;
		else if (pawns & (location << A6)) result += 5;

		if (pawns & (location << B7)) result += 10;
		else if (pawns & (location << B6)) result += 5;

		if (pawns & (location << C7)) result += 10;
		else if (pawns & (location << C6)) result += 5;
	}
	return result;
}

void Evaluate::saveTT(bool isWhite, int result, int hash, const BitBoards &boards)
{
	//store eval into eval hash table
	transpositionEval[hash].eval = result;
	transpositionEval[hash].zobrist = boards.zobrist.zobristKey;

	//set flag for TT so we can switch value if we get a TT hit but
	//the color of the eval was opposite
	if (isWhite) transpositionEval[hash].flag = 0;
	else transpositionEval[hash].flag = 1;
}

int Evaluate::getPawnScore(const BitBoards & boards, EvalInfo & ev)
{

	//get zobristE/bitboards of current pawn positions
	U64 pt = boards.byPieceType[PAWN];
	int hash = pt & 399999;

	//probe pawn hash table using bit-wise OR of white pawns and black pawns as zobrist key
	if (transpositionPawn[hash].zobrist == pt) {
		return transpositionPawn[hash].eval;
	}

	//U64 pt = boards.BBWhitePawns | boards.BBBlackPawns;
	//const PawnEntry *ttpawnentry;
	//ttpawnentry = TT.probePawnT(pt);
	//if (ttpawnentry) return ttpawnentry->eval;

	//if we don't get a hash hit, search through all pawns on boards and return score
	U64 wPawns = boards.byColorPiecesBB[WHITE][PAWN];
	U64 bPawns = boards.byColorPiecesBB[BLACK][PAWN];

	int score = 0;
	while (wPawns) {
		int loc = pop_lsb(&wPawns);
		score += pawnEval(boards, WHITE, loc);
		
	}

	while (bPawns) {
		int loc = pop_lsb(&bPawns);
		score -= pawnEval(boards, BLACK, loc);
	}

	//store entry to pawn hash table
	transpositionPawn[hash].eval = score;
	transpositionPawn[hash].zobrist = pt;

	//TT.savePawnEntry(pt, score);

	return score;
}

int Evaluate::pawnEval(const BitBoards & boards, int side, int location)
{
	int result = 0; // WE will be replacing this entire function soon.
	int flagIsPassed = 1; // we will be trying to disprove that
	int flagIsWeak = 1;   // we will be trying to disprove that
	int flagIsOpposed = 0;

	U64 pawn = boards.squareBB[location];
	U64 opawns = boards.byColorPiecesBB[side][PAWN];
	U64 epawns = boards.byColorPiecesBB[!side][PAWN];


	int file = location % 8;
	int rank = location / 8;
	int flip[8] = {7, 6, 5, 4, 3, 2, 1, 0};
	rank = flip[rank];

	U64 doubledPassMask = FileMasks8[file]; //mask for finding doubled or passed pawns

	U64 left = 0LL;
	if (file > 0) left = FileMasks8[file - 1]; //masks are accoring to whites perspective

	U64 right = 0LL;
	if (file < 7) right = FileMasks8[file + 1]; //files to the left and right of pawn

	U64 supports = right | left, tmpSup = 0LL; //mask for area behind pawn and to the left an right, used to see if weak + mask for holding and values

	opawns &= ~pawn; //remove this pawn from his friendly pawn BB so as not to count himself in doubling

	if (doubledPassMask & opawns) result -= 10; //real value for doubled pawns is -20, because this method counts them twice it's set at half real

	if (!side) { //if is white
		for (int i = 0; i < rank + 1; i++) {
			doubledPassMask &= ~RankMasks8[i];
			left &= ~RankMasks8[i];
			right &= ~RankMasks8[i];
			tmpSup |= RankMasks8[i];
		}
	}
	else {		
		
		for (int i = 7; i > rank - 1; i--) {
			doubledPassMask &= ~RankMasks8[i];
			left &= ~RankMasks8[i];
			right &= ~RankMasks8[i];
			tmpSup |= RankMasks8[i];
		}
	}


	//if there is an enemy pawn ahead of this pawn
	if (doubledPassMask & epawns) flagIsOpposed = 1;

	//if there is an enemy pawn on the right or left ahead of this pawn
	if (right & epawns || left & epawns) flagIsPassed = 0;

	opawns |= pawn; // restore real our pawn boards

	tmpSup &= ~RankMasks8[rank]; //remove our rank from supports
	supports &= tmpSup; // get BB of area whether support pawns could be

						//if there are pawns behing this pawn and to the left or the right pawn is not weak
	if (supports & opawns) flagIsWeak = 0;


	//evaluate passed pawns, scoring them higher if they are protected or
	//if their advance is supported by friendly pawns
	if (flagIsPassed) {
		if (isPawnSupported(side, pawn, opawns)) {
			result += (passed_pawn_pcsq[side][location] * 10) / 8;
		}
		else {
			result += passed_pawn_pcsq[side][location];
		}
	}

	//eval weak pawns, increasing the penalty if they are in a half open file
	if (flagIsWeak) {
		result += weak_pawn_pcsq[side][location];

		if (flagIsOpposed) {
			result -= 4;
		}
	}

	return result;

}

int Evaluate::isPawnSupported(int side, U64 pawn, U64 pawns)
{
	if ((pawn >> 1) & pawns) return 1;
	if ((pawn << 1) & pawns) return 1;

	if (side == WHITE) {
		if ((pawn << 7) & pawns) return 1;
		if ((pawn << 9) & pawns) return 1;
	}
	else {
		if ((pawn >> 9) & pawns) return 1;
		if ((pawn >> 7) & pawns) return 1;
	}
	return 0;
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
