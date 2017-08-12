#include "UCI.h"

#include <iostream>
#include <string>
#include <sstream>

#include "externs.h"
#include "bitboards.h"
#include "zobristh.h"
#include "ai_logic.h"

#define ENGINENAME "StuckFish 0.1"

//master BitBoard
BitBoards bboard;

//dummy zobrist object
ZobristH zDummy;

//is it whites turn?
bool isWhite = true;

//UCI inputs for go
int wtime; //time left on whites clock
int btime; //black clock
int winc;
int binc;
int movestogo;

UCI::UCI()
{
}

void UCI::uciLoop()
{
	std::string line;
	std::string token;

	// Make sure that the outputs are sent straight away to the GUI
	std::cout.setf(std::ios::unitbuf);

	//printOptions();

	while (std::getline(std::cin, line))
	{
		std::istringstream is(line);

		token.clear(); // getline() could return empty or blank line

		is >> std::skipws >> token;
		//	std::cout << token << std::endl;

		if (token == "uci")
		{
			std::cout << "id name " << ENGINENAME << std::endl;
			std::cout << "id author Romain Goussault - Navid Hedjazian" << std::endl;
			//printOptions();
			std::cout << "uciok" << std::endl;
		}
		else if (token == "isready")
		{
			std::cout << "readyok" << std::endl;
		}

		else if (token == "setoption") {
			//setoption(is);
		}
		else if (token == "color") {

			//std::cout << "colorToPlay: " << myBoardPtr->getColorToPlay() << std::endl;
		}
		else if (token == "ucinewgame")
		{
			newGame();
		}

		else if (token == "position") {
			updatePosition(is);
		}
		else if (token == "print"){
			//std::cout << *myBoardPtr << std::endl;
		}
		else if (token == "printOptions") {
			//printOptions();
		}
		else if (token == "go")
		{	
			while (is >> token)
			{
				if (token == "wtime")          is >> wtime;
				else if (token == "btime")     is >> btime;
				else if (token == "winc")      is >> winc;
				else if (token == "binc")      is >> binc;
				else if (token == "movestogo") is >> movestogo;
			}

			search(); //search position

			/*
			//http://stackoverflow.com/questions/12624271/c11-stdthread-giving-error-no-matching-function-to-call-stdthreadthread
			thrd::thread thr(&Uci::search, this);
			thrd::swap(thr, myThread);
			myThread.join();
			*/
		}
		else if (token == "quit")
		{
			std::cout << "Terminating.." << std::endl;
			break;
		}
		else
			// Command not handled
			std::cout << "what?" << std::endl;
	}
}

void UCI::newGame()
{
	bboard.constructBoards();

	for (int i = 0; i < 4; i++) {
		rookMoved[i] = false;
		castled[i] = false;
	}

	isWhite = true;
}

void UCI::updatePosition(std::istringstream& input)
{
	Move m;
	std::string token, fen;

	input >> token;

	if (token == "startpos")
	{
		newGame();
	}
	else if (token == "fen")
	{
		while (input >> token && token != "moves")
			fen += token + " ";
		//add fen code
	}
	else
	{
		return;
	}

	// Parse move list (if any)
	while (input >> token)
	{
		if (token != "moves")
		{
			m = strToMove(token);
			bboard.makeMove(m, zDummy, isWhite); //test ~~/ position startpos moves c2c4 g8f6
			bboard.drawBBA();

			isWhite = !isWhite;
		}
	}

}

void UCI::search()
{
	Ai_Logic search;
	Move m = search.search(isWhite);

	moveToStr(m);
}

void UCI::moveToStr(Move& m) {

}

Move UCI::strToMove(std::string& input) 
{
	Move m;
	int flipsN[9] = {0, 7, 6, 5, 4, 3, 2, 1, 0};

	int x, y, x1, y1, xyI, xyE;

	char cx = input[0];
	char cx1 = input[2];
	if (cx == 'a') x = 0;
	else if (cx == 'b') x = 1;
	else if (cx == 'c') x = 2;
	else if (cx == 'd') x = 3;
	else if (cx == 'e') x = 4;
	else if (cx == 'f') x = 5;
	else if (cx == 'g') x = 6;
	else if (cx == 'g') x = 7;

	if (cx1 == 'a') x1 = 0;
	else if (cx1 == 'b') x1 = 1;
	else if (cx1 == 'c') x1 = 2;
	else if (cx1 == 'd') x1 = 3;
	else if (cx1 == 'e') x1 = 4;
	else if (cx1 == 'f') x1 = 5;
	else if (cx1 == 'g') x1 = 6;
	else if (cx1 == 'g') x1 = 7;


	y = flipsN[input[1] - '0'];
	y1 = flipsN[input[3] - '0'];

	xyI = y * 8 + x;
	xyE = y1 * 8 + x1;

	m.from = xyI; //store from and to for move
	m.to = xyE;

	U64 f = 1LL << xyI; //create bitboards of initial 
	U64 t = 1LL << xyE; //and landing pos

	if (f & bboard.BBWhitePawns) m.piece = 'P'; //very ugly, better way to do it?
	else if (f & bboard.BBWhiteKnights) m.piece = 'N';
	else if (f & bboard.BBWhiteBishops) m.piece = 'B';
	else if (f & bboard.BBWhiteRooks) m.piece = 'R';
	else if (f & bboard.BBWhiteQueens) m.piece = 'Q';
	else if (f & bboard.BBWhiteKing) m.piece = 'K';
	else if (f & bboard.BBBlackPawns) m.piece = 'p';
	else if (f & bboard.BBBlackKnights) m.piece = 'n';
	else if (f & bboard.BBBlackBishops) m.piece = 'b';
	else if (f & bboard.BBBlackRooks) m.piece = 'r';
	else if (f & bboard.BBBlackQueens) m.piece = 'q';
	else if (f & bboard.BBBlackKing) m.piece = 'k';

	if (t & bboard.BBWhitePawns) m.captured = 'P';
	else if (t & bboard.BBWhiteKnights) m.captured = 'N';
	else if (t & bboard.BBWhiteBishops) m.captured = 'B';
	else if (t & bboard.BBWhiteRooks) m.captured = 'R';
	else if (t & bboard.BBWhiteQueens) m.captured = 'Q';
	else if (t & bboard.BBWhiteKing) m.captured = 'K';
	else if (t & bboard.BBBlackPawns) m.captured = 'p';
	else if (t & bboard.BBBlackKnights) m.captured = 'n';
	else if (t & bboard.BBBlackBishops) m.captured = 'b';
	else if (t & bboard.BBBlackRooks) m.captured = 'r';
	else if (t & bboard.BBBlackQueens) m.captured = 'q';
	else if (t & bboard.BBBlackKing) m.captured = 'k';
	else m.captured = '0'; //no capture

	std::string promotionL = "";

	if (input[4] == 'q') m.flag = 'Q';
	else if (input[4] == 'r') m.flag = 'R';
	else if (input[4] == 'b') m.flag = 'B';
	else if (input[4] == 'n') m.flag = 'N';
	
	
	return m;
}

