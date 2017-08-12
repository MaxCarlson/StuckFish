#include "ai_logic.h"

#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "externs.h"

//#include "move.h"
#include "evaluatebb.h"
#include "bitboards.h"
#include "movegen.h"
#include "zobristh.h"
#include "hashentry.h"

#define DO_NULL    true //allow null moves or not
#define NO_NULL    false
#define IS_PV      true //is this search in a principal variation
#define NO_PV      false
#define ASP        50  //aspiration windows size

//holds historys and killers + eventually nodes searched + other data
searchDriver sd;

//constructed once so as to pass to eval where constructing was slow
//for quiet search
MoveGen evalGenMoves;

//value to determine if time for search has run out
extern bool searchCutoff;
bool searchCutoff = false;

//master bitboard for turn
BitBoards newBoard;
//master zobrist for turn
ZobristH zobrist;

Ai_Logic::Ai_Logic()
{

}

Move Ai_Logic::iterativeDeep(int depth)
{
    //read array and construct the bitboards to match
    newBoard.constructBoards();
    //generate accurate zobrist key based on bitboards
    zobrist.getZobristHash(newBoard);
    //ai is always black atm, if it wasn't update color only applied on black turn
    zobrist.UpdateColor();

    //iterative deepening start time
    clock_t IDTimeS = clock();

    //time limit in miliseconds
    int timeLimmit = 1000099, ply = 0;
    long endTime = IDTimeS + timeLimmit;

    //best overall move as calced
    Move bestMove;
    int distance = 1, bestScore, alpha = -INF, beta = INF;
    //search has not run out of time
    searchCutoff = false;

    //std::cout << zobrist.zobristKey << std::endl;

    //iterative deepening loop starts at depth 1, iterates up till max depth or time cutoff
    while(distance <= depth && IDTimeS < endTime){
        clock_t currentTime = clock();

        //if we're past 2/3's of our time limit, stop searching
        if(currentTime >= endTime - (1/3 * timeLimmit)){
            break;
        }

        //main search
        //bestScore = alphaBeta(distance, alpha, beta, false, currentTime, timeLimmit, ply +1, true, IS_PV);
        bestScore = searchRoot(distance, alpha, beta, false, currentTime, timeLimmit, ply+1);
/*
        if(bestScore <= alpha || bestScore >= beta){
            alpha = -INF;
            beta = INF;
            continue;
        }
        alpha = bestScore - ASP;
        beta = bestScore + ASP;
*/
        //if the search is not cutoff
        if(!searchCutoff){

            //grab best move out of PV array ~~ need better method of grabbing best move, not based on "distance"
            //Maybe if statement is a bad fix? sometimes a max of specified depth is not reached near checkmates/possibly checks
            //if statement makes sure the move has been tried before storing it
            if(pVArr[distance].tried) bestMove = pVArr[distance];

        }
        //increment distance to travel (same as depth at max depth)
        distance++;
    }


    //make final move on bitboards + draw
    newBoard.makeMove(bestMove, zobrist, false);
    newBoard.drawBBA();

    evaluateBB ev; //used for prininting static eval after move
    clock_t IDTimeE = clock();
    //postion count and time it took to find move
    std::cout << positionCount << " positions searched." << std::endl;
    std::cout << (double) (IDTimeE - IDTimeS) / CLOCKS_PER_SEC << " seconds" << std::endl;
    std::cout << "Depth of " << distance-1 << " reached."<<std::endl;
    std::cout << qCount << " non-quiet positions searched."<< std::endl;
    std::cout << "Board evalutes to: " << ev.evalBoard(true, newBoard, zobrist) << " for white." << std::endl;

    //decrease history values after a search
    ageHistorys();

    return bestMove;
}

