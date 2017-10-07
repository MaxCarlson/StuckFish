#pragma once

#include <sstream> 
#include "defines.h"

class BitBoards;
class StateInfo;

class UCI
{
public:
	UCI();
	
	//main loop for UCI communication
	void uciLoop();
	void newGame(BitBoards& newBoard);
	void updatePosition(BitBoards& newBoard, std::istringstream & input, StateInfo & si);

	void printOptions();
	void setOption(std::istringstream & input);

	void search(BitBoards& newBoard);

	void test(BitBoards& newBoard, StateInfo & si);

	void perftUCI(BitBoards& newBoard, std::istringstream & input);

	void divideUCI(BitBoards& newBoard, std::istringstream & input);

	void drawUCI(BitBoards& newBoard);

	// Prints a list of commands for user
	void helpUCI();

	std::string moveToStr(const Move & m);

	Move strToMove(BitBoards& newBoard, std::string & input);


};

