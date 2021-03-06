#include "UCI.h"

#include <iostream>
#include <string>
#include <sstream>

#include "externs.h"
#include "bitboards.h"
#include "ai_logic.h"
#include "TranspositionT.h"
#include "Thread.h"
#include "movegen.h"


//is it whites turn?
bool isWhite = true; // get rid of this and replace with int color


UCI::UCI()
{
	TT.resize(1024); // This should eventually be removed. Just here for ease of use testing. 
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

	Threads.initialize();
	
	// Holds all the stateInfo's throughout plys of the search,
	// that allow easy recovery of board information after undoing a move.
	StateListPtr states(new std::deque<StateInfo>(1));

	// Make sure that the outputs are sent straight away to the GUI
	std::cout.setf(std::ios::unitbuf);

	newGame(newBoard, states);
	Search::initSearch();

	sync_out << "Type 'help' or '?' for list of engine options." << sync_endl;

	while (std::getline(std::cin, line))
	{
		std::istringstream is(line);

		token.clear(); // getline() could return empty or blank line

		is >> std::skipws >> token;

		if (token == "uci")
		{
			sync_out << "id name " << ENGINE_NAME << sync_endl;
			sync_out << "id author Maxwell Carlson" << sync_endl;
			printOptions();
			sync_out << "uciok" << sync_endl;
		}
		else if (token == "isready")
		{
			sync_out << "readyok" << sync_endl;
		}

		else if (token == "setoption") {
			setOption(is);
		}
		else if (token == "ucinewgame")
		{
			newGame(newBoard, states);
			Search::clear();
		}
		else if (token == "test") { // Used to enable quick testing
			test(newBoard, states);
		}
		else if (token == "threads") { // Debug set threads
			is >> token;
			Threads.numberOfThreads(stoi(token));
		}
		else if (token == "perft" || token == "divide") {
			bool div = token == "divide";

			perftUCI(newBoard, div, is);
		}
		else if (token == "stop") {
			Threads.stop = true;
		}
		else if (token == "position") {
			updatePosition(newBoard, is, states);
		}
		else if (token == "print" || token == "draw"){
			newBoard.drawBBA();
		}
		else if (token == "help" || token == "?")
		{
			helpUCI();
		}
		else if (token == "printOptions") {
			printOptions();
		}
		else if (token == "go") go(newBoard, is, states);

		else if (token == "quit")
		{
			sync_out << "Terminating.." << sync_endl;
			break;
		}
		else
			// Command not handled
			sync_out << "what?" << sync_endl;
	}
}

void UCI::updatePosition(BitBoards& newBoard, std::istringstream& input, StateListPtr& states)
{
	Move m;
	std::string token, fen;

	input >> token;

	// This is important, with how state ptrs are handled in 
	// Threads.searchStart it's neccasary to create a new one each time we want to
	// change the state. 
	states = StateListPtr(new std::deque<StateInfo>(1)); // Drop old and create a new one

	if (token == "startpos")
	{
		newGame(newBoard, states);
	}
	else if (token == "fen")
	{
		while (input >> token && token != "moves")
			fen += token + " ";

		newBoard.constructBoards(&fen, Threads.main(), &states->back());
		isWhite = !newBoard.bInfo.sideToMove;
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
			//parse string from gui/command line and create move
			m = strToMove(newBoard, token);

			states->emplace_back();
			//make move + increment turns
			newBoard.makeMove(m, states->back(), !isWhite);
			turns += 1;
			
			isWhite = !isWhite;
		}
	}

}

void UCI::newGame(BitBoards& newBoard, StateListPtr& states)
{
	states = StateListPtr(new std::deque<StateInfo>(1));

	newBoard.constructBoards(NULL, Threads.main(), &states->back());

	//create zobrist hash for startpos that is used to check TT entrys and repetitions 
	newBoard.zobrist.getZobristHash(newBoard);

	turns = 0;

	isWhite = true;
}

void UCI::printOptions()
{
	// Print the options avaibible to us. 
	sync_out << "option name Hash type spin default 1024 min 1 max 1048576" << sync_endl;
	sync_out << "option name Threads type spin default 1 min 1 max 128" << sync_endl;
	sync_out << "option name Clear Hash type button" << sync_endl;

	//other options ....
}

// If UCI wants to use any of the above options
// They're set through this function. 
void UCI::setOption(std::istringstream & input)
{
	std::string token;

	input >> token;

	if (token == "name") {
		while (input >> token) {
			if (token == "Hash") {
				input >> token; input >> token;
				TT.resize(stoi(token));
				break;
			}
			else if (token == "Clear") {
				TT.clearTable();
				break;
			}
			else if (token == "Threads") {
				input >> token; input >> token;
				Threads.numberOfThreads(stoi(token));
				break;
			}
		}
	}
	
}