int Ai_Logic::searchRoot(U8 depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, U8 ply)
{
    bool flagInCheck;
    int score = 0;
    int best = -INF;
    int legalMoves;

    MoveGen gen_moves;
    //grab bitboards from newBoard object and store color and board to var
    gen_moves.grab_boards(newBoard, isWhite);
    gen_moves.generatePsMoves(false);

    //who's our king?
    U64 king;
    if(isWhite) king = newBoard.BBWhiteKing;
    else king = newBoard.BBBlackKing;
    //are we in check?
    flagInCheck = gen_moves.isAttacked(king, isWhite, true);
    U8 moveNum = gen_moves.moveCount;


    if(flagInCheck) ++depth;

    Move bestMove;
    for(U8 i = 0; i < moveNum; ++i){
        //find best move generated
        Move newMove = gen_moves.movegen_sort(ply);
        newBoard.makeMove(newMove, zobrist, isWhite);  /// !~!~!~!~!~!~!~!~!~! try SWITCHing from a move gen object haveing a local array to the psuedo movegen funcion
        gen_moves.grab_boards(newBoard, isWhite);      /// returning a vector or a pointer to an array that's stored in the search, so we can stop constructing movegen
                                                       /// objects and instead just have a local global for the duration of the search. sort could return the index of the best move
        //is move legal? if not skip it                ///and take the array or vector as a const refrence argument
        if(gen_moves.isAttacked(king, isWhite, true)){
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }
        positionCount ++;
        legalMoves ++;
        sd.cutoffs[isWhite][newMove.from][newMove.to] -= 1;

        //PV search at root
        if(best == -INF){
            //full window PV search
            score = -alphaBeta(depth-1, -beta, -alpha, !isWhite, currentTime, timeLimmit, ply +1, DO_NULL, IS_PV);

       } else {
            //zero window search
            score = -alphaBeta(depth-1, -alpha -1, -alpha, !isWhite, currentTime, timeLimmit, ply +1, DO_NULL, NO_PV);

            //if we've gained a new alpha we need to do a full window search
            if(score > alpha){
                score = -alphaBeta(depth-1, -beta, -alpha, !isWhite, currentTime, timeLimmit, ply +1, DO_NULL, IS_PV);
            }
        }

        //undo move on BB's
        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        if(score > best) best = score;

        if(score > alpha){
            //store the principal variation
            pVArr[depth] = newMove;

            if(score > beta){
                addMoveTT(newMove, depth, score, TT_BETA);
                return beta;
            }

            alpha = score;
            bestMove = newMove;
            addMoveTT(bestMove, depth, score, TT_ALPHA);
        }

    }
    addMoveTT(bestMove, depth, score, TT_EXACT);
    return alpha;
}

