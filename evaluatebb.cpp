#include "evaluatebb.h"
#include "externs.h"
#include "movegen.h"
#include "bitboards.h"
#include "hashentry.h"
#include "zobristh.h"
#include "TranspositionT.h"
#include "psqTables.h"


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

U64 wKingSide = FileMasks8[5] | FileMasks8[6] | FileMasks8[7];
U64 wQueenSide = FileMasks8[2] | FileMasks8[1] | FileMasks8[0];

const U64 full  = 0xffffffffffffffffULL;

//adjustments of piece value based on our color pawn count
const int knight_adj[9] = { -20, -16, -12, -8, -4,  0,  4,  8, 10};
const int rook_adj[9] =   {  15,  12,   9,  6,  3,  0, -3, -6, -9};

//values definitely need to be adjusted and file/rank/side variable
const int rookOpenFile = 10;
const int rookHalfOpenFile = 5;

//positive value
const int BISHOP_PAIR = 30;
//used as negatives to incourage bishop pair
const int KNIGHT_PAIR = 8;
const int ROOK_PAIR = 16;

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
	{}, {}, //no piece and pawn indexs
	{-20, -16, -2, 3, 6, 10, 15, 17, 19}, //knights 

	{-17, -10, 1, 4, 7, 11, 14, //bishops
	17, 19, 20, 21, 22, 22, 23}, 

	{-12, -8, -3, 1, 4, 6, 9, 11, //rooks
	12, 13, 14, 15, 15, 15, 16}, 

	{-14, -9, -3, 0, 2, 4, 5, 6, 7, 7,//queens
	7, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9 } 

};

//end game mobility bonus
const int endGMobilityBonus[6][28]{
	{},{},
	{-17, -14, -3, 0, 3, 6, 9, 10, 11}, //knights 

	{-15, -7, 1, 6, 9, 12, 14, //bishops
	15, 17, 17, 17, 19, 19, 19},

	{-14, -7, 0, 4, 9, 13, 17, 21, //rooks 
	24, 27, 29, 29, 30, 31, 31}, 

	{-14, -8, -4, 0, 3, 5, 10, 15, //queens
	16, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17}
};

const int pieceVal[7] = { 0, 100, 325, 335, 500, 975, 0 };
/*
piece values
  NoP= 0
  P = 100
  N = 325
  B = 335
  R = 500
  Q = 975
  K = 0
*/


