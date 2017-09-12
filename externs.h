#ifndef EXTERNS_H
#define EXTERNS_H

typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include "move.h"
#include "slider_attacks.h"
#include "Pawns.h"
#include "material.h"

class HashEntry;
class ZobristH;
class BitBoards;
class MoveGen;

extern const int SORT_VALUE[7];

//magics object
extern SliderAttacks slider_attacks;
//two hash tables for respective names
extern Pawns::Table pawnsTable;
extern Material::Table MaterialTable;

//half turns
extern int turns;

//master zobrist for turn
//extern ZobristH zobrist;

//array for storing objects containing zorbist key for position as well as depth and eval
extern HashEntry transpositionEval[5021983];
extern HashEntry transpositionPawn[400000];

//reduction tables: pv, is node improving?, depth, move number
extern int reductions[2][2][64][64];
//futile move count arrays
extern int futileMoveCounts[2][32];

//holds search info, killers, historys, PV, etc
struct searchDriver{

	Move PV[130];
	int nodes = 0;
	int depth = 0;
	long startTime;
	long moveTime = 2500;
	bool isWhite;
	bool skipEarlyPruning = false;
};
extern searchDriver sd;

struct Historys { //holds history info for search 

	//color, piece  from, piece loc to
	int history[2][64][64] = { { { 0 } } };

	//TEST THIS FOR SPEED/ELO below ///NOT IN USE
	//int hist[2][7][64] = { { { 0 } } }; //color, piece, square to

	int cutoffs[2][64][64] = { { { 0 } } };

	//holds info about gain move made in static eval from last turn, use in futility
	int gains[2][7][64] = { { { 0 } } }; //color, piece type, to square ////TRY same method as history for storing instead??????

	std::vector<U64> twoFoldRep; //stores zobrist keys of all positions encountered thus far

	void updateHist(Move m, int v, bool isWhite) {
		if (abs(history[isWhite][m.from][m.to]) < SORT_KILL) {
			history[isWhite][m.from][m.to] += v;
		}
	}

	void updateGain(Move m, int v, bool isWhite) { 
		gains[isWhite][m.piece][m.to] = std::max(v, gains[isWhite][m.piece][m.to] - 1);
	}

};
extern Historys history; 

//Bitboard of all king movements that can then be shifted
extern const U64 KING_SPAN;
//board for knight moves that can be shifted
extern const U64 KNIGHT_SPAN;
extern const U64 FileABB;
extern const U64 FileBBB;
extern const U64 FileCBB;
extern const U64 FileDBB;
extern const U64 FileEBB;
extern const U64 FileFBB;
extern const U64 FileGBB;
extern const U64 FileHBB;
//files for keeping knight moves from wrapping
extern const U64 FILE_AB;
extern const U64 FILE_GH;


//holds a array of lines "forward" relative from side to move
//in front of a particualar square
extern U64 forwardBB[COLOR][SQ_ALL];
extern U64 PassedPawnMask[COLOR][SQ_ALL];
extern U64 PawnAttackSpan[COLOR][SQ_ALL];
extern int SquareDistance[SQ_ALL][SQ_ALL];


//UCI input varibles for "go"
extern int wtime; //time left on whites clock
extern int btime; //black clock
extern int winc;
extern int binc;
extern int movestogo;
extern int fixedDepthSearch;


#endif // EXTERNS_H
