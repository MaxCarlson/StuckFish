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

    //iterative deepening
    Move iterativeDeep(int depth);


private:
    //root of search ~~ experimental
    int searchRoot(U8 depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, U8 ply);

    //minmax with alpha beta, the main component of our search
    int alphaBeta(U8 depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, U8 ply, bool allowNull, bool is_pv);

    //Quiescent search ~~ search positions farther if there are captures on horizon
    int quiescent(int alpha, int beta, bool isWhite, int ply, int quietDepth, long currentTime, long timeLimmit);
    //if a capture cannot increase alpha, don't bother searching it
    /*
    bool deltaPruning(std::string move, int eval, bool isWhite, int alpha, bool isEndGame, BitBoards *BBBoard);
    */

    //make ai prefer checkmate over stalemate
    int contempt(); //need some way to check board material before implementing

//heuristics

    void addKiller(Move move, int ply);
    void ageHistorys();

//transposition table functions
    //add best move to TT
    void addMoveTT(Move move, int depth, int eval, int flag);

//object variables

    //counts number of piece postitions tried
    int positionCount = 0;

    //count of quiescnce positions checked
    int qCount = 0;

    //principal variation array
    Move pVArr[29];


};

#endif // AI_LOGIC_H