int evaluateBB::evalBoard(bool isWhite, const BitBoards& boards)
{
    //transposition hash for boards evals
    //int hash = (int)(zobristE.zobristKey % 5021983);
	int hash = boards.zobrist.zobristKey & 5021982;
    HashEntry entry = transpositionEval[hash];   
    //if we get a hash-table hit, return the evaluation
    if(entry.zobrist == boards.zobrist.zobristKey){
        if(isWhite){
            //if eval was from blacks POV, return -eval
            if(entry.flag) return -entry.eval;
            else return entry.eval; //if eval was from our side, return normally
        } else {

            if(entry.flag) return entry.eval;
            else return -entry.eval;
        }

    }
	//create struct containing eval data
	EvalInfo ev;

    //reset values
    int result = 0, midGScore = 0, endGScore = 0;

    //generate zones around kings
    generateKingZones(boards);

	//evaluate all pieces and positions and store info
	evalPieces(boards, ev);

    //calcualte mid game and end game scores, they differ only by pcsq table scores
    //midGScore = ev.pieceMaterial[WHITE] + ev.psqTabMat[WHITE][0] - ev.pieceMaterial[BLACK] - ev.psqTabMat[BLACK][0];
    //endGScore = ev.pieceMaterial[WHITE] + ev.psqTabMat[WHITE][1] - ev.pieceMaterial[BLACK] - ev.psqTabMat[BLACK][1];

    midGScore = boards.bInfo.sideMaterial[WHITE] + ev.psqTabMat[WHITE][0] - boards.bInfo.sideMaterial[BLACK] - ev.psqTabMat[BLACK][0];
    endGScore = boards.bInfo.sideMaterial[WHITE] + ev.psqTabMat[WHITE][1] - boards.bInfo.sideMaterial[BLACK] - ev.psqTabMat[BLACK][1];

    //gather game phase data based on piece counts of both sides
    for(int color = 1; color < 2; color++){
        ev.gamePhase += ev.knightCount[color];
        ev.gamePhase += ev.bishopCount[color];
        ev.gamePhase += ev.rookCount[color] * 2;
        ev.gamePhase += ev.queenCount[color] * 4;
    }

    //evaluate king shield and part of blocking later once finished

    ev.kingShield[WHITE] = wKingShield(boards);
    ev.kingShield[BLACK] = bKingShield(boards);
	blockedPieces(WHITE, boards, ev);
	blockedPieces(BLACK, boards, ev);

    midGScore += (ev.kingShield[WHITE] - ev.kingShield[BLACK]);


    //adjusting material value of pieces bonus for bishop, small penalty for others
    if(ev.bishopCount[WHITE] > 1) ev.adjustMaterial[WHITE] += BISHOP_PAIR;
    if(ev.bishopCount[BLACK] > 1) ev.adjustMaterial[BLACK] -= BISHOP_PAIR;
    if(ev.knightCount[WHITE] > 1) ev.adjustMaterial[WHITE] -= KNIGHT_PAIR;
    if(ev.knightCount[BLACK] > 1) ev.adjustMaterial[BLACK] += KNIGHT_PAIR;
    if(ev.rookCount[WHITE] > 1) ev.adjustMaterial[WHITE] -= ROOK_PAIR;
    if(ev.rookCount[BLACK] > 1) ev.adjustMaterial[BLACK] += ROOK_PAIR;


    ev.adjustMaterial[WHITE] += knight_adj[ev.pawnCount[WHITE]] * ev.knightCount[WHITE];
    ev.adjustMaterial[BLACK] -= knight_adj[ev.pawnCount[BLACK]] * ev.knightCount[BLACK];
    ev.adjustMaterial[WHITE] += rook_adj[ev.pawnCount[WHITE]] * ev.rookCount[WHITE];
    ev.adjustMaterial[BLACK] -= rook_adj[ev.pawnCount[BLACK]] * ev.rookCount[BLACK];

    //probe pawn hash table for a hit, if we don't get a hit
    //proceed with pawnEval
    result += getPawnScore(boards);

    /********************************************************************
    *  Merge midgame and endgame score. We interpolate between these    *
    *  two values, using a ev.gamePhase value, based on remaining piece *
    *  material on both sides. With less pieces, endgame score beco-    *
    *  mes more influential.                                            *
    ********************************************************************/

    midGScore +=(ev.midGMobility[WHITE] - ev.midGMobility[BLACK]);
    endGScore +=(ev.endGMobility[WHITE] - ev.endGMobility[BLACK]);
    if (ev.gamePhase > 24) ev.gamePhase = 24;
    int mgWeight = ev.gamePhase;
    int egWeight = 24 - ev.gamePhase;

    result +=( (midGScore * mgWeight) + (endGScore * egWeight)) / 24;

    //score blockages as well as piece pair bonuses and penalties,
	//as above prefering a bishop pair

	result += (ev.blockages[WHITE] - ev.blockages[BLACK]);
    result += (ev.adjustMaterial[WHITE] - ev.adjustMaterial[BLACK]);

    /********************************************************************
     *  Merge king attack score. We don't apply this value if there are *
     *  less than two attackers or if the attacker has no queen.        *
     *******************************************************************/

    if(ev.attCount[WHITE] < 2 || boards.byColorPiecesBB[0][5] == 0LL) ev.attWeight[WHITE] = 0;
    if(ev.attCount[BLACK] < 2 || boards.byColorPiecesBB[1][5] == 0LL) ev.attWeight[BLACK] = 0;
    result += SafetyTable[ev.attWeight[WHITE]];
    result -= SafetyTable[ev.attWeight[BLACK]];

    //low material adjustment scoring here
	int strong, weak;
	if (result > 0) {
		strong = WHITE;
		weak = BLACK;
	}
	else {
		strong = BLACK;
		weak = WHITE;
	}

	if (ev.pawnCount[strong] == 0 ){ // NEED more intensive filters for low material

		if (boards.bInfo.sideMaterial[strong] < 400) return 0;

		if (ev.pawnCount[weak] == 0 && boards.bInfo.sideMaterial[strong] == 2*pieceVal[KNIGHT])  return 0;//two knights

		if (boards.bInfo.sideMaterial[strong] == pieceVal[ROOK] && boards.bInfo.sideMaterial[weak] == pieceVal[BISHOP]) result /= 2;

		if (boards.bInfo.sideMaterial[strong] == pieceVal[ROOK] + pieceVal[BISHOP]
			&& boards.bInfo.sideMaterial[weak] == pieceVal[ROOK]) result /= 2;

		if (boards.bInfo.sideMaterial[strong] == pieceVal[ROOK] + pieceVal[KNIGHT]
			&& boards.bInfo.sideMaterial[weak] == pieceVal[ROOK]) result /= 2;
	}


    //switch score for color
    if(!isWhite) result = -result;

    //save to TT eval table
    saveTT(isWhite, result, hash, boards);
	
    return result;
}

void evaluateBB::saveTT(bool isWhite, int result, int hash, const BitBoards &boards)
{
    //store eval into eval hash table
    transpositionEval[hash].eval = result;
    transpositionEval[hash].zobrist = boards.zobrist.zobristKey;

    //set flag for TT so we can switch value if we get a TT hit but
    //the color of the eval was opposite
    if(isWhite) transpositionEval[hash].flag = 0;
    else transpositionEval[hash].flag = 1;
}

template<int pT, int color>
int evaluatePieces(const BitBoards & boards, EvalInfo & ev) {

	int i = 0;
	while (boards.pieceLoc[color][pT][i] != SQ_NONE) {

	}

}

void evaluateBB::evalPieces(const BitBoards & boards, EvalInfo & ev) 
{
	//evaluate all pieces and store relvent info to struct ev
	U64  wpawns, wknights, wrooks, wbishops, wqueens, wking, bpawns, bknights, brooks, bbishops, bqueens, bking;

	//white
	wpawns = boards.byColorPiecesBB[0][1];
	wknights = boards.byColorPiecesBB[0][2];
	wbishops = boards.byColorPiecesBB[0][3];
	wrooks = boards.byColorPiecesBB[0][4];
	wqueens = boards.byColorPiecesBB[0][5];
	wking = boards.byColorPiecesBB[0][6];
	//black
	bpawns = boards.byColorPiecesBB[1][1];
	bknights = boards.byColorPiecesBB[1][2];
	bbishops = boards.byColorPiecesBB[1][3];
	brooks = boards.byColorPiecesBB[1][4];
	bqueens = boards.byColorPiecesBB[1][5];
	bking = boards.byColorPiecesBB[1][6];


	//pawns
	while (wpawns) {
		int x = pop_lsb(&wpawns); //pawns are evaluated in pawn_eval
		ev.pawnCount[WHITE] ++; 
		ev.psqTabMat[WHITE][0] += pieceSqTab[PAWN][0][x]; //psq; piece type, midgame/endgame/location
		ev.psqTabMat[WHITE][1] += pieceSqTab[PAWN][1][x];
	}
	
	while (bpawns) {
		int x = pop_lsb(&bpawns);
		ev.pawnCount[BLACK] ++;
		ev.psqTabMat[BLACK][0] += pieceSqTab[PAWN][0][bSQ[x]];
		ev.psqTabMat[BLACK][1] += pieceSqTab[PAWN][1][bSQ[x]];
	}
	//knights
	while (wknights) {
		int x = pop_lsb(&wknights);
		ev.knightCount[WHITE] ++;
		evalKnight(boards, ev, WHITE, x);
		ev.psqTabMat[WHITE][0] += pieceSqTab[KNIGHT][0][x]; //mid game piece square table
		ev.psqTabMat[WHITE][1] += pieceSqTab[KNIGHT][1][x]; //end game psqt
	}
	while (bknights) {
		int x = pop_lsb(&bknights);
		ev.knightCount[BLACK] ++;
		evalKnight(boards, ev, BLACK, x);
		ev.psqTabMat[BLACK][0] += pieceSqTab[KNIGHT][0][bSQ[x]]; //piece square table values need to be 
		ev.psqTabMat[BLACK][1] += pieceSqTab[KNIGHT][1][bSQ[x]]; //flipped along horizontal line for black
	}
	//bishops
	while (wbishops) {
		int x = pop_lsb(&wbishops);
		ev.bishopCount[WHITE] ++;
		evalBishop(boards, ev, WHITE, x);
		ev.psqTabMat[WHITE][0] += pieceSqTab[BISHOP][0][x];
		ev.psqTabMat[WHITE][1] += pieceSqTab[BISHOP][1][x];
	}
	while (bbishops) {
		int x = pop_lsb(&bbishops);
		ev.bishopCount[BLACK] ++;
		evalBishop(boards, ev, BLACK, x);
		ev.psqTabMat[BLACK][0] += pieceSqTab[BISHOP][0][bSQ[x]];
		ev.psqTabMat[BLACK][1] += pieceSqTab[BISHOP][1][bSQ[x]];
	}
	//rooks
	while (wrooks) {
		int x = pop_lsb(&wrooks);
		ev.rookCount[WHITE] ++;
		evalRook(boards, ev, WHITE, x);
		ev.psqTabMat[WHITE][0] += pieceSqTab[ROOK][0][x];
		ev.psqTabMat[WHITE][1] += pieceSqTab[ROOK][1][x];
	}
	while (brooks) {
		int x = pop_lsb(&brooks);
		ev.rookCount[BLACK] ++;
		evalRook(boards, ev, BLACK, x);
		ev.psqTabMat[BLACK][0] += pieceSqTab[ROOK][0][bSQ[x]];
		ev.psqTabMat[BLACK][1] += pieceSqTab[ROOK][1][bSQ[x]];
	}
	//queens
	while (wqueens) {
		int x = pop_lsb(&wqueens);
		evalQueen(boards, ev, WHITE, x);
		ev.queenCount[WHITE] ++;
		ev.psqTabMat[WHITE][0] += pieceSqTab[QUEEN][0][x];
		ev.psqTabMat[WHITE][1] += pieceSqTab[QUEEN][1][x];
	}
	while (bqueens) {
		int x = pop_lsb(&bqueens);
		evalQueen(boards, ev, BLACK, x);
		ev.queenCount[BLACK] ++;
		ev.psqTabMat[BLACK][0] += pieceSqTab[QUEEN][0][bSQ[x]];
		ev.psqTabMat[BLACK][1] += pieceSqTab[QUEEN][1][bSQ[x]];
	}
	//kings
	while (wking) {
		int x = pop_lsb(&wking);
		ev.psqTabMat[WHITE][0] += pieceSqTab[KING][0][x];
		ev.psqTabMat[WHITE][1] += pieceSqTab[KING][1][x];
	}
	while (bking) {
		int x = pop_lsb(&bking);
		ev.psqTabMat[BLACK][0] += pieceSqTab[KING][0][bSQ[x]];
		ev.psqTabMat[BLACK][1] += pieceSqTab[KING][1][bSQ[x]];
	}
}

