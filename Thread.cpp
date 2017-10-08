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
}

void ThreadPool::initialize()
{
	push_back(new MainThread());
}

Move ThreadPool::searchStart(BitBoards & board)
{

	//Once there are multiple threads, we need to sepperate the stateInfo
	//pointers from eachother
	main()->board = board;

	// Bitboards currently hold things like PseudoMoves that are initilized in BitBoards::initilize, only once
	// At beginning of UCI::loop. We can either move those outside(best option), or set thread boards equal to, than reconstruct the boards through
	// BitBoards::contructBoards() once we want to start some type of SMP.

	return main()->search();
}
