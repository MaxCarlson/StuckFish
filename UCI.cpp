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

	//master bitboard for games
	BitBoards newBoard;
	//fill zobrist arrays with random numbers
	newBoard.zobrist.zobristFill();
	//initalize things
	newBoard.initBoards();

	StateInfo si;

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

		}
		else if (token == "ucinewgame")
		{
			newGame(newBoard); 
			searchM.clearHistorys();
			TT.clearTable(); //need to clear other TTables too at somepoint
			searchM.initSearch();
		}
		else if (token == "test") { //used to enable quick testing
			newGame(newBoard); 
			searchM.clearHistorys();
			TT.clearTable(); //need to clear other TTables too at somepoint
			searchM.initSearch();
			wtime = btime = 2200000;

			search(newBoard);
		}
		else if (token == "position") {
			updatePosition(newBoard, is, si);
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

			//std::thread thr(&UCI::search, this, newBoard);  //THREAD CAUSES ERRORS WITH LARGE BTIBOARD OBJECT????
			//thr.join(); //search on new thread -- need to implement so search thread can receive signals to stop from GUI.
			search(newBoard);
			
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

void UCI::updatePosition(BitBoards& newBoard, std::istringstream& input, StateInfo & si)
{
	Move m;
	std::string token, fen;

	input >> token;

	if (token == "startpos")
	{
		newGame(newBoard);
	}
	else if (token == "fen")
	{
		while (input >> token && token != "moves")
			fen += token + " ";

		newBoard.constructBoards(&fen);
		isWhite = !newBoard.bInfo.sideToMove;
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
			newBoard.makeMove(m, si, !isWhite);
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
	newBoard.constructBoards(NULL);

	//create zobrist hash for startpos that is used to check TT entrys and repetitions 
	newBoard.zobrist.getZobristHash(newBoard);

	turns = 0;

	//clear move repetitions vector
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
	Move m = searchM.searchStart(newBoard, isWhite); 

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

	m.captured = PIECE_EMPTY;

	for (int i = PIECE_EMPTY; i < PIECES; ++i) {

		if (f & newBoard.piecesByType(i)) m.piece = i;
		if (t & newBoard.piecesByType(i)) m.captured = i;
	}

	m.flag = '0';

	// en passants
	if (m.piece == PAWN && m.captured == PIECE_EMPTY
		&& (m.to != m.from + pawn_push(!isWhite)
		&& (m.to != m.from + pawn_push(!isWhite) * 2) )) {

		m.flag = 'E';
		m.captured = PAWN;
	}

	//promotions
	if (input.length() == 5) {
		if (input[4] == 'q') m.flag = 'Q';
		//below not implemented!!!!!!!!!!!
		//else if (input[4] == 'r') m.flag = 'R';
		//else if (input[4] == 'b') m.flag = 'B';
		//else if (input[4] == 'n') m.flag = 'N';
	}	

	//NEED CASTLING CODE ONCE IMPLEMENTED
	if (m.piece == KING && !m.captured) {
		if (newBoard.psuedoAttacks(KING, 0, xyI) & newBoard.squareBB[xyE]) return m;
		else m.flag = 'C';
	}


	
	return m;
}

