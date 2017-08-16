#include <iostream>

#include "externs.h"
#include "hashentry.h"
#include "zobristh.h"
#include "movegen.h"
#include "UCI.h"
#include "bitboards.h"

//#include <vld.h> //memory leak detection in visual studio

// number of half turns
int turns = 0;

//array for random numbers used to gen zobrist key
U64 zArray[2][6][64];
//denotes a castling has taken place for zobrist key
U64 zCastle[4];
//denotes if castling has occured
bool castled[4] = {false};
bool rookMoved[4] = {false};
//used to change color of move
U64 zBlackMove;
//test ~~ used to indicate a NULL move state
U64 zNullMove;

//master zobrist object
ZobristH zobrist;

//global com interface
extern UCI com;
UCI com;

//transposition table array
HashEntry transpositionT[15485843]; //add megabye size conrtorl later
//TTable of evals
HashEntry transpositionEval[5021983];
//pawn config hash table
HashEntry transpositionPawn[400000];

//user for magic sliders
SliderAttacks slider_attacks;

//used for move gen functions/properies in evals
MoveGen evalMoveGen;

int main(int argc, char *argv[])
{
	ZobristH ZKey; //object only used to generate zobrist key arrays at start of program

	//calculate all zobrist numbers to later use with transpostion table
	ZKey.zobristFill();

    //Fill slider arrays and magic index arrays
    slider_attacks.Initialize();

	//communicate with UCI compatible GUI
	com.uciLoop();
	 
	return 0;
}
