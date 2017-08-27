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

//TTable of evals //UPDATE TO NEW TT SCHEME
HashEntry transpositionEval[5021983];
//pawn config hash table
HashEntry transpositionPawn[400000];

//user for magic sliders
SliderAttacks slider_attacks;

int main(int argc, char *argv[])
{
    //Fill slider arrays and magic index arrays
    slider_attacks.Initialize();

	UCI com;
	//communicate with UCI compatible GUI
	com.uciLoop();
	 
	return 0;
}
