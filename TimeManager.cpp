#include "TimeManager.h"

#include <iostream>

#include "defines.h"
#include "externs.h"


#define TIMEBUFFER 500
#define MOVESTOGO 24

//structtime chronos;

extern bool timeOver;

extern TimeManager timeM;


TimeManager::TimeManager()
{
}

void TimeManager::calcMoveTime(bool isWhite)
{
	//add code for infinite and other search types search time

	//reset search timer
	timeOver = false;

	sd.moveTime = 0;

	int movesToGo = MOVESTOGO;

	if (isWhite) sd.moveTime += wtime / movesToGo;
	else sd.moveTime += btime / movesToGo;

	if (winc) sd.moveTime += winc; //change later if both binc or winc are sent out of turn/at same time
	if (binc) sd.moveTime += binc;

	if (sd.moveTime > TIMEBUFFER) sd.moveTime -= TIMEBUFFER;

	sd.startTime = now();

	return;
}

bool TimeManager::timeStopRoot()
{
	if (timeOver) return true;

	return (((int)(now() - sd.startTime) * 2) > sd.moveTime);

	return false;
}

bool TimeManager::timeStopSearch()
{
	if (sd.depth <= 1) return false;

	if ((int)(now() - sd.startTime) > sd.moveTime) {
		int movesToGo = MOVESTOGO;

		if ((movesToGo > 5) &&
			((int)(now() - sd.startTime) < (sd.moveTime * 2)) &&
			(sd.moveTime > 5000)) {
			return false;
		}
		else {
			return true;
		}
	}
	
	return false;
}

int TimeManager::getNPS()
{
	std::cout << (double)(now() - sd.startTime) / 1000 << std::endl;
	return (sd.nodes / ((double)(now() - sd.startTime)/1000));
}