void evaluateBB::generateKingZones(const BitBoards & boards)
{
	//draw zone around king all 8 tiles around him, plus three in front -- north = front for white, south black
    U64 kZone;

	for (int side = 0; side < 2; ++side) {

		if (side) kZone = boards.byColorPiecesBB[BLACK][KING];
		else kZone = boards.byColorPiecesBB[WHITE][KING];

		int location = msb(kZone);

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

		if (side) kingZones[BLACK] = kZone;
		else kingZones[WHITE] = kZone;
	}
}

int evaluateBB::wKingShield(const BitBoards & boards)
{
    //gather info on defending pawns
    int result = 0;
	U64 pawns = boards.byColorPiecesBB[WHITE][PAWN];
    U64 king = boards.byColorPiecesBB[WHITE][KING];

    U64 location = 1LL;

    //king on kingside
    if(wKingSide & king){
        if(pawns & (location << F2)) result += 10;
        else if (pawns & (location << F3)) result += 5;

        if(pawns & (location << G2)) result += 10;
        else if (pawns & (location << G3)) result += 5;

        if(pawns & (location << H2)) result += 10;
        else if (pawns & (location << H3)) result += 5;
    }
    else if (wQueenSide & king){
        if(pawns & (location << A2)) result += 10;
        else if (pawns & (location << A3)) result += 5;

        if(pawns & (location << B2)) result += 10;
        else if (pawns & (location << B3)) result += 5;

        if(pawns & (location << C2)) result += 10;
        else if (pawns & (location << C3)) result += 5;
    }
    return result;
}

