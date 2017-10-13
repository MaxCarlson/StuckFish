#include "Thread.h"


ThreadPool Threads;


Thread::Thread()
{
	clear();
}
 
void Thread::clear()
{
	counterMoves.fill(MOVE_NONE);
	mainHistory.fill(0);
	///*
	for (auto& to : contiHistory)
		for (auto& h : to)
			h.fill(0);

	contiHistory[PIECE_EMPTY][0].fill(Search::counterMovePruneThreshold - 1);
	//*/
}

void ThreadPool::initialize()
{
	push_back(new MainThread());
}

Move ThreadPool::searchStart(BitBoards & board, StateListPtr& states, const Search::SearchControls & sc)
{

	//Once there are multiple threads, we need to sepperate the stateInfo
	//pointers from eachother
	main()->board = board;

	// Transfer search control variables to
	// SearchCondition internal to Search::
	Search::SearchControl = sc;

	// Ownership transfer
	if (states.get())
		setStates = std::move(states);

	// Bitboards currently hold things like PseudoMoves that are initilized in BitBoards::initilize, only once
	// At beginning of UCI::loop. We can either move those outside(best option), or set thread boards equal to, than reconstruct the boards through
	// BitBoards::contructBoards() once we want to start some type of SMP.
	StateInfo st = setStates->back();

	for (Thread * th : Threads)
	{
		th->nodes = 0;
		th->board = board;
		th->board.set_state(&setStates->back(), th);
	}

	setStates->back() = st;

	return main()->search();
}
