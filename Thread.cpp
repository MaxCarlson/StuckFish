#include "Thread.h"


ThreadPool threadPool;


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
