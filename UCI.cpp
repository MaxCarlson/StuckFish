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
			test(newBoard, si);
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
			history.twoFoldRep.push_back(newBoard.TTKey());
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
	int flipsN[8]         = {  8,   7,   6,   5,   4,   3,   2,   1  };

	int x  = file_of(from_sq(m));
	int y  = rank_of(from_sq(m)) ^ 7; // Note: Move scheme was changed and too lazy to change array
	int x1 = file_of(to_sq(  m));
	int y1 = rank_of(to_sq(  m)) ^ 7;

	std::string promL = "";

	if (move_type(m) == PROMOTION) promL = "q"; //promotion flags


	std::stringstream ss;
	ss << flipsL[x] << flipsN[y] << flipsL[x1] << flipsN[y1] << promL;

	return ss.str();
}

Move UCI::strToMove(BitBoards& newBoard, std::string& input)
{
	Move m;
	int flipsN[9] = {0, 7, 6, 5, 4, 3, 2, 1, 0};
	char flipsA[8]{ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };

	int x, y, x1, y1;

	char cx = input[0];
	char cx1 = input[2];

	for (int i = 0; i < 8; ++i) {
		if (cx == flipsA[i]) x = i;
		if (cx1 == flipsA[i]) x1 = i;
	}

	y = flipsN[input[1] - '0'];
	y1 = flipsN[input[3] - '0'];

	int from = y * 8 + x;
	int to = y1 * 8 + x1;
	int piece = newBoard.pieceOnSq(from);


	int typeOfMove = NORMAL;

	// en passants
	if (piece == PAWN && to == newBoard.ep_square()) {

		typeOfMove = ENPASSANT;
	}

	//promotions
	if (input.length() == 5) {
		if (input[4] == 'q')
			typeOfMove = PROMOTION;
		//below not implemented!!!!!!!!!!!
		//else if (input[4] == 'r') m.flag = 'R';
		//else if (input[4] == 'b') m.flag = 'B';
		//else if (input[4] == 'n') m.flag = 'N';
	}	

	//NEED CASTLING CODE ONCE IMPLEMENTED
	if (piece == KING) {
		if (!(newBoard.psuedoAttacks(KING, 0, from) & newBoard.squareBB[to]))
			typeOfMove = CASTLING;
	}

	if (typeOfMove == NORMAL)
		m = create_move(from, to);

	else if (typeOfMove == ENPASSANT)
		m = create_special<ENPASSANT, PIECE_EMPTY>(from, to);

	else if (typeOfMove == PROMOTION)
		m = create_special<PROMOTION, QUEEN>(from, to);

	else if (typeOfMove == CASTLING)
		m = create_special<CASTLING, PIECE_EMPTY>(from, to);



	return m;
}

static const char* Defaults[] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
	"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
	"4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
	"rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
	"r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
	"r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
	"r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
	"r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
	"4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
	"2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
	"r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
	"3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
	"r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
	"4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
	"3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
	"6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
	"3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
	"2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 0 1",
	"8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
	"7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
	"8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
	"8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
	"8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
	"8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1",
	"5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1",
	"6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1",
	"1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1",
	"6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1",
	"8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1"
};


// Function runs through and searches all the positions
// that are in Defaults above. Good for benchmarking.
// Just be sure to keep wtime and btime the same.
void UCI::test(BitBoards & newBoard, StateInfo & si)
{
	searchM.initSearch();

	long nodes = 0;

	time_t starttimer;

	time(&starttimer);

	for (int i = 0; i < 30; ++i) {
		newGame(newBoard);
		searchM.clearHistorys();
		TT.clearTable(); //need to clear other TTables too at somepoint ??
		wtime = btime = 290000;

		std::string testFen = "fen ";

		testFen += Defaults[i];

		std::istringstream is(testFen);

		updatePosition(newBoard, is, si);

		search(newBoard);

		nodes += sd.nodes;
	}

	time_t endtimer;

	time(&endtimer);

	long endTotalTime = (endtimer - starttimer);

	std::cout << "Total time taken    : " << endTotalTime << " seconds." << std::endl;
	std::cout << "Total nodes searched: " << nodes << std::endl;
	std::cout << "Nodes per second    : " << nodes / endTotalTime << std::endl;
}

