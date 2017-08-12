#ifndef EXTERNS_H
#define EXTERNS_H

typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <vector>
#include <string>
#include <unordered_map>
#include "move.h"
#include "slider_attacks.h"
class HashEntry;
class ZobristH;
class BitBoards;
class MoveGen;

//magics object
extern SliderAttacks slider_attacks;

extern MoveGen evalMoveGen;

//chess board represented
extern std::string boardArr[8][8];

//string of pieces
extern std::string blackPieces[7];
extern std::string whitePieces[7];

//half turns
extern int turns;

//values for computing if a move is valid
extern int tempx, tempy, tempx2, tempy2;

//king safety arrays
extern bool whiteSafe[8][8];
extern bool blackSafe[8][8];

extern int count, numberOfPieceMoved;

// is ai on
extern int aiOn;

extern int turnsTemp;

//array holding U64 numbers for changing zobrist keys
extern U64 zArray[2][6][64];
//used to denote a castling has taken place for zobrist; wqs, wks, bqs, bks
extern U64 zCastle[4];
//array used to denote if castling has occured
extern bool castled[4]; //wqs, wks, bqs, bks
extern bool rookMoved[4]; //wqR, wkR, bqr, bkr
//used to change color of move
extern U64 zBlackMove;
//test ~~ used to indicate a NULL move state
extern U64 zNullMove;

//array for storing objects containing zorbist key for position as well as depth and eval
extern HashEntry transpositionT[15485843];
extern HashEntry transpositionEval[5021983];
extern HashEntry transpositionPawn[400000];

struct searchDriver{
    //color, piece loc from, piece loc to
    int history[2][64][64] = {{{0}}};
    int cutoffs[2][64][64] = {{{0}}};
    Move killers[35][2];
};
extern searchDriver sd;


#endif // EXTERNS_H
