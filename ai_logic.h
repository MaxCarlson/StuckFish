#ifndef AI_LOGIC_H
#define AI_LOGIC_H
#include <string>
#include <stack>
#include <thread>
#include "move.h"
//#include "hashentry.h"
#include "slider_attacks.h"
class ZobristH;
class BitBoards;
class evaluateBB;
class HashEntry;
class Move;

class Ai_Logic
{
public:
    Ai_Logic();

	Move search(bool isWhite);

    //iterative deepening
    Move iterativeDeep(int depth, bool isWhite);

	//clear historys, for new games
	void clearHistorys();


private:
	
	//root of search ~~ experimental
	int searchRoot(int depth, int alpha, int beta, bool isWhite, int ply);

	//minmax with alpha beta, the main component of our search
	int alphaBeta(int depth, int alpha, int beta, bool isWhite, int ply, bool allowNull, bool is_pv);

	//Quiescent search ~~ search positions farther if there are captures on horizon
	int quiescent(int alpha, int beta, bool isWhite, int ply, int quietDepth, bool is_pv);

    //make ai prefer checkmate over stalemate
    int contempt(bool isWhite); //need some way to check board material before implementing

	//repetition checker
	bool isRepetition(const Move& m);
	

//helpers

    void addKiller(Move move, int ply);
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

