#include "UCI.h"

#include <iostream>
#include <string>
#include <sstream>

#include "externs.h"
#include "bitboards.h"
#include "zobristh.h"
#include "ai_logic.h"
#include "TranspositionT.h"

//master bitboard for turn
BitBoards newBoard;

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

UCI::UCI()
{
	TT.resize(1024); //change later to be an input option for TT!!!!!
	//TT.resizePawnT(128); //play with sizes for speed
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
			newGame(); //add function to reset TTables ? plus / only
			searchM.clearHistorys();
			TT.clearTable(); //need to clear other TTables too at somepoint
			searchM.initSearch();
		}

		else if (token == "position") {
			updatePosition(is);
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
			}

			std::thread thr(&UCI::search, this); 
			thr.join(); //search on new thread
			
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
	newBoard.constructBoards();

	for (int i = 0; i < 4; i++) {
		rookMoved[i] = false;
		castled[i] = false;
	}

	turns = 0;

	//clear move repetitions
	h.twoFoldRep.clear();

	isWhite = true;
}

void UCI::updatePosition(std::istringstream& input)
{
	Move m;
	std::string token, fen;

	//dummy zobrist object
	ZobristH zDummy;
	

	input >> token;

	if (token == "startpos")
	{
		newGame();

		//create zobrist hash for startpos that is used to check repetitions
		zobrist.getZobristHash(newBoard);
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
			m = strToMove(token);

			//make move + increment turns
			newBoard.makeMove(m, zobrist, isWhite);
			turns += 1;
			
			//push board position U64 to search driver.two fold repeitions
			h.twoFoldRep.push_back(zobrist.zobristKey);
			repCount++;
			
			isWhite = !isWhite;
		}
	}

}

void UCI::printOptions()
{
	//tell GUI we can change that transposition table size
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

void UCI::search()
{	
	Move m = searchM.search(isWhite);

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
	else if (cx == 'h') x = 7;

	if (cx1 == 'a') x1 = 0;
	else if (cx1 == 'b') x1 = 1;
	else if (cx1 == 'c') x1 = 2;
	else if (cx1 == 'd') x1 = 3;
	else if (cx1 == 'e') x1 = 4;
	else if (cx1 == 'f') x1 = 5;
	else if (cx1 == 'g') x1 = 6;
	else if (cx1 == 'h') x1 = 7;


	y = flipsN[input[1] - '0'];
	y1 = flipsN[input[3] - '0'];

	xyI = y * 8 + x;
	xyE = y1 * 8 + x1;

	m.from = xyI; //store from and to for move
	m.to = xyE;

	U64 f = 1LL << xyI; //create bitboards of initial 
	U64 t = 1LL << xyE; //and landing pos

						//very ugly, better way to do it?
	if (f & newBoard.BBWhitePawns || f & newBoard.BBBlackPawns) m.piece = PAWN; 
	else if (f & newBoard.BBWhiteKnights || f & newBoard.BBBlackKnights) m.piece = KNIGHT;
	else if (f & newBoard.BBWhiteBishops || f & newBoard.BBBlackBishops) m.piece = BISHOP;
	else if (f & newBoard.BBWhiteRooks || f & newBoard.BBBlackRooks) m.piece = ROOK;
	else if (f & newBoard.BBWhiteQueens || f & newBoard.BBBlackQueens) m.piece = QUEEN;
	else if (f & newBoard.BBWhiteKing || f & newBoard.BBBlackKing) m.piece = KING;

	if (t & newBoard.BBWhitePawns || t & newBoard.BBBlackPawns) m.captured = PAWN;
	else if (t & newBoard.BBWhiteKnights || t & newBoard.BBBlackKnights) m.captured = KNIGHT;
	else if (t & newBoard.BBWhiteBishops || t & newBoard.BBBlackBishops) m.captured = BISHOP;
	else if (t & newBoard.BBWhiteRooks || t & newBoard.BBBlackRooks) m.captured = ROOK;
	else if (t & newBoard.BBWhiteQueens || t & newBoard.BBBlackQueens) m.captured = QUEEN;
	else if (t & newBoard.BBWhiteKing || t & newBoard.BBBlackKing) m.captured = KING;
	else m.captured = PIECE_EMPTY; //no capture

	m.flag = '0';

	if (input.length() == 5) {
		if (input[4] == 'q') m.flag = 'Q';
		else if (input[4] == 'r') m.flag = 'R';
		else if (input[4] == 'b') m.flag = 'B';
		else if (input[4] == 'n') m.flag = 'N';
	}	
	
	return m;
}