void UCI::go(BitBoards & newBoard, std::istringstream & input, StateListPtr& states)
{
	Search::SearchControls scs;

	// Start clock !!
	scs.startTime = now();

	std::string token;

	while (input >> token)
	{
		if (token == "wtime")          input >> scs.time[WHITE];
		else if (token == "btime")     input >> scs.time[BLACK];
		else if (token == "winc")      input >> scs.inc[ WHITE]; //Not supported atm
		else if (token == "binc")      input >> scs.inc[ BLACK];
		else if (token == "movestogo") input >> scs.movesToGo; // Not supported atm
		else if (token == "depth")     input >> scs.depth;
		else if (token == "infinite")  scs.infinite = 1;  // Not supported
	}

	Threads.searchStart(newBoard, states, scs);

	// Moved print move into search.
	//sync_out << "bestmove " << Uci::moveToStr(m) << sync_endl;

	isWhite = !isWhite; //switch color after move 					
	turns += 1;    //// IS this and ^^ redundant? state isn't saved unless run through update pos from beginning anyways, TEST
}

// Move all of UCI to namespace eventually, no reason to have it as a class.
namespace Uci {

	std::string moveToStr(const Move& m)
	{
		std::string flipsL[8] = { "a", "b", "c", "d", "e", "f", "g", "h" };
		int flipsN[8] = { 8,   7,   6,   5,   4,   3,   2,   1 };

		if (m == MOVE_NONE)
			return "(none)";

		if (m == MOVE_NULL)
			return "0000";

		int x = file_of(from_sq(m));
		int y = rank_of(from_sq(m)) ^ 7; // Note: Move scheme was changed and too lazy to change array
		int x1 = file_of(to_sq(m));
		int y1 = rank_of(to_sq(m)) ^ 7;

		std::string promL;
		if (move_type(m) == PROMOTION)
			promL = " pnbrqk"[promotion_type(m)];


		std::stringstream ss;
		ss << flipsL[x] << flipsN[y] << flipsL[x1] << flipsN[y1] << promL;

		return ss.str();
	}
}


Move UCI::strToMove(BitBoards& newBoard, std::string& input)
{
	// In case promotion piece character is sent
	// as upper case. 
	if (input.length() == 5)
		input[4] = char(tolower(input[4]));

	// If move is in the list of legal moves, then it's valid.
	// Return it.
	for (const auto& m : MoveList<LEGAL>(newBoard))
		if (input == Uci::moveToStr(m))
			return m;


	return MOVE_NONE;
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
void UCI::test(BitBoards & newBoard, StateListPtr& states)		// Give this an input for searching to certain depth, and possibly an option to choose divide or perft as well
{
	long nodes = 0;

	time_t starttimer;

	time(&starttimer);

	for (int i = 0; i < 30; ++i) {
		Search::clear();

		std::string testFen = "fen ";

		testFen += Defaults[i];

		std::istringstream is(testFen);

		updatePosition(newBoard, is, states);

		// Pass go the desired depth. Make this an input for test? 
		std::string sdepth = "depth 15";
		std::istringstream iss(sdepth);

		go(newBoard, iss, states);

		nodes += Threads.nodes_searched();
	}


	time_t endtimer;

	time(&endtimer);

	long endTotalTime = (endtimer - starttimer);

	sync_out << "Total time taken    : " << endTotalTime << " seconds." << sync_endl;
	sync_out << "Total nodes searched: " << nodes << sync_endl;
	sync_out << "Nodes per second    : " << nodes / endTotalTime << sync_endl;
}

void UCI::perftUCI(BitBoards & newBoard, bool isDivide, std::istringstream & input)
{
	std::string tk;

	input >> tk;

	int d = std::stoi(tk);

	Search::perftInit(newBoard, isDivide, d);
}

void UCI::drawUCI(BitBoards & newBoard)
{
	newBoard.drawBBA();
}

void UCI::helpUCI()
{
	sync_indent;
	sync_out <<  ENGINE_NAME << " by Max Carlson" << sync_endl;
	sync_out << "ucinewgame...............Resets current position to a clean slate. Clears TTable as well" << sync_endl;
	sync_out << "draw.....................Draws ASCI representation of the current board"<< sync_endl;
	sync_out << "perft x..................Perft results for current position at depth x" << sync_endl;
	sync_out << "divide y.................Like perft, but move counts for all root moves are printed instead of just a total count." << sync_endl;
	sync_out << "(Note! Perft and divide can be multi-threaded, specify threads first)" << sync_endl << sync_endl;
	sync_out << "position fen <FEN>.......Sets position to input fen string"<< sync_endl;
	sync_out << "position startpos........Sets position to start position"<< sync_endl;
	sync_out << "position x moves x.......Sets position fen or startpos and makes input moves "<< sync_endl;
	sync_out << "go wtime x  btime y......Starts search with white remaining time x and black remaining time y"<< sync_endl;
	sync_out << "go depth x...............Starts search and will search to x depth"<< sync_endl;
	sync_out << "go infinite..............Searches until the user types 'stop' or the GUI sends the command" << sync_endl;
	sync_out << "stop.....................Stops search as soon as possible" << sync_endl;
	sync_out << "test.....................Runs through a test suite of positions at a fixed depth, designed for benchmarking search"<< sync_endl;
	sync_out << "threads x................Sets program to use x number of threads." << sync_endl;
	sync_out << "quit.....................Quits engine"<< sync_endl;
	sync_indent;
}

