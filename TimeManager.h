#pragma once
#include "ai_logic.h"

class TimeManager
{
public:
	TimeManager();
	
	void calcMoveTime(int color, const Search::SearchControls & sc);

	bool timeStopRoot();
	bool timeStopSearch();
	int  getNPS(int nodes);

	long elapsed() const { return now() - startTime; }

private:
	TimePoint startTime;

	int MaxMoveTime;
};

