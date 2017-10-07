#pragma once

#include "defines.h"
#include "bitboards.h"



class Historys;
class BitBoards;
class searchStack;


enum Stages {
	MAIN_M,    CAPTURES_M, KILLERS_M, QUIETS_M, QUIETS_M1, BAD_CAPTURES_M,
	QSEARCH_,  CAPTURES_Q, 
	EVASIONS_, EVASIONS_S1,
	STOP
};

class MovePicker
{

public:
	MovePicker(const BitBoards & board, Move ttm, int depth, const Historys & hist, searchStack * ss);
	MovePicker(const BitBoards & board, Move ttm, const Historys & hist, searchStack * ss);

	Move nextMove();
	
private:

	template<int genType>
	void score();

	int Stage;

	void generateNextStage();

	const BitBoards   & b;
	const Historys    & h;
	const searchStack * s;

	int Depth;
	Move ttMove;

	SMove killers[2];
	SMove *current, *end, *endBadCaptures, *endQuiets;
	SMove mList[256];
};
							
// Main search initilization 
MovePicker::MovePicker(const BitBoards & board, Move ttm, int depth, const Historys & hist, searchStack * ss) : b(board), h(hist), s(ss), Depth(depth)
{
	current = end  = mList;
	endBadCaptures = mList + 255;

	if (b.checkers()) 
		Stage = EVASIONS_;

	else 
		Stage = MAIN_M;

	// Check for psuedo legality!!!
	ttMove = ( ttm && b.pseudoLegal(ttm)  ? ttm : MOVE_NONE);

	end += (ttMove != MOVE_NONE);
}

// For Qsearch
MovePicker::MovePicker(const BitBoards & board, Move ttm, const Historys & hist, searchStack * ss) : b(board), h(hist), s(ss)
{
	current = end = mList;
	endBadCaptures = mList + 255;

	Stage = QSEARCH_;

	if (ttm && !b.capture_or_promotion(ttm)) 
		ttm = MOVE_NONE;

	
	// Check for psuedo legality!!!
	ttMove = (ttm && b.pseudoLegal(ttm) ? ttm : MOVE_NONE);

	end += (ttMove != MOVE_NONE);
}