#include "TimeManager.h"

#include <iostream>

#include "defines.h"
#include "externs.h"


#define TIMEBUFFER 500
#define MOVESTOGO   24

//structtime chronos;

extern bool timeOver;

extern TimeManager timeM;


TimeManager::TimeManager()
{
}

void TimeManager::calcMoveTime(int color, const Search::SearchControls & sc)
{
	//add code for infinite and other search types search time

	//reset search timer
	timeOver = false;

	MaxMoveTime = 0;
	startTime   = sc.startTime;

	int movesToGo = MOVESTOGO;

	if (color == WHITE) MaxMoveTime += sc.time[WHITE] / movesToGo;
	else				MaxMoveTime += sc.time[BLACK] / movesToGo;

	if (sc.inc[WHITE])
		MaxMoveTime += sc.inc[WHITE]; //change later if both binc or winc are sent out of turn/at same time

	else if (sc.inc[BLACK])
		MaxMoveTime += sc.inc[BLACK];

	if (MaxMoveTime > TIMEBUFFER) MaxMoveTime -= TIMEBUFFER;

	return;
}

bool TimeManager::timeStopRoot()
{
	if (timeOver) 
		return true;

	long aa = now() - startTime;

	return (((int)(now() - startTime) * 2) > MaxMoveTime);

	return false;
}

bool TimeManager::timeStopSearch()
{
	long elps = elapsed();

	if ((int)elps > MaxMoveTime) {
		int movesToGo = MOVESTOGO;

		if ((movesToGo > 5) &&
			((int)elps < (MaxMoveTime * 2)) &&
			(MaxMoveTime > 5000)) {
			return false;
		}
		else {
			return true;
		}
	}
	
	return false;
}

int TimeManager::getNPS(int nodes)
{
	std::cout << (double)(now() - startTime) / 1000 << std::endl;
	return (nodes / ((double)(now() - startTime)/1000));
}


