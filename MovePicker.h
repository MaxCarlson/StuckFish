#pragma once

#include "defines.h"

class Historys;
class BitBoards;
class searchStack;

class MovePicker
{
public:
	MovePicker(const BitBoards & board, Moves ttmove, int depth, const Historys & hist, searchStack * ss);

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
	SMoves *current, *end, *endBadCaptures;
	SMoves mList[256];
};