int evaluateBB::bKingShield(const BitBoards & boards)
{
    int result = 0;
	U64 pawns = boards.byColorPiecesBB[BLACK][PAWN];
    U64 king = boards.byColorPiecesBB[BLACK][KING];

    U64 location = 1LL;

    //king on kingside
    if(wQueenSide & king){
        if(pawns & (location << F7)) result += 10;
        else if (pawns & (location << F6)) result += 5;

        if(pawns & (location << G7)) result += 10;
        else if (pawns & (location << G6)) result += 5;

        if(pawns & (location << H7)) result += 10;
        else if (pawns & (location << H6)) result += 5;
    }
    //queen side
    else if (wKingSide & king){
        if(pawns & (location << A7)) result += 10;
        else if (pawns & (location << A6)) result += 5;

        if(pawns & (location << B7)) result += 10;
        else if (pawns & (location << B6)) result += 5;

        if(pawns & (location << C7)) result += 10;
        else if (pawns & (location << C6)) result += 5;
    }
    return result;
}

int evaluateBB::getPawnScore(const BitBoards & boards)
{
	
    //get zobristE/bitboards of current pawn positions
	U64 pt = boards.byPieceType[PAWN];
    int hash = pt & 399999;

    //probe pawn hash table using bit-wise OR of white pawns and black pawns as zobrist key
    if(transpositionPawn[hash].zobrist == pt){
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

int evaluateBB::pawnEval(const BitBoards & boards, int side, int location)
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
    if(file > 0) left = FileMasks8[file-1]; //masks are accoring to whites perspective

    U64 right = 0LL;
    if(file < 7) right = FileMasks8[file+1]; //files to the left and right of pawn

    U64 supports = right | left, tmpSup = 0LL; //mask for area behind pawn and to the left an right, used to see if weak + mask for holding and values

    opawns &= ~ pawn; //remove this pawn from his friendly pawn BB so as not to count himself in doubling

    if(doubledPassMask & opawns) result -= 10; //real value for doubled pawns is -20, because this method counts them twice it's set at half real

    if(!side){ //if is white
        for(int i = 7; i > rank-1; i--) {
            doubledPassMask &= ~RankMasks8[i];
            left &= ~RankMasks8[i];
            right &= ~RankMasks8[i];
            tmpSup |= RankMasks8[i];
        }
    } else {
        for(int i = 0; i < rank+1; i++) {
            doubledPassMask &= ~RankMasks8[i];
            left &= ~RankMasks8[i];
            right &= ~RankMasks8[i];
            tmpSup |= RankMasks8[i];
        }
    }


    //if there is an enemy pawn ahead of this pawn
    if(doubledPassMask & epawns) flagIsOpposed = 1;

    //if there is an enemy pawn on the right or left ahead of this pawn
    if(right & epawns || left & epawns) flagIsPassed = 0;

    opawns |= pawn; // restore real our pawn boards

    tmpSup &= ~ RankMasks8[rank]; //remove our rank from supports
    supports &= tmpSup; // get BB of area whether support pawns could be

    //if there are pawns behing this pawn and to the left or the right pawn is not weak
    if(supports & opawns) flagIsWeak = 0;


    //evaluate passed pawns, scoring them higher if they are protected or
    //if their advance is supported by friendly pawns
    if(flagIsPassed){
        if(isPawnSupported(side, pawn, opawns)){
            result += (passed_pawn_pcsq[side][location] * 10) / 8;
        } else {
            result += passed_pawn_pcsq[side][location];
        }
    }

    //eval weak pawns, increasing the penalty if they are in a half open file
    if(flagIsWeak){
        result += weak_pawn_pcsq[side][location];

        if(flagIsOpposed){
            result -= 4;
        }
    }

    return result;

}

int evaluateBB::isPawnSupported(int side, U64 pawn, U64 pawns)
{
    if((pawn >> 1) & pawns) return 1;
    if((pawn << 1) & pawns) return 1;

    if(side == WHITE){
        if((pawn << 7) & pawns) return 1;
        if((pawn << 9) & pawns) return 1;
    } else {
        if((pawn >> 9) & pawns) return 1;
        if((pawn >> 7) & pawns) return 1;
    }
    return 0;
}

void evaluateBB::evalKnight(const BitBoards & boards, EvalInfo & ev, int side, int location)
{
    int kAttks = 0, mob = 0;
    ev.gamePhase += 1;

	U64 friends = boards.allPiecesColorBB[side];
	U64 eking = boards.byColorPiecesBB[!side][KING];

//similar to move generation code except we increment mobility counter and king area attack counters
    U64 moves;

    if(location > 18){
        moves = KNIGHT_SPAN<<(location-18);
    } else {
        moves = KNIGHT_SPAN>>(18-location);
    }

    if(location % 8 < 4){
        moves &= ~FILE_GH & ~friends;
    } else {
        moves &= ~FILE_AB & ~friends;
    }

    while(moves){
        //for each move not on friends increment mobility
        ++mob;
		U64 loc = 1LL << pop_lsb(&moves);

        if(loc & kingZones[!side]){
            ++kAttks; //this knight is attacking zone around enemy king
        }
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    //ev.midGMobility[side] += 4 *(mob-4);
    //ev.endGMobility[side] += 4 *(mob-4);
	ev.midGMobility[side] += midGmMobilityBonus[KNIGHT][mob];
	ev.endGMobility[side] += endGMobilityBonus[KNIGHT][mob];

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 2 * kAttks;
    }

}

void evaluateBB::evalBishop(const BitBoards & boards, EvalInfo & ev, int side, int location)
{
    int kAttks = 0, mob = 0;
    ev.gamePhase += 1;

	U64 friends = boards.allPiecesColorBB[side];
	U64 eking = boards.byColorPiecesBB[!side][KING];


    U64 moves = slider_attacks.BishopAttacks(boards.FullTiles, location);
    moves &= ~friends;

    while(moves){
        ++mob; //increment bishop mobility
		U64 loc = 1LL << pop_lsb(&moves);

        if(loc & kingZones[!side]){
            ++kAttks; //this bishop is attacking zone around enemy king
        }
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    //ev.midGMobility[side] += 3 *(mob-7);
    //ev.endGMobility[side] += 3 *(mob-7);
	ev.midGMobility[side] += midGmMobilityBonus[BISHOP][mob];
	ev.endGMobility[side] += endGMobilityBonus[BISHOP][mob];

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 2 * kAttks;
    }


}

void evaluateBB::evalRook(const BitBoards & boards, EvalInfo & ev, int side, int location)
{
    bool  ownBlockingPawns = false, oppBlockingPawns = false;
    int kAttks = 0, mob = 0;
    ev.gamePhase += 2;

    U64 friends, eking, currentFile, opawns, epawns;


    currentFile = FileMasks8[location & 7];

	friends = boards.allPiecesColorBB[side];
	eking = boards.byColorPiecesBB[!side][KING];

	opawns = boards.byColorPiecesBB[side][PAWN];
	epawns = boards.byColorPiecesBB[!side][PAWN];


//open and half open file detection add bonus to mobility score of side
    if(currentFile & opawns){
        ownBlockingPawns = true;		
    }
	if (currentFile & epawns) { //move inside if above???
		oppBlockingPawns = true;
	}

    if(!ownBlockingPawns){
        if(!oppBlockingPawns){
            ev.midGMobility[side] += rookOpenFile;
            ev.endGMobility[side] += rookOpenFile;
        } else {
            ev.midGMobility[side] += rookHalfOpenFile;
            ev.endGMobility[side] += rookHalfOpenFile;
        }
    }

//mobility and king attack gen/detection
    U64 moves = slider_attacks.RookAttacks(boards.FullTiles, location);
    moves &= ~friends;

    while(moves){
        ++mob; //increment rook mobility
		U64 loc = 1LL << pop_lsb(&moves);

        if(loc & kingZones[!side]){
            ++kAttks; //this rook is attacking zone around enemy king
        }
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    //ev.midGMobility[side] += 2 *(mob-7);
    //ev.endGMobility[side] += 4 *(mob-7);
	ev.midGMobility[side] += midGmMobilityBonus[ROOK][mob];
	ev.endGMobility[side] += endGMobilityBonus[ROOK][mob];

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 3 * kAttks;
    }
}

void evaluateBB::evalQueen(const BitBoards & boards, EvalInfo & ev, int side, int location)
{
    ev.gamePhase += 4;
    int kAttks = 0, mob = 0;

	U64 friends = boards.allPiecesColorBB[side];
	U64 eking = boards.byColorPiecesBB[!side][KING];


//similar to move gen, increment mobility and king attacks
    U64 moves = slider_attacks.QueenAttacks(boards.FullTiles, location);
    moves &= ~friends;

    while(moves){
        ++mob; //increment queen mobility
		int loc = 1LL << pop_lsb(&moves);

        if(loc & kingZones[!side]){
            ++kAttks; //this queen is attacking zone around enemy king
        }
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    //ev.midGMobility[side] += 1 *(mob-14);
    //ev.endGMobility[side] += 2 *(mob-14);
	ev.midGMobility[side] += midGmMobilityBonus[QUEEN][mob];
	ev.endGMobility[side] += endGMobilityBonus[QUEEN][mob];

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 4 * kAttks;
    }


}

void evaluateBB::blockedPieces(int side, const BitBoards& boards, EvalInfo & ev)
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
    if(isPiece(bishop, flip(side, C1))
            && isPiece(pawn, flip(side, D2))
            && emptyLoc << flip(side, D3))
        ev.blockages[side] -= 24;

    if(isPiece(bishop, flip(side, F1))
            && isPiece(pawn, flip(side, E2))
            && emptyLoc << flip(side, E3))
        ev.blockages[side] -= 24;

    //trapped knights
    if(isPiece(knight, flip(side, A8))
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
	if(isPiece(bishop, flip(side, A7))
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
	if((isPiece(king, flip(side, F1)) || isPiece(king, flip(side, G1)))
		&& (isPiece(rook, flip(side, H1)) || isPiece(rook, flip(side, G1))))
			ev.blockages[side] -= 24;

	if ((isPiece(king, flip(side, C1)) || isPiece(king, flip(side, B1)))
		&& (isPiece(rook, flip(side, A1)) || isPiece(rook, flip(side, B1))))
		ev.blockages[side] -= 24;
}

bool evaluateBB::isPiece(const U64 &piece, U8 sq)
{
    //is the piece on the square?
    U64 loc = 1LL << sq;
    if(loc & piece) return true;
    return false;
}

int evaluateBB::flip(int side, S8 sq)
{
    if(side == WHITE) return sq;
    return bSQ[sq];
}


