#include "Thread.h"

#include "movegen.h"

ThreadPool Threads;


Thread::Thread() : stdThread(&Thread::idle_loop, this)
{
	wait_for_search_stop();
	// Set heuristics to desired values
	clear();
}

Thread::~Thread()
{
	exit = true;
	start_searching();
	stdThread.join();
}

void ThreadPool::initialize()
{
	push_back(new MainThread());
}
 
void Thread::clear()
{
	counterMoves.fill(MOVE_NONE);
	mainHistory.fill(0);

	for (auto& to : contiHistory)
		for (auto& h : to)
			h.fill(0);

	contiHistory[PIECE_EMPTY][0].fill(Search::counterMovePruneThreshold - 1);
}

void Thread::start_searching()
{
	std::lock_guard<Mutex> lock(mutex);

	searching = true;
	// Unlock this thread
	cv.notify_one(); // Wake up thread in idle loop!
}

void Thread::wait_for_search_stop()
{
	std::unique_lock<Mutex> lock(mutex);

	// Lock this thread untill
	cv.wait(lock, [&] { return !searching; });
}

void Thread::idle_loop()
{
	while (true)
	{
		std::unique_lock<Mutex> lock(mutex);
		searching = false;
		cv.notify_one();
		cv.wait(lock, [&] { return searching; });

		if (exit)
			return;

		lock.unlock();

		search();
	}
}

// Set the requested number of threads
void ThreadPool::numberOfThreads(size_t n)
{
	while (size() < n)
		push_back(new Thread());

	while (size() > n)
		delete back(), pop_back();
}

void ThreadPool::searchStart(BitBoards & board, StateListPtr& states, const Search::SearchControls & sc)
{
	main()->wait_for_search_stop();

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

	main()->start_searching(); // Re enable this to continue debugging multi threading!!!
}
