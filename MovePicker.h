#pragma once

#include "defines.h"
#include "bitboards.h"



class Historys;
class BitBoards;
class searchStack;


enum Stages {
	MAIN_M, CAPTURES_M, KILLERS_M, QUIETS_M, QUIETS_M1, BAD_CAPTURES_M,
	QSEARCH_, CAPTURES_Q, 
	EVASIONS_, EVASIONS_S1,
	STOP
};

class MovePicker
{

public:
	MovePicker(const BitBoards & board, Moves ttmove, int depth, const Historys & hist, searchStack * ss);
	MovePicker(const BitBoards & board, Moves ttmove, const Historys & hist, searchStack * ss);

	Moves nextMove();
	
private:

	template<int genType>
	void score();

	int Stage;

	void generateNextStage();

	const BitBoards   & b;
	const Historys    & h;
	const searchStack * s;

	int Depth;
	Moves ttMove;

	SMoves killers[2];
	SMoves *current, *end, *endBadCaptures, *endQuiets;
	SMoves mList[256];
};
							////Causes extern symbol error in .cpp file?
// Main search initilization 
MovePicker::MovePicker(const BitBoards & board, Moves ttmove, int depth, const Historys & hist, searchStack * ss) : b(board), h(hist), s(ss), Depth(depth)
{
	current = end = mList;
	endBadCaptures = mList + 255;

	if (b.checkers()) Stage = EVASIONS_;

	else Stage = MAIN_M;


	//check for psuedo legality!!!
	ttMove = ttmove ? ttmove : MOVE_NONE;

	end += (ttmove != MOVE_NONE);
}

// For Qsearch
MovePicker::MovePicker(const BitBoards & board, Moves ttmove, const Historys & hist, searchStack * ss) : b(board), h(hist), s(ss)
{
	Stage = CAPTURES_Q; //Modify this??

					   //check for psuedo legality!!!
	ttMove = ttmove ? ttmove : MOVE_NONE;

	end += (ttmove != MOVE_NONE);
}