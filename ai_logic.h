#ifndef AI_LOGIC_H
#define AI_LOGIC_H
#include <string>
#include <stack>
#include <thread>
#include "move.h"
#include "slider_attacks.h"

class ZobristH;
class BitBoards;
class evaluateBB;
class HashEntry;
class Move;

//holds info about past and current searches, passed to search
struct searchStack {
	int ply;
	//Move currentMove;
	Move killers[2];
	Move ttMove;
	Move currentMove;
	Move excludedMove;

	int reduction;
	int staticEval;
	bool skipNull = false;
};

class Ai_Logic
{
public:
	//initialze values called at program start
	void initSearch();

	Move search(BitBoards& newBoard, bool isWhite);

    //iterative deepening
    Move iterativeDeep(BitBoards& newBoard, int depth, bool isWhite);

	//clear historys, for new games
	void clearHistorys();

private:
	
	//root of search ~~ experimental
	int searchRoot(BitBoards& newBoard, int depth, int alpha, int beta, searchStack *ss, bool isWhite);

	//minmax with alpha beta, the main component of our search
	int alphaBeta(BitBoards& newBoard, int depth, int alpha, int beta, searchStack *ss, bool isWhite, bool allowNull, bool is_pv);

	//Quiescent search ~~ search positions farther if there are captures on horizon
	int quiescent(BitBoards& newBoard, int alpha, int beta, bool isWhite, searchStack *ss, int quietDepth, bool is_pv);

    //make ai prefer checkmate over stalemate
    int contempt(const BitBoards& newBoard, bool isWhite); //need some way to check board material before implementing

	//repetition checker
	bool isRepetition(const BitBoards& newBoard, const Move& m);
	

//helpers
	void updateStats(Move move, searchStack *ss, int depth, Move * quiets, int qCount, bool isWhite);

    void ageHistorys();
	

	

//transposition table functions
    //add best move to TT
    //void addMoveTT(Move move, int depth, int eval, int flag);

	//checks whether time is out through time manaager,
	//will later check for input as well
	void checkInput();

	//prints data to console/gui
	void print(bool isWhite, int bestScore);



};

#endif // AI_LOGIC_H

