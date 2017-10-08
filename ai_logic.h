#ifndef AI_LOGIC_H
#define AI_LOGIC_H
#include <string>
#include <stack>
#include <thread>
#include "slider_attacks.h"
#include "defines.h"


class BitBoards;
class evaluateBB;
class HashEntry;

namespace Search {

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
	void initSearch();
	Move searchStart(BitBoards & board, bool isWhite);
	Move iterativeDeep(BitBoards & board, int depth, bool isWhite);
	int searchRoot(BitBoards & board, int depth, int alpha, int beta, searchStack * ss);
	int alphaBeta(BitBoards & board, int depth, int alpha, int beta, searchStack * ss, bool allowNull, int isPV);
	int quiescent(BitBoards & board, int alpha, int beta, searchStack * ss, int isPV);
	int contempt(const BitBoards & board, int color);
	bool isRepetition(const BitBoards & board, Move m);
	void updateStats(Move move, searchStack * ss, int depth, Move * quiets, int qCount, int color);
	void ageHistorys();
	void clearHistorys();
	void checkInput();
	void print(bool isWhite, int bestScore);

	template<bool Root>
	U64 perft(BitBoards & board, int depth);

	template<bool Root>
	U64 perftDivide(BitBoards & board, int depth);
}
/*
class Ai_Logic
{
public:
	//initialze values called at program start
	void initSearch();

	Move searchStart(BitBoards& newBoard, bool isWhite);

    //iterative deepening
    Move iterativeDeep(BitBoards& newBoard, int depth, bool isWhite);

	//clear historys, for new games
	void clearHistorys();

	template<bool Root>
	U64 perft(BitBoards & board, int depth);

	template<bool Root>
	U64 perftDivide(BitBoards & board, int depth);

private:
	
	//root of search ~~ experimental
	int searchRoot(BitBoards& newBoard, int depth, int alpha, int beta, searchStack *ss);

	//minmax with alpha beta, the main component of our search
	int alphaBeta(BitBoards& newBoard, int depth, int alpha, int beta, searchStack *ss, bool allowNull, int isPV);

	//Quiescent search ~~ search positions farther if there are captures on horizon
	int quiescent(BitBoards& newBoard, int alpha, int beta, searchStack *ss, int isPV);

    //make ai prefer checkmate over stalemate
    int contempt(const BitBoards& newBoard, int color); //need some way to check board material before implementing

	//repetition checker
	bool isRepetition(const BitBoards& newBoard, Move m);

	void insert_pv(BitBoards & board);
	

//helpers
	void updateStats(Move move, searchStack *ss, int depth, Move * quiets, int qCount, int color);

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
*/

#endif // AI_LOGIC_H

