#pragma once
#include "ai_logic.h"

enum TimeType {
	Maximum,
	Optimum
};

class TimeManager
{
public:

	void initTime(int color, int ply, const Search::SearchControls & sc);
	int findMoveTime(long ourTime, int moveNumber, TimeType t);

	//void calcMoveTime(int color, const Search::SearchControls & sc);

	//bool timeStopRoot();
	//bool timeStopSearch();
	int  getNPS(int nodes);

	int maximum() { return MaxMoveTime; }
	int optimum() { return optimumMoveTime; }

	long elapsed() const { return now() - startTime; }

private:
	TimePoint startTime;

	int optimumMoveTime;
	int MaxMoveTime;
};

