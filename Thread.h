#pragma once


//#include "defines.h"
#include "MovePicker.h"
//#include "Pawns.h"
//#include "material.h"

#include <thread>






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

	Move searchStart(BitBoards & board);

	MainThread * main() const { return static_cast<MainThread*>(front()); }
};

extern ThreadPool Threads;

