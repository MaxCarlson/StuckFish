#pragma once
#include "ai_logic.h"

class TimeManager
{
public:
	TimeManager();
	
	void calcMoveTime(int color, const Search::SearchControls & sc);

	bool timeStopRoot();
	bool timeStopSearch();
	int getNPS();

private:
	TimePoint startTime;
	Search::SearchControls tscs;
	int MaxMoveTime;
};

