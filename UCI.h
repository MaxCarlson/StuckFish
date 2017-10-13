#pragma once

#include "defines.h"
#include "bitboards.h"

#include <sstream> 


class BitBoards;
class StateInfo;


class UCI
{
public:
	UCI();
	
	//main loop for UCI communication
	void uciLoop();
	void newGame(BitBoards& newBoard, StateListPtr& states);
	void updatePosition(BitBoards& newBoard, std::istringstream & input, StateListPtr& states);

	void printOptions();
	void setOption(std::istringstream & input);

	void go(BitBoards& newBoard, std::istringstream & input, StateListPtr &states);

	void test(BitBoards& newBoard, StateListPtr& states);

	void perftUCI(BitBoards& newBoard, std::istringstream & input);

	void divideUCI(BitBoards& newBoard, std::istringstream & input);

	void drawUCI(BitBoards& newBoard);

	// Prints a list of commands for user
	void helpUCI();

	std::string moveToStr(const Move & m);

	Move strToMove(BitBoards& newBoard, std::string & input);


};

