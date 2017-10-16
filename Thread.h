#pragma once


//#include "defines.h"
#include "MovePicker.h"
#include "ai_logic.h"
#include "Pawns.h"
#include "material.h"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#ifndef NOMINMAX
#  define NOMINMAX // Disable macros min() and max()
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

// Low level mutex lock/unlock as in Stockfish
// 
struct Mutex {
	Mutex()       { InitializeCriticalSection(&cs); }
	~Mutex()      {     DeleteCriticalSection(&cs); }
	void lock()   {		 EnterCriticalSection(&cs); }
	void unlock() {      LeaveCriticalSection(&cs); }

private:
	CRITICAL_SECTION cs;
};

typedef std::condition_variable_any ConditionVariable;

// Holds all things that need to be 
// sepperated for multi-threading. 
class Thread
{
	Mutex mutex;
	ConditionVariable cv;

	bool exit = false, searching = true;
	std::thread stdThread;

public:
	explicit Thread(size_t n);
	virtual ~Thread();

	virtual void search();

	void clear();

	void idle_loop();
	void start_searching();
	void wait_for_search_stop();

	int threadID; 

	Pawns::Table       pawnsTable;
	Material::Table materialTable;

	std::atomic<U64> nodes;

	BitBoards board;
	CounterMoveHistory  counterMoves;
	ButterflyHistory     mainHistory;
	ContinuationHistory contiHistory;

	int rootDepth;
	int completedDepth;
	Search::RootMoves rootMoves;
};



struct MainThread : public Thread
{
	using Thread::Thread;

	void search() override;
	void check_time();

	int previousScore;
	double bestMoveChanges; 
};

// Holds all the threads we've created
// including the MainThread. Is used for all access to threads
// in search.
struct ThreadPool : public std::vector<Thread*>
{
	void initialize();
	void numberOfThreads(size_t);

	void searchStart(BitBoards & board, StateListPtr& states, const Search::SearchControls & sc);

	MainThread * main()  const { return static_cast<MainThread*>(front() ); }

	U64 nodes_searched() const { return accumulateMember( &Thread::nodes ); }

	// Stop condition for all threads
	std::atomic_bool stop;

private:
	// Used to detach the main thread's stateListPtr
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