int Ai_Logic::alphaBeta(U8 depth, int alpha, int beta, bool isWhite, long currentTime, long timeLimmit, U8 ply, bool allowNull, bool is_pv)
{

    //iterative deeping timer stuff
    clock_t time = clock();
    long elapsedTime = time - currentTime;
    bool FlagInCheck = false;
    bool raisedAlpha = false;
    char R = 2;
    U8 reductionDepth;
    U8 newDepth;
    //U8 newDepth; //use with futility + other pruning later
    int queitSD = 25, f_prune = 0;
    //int  mateValue = INF - ply; // used for mate distance pruning


    //create unqiue hash from zobrist key
    int hash = (int)(zobrist.zobristKey % 15485843);
    HashEntry entry = transpositionT[hash];

    //if the depth of the stored evaluation is greater and the zobrist key matches
    //don't return eval on root node
    if(entry.depth >= depth && entry.zobrist == zobrist.zobristKey){
        //return either the eval, the beta, or the alpha depending on flag
        switch(entry.flag){
            case 1:
            if(entry.eval <= alpha){
                return alpha;
            }
            break;
            case 2:
            if(entry.eval >= beta){
                return beta;
            }
            break;            
            case 3:
            if(entry.flag == 3){
                return entry.eval;
            }
            break;
        }

    }

    //if the time limmit has been exceded set stop search flag
    if(elapsedTime >= timeLimmit){
        searchCutoff = true;
    }

    int score;
    if(depth < 1 || searchCutoff){
        //run capture search to max depth of queitSD
        score = quiescent(alpha, beta, isWhite, ply, queitSD, currentTime, timeLimmit);
        return score;
    }

    MoveGen gen_moves;
    //grab bitboards from newBoard object and store color and board to var
    gen_moves.grab_boards(newBoard, isWhite);


    U64 king, eking;
    if(isWhite){ king = newBoard.BBWhiteKing; eking = newBoard.BBBlackKing; }
    else { king = newBoard.BBBlackKing; eking = newBoard.BBWhiteKing; }
    //are we in check?
    FlagInCheck = gen_moves.isAttacked(king, isWhite, true);

    if(FlagInCheck) goto moves_loop; //if in check, skip nulls, statics evals, razoring, etc

//eval pruning / static null move
    if(depth < 3 && !is_pv && abs(beta - 1) > -INF + 100){
        evaluateBB eval;
        int static_eval = eval.evalBoard(isWhite, newBoard, zobrist);
        int eval_margin = 120 * depth;
        if(static_eval - eval_margin >= beta){
            return static_eval - eval_margin;
        }
    }


//Null move heuristics, disabled if in PV, check, or depth is too low
    if(allowNull && !is_pv && depth > R){
        if(depth > 6) R = 3;
        zobrist.UpdateColor();

        score = -alphaBeta(depth -R -1, -beta, -beta +1, !isWhite, currentTime, timeLimmit, ply +1, NO_NULL, NO_PV);
        zobrist.UpdateColor();
        //if after getting a free move the score is too good, prune this branch
        if(score >= beta){
            return score;
        }
    }


//razoring if not PV and is close to leaf and has a low score drop directly into quiescence
    if(!is_pv && allowNull && depth <= 3){
        evaluateBB eval;
        int threshold = alpha - 300 - (depth - 1) * 60;        
        if(eval.evalBoard(isWhite, newBoard, zobrist) < threshold){
            score = quiescent(alpha, beta, isWhite, ply, queitSD, currentTime, timeLimmit);

            if(score < threshold) return alpha;
        }
    }

/*
//do we want to futility prune?
    int fmargin[4] = { 0, 200, 300, 500 };
    if(depth <= 3 && !is_pv
    && !FlagInCheck && abs(alpha) < 9000
    && eval.evalBoard(isWhite, newBoard, zobrist) + fmargin[depth] <= alpha){
        f_prune = 1;
    }
*/

//jump to here if in check
moves_loop:
//generate psuedo legal moves (not just captures)
    gen_moves.generatePsMoves(false);

    //add killers scores and hash moves scores to moves if there are any
    gen_moves.reorderMoves(ply, entry);

    int hashFlag = TT_ALPHA, movesNum = gen_moves.moveCount, legalMoves = 0;

    Move hashMove; //move to store alpha in and pass to TTable
    for(U8 i = 0; i < movesNum; ++i){
        //grab best scoring move
        Move newMove = gen_moves.movegen_sort(ply);

        //make move on BB's store data to string so move can be undone
        newBoard.makeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        //is move legal? if not skip it
        if(gen_moves.isAttacked(king, isWhite, true)){ ///CHANGE METHOD OF UPDATING BOARDS, TOO INEFFICIENT
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }
        positionCount ++;
        legalMoves ++;
        reductionDepth = 0;
        newDepth = depth - 1;
        sd.cutoffs[isWhite][newMove.from][newMove.to] -= 1;

        /* STILL TOO SLOW
        //futility pruning ~~ is not a promotion, is not a capture, and does not give check, and we've tried one move already
        if(f_prune && i > 0 && newMove.flag != 'Q'
            && newMove.captured == '0' && legalMoves
            && gen_moves.isAttacked(eking, !isWhite, false)){
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            continue;
        }
        */

        //late move reduction, still in TESTING
        if(!is_pv && newDepth > 3
                && legalMoves > 3
                && !gen_moves.isAttacked(eking, !isWhite, false)
                && !FlagInCheck
                && sd.cutoffs[isWhite][newMove.from][newMove.to] < 50
                && (newMove.from != sd.killers[0][ply].from || newMove.to != sd.killers[0][ply].to)
                && (newMove.from != sd.killers[1][ply].from || newMove.to != sd.killers[1][ply].to)
                && newMove.captured == '0' && newMove.flag != 'Q'){


            sd.cutoffs[isWhite][newMove.from][newMove.to] = 50;
            reductionDepth = 1;
            if(legalMoves > 6) reductionDepth += 1;
            newDepth -= reductionDepth;
        }

re_search:

        if(!raisedAlpha){
            //we're in princiapl variation search or full window search
            score = -alphaBeta(newDepth, -beta, -alpha, !isWhite, currentTime, timeLimmit, ply +1, DO_NULL, is_pv);
        } else {
            //zero window search
            score = -alphaBeta(newDepth, -alpha -1, -alpha, !isWhite, currentTime, timeLimmit, ply +1, DO_NULL, NO_PV);
            //if our zero window search failed, do a full window search
            if(score > alpha){
                //PV search after failed zero window
                score = -alphaBeta(newDepth, -beta, -alpha,  !isWhite, currentTime, timeLimmit, ply +1, DO_NULL, IS_PV);
            }
        }

        //if a reduced search brings us above alpha, do a full non-reduced search
        if(reductionDepth && score > alpha){
            newDepth += reductionDepth;
            reductionDepth = 0;
            goto re_search;
        }


        //undo move on BB's
        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        if(score > alpha){            
            hashMove = newMove;
            sd.cutoffs[isWhite][newMove.from][newMove.to] += 6;

            //if move causes a beta cutoff stop searching current branch
            if(score >= beta){

                if(newMove.captured == '0' && newMove.flag != 'Q'){
                    //add move to killers
                    addKiller(hashMove, ply);

                    //add score to historys
                    sd.history[isWhite][newMove.from][newMove.to] += depth * depth;
                    //don't want historys to overflow if search is really big
                    if(sd.history[isWhite][newMove.from][newMove.to] > SORT_KILL){
                        for(int i = 0; i < 64; i++){
                            for(int i = 0; i < 64; i++){
                                sd.history[isWhite][newMove.from][newMove.to] /= 2;
                            }
                        }
                    }

                }
                hashFlag = TT_BETA;
                //stop search and return beta
                alpha = beta;
                break;
            }
            //new best move
            alpha = score;
            raisedAlpha = true;
            //if we've gained a new alpha set hash Flag equal to exact and store best move
            hashFlag = TT_EXACT;

        }
    }

    if(!legalMoves){
        if(FlagInCheck) alpha = -INF + ply;
        //else alpha = contempt(); //NEED to add contempt, make program favor not draws
    }

    //add alpha eval to hash table don't save a real move
    addMoveTT(hashMove, depth, alpha, hashFlag);

    return alpha;
}

