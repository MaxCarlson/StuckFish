#include "evaluatebb.h"
#include "externs.h"
#include "movegen.h"
#include "bitboards.h"
#include "hashentry.h"
#include "zobristh.h"


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
const int bSQ[64] = {
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


/*
piece values
  P = 100
  N = 320
  B = 330
  R = 500
  Q = 900
  K = 9000
*/

evaluateBB::evaluateBB()
{

}

struct evalVect{
    int gamePhase;
    int pieceMaterial[2];
    int midGMobility[2];
    int endGMobility[2];
    int attCount[2];
    int attWeight[2];
    int mgTropism[2]; // still need to add
    int egTropism[2]; // still need to add
    int kingShield[2];
    int adjustMaterial[2];
    int blockages[2]; // still need to add
    int pawnCount[2];
    int pawnMaterial[2];
    int knightCount[2];
    int bishopCount[2];
    int rookCount[2];
    int queenCount[2];
} ev; //object to hold values incase we want to print

int evaluateBB::evalBoard(bool isWhite, const BitBoards& BBBoard, const ZobristH& zobristE)
{
    //transposition hash for board evals
    int hash = (int)(zobristE.zobristKey % 5021983);
    HashEntry entry = transpositionEval[hash];    
    //if we get a hash-table hit, return the evaluation
    if(entry.zobrist == zobristE.zobristKey){
        if(isWhite){
            //if eval was from blacks POV, return -eval
            if(entry.flag) return -entry.eval;
            else return entry.eval; //if eval was from our side, return normally
        } else {

            if(entry.flag) return entry.eval;
            else return -entry.eval;
        }

    }

    //if we don't get a hash hit, setup boards in global to mimic real boards
    //and proceed with eval, we still might get a pawn hash-table hit later
    evalMoveGen.grab_boards(BBBoard, isWhite);

    //reset values
    int result = 0, midGScore = 0, endGScore = 0;
    ev.gamePhase = 0;

    for (int color = 0; color < 2; color++){
        ev.pieceMaterial[color] = 0;
        ev.midGMobility[color] = 0;
        ev.endGMobility[color] = 0;
        ev.attCount[color] = 0;
        ev.attWeight[color] = 0;
        ev.mgTropism[color] = 0;
        ev.egTropism[color] = 0;
        ev.kingShield[color] = 0;
        ev.adjustMaterial[color] = 0;
        ev.blockages[color] = 0;
        ev.pawnCount[color] = 0;
        ev.pawnMaterial[color] = 0;
        ev.knightCount[color] = 0;
        ev.bishopCount[color] = 0;
        ev.rookCount[color] = 0;
        ev.queenCount[color] = 0;

    }

    //generate zones around kings
    generateKingZones(true);
    generateKingZones(false);


    //loop through all pieces and gather numbers, mobility, king attacks..
    //and add piece square table + material value too endGScore and midGScore
    for(U8 i = 0; i < 64; i++){
        getPieceMaterial(i);
    }

    //need to add end game piece square tables
    midGScore = ev.pieceMaterial[WHITE] - ev.pieceMaterial[BLACK];
    endGScore = midGScore;

    //gather game phase data based on piece counts of both sides
    for(int color = 1; color < 2; color++){
        ev.gamePhase += ev.knightCount[color];
        ev.gamePhase += ev.bishopCount[color];
        ev.gamePhase += ev.rookCount[color] * 2;
        ev.gamePhase += ev.queenCount[color] * 4;
    }

    //evaluate king shield and part of blocking later once finished

    ev.kingShield[WHITE] = wKingShield();
    ev.kingShield[BLACK] = bKingShield();

    midGScore += (ev.kingShield[WHITE] - ev.kingShield[BLACK]);


    //adjusting meterial value of pieces bonus for bishop, small penalty for others
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
    result += getPawnScore();

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

    //add phase independent scoring

    result += (ev.adjustMaterial[WHITE] - ev.adjustMaterial[BLACK]);

    /********************************************************************
     *  Merge king attack score. We don't apply this value if there are *
     *  less than two attackers or if the attacker has no queen.        *
     *******************************************************************/

    if(ev.attCount[WHITE] < 2 || BBBoard.BBWhiteQueens == 0LL) ev.attWeight[WHITE] = 0;
    if(ev.attCount[BLACK] < 2 || BBBoard.BBBlackQueens == 0LL) ev.attWeight[BLACK] = 0;
    result += SafetyTable[ev.attWeight[WHITE]];
    result -= SafetyTable[ev.attWeight[BLACK]];

    //NEED low material adjustment scoring here

    //switch score for color
    if(!isWhite) result = -result;

    //save to TT eval table
    saveTT(isWhite, result, hash, zobristE);

    return result;
}

void evaluateBB::saveTT(bool isWhite, int result, int hash, const ZobristH &zobristE)
{
    //store eval into eval hash table
    transpositionEval[hash].eval = result;
    transpositionEval[hash].zobrist = zobristE.zobristKey;

    //set flag for TT so we can switch value if we get a TT hit but
    //the color of the eval was opposite
    if(isWhite) transpositionEval[hash].flag = 0;
    else transpositionEval[hash].flag = 1;
}

//white piece square lookup tables
int wPawnsSqT[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

int wKnightsSqT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

int wBishopsSqT[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

int wRooksSqT[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0
};

int wQueensSqt[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

int wKingMidSqT[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

int wKingEndSqT[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

//black piece square lookup tables
int bPawnSqT[64] = {
     0,  0,   0,   0,   0,   0,  0,   0,
     5, 10,  10, -20, -20,  10, 10,   5,
     5, -5, -10,   0,   0, -10, -5,   5,
     0,  0,   0,  20,  20,   0,  0,   0,
     5,  5,  10,  25,  25,  10,  5,   5,
    10, 10,  20,  30,  30,  20,  10, 10,
    50, 50,  50,  50,  50,  50,  50, 50,
     0,  0,   0,   0,   0,   0,   0,  0,

};

int bKnightSqT[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50,

};

int bBishopsSqT[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,

};

int bRookSqT[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,

};

int bQueenSqT[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10,   0,   0,  0,  0,   5,   0, -10,
    -10,   0,   5,  5,  5,   5,   5, -10,
    -5,    0,   5,  5,  5,   5,   0,   0,
    -5,    0,   5,  5,  5,   5,   0,  -5,
    -10,   0,   5,  5,  5,   5,   0, -10,
    -10,   0,   0,  0,  0,   0,   0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20,
};

int bKingMidSqT[64]= {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
};

int bKingEndSqT[64] = {
    -50, -30, -30, -30, -30, -30, -30, -50,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -50, -40, -30, -20, -20, -30, -40, -50,
};

int weak_pawn_pcsq[2][64] = { {
     0,   0,   0,   0,   0,   0,   0,   0,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
     0,   0,   0,   0,   0,   0,   0,   0
}, {
   0, 0, 0, 0, 0, 0, 0, 0,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   -10, -12, -14, -16, -16, -14, -12, -10,
   0, 0, 0, 0, 0, 0, 0, 0,
}
};

int passed_pawn_pcsq[2][64] = { {
     0,   0,   0,   0,   0,   0,   0,   0,
   140, 140, 140, 140, 140, 140, 140, 140,
    92,  92,  92,  92,  92,  92,  92,  92,
    56,  56,  56,  56,  56,  56,  56,  56,
    32,  32,  32,  32,  32,  32,  32,  32,
    20,  20,  20,  20,  20,  20,  20,  20,
    20,  20,  20,  20,  20,  20,  20,  20,
     0,   0,   0,   0,   0,   0,   0,   0
}, {
    0, 0, 0, 0, 0, 0, 0, 0,
    20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20,
    32, 32, 32, 32, 32, 32, 32, 32,
    56, 56, 56, 56, 56, 56, 56, 56,
    92, 92, 92, 92, 92, 92, 92, 92,
    140, 140, 140, 140, 140, 140, 140, 140,
    0, 0, 0, 0, 0, 0, 0, 0,
}
};

void evaluateBB::getPieceMaterial(int location)
{
    //create an empty board then shift a 1 over to the current i location
    U64 pieceLocation = 1LL << location;

    //white pieces
    if(evalMoveGen.BBWhitePieces & pieceLocation){
        if(pieceLocation & evalMoveGen.BBWhitePawns){
            ev.pawnCount[WHITE] ++;
            ev.pieceMaterial[WHITE] += 100; //eval pawns sepperatly

        } else if(pieceLocation & evalMoveGen.BBWhiteRooks){
            ev.rookCount[WHITE] ++;
            evalRook(true, location);
            ev.pieceMaterial[WHITE] += 500 + wRooksSqT[location];

        } else if(pieceLocation & evalMoveGen.BBWhiteKnights){
            ev.knightCount[WHITE] ++;
            evalKnight(true, location);
            ev.pieceMaterial[WHITE] += 325 + wKnightsSqT[location];

        } else if(pieceLocation & evalMoveGen.BBWhiteBishops){
            ev.bishopCount[WHITE] ++;
            evalBishop(true, location);
            ev.pieceMaterial[WHITE] += 335 + wBishopsSqT[location];

        } else if(pieceLocation & evalMoveGen.BBWhiteQueens){
            evalQueen(true, location);
            ev.queenCount[WHITE] ++;
            ev.pieceMaterial[WHITE] += 975 + wQueensSqt[location];

        } else if(pieceLocation & evalMoveGen.BBWhiteKing){

            //If both sides have no queens use king end game board
           // if((evalMoveGen.BBWhiteQueens | evalMoveGen.BBBlackQueens) & full){
            //    ev.pieceMaterial[WHITE] += 20000 + wKingEndSqT[location];
            //}
            //if end game conditions fail use mid game king board
            ev.pieceMaterial[WHITE] += wKingMidSqT[location]; ///NEEED to add better mid/end psqT stuff

        }
    } else if (evalMoveGen.BBBlackPieces & pieceLocation) {
        if(pieceLocation & evalMoveGen.BBBlackPawns ){
            ev.pawnCount[BLACK] ++;
            ev.pieceMaterial[BLACK] += 100;

        } else if(pieceLocation & evalMoveGen.BBBlackRooks){
            ev.rookCount[BLACK] ++;
            evalRook(false, location);
            ev.pieceMaterial[BLACK] += 500 + bRookSqT[location];

        } else if(pieceLocation & evalMoveGen.BBBlackKnights){
            ev.knightCount[BLACK] ++;
            evalKnight(false, location);
            ev.pieceMaterial[BLACK] += 325 + bKnightSqT[location];

        } else if(pieceLocation & evalMoveGen.BBBlackBishops){
            ev.bishopCount[BLACK] ++;
            evalBishop(false, location);
            ev.pieceMaterial[BLACK] += 335 + bBishopsSqT[location];

        } else if(pieceLocation & evalMoveGen.BBBlackQueens){
            evalQueen(false, location);
            ev.queenCount[BLACK] ++;
            ev.pieceMaterial[BLACK] += 975 + bQueenSqT[location];

        } else if(pieceLocation & evalMoveGen.BBBlackKing){
           // if((evalMoveGen.BBBlackQueens | evalMoveGen.BBWhiteQueens) & full){
            //    ev.pieceMaterial[BLACK] += 20000 + bKingEndSqT[location];
           // }
         ev.pieceMaterial[BLACK] += bKingMidSqT[location];
        }
    }
}

void evaluateBB::generateKingZones(bool isWhite)
{
	//draw zone around king all 8 tiles around him, plus three in front -- north = front for white, south black
    U64 kZone;
    if(isWhite) kZone = evalMoveGen.BBWhiteKing;
    else kZone = evalMoveGen.BBBlackKing;
    
	int location = msb(kZone);

	if (location > 9) {
		kZone |= evalMoveGen.KING_SPAN << (location - 9);

	}
	else {
		kZone |= evalMoveGen.KING_SPAN >> (9 - location);
	}

	if (isWhite) kZone |= kZone >> 8;
	else kZone |= kZone << 8;

	if (location % 8 < 4) {
		kZone &= ~evalMoveGen.FILE_GH;

	}
	else {
		kZone &= ~evalMoveGen.FILE_AB;
	}

    if(isWhite) wKingZ = kZone;
    else bKingZ = kZone;
}

int evaluateBB::wKingShield()
{
    //gather info on defending pawns
    int result = 0;
    U64 king = evalMoveGen.BBWhiteKing;
    U64 pawns = evalMoveGen.BBWhitePawns;
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

int evaluateBB::bKingShield()
{
    int result = 0;
    U64 king = evalMoveGen.BBBlackKing;
    U64 pawns = evalMoveGen.BBBlackPawns;
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

int evaluateBB::getPawnScore()
{
    //get zobristE/bitboard of current pawn positions
    U64 pt = evalMoveGen.BBWhitePawns | evalMoveGen.BBBlackPawns;
    int hash = (int)(pt % 400000);
    //probe pawn hash table using bit-wise OR of white pawns and black pawns as zobrist key
    if(transpositionPawn[hash].zobrist == pt){
        return transpositionPawn[hash].eval;
    }

    //if we don't get a hash hit, search through all pawns on board and return score
    int score = 0;
    U64 pieceLocation;
    for(int i = 0; i < 64; i++){
        pieceLocation = 1LL << i;
        if(pieceLocation & evalMoveGen.BBWhitePawns){
            score += pawnEval(true, i);
        } else if (pieceLocation & evalMoveGen.BBBlackPawns){
            score -= pawnEval(false, i);
        }
    }

    //store entry to pawn hash table
    transpositionPawn[hash].eval = score;
    transpositionPawn[hash].zobrist = pt;

    return score;
}

int evaluateBB::pawnEval(bool isWhite, int location)
{
    int side;
    int result = 0;
    int flagIsPassed = 1; // we will be trying to disprove that
    int flagIsWeak = 1;   // we will be trying to disprove that
    int flagIsOpposed = 0;

    U64 pawn = 0LL, opawns, epawns;
    pawn += 1LL << location;
    if(isWhite){
        opawns = evalMoveGen.BBWhitePawns;
        epawns = evalMoveGen.BBBlackPawns;
        side = 0;
    } else {
        opawns = evalMoveGen.BBBlackPawns;
        epawns = evalMoveGen.BBWhitePawns;
        side = 1;
    }

    int file = location % 8;
    int rank = location / 8;

    U64 doubledPassMask = FileMasks8[file]; //mask for finding doubled or passed pawns

    U64 left = 0LL;
    if(file > 0) left = FileMasks8[file-1]; //masks are accoring to whites perspective

    U64 right = 0LL;
    if(file < 7) right = FileMasks8[file+1]; //files to the left and right of pawn

    U64 supports = right | left, tmpSup = 0LL; //mask for area behind pawn and to the left an right, used to see if weak + mask for holding and values

    opawns &= ~ pawn; //remove this pawn from his friendly pawn BB so as not to count himself in doubling

    if(doubledPassMask & opawns) result -= 10; //real value for doubled pawns is -twenty, because this method counts them twice it's set at half real

    if(isWhite){
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

    opawns |= pawn; // restore real our pawn board

    tmpSup &= ~ RankMasks8[rank]; //remove our rank from supports
    supports &= tmpSup; // get BB of area whether support pawns could be

    //if there are pawns behing this pawn and to the left or the right pawn is not weak
    if(supports & opawns) flagIsWeak = 0;


    //evaluate passed pawns, scoring them higher if they are protected or
    //if their advance is supported by friendly pawns
    if(flagIsPassed){
        if(isPawnSupported(isWhite, pawn, opawns)){
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

int evaluateBB::isPawnSupported(bool isWhite, U64 pawn, U64 pawns)
{
    if(evalMoveGen.westOne(pawn) & pawns) return 1;
    if(evalMoveGen.eastOne(pawn) & pawns) return 1;

    if(isWhite){
        if(evalMoveGen.soWeOne(pawn) & pawns) return 1;
        if(evalMoveGen.soEaOne(pawn) & pawns) return 1;
    } else {
        if(evalMoveGen.noWeOne(pawn) & pawns) return 1;
        if(evalMoveGen.noEaOne(pawn) & pawns) return 1;
    }
    return 0;
}

void evaluateBB::evalKnight(bool isWhite, int location)
{
    int kAttks = 0, mob = 0, side;
    ev.gamePhase += 1;

    U64 knight = 0LL, friends, eking, kingZone;
    knight += 1LL << location;
    if(isWhite){
        friends = evalMoveGen.BBWhitePieces;
        eking = evalMoveGen.BBBlackKing;
        kingZone = bKingZ;
        side = 0;
    } else {
        friends = evalMoveGen.BBBlackPieces;
        eking = evalMoveGen.BBWhiteKing;
        kingZone = wKingZ;
        side = 1;
    }

//similar to move generation code except we increment mobility counter and king area attack counters
    U64 moves;

    if(location > 18){
        moves = evalMoveGen.KNIGHT_SPAN<<(location-18);
    } else {
        moves = evalMoveGen.KNIGHT_SPAN>>(18-location);
    }

    if(location % 8 < 4){
        moves &= ~evalMoveGen.FILE_GH & ~friends & ~eking;
    } else {
        moves &= ~evalMoveGen.FILE_AB & ~friends & ~eking;
    }

    U64 j = moves & ~(moves-1);

    while(j != 0){
        //for each move not on friends increment mobility
        ++mob;

        if(j & kingZone){
            ++kAttks; //this knight is attacking zone around enemy king
        }
        moves &= ~j;
        j = moves & ~(moves-1);
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    ev.midGMobility[side] += 4 *(mob-4);
    ev.endGMobility[side] += 4 *(mob-4);

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 2 * kAttks;
    }

}

void evaluateBB::evalBishop(bool isWhite, int location)
{
    int kAttks = 0, mob = 0, side;
    ev.gamePhase += 1;

    U64 bishop = 0LL, friends, eking, kingZone;
    bishop += 1LL << location;
    if(isWhite){
        friends = evalMoveGen.BBWhitePieces;
        eking = evalMoveGen.BBBlackKing;
        kingZone = bKingZ;
        side = 0;
    } else {
        friends = evalMoveGen.BBBlackPieces;
        eking = evalMoveGen.BBWhiteKing;
        kingZone = wKingZ;
        side = 1;
    }

    //U64 moves = evalMoveGen.DAndAntiDMoves(location) & ~friends & ~eking;
    U64 moves = slider_attacks.BishopAttacks(evalMoveGen.FullTiles, location);
    moves &= ~friends & ~ eking;

    U64 j = moves & ~ (moves-1);
    while(j != 0){
        ++mob; //increment bishop mobility

        if(j & kingZone){
            ++kAttks; //this bishop is attacking zone around enemy king
        }
        moves &= ~j;
        j = moves & ~(moves-1);
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    ev.midGMobility[side] += 3 *(mob-7);
    ev.endGMobility[side] += 3 *(mob-7);

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 2 * kAttks;
    }


}

void evaluateBB::evalRook(bool isWhite, int location)
{
    bool  ownBlockingPawns = false, oppBlockingPawns = false;
    int kAttks = 0, mob = 0, side;
    ev.gamePhase += 2;

    U64 rook = 0LL, friends, eking, kingZone, currentFile, opawns, epawns;
    rook += 1LL << location;

    int x = location % 8;
    currentFile = evalMoveGen.FileABB << x;

    if(isWhite){
        friends = evalMoveGen.BBWhitePieces;
        eking = evalMoveGen.BBBlackKing;
        kingZone = bKingZ;
        side = 0;

        opawns = evalMoveGen.BBWhitePawns;
        epawns = evalMoveGen.BBBlackPawns;
    } else {
        friends = evalMoveGen.BBBlackPieces;
        eking = evalMoveGen.BBWhiteKing;
        kingZone = wKingZ;
        side = 1;

        opawns = evalMoveGen.BBBlackPawns;
        epawns = evalMoveGen.BBWhitePawns;
    }

//open and half open file detection add bonus to mobility score of side
    if(currentFile & opawns){
        ownBlockingPawns = true;
    }
    if (currentFile & epawns){
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
    U64 moves = slider_attacks.RookAttacks(evalMoveGen.FullTiles, location);
    moves &= ~friends & ~ eking;

    U64 j = moves & ~ (moves-1);
    while(j != 0){
        ++mob; //increment bishop mobility

        if(j & kingZone){
            ++kAttks; //this bishop is attacking zone around enemy king
        }
        moves &= ~j;
        j = moves & ~(moves-1);
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    ev.midGMobility[side] += 2 *(mob-7);
    ev.endGMobility[side] += 4 *(mob-7);

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 3 * kAttks;
    }
}

void evaluateBB::evalQueen(bool isWhite, int location)
{
    ev.gamePhase += 4;
    int kAttks = 0, mob = 0, side;

    U64 queen = 0LL, friends, eking, kingZone;
    queen += 1LL << location;
    if(isWhite){
        friends = evalMoveGen.BBWhitePieces;
        eking = evalMoveGen.BBBlackKing;
        kingZone = bKingZ;
        side = 0;
    } else {
        friends = evalMoveGen.BBBlackPieces;
        eking = evalMoveGen.BBWhiteKing;
        kingZone = wKingZ;
        side = 1;
    }

//similar to move gen, increment mobility and king attacks
    U64 moves = slider_attacks.QueenAttacks(evalMoveGen.FullTiles, location);
    moves &= ~friends & ~ eking;

    U64 j = moves & ~ (moves-1);
    while(j != 0){
        ++mob; //increment bishop mobility

        if(j & kingZone){
            ++kAttks; //this bishop is attacking zone around enemy king
        }
        moves &= ~j;
        j = moves & ~(moves-1);
    }

    //Evaluate mobility. We try to do it in such a way zero represent average mob
    ev.midGMobility[side] += 1 *(mob-14);
    ev.endGMobility[side] += 2 *(mob-14);

    //save data on king attacks
    if(kAttks){
        ev.attCount[side] ++;
        ev.attWeight[side] += 4 * kAttks;
    }


}

void evaluateBB::blockedPieces(int side, const BitBoards &BBBoard)
{
    U64 pawn, epawn, knight, bishop, king;
    U64 empty = BBBoard.EmptyTiles;
    U64 emptyLoc = 1LL;
    int oppo;
    if(side == WHITE){
        pawn = BBBoard.BBWhitePawns;
        epawn = BBBoard.BBBlackPawns;
        knight = BBBoard.BBWhiteKnights;
        bishop = BBBoard.BBWhiteBishops;
        king = BBBoard.BBWhiteKing;
        oppo = 1;
    } else {
        pawn = BBBoard.BBBlackPawns;
        epawn = BBBoard.BBWhitePawns;
        knight = BBBoard.BBBlackKnights;
        bishop = BBBoard.BBBlackBishops;
        king = BBBoard.BBBlackKing;
        oppo = 0;
    }

    //central pawn block bishop blocked
    if(isPiece(bishop, flip(side, C1))
            && isPiece(pawn, flip(side, D2))
            && emptyLoc << flip(side, D3))
        ev.blockages[side] -= 24;

    if(isPiece(bishop, flip(side, F1))
            && isPiece(pawn, flip(side, E2))
            && emptyLoc << flip(side, E3))
        ev.blockages[side] -= 24;

    //blocked knights
    if(isPiece(knight, flip(side, A8))
            && isPiece(epawn, flip(oppo, A7))
            || isPiece(epawn, flip(oppo, C7)))
        ev.blockages[side] -= 150;
    //NOT DONE
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


