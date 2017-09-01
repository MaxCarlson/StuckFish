#include "Evaluate.h"

#include <iostream>

#include "defines.h"
#include "bitboards.h"
#include "externs.h"
#include "psqTables.h"
#include "hashentry.h"


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

/*
const int threatenedByPawn[STAGE][6] = //midgame/endgame - piece type threatened
{ { 0, 0, 24, 25, 35, 40 }, 
  { 0, 0, 32, 32, 57, 65 } };
*/
const int threatenedByPawn[STAGE][6] = //midgame/endgame - piece type threatened
{ { 0, 0, 10, 13, 23, 32 },
{ 0, 0, 12, 15, 35, 45 } };

const int rookHalfOpen[STAGE] = {5, 3};
const int rookOpen[STAGE] = {10, 5};

const int QueenContactCheck = 7;
const int RookContactCheck = 5;
const int QueenCheck = 3;
const int RookCheck = 2;
const int BishopCheck = 1;
const int KnightCheck = 1;

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
	int gameScore[COLOR][STAGE] = { { 0 } }; //white, black - mid game = 0, end game = 1

	int psqScores[COLOR][STAGE] = { { 0 } }; //piece square table scores

	U64 kingZone[COLOR] = {0LL}; //zone around king, by color
	int kingAttackers[COLOR] = {0}; //number of pieces attacking opponent king
	int kingAttWeights[COLOR] = { 0 }; //added weight of king attackers
	int adjacentKingAttckers[COLOR] = { 0 }; //num pieces attacking sqs adjacent to king

	U64 attackedBy[COLOR][7]; //bitboards of attacked squares by color and piece type, 0 for second index is all pieces of that color

	int mobility[COLOR][STAGE] = { { 0 } }; //color, midgame/endgame scoring

	int blockages[COLOR] = { 0 };

	int adjustMaterial[COLOR] = { 0 };
};

