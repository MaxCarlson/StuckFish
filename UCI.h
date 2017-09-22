#pragma once

#include <sstream> 
class Move;
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

	std::string moveToStr(const Move & m);

	Move strToMove(BitBoards& newBoard, std::string & input);


};

