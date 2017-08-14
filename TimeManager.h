#pragma once
class TimeManager
{
public:
	TimeManager();
	
	void calcMoveTime(bool isWhite);

	bool timeStopRoot();
	bool timeStopSearch();
};