void Evaluate::generateKingZones(const BitBoards & boards, EvalInfo & ev)
{
	//draw zone around king all 8 tiles around him, plus three in front -- north = front for white, south black
	
	for (int side = 0; side < 2; ++side) {
		U64 kZone, tkz;

		if (side) kZone = boards.byColorPiecesBB[BLACK][KING];
		else kZone = boards.byColorPiecesBB[WHITE][KING];

		int location = boards.pieceLoc[side][KING][0];

		if (location > 9) {
			kZone |= KING_SPAN << (location - 9);
		}
		else {
			kZone |= KING_SPAN >> (9 - location);
		}

		if (location % 8 < 4) {
			tkz = kZone & ~FILE_GH;

		}
		else {
			tkz = kZone & ~FILE_AB;
		}

		//add king attacks to attacked by
		ev.attackedBy[side][KING] = tkz;

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

	int square;
	
	ev.attackedBy[color][pT] = 0LL;

	while (( square = *piece++ ) != SQ_NONE) {

		//adjust square table lookup for black if need be
		int adjSq = color == WHITE ? square : bSQ[square];

		//add piece square table scores 
		ev.psqScores[color][mg] += pieceSqTab[pT][mg][adjSq];
		ev.psqScores[color][eg] += pieceSqTab[pT][eg][adjSq];

		

		if (pT == PAWN) {
			ev.attackedBy[color][PAWN] |= ev.attackedBy[color][0] |= boards.psuedoAttacks(PAWN, color, square);
			continue;
		}
		
		//find attack squares, including xray bishop and rook attacks blocked by queens / rooks/queens
		U64 bb = pT == BISHOP ? slider_attacks.BishopAttacks(boards.FullTiles ^ boards.pieces(color, QUEEN), square)
			: pT == ROOK ? slider_attacks.RookAttacks(boards.FullTiles ^ boards.pieces(color, QUEEN, ROOK), square)
			: pT == QUEEN ? slider_attacks.QueenAttacks(boards.FullTiles, square)
			 : boards.PseudoAttacks[KNIGHT][square]; // need to add knight attacks, pawns evaled sepperatly

		//add pinned piece code, not moving pieces out of line from their pin

		//add attack squares to both piece attacks by color, and all piece attacks by color
		ev.attackedBy[color][pT] |= ev.attackedBy[color][0] |= bb;

		//if pieces is attacking zone around king..
		if (bb & ev.kingZone[them]) {

			ev.kingAttackers[color]++;
			ev.kingAttWeights[color] += KingAttackerWeights[pT];		
			U64 adjacent = bb & ev.attackedBy[them][KING]; //NEEDS TO BE KING ATTACKS INSTEAD OF KING ZONE!@@!@!@!@!@!@!@@!@@!@!@!@!@!@!@@@!@!@

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

template<> //get rid of recursive function too complex error, also return when piece type hits king
void evaluatePieces<KING, WHITE>(const BitBoards & boards, EvalInfo & ev) { return; };

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

		attackUnits = std::min(10, (ev.kingAttackers[them] * ev.kingAttWeights[them]) / 2
					+ (ev.adjacentKingAttckers[them] + bit_count(undefended)));

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
int evalThreats(const BitBoards & boards, EvalInfo & ev) {

	int them = color == WHITE ? BLACK : WHITE;

	U64 bb, weakEnemys, protectedEnemys;

	//enemys (that are not their pawns) which are protected by knights or bishops
	protectedEnemys = (boards.pieces(them) ^ boards.pieces(them, PAWN))
					& ev.attackedBy[them][PAWN]
					& (ev.attackedBy[them][KNIGHT] | ev.attackedBy[them][BISHOP]);

	if (protectedEnemys) {
		//ev.gameScore
	}
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

	//initilize king zones and king attacks
	generateKingZones(boards, ev);

	evaluatePieces<PAWN, WHITE>(boards, ev);

	score[mg] = boards.bInfo.sideMaterial[WHITE] + ev.psqScores[WHITE][mg] - boards.bInfo.sideMaterial[BLACK] - ev.psqScores[BLACK][mg];
	score[eg] = boards.bInfo.sideMaterial[WHITE] + ev.psqScores[WHITE][eg] - boards.bInfo.sideMaterial[BLACK] - ev.psqScores[BLACK][eg];

	int gamePhase = 0;

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
	result += getPawnScore(boards, ev);

	//add threat and some other scoring done in eval pieces, etc to scores
	score[mg] += ev.gameScore[WHITE][mg] - ev.gameScore[BLACK][mg];
	score[eg] += ev.gameScore[WHITE][eg] - ev.gameScore[BLACK][eg];

	score[mg] += (ev.mobility[WHITE][mg] - ev.mobility[BLACK][mg]);
	score[eg] += (ev.mobility[WHITE][eg] - ev.mobility[BLACK][eg]);
	if (gamePhase > 24) gamePhase = 24;
	int mgWeight = gamePhase;
	int egWeight = 24 - gamePhase;

	result += ((score[mg] * mgWeight) + (score[eg] * egWeight)) / 24;

	result += (ev.blockages[WHITE] - ev.blockages[BLACK]);
	result += (ev.adjustMaterial[WHITE] - ev.adjustMaterial[BLACK]);

	//if (ev.kingAttackers[WHITE] < 2 || boards.byColorPiecesBB[WHITE][QUEEN] == 0LL) ev.kingAttWeights[WHITE] = 0;
	//if (ev.kingAttackers[BLACK] < 2 || boards.byColorPiecesBB[BLACK][QUEEN] == 0LL) ev.kingAttWeights[BLACK] = 0;

	if (!(ev.kingAttackers[WHITE] < 2 || boards.byColorPiecesBB[WHITE][QUEEN] == 0LL)) result += evaluateKing<WHITE>(boards, ev, score[mg]);
	if (!(ev.kingAttackers[BLACK] < 2 || boards.byColorPiecesBB[BLACK][QUEEN] == 0LL)) result -= evaluateKing<BLACK>(boards, ev, score[mg]);

	//result += evaluateKing<WHITE>(boards, ev, score[mg]) - evaluateKing<BLACK>(boards, ev, score[mg]);

	//result += SafetyTable[ev.kingAttWeights[WHITE]];
	//result -= SafetyTable[ev.kingAttWeights[BLACK]];

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
	int result = 0;
	int flagIsPassed = 1; // we will be trying to disprove that
	int flagIsWeak = 1;   // we will be trying to disprove that
	int flagIsOpposed = 0;

	U64 pawn = boards.squareBB[location];
	U64 opawns = boards.byColorPiecesBB[side][PAWN];
	U64 epawns = boards.byColorPiecesBB[!side][PAWN];


	int file = location % 8;
	int rank = location / 8;

	U64 doubledPassMask = FileMasks8[file]; //mask for finding doubled or passed pawns

	U64 left = 0LL;
	if (file > 0) left = FileMasks8[file - 1]; //masks are accoring to whites perspective

	U64 right = 0LL;
	if (file < 7) right = FileMasks8[file + 1]; //files to the left and right of pawn

	U64 supports = right | left, tmpSup = 0LL; //mask for area behind pawn and to the left an right, used to see if weak + mask for holding and values

	opawns &= ~pawn; //remove this pawn from his friendly pawn BB so as not to count himself in doubling

	if (doubledPassMask & opawns) result -= 10; //real value for doubled pawns is -20, because this method counts them twice it's set at half real

	if (!side) { //if is white
		for (int i = 7; i > rank - 1; i--) {
			doubledPassMask &= ~RankMasks8[i];
			left &= ~RankMasks8[i];
			right &= ~RankMasks8[i];
			tmpSup |= RankMasks8[i];
		}
	}
	else {
		for (int i = 0; i < rank + 1; i++) {
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