int Ai_Logic::quiescent(int alpha, int beta, bool isWhite, int ply, int quietDepth, long currentTime, long timeLimmit)
{
    //iterative deeping timer stuff
    clock_t time = clock();
    long elapsedTime = time - currentTime;

    //create unqiue hash from zobrist key
    int hash = (int)(zobrist.zobristKey % 15485843);
    HashEntry entry = transpositionT[hash];

    //if the time limmit has been exceeded finish searching
    if(elapsedTime >= timeLimmit){
        searchCutoff = true;
    }

    evaluateBB eval;
    //evaluate board position
    int standingPat = eval.evalBoard(isWhite, newBoard, zobrist);

    if(quietDepth <= 0 || searchCutoff){
        return standingPat;
    }

    if(standingPat >= beta){
       return beta;
    }

    if(alpha < standingPat){
       alpha = standingPat;
    }

    //generate only captures with true capture gen var
    MoveGen gen_moves;
    gen_moves.grab_boards(newBoard, isWhite);
    gen_moves.generatePsMoves(true);
    gen_moves.reorderMoves(ply, entry);

    int score;
    //set hash flag equal to alpha Flag
    int hashFlag = 1, moveNum = gen_moves.moveCount;


    U64 king;
    Move hashMove;
    if(isWhite) king = newBoard.BBWhiteKing;
    else king = newBoard.BBBlackKing;

    for(int i = 0; i < moveNum; ++i)
    {        
        Move newMove = gen_moves.movegen_sort(ply);

        newBoard.makeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        //is move legal? if not skip it ~~~~~~~~ possibly remove check later?
        if(gen_moves.isAttacked(king, isWhite, true)){
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }

        qCount ++;

        /*
        // ~~~NEEDS WORK + Testing + stop pruning in end game
        if(deltaPruning(tempMove, standingPat, isWhite, alpha, false, newBoard)){
            continue; //uncomment to enable delta pruning!!!
        }

        //bad capture pruning
        if(badCapture(tempMove, isWhite, newBoard) && tempMove[3] != 'Q'){
            continue;
        }
        */
        score = -quiescent(-beta, -alpha, !isWhite, ply+1, quietDepth-1, currentTime, timeLimmit);

        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        if(score > alpha){
            if(score >= beta){
                //if(newMove.captured == '0' && newMove.flag != 'Q'){
                    //add move to killers
                 //   addKiller(newMove, ply);
                //}
                return beta;
            }

           alpha = score;
           //if we've gained a new alpha set hash Flag to exact
           hashFlag = TT_EXACT;
           hashMove = newMove;
        }
    }
    //add alpha eval to hash table
    //addTTQuiet(hashMove, ply, alpha, hashFlag, zobrist);
    return alpha;
}

