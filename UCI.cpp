#include "UCI.h"

#include <iostream>
#include <string>
#include <sstream>

#include "externs.h"
#include "bitboards.h"
#include "ai_logic.h"
#include "TranspositionT.h"

//master search obj
Ai_Logic searchM;

//is it whites turn?
bool isWhite = true;

//UCI inputs for go
int wtime; //time left on whites clock
int btime; //black clock
int winc;
int binc;
int movestogo;
int fixedDepthSearch = 0;

UCI::UCI()
{
	TT.resize(1024); //change later to be an input option for TT!!!!!
	//TT.resizePawnT(128); //play with sizes for speed
}

void UCI::uciLoop()
{
	std::string line;
	std::string token;

	//master bitboard for turn
	BitBoards newBoard;
	//initalize things
	newBoard.initBoards();
	//fill zobrist arrays with random numbers
	newBoard.zobrist.zobristFill();

	// Make sure that the outputs are sent straight away to the GUI
	std::cout.setf(std::ios::unitbuf);

	//printOptions();

	while (std::getline(std::cin, line))
	{
		std::istringstream is(line);

		token.clear(); // getline() could return empty or blank line

		is >> std::skipws >> token;

		if (token == "uci")
		{
			std::cout << "id name " << ENGINE_NAME << std::endl;
			std::cout << "id author Maxwell Carlson" << std::endl;
			printOptions();
			std::cout << "uciok" << std::endl;
		}
		else if (token == "isready")
		{
			std::cout << "readyok" << std::endl;
		}

		else if (token == "setoption") {
			setOption(is);
		}
		else if (token == "color") {

			//std::cout << "colorToPlay: " << myBoardPtr->getColorToPlay() << std::endl;
		}
		else if (token == "ucinewgame")
		{
			newGame(newBoard); //add function to reset TTables ? plus / only
			searchM.clearHistorys();
			TT.clearTable(); //need to clear other TTables too at somepoint
			searchM.initSearch();
		}
		else if (token == "test") { //used to enable quick testing
			newGame(newBoard); //add function to reset TTables ? plus / only
			searchM.clearHistorys();
			TT.clearTable(); //need to clear other TTables too at somepoint
			searchM.initSearch();
			wtime = btime = 2200000;
			std::thread thr(&UCI::search, this, newBoard);
			thr.join();
		}
		else if (token == "position") {
			updatePosition(newBoard, is);
		}
		else if (token == "print"){
			newBoard.drawBBA();
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
				else if (token == "depth")     is >> fixedDepthSearch;
			}

			std::thread thr(&UCI::search, this, newBoard); 
			thr.join(); //search on new thread -- need to implement so search thread can receive signals to stop from GUI.
			
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

void UCI::updatePosition(BitBoards& newBoard, std::istringstream& input)
{
	Move m;
	std::string token, fen;

	input >> token;

	if (token == "startpos")
	{
		newGame(newBoard);

		//create zobrist hash for startpos that is used to check repetitions
		newBoard.zobrist.getZobristHash(newBoard);
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

	int repCount = 0;
	// Parse move list (if any)
	while (input >> token)
	{
		if (token != "moves")
		{
			//parse string from gui/command line and create move
			m = strToMove(newBoard, token);

			//make move + increment turns
			newBoard.makeMove(m, isWhite);
			turns += 1;
			
			//push board position U64 to search driver.two fold repeitions
			history.twoFoldRep.push_back(newBoard.zobrist.zobristKey);
			repCount++;
			
			isWhite = !isWhite;
		}
	}

}

void UCI::newGame(BitBoards& newBoard)
{
	newBoard.constructBoards();

	turns = 0;

	//clear move repetitions
	history.twoFoldRep.clear();

	isWhite = true;
}

void UCI::printOptions()
{
	//tell GUI we can change that transposition table size //NOT WOKRING, probably not correct
	std::cout << "option name Hash" << std::endl;

	//other options ....
}

void UCI::setOption(std::istringstream & input)
{
	std::string token;

	input >> token;

	if (token == "name") {
		while (input >> token) {
			if (token == "hashsize") {
				input >> token;
			}
		}
	}
	
}

void UCI::search(BitBoards& newBoard)
{	
	Move m = searchM.search(newBoard, isWhite);

	std::cout << "bestmove " << moveToStr(m) << std::endl; //send move to std output for UCI GUI to pickup

	isWhite = !isWhite; //switch color after move
	turns += 1;
}

std::string UCI::moveToStr(const Move& m) 
{
	std::string flipsL[8] = { "a", "b", "c", "d", "e", "f", "g", "h" };
	int flipsN[8] = {8, 7, 6, 5, 4, 3, 2, 1};

	int x = m.from % 8; //extract x's and y's from "from/to" 
	int y = m.from / 8;
	int x1 = m.to % 8;
	int y1 = m.to / 8;

	std::string promL = "";

	if (m.flag == 'Q') promL = "q"; //promotion flags
	else if (m.flag == 'R') promL = "r";
	else if (m.flag == 'B') promL = "b";
	else if (m.flag == 'N') promL = "n";

	std::stringstream ss;
	ss << flipsL[x] << flipsN[y] << flipsL[x1] << flipsN[y1] << promL;

	return ss.str();
}

Move UCI::strToMove(BitBoards& newBoard, std::string& input)
{
	Move m;
	int flipsN[9] = {0, 7, 6, 5, 4, 3, 2, 1, 0};
	char flipsA[8]{ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };

	int x, y, x1, y1, xyI, xyE;

	char cx = input[0];
	char cx1 = input[2];

	for (int i = 0; i < 8; ++i) {
		if (cx == flipsA[i]) x = i;
		if (cx1 == flipsA[i]) x1 = i;
	}

	y = flipsN[input[1] - '0'];
	y1 = flipsN[input[3] - '0'];

	xyI = y * 8 + x;
	xyE = y1 * 8 + x1;

	m.from = xyI; //store from and to for move
	m.to = xyE;

	U64 f = newBoard.squareBB[xyI]; //create bitboards of initial 
	U64 t = newBoard.squareBB[xyE]; //and landing pos

						//very ugly, better way to do it?
	if (f & newBoard.byPieceType[PAWN]) m.piece = PAWN; 
	else if (f & newBoard.byPieceType[KNIGHT]) m.piece = KNIGHT;
	else if (f & newBoard.byPieceType[BISHOP]) m.piece = BISHOP;
	else if (f & newBoard.byPieceType[ROOK]) m.piece = ROOK;
	else if (f & newBoard.byPieceType[QUEEN]) m.piece = QUEEN;
	else if (f & newBoard.byPieceType[KING]) m.piece = KING;

	if (t & newBoard.byPieceType[PAWN]) m.captured = PAWN;
	else if (t & newBoard.byPieceType[KNIGHT]) m.captured = KNIGHT;
	else if (t & newBoard.byPieceType[BISHOP]) m.captured = BISHOP;
	else if (t & newBoard.byPieceType[ROOK]) m.captured = ROOK;
	else if (t & newBoard.byPieceType[QUEEN]) m.captured = QUEEN;
	else if (t & newBoard.byPieceType[KING]) m.captured = KING;
	else m.captured = PIECE_EMPTY; //no capture

	m.flag = '0';

	//promotions
	if (input.length() == 5) {
		if (input[4] == 'q') m.flag = 'Q';
		//below not implemented!!!!!!!!!!!
		else if (input[4] == 'r') m.flag = 'R';
		else if (input[4] == 'b') m.flag = 'B';
		else if (input[4] == 'n') m.flag = 'N';
	}	
	
	return m;
}

