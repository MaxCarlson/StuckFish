#pragma once


//#include "defines.h"
#include "MovePicker.h"
#include "ai_logic.h"
//#include "Pawns.h"
//#include "material.h"

#include <thread>
#include <atomic>





// At the moment this class is only a place holder for things
// I want to be accessable to threads, as well as individual per
// each search thread. Multi-threading is not implemented.
class Thread
{

	std::thread stdThread;

public:
	explicit Thread();

	virtual Move search();

	void clear();

	//Pawns::Table pawnsTable;
	//Material::Table materialTable;

	std::atomic<U64> nodes;

	BitBoards board;
	CounterMoveHistory  counterMoves;
	ButterflyHistory     mainHistory;
	ContinuationHistory contiHistory;

	int rootDepth;
};



struct MainThread : public Thread
{
	using Thread::Thread;

	Move search() override;

	//double bestMoveChanges; //Not in use yet
};

struct ThreadPool : public std::vector<Thread*>
{
	void initialize();

	Move searchStart(BitBoards & board, StateListPtr& states, const Search::SearchControls & sc);

	MainThread * main() const { return static_cast<MainThread*>(front()); }

	U64 nodes_searched() const { return accumulateMember( &Thread::nodes ); }

private:
	// Will be used later to detach the main thread's stateListPtr
	// when transfering state list ptr's to other threads
	StateListPtr setStates;

	U64 accumulateMember(std::atomic<U64> Thread::* member) const
	{
		U64 sum = 0;
		for (Thread * th : *this)
		{
			sum += (th->*member).load(std::memory_order_relaxed);
		}
		return sum;
	}
};

extern ThreadPool Threads;

