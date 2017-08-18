#pragma once

#include <sstream> 
class Move;

class UCI
{
public:
	UCI();
	
	//main loop for UCI communication
	void uciLoop();
	void newGame();
	void updatePosition(std::istringstream & input);

	void printOptions();
	void setOption(std::istringstream & input);

	void search();

	std::string moveToStr(const Move & m);

	Move strToMove(std::string & input);


};