int Ai_Logic::contempt()
{
    //need some way to check board material before implementing
}

void Ai_Logic::addKiller(Move move, int ply)
{
    if(move.captured == '0'){ //if move isn't a capture save it as a killer
        //make sure killer is different
        if(move.from != sd.killers[ply][0].from
          && move.to != sd.killers[ply][0].to){
            //save primary killer to secondary slot
            sd.killers[ply][1] = sd.killers[ply][0];
        }
        //save primary killer
        sd.killers[ply][0] = move;
        sd.killers[ply][0].tried = false;
    }
}

void Ai_Logic::ageHistorys()
{
    //used to decrease value after a search
    for (int cl = 0; cl < 2; cl++)
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 64; j++) {
                sd.history[cl][i][j] = sd.history[cl][i][j] / 8;
                sd.cutoffs[cl][i][j] = 100;
            }
}

void Ai_Logic::addMoveTT(Move move, int depth, int eval, int flag)
{
    //get hash of current zobrist key
    int hash = (int)(zobrist.zobristKey % 15485843);
    //if the depth of the current move is greater than the one it's replacing or if it's older than
    if(depth >= transpositionT[hash].depth || transpositionT[hash].ancient < turns - 5){
        //add position to the table
        transpositionT[hash].zobrist = zobrist.zobristKey;
        transpositionT[hash].depth = depth;
        transpositionT[hash].eval = eval;
        transpositionT[hash].flag = flag;
        transpositionT[hash].move = move;
        transpositionT[hash].ancient = turns;

    }

}

/*
bool Ai_Logic::deltaPruning(std::string move, int eval, bool isWhite, int alpha, bool isEndGame, MoveGen *newBoard)
{
    //if is end game, search all nodes
    if(isEndGame){
        return false;
    }

    U64 epawns, eknights, ebishops, erooks, equeens;
    //set enemy bitboards
    if(isWhite){
        //enemies
        epawns = newBoard.BBBlackPawns;
        eknights = newBoard.BBBlackKnights;
        ebishops = newBoard.BBBlackBishops;
        erooks = newBoard.BBBlackRooks;
        equeens = newBoard.BBBlackQueens;
    } else {
        epawns = newBoard.BBWhitePawns;
        eknights = newBoard.BBWhiteKnights;
        ebishops = newBoard.BBWhiteBishops;
        erooks = newBoard.BBWhiteRooks;
        equeens = newBoard.BBWhiteQueens;
    }



    U64 pieceMaskE = 0LL;
    int x, y, xyE;
    int score = 0, Delta = 200;
    //normal captures
    if(move[3] != 'Q'){
        x = move[2]-0; y = move[3]-0;

    } else {
        //pawn promotions
        score = 800;
        //forward non capture
        if(move[2] == 'F'){
            x = move[0];
        //capture
        } else {
            x = move[2]-0;
        }

        if(isWhite){
            y = 0;
        } else {
            y = 7;
        }
    }

    xyE = y*8+x;
    //create mask of move end position
    pieceMaskE += 1LL << xyE;

    //find whice piece is captured
    if(pieceMaskE & epawns){
        score += 100;
    } else if(pieceMaskE & eknights){
        score += 320;
    } else if(pieceMaskE & ebishops){
        score += 330;
    } else if(pieceMaskE & erooks){
        score += 500;
    } else if(pieceMaskE & equeens){
        score += 900;
    }

    //if score plus safety margin is less then alpha, don't bother searching node
    if(eval + score + Delta < alpha){
        return true;
    }

    return false;

}
*/

