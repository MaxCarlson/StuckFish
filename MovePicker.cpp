#include "MovePicker.h"

#include "bitboards.h"
#include "movegen.h"

enum Stages {
	MAIN_, 
	QSEARCH_,
	CAPTURES_, 
	KILLERS_,
	QUIETS_0,
	QUIETS_1,
	BAD_CAPTURES_,
	EVASIONS_
};

// Main search initilization !!! add QSEARCH specific init once all is working
MovePicker::MovePicker(const BitBoards & board, Moves ttmove, int depth, const Historys & hist, searchStack * ss) : b(board), h(hist), s(ss), Depth(depth)
{
	current = end = mList;
	endBadCaptures = mList + 255;

	if (b.checkers()) Stage = EVASIONS_;

	else Stage = MAIN_;
	

	//check for psuedo legality!!!
	ttMove = ttmove ? ttmove : MOVE_NONE;
	
	end += (ttmove != MOVE_NONE);
}

template<>
void MovePicker::score<CAPTURES_>()
{
	Moves m;

	for (SMoves * i = mList; i != end; ++i) {
		m = i->move;
		i->score = SORT_VALUE[b.pieceOnSq(  to_sq(m))] 
			     - SORT_VALUE[b.pieceOnSq(from_sq(m))];

		if (move_type(m) == ENPASSANT)
			i->score += SORT_VALUE[PAWN];

		else if (move_type(m) == PROMOTION)
			i->score += SORT_VALUE[promotion_type(m)] - SORT_VALUE[PAWN];
	}
}

void MovePicker::generateNextStage()
{
	current = mList;

	switch (Stage) {

	case CAPTURES_:
		return;
	}
}

Moves MovePicker::nextMove()
{
	Moves m;

	while (true) {

		while(current == end)
			generateNextStage();

		switch (Stage) {

		}
	}
}
