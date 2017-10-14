#include "Thread.h"

#include "movegen.h"

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

	// Not stopping search just yet!
	stop = false;

	Search::RootMoves rootMoves;

	for (const auto m : MoveList<LEGAL>(board))
		rootMoves.emplace_back(m);

	// Transfer search control variables to
	// SearchCondition internal to Search::
	Search::SearchControl = sc;

	// Ownership transfer,
	// New position needs to be set before
	// we can call search again
	if (states.get())
		setStates = std::move(states);

	StateInfo st = setStates->back();

	// Copy all needed board info, then reset the states
	// with a dislocated ptr
	for (Thread * th : Threads)
	{
		th->nodes     = 0;
		th->board     = board;
		th->rootMoves = rootMoves;
		th->rootDepth = 1;
		th->board.set_state(&setStates->back(), th);
	}

	setStates->back() = st;

	return main()->search();
}
