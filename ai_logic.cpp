#include "ai_logic.h"

#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>

#include "externs.h"
#include "evaluatebb.h"
#include "bitboards.h"
#include "movegen.h"
#include "zobristh.h"
#include "hashentry.h"
#include "TimeManager.h"
#include "TranspositionT.h"

#define DO_NULL    true //allow null moves or not
#define NO_NULL    false
#define IS_PV      true //is this search in a principal variation
#define NO_PV      false
#define ASP        50  //aspiration windows size
#define Move_None   Move n //possible to use this to identify a non move submited?

//holds historys and killers + eventually nodes searched + other data
searchDriver sd;

//constructed once so as to pass to eval where constructing was slow
//for quiet search
MoveGen evalGenMoves;

//value to determine if time for search has run out
bool timeOver;

//master time manager
TimeManager timeM;

int futileC = 0; //count of futile moves

//mate values are measured in distance from root, so need to be converted to and from TT
inline int valueFromTT(int val, int ply); 
inline int valueToTT(int val, int ply); 

//reduction tables: pv, is node improving?, depth, move number
int reductions[2][2][64][64];
//futile move count arrays
int futileMoveCounts[2][32];

//holds history and cutoff info for search, global
Historys h;

void Ai_Logic::initSearch()
{
	//values taken from stock fish, playing with numbers still
	int hd, d, mc; //half depth, depth, move count
	for (hd = 1; hd < 64; ++hd) for (mc = 1; mc < 64; ++mc) {

		double    pvReduciton = 0.00 + log(double(hd)) * log(double(mc)) / 3.00;
		double nonPVReduction = 0.30 + log(double(hd)) * log(double(mc)) / 2.25;

		reductions[1][1][hd][mc] = int8_t(pvReduciton >= 1.0 ? pvReduciton + 0.5 : 0);
		reductions[0][1][hd][mc] = int8_t(nonPVReduction >= 1.0 ? nonPVReduction + 0.5 : 0);

		reductions[1][0][hd][mc] = reductions[1][1][hd][mc];
		reductions[0][0][hd][mc] = reductions[0][1][hd][mc];

		if (reductions[0][0][hd][mc] >= 2)
			reductions[0][0][hd][mc] += 1;
	}

	for (d = 0; d < 32; ++d)
	{
		futileMoveCounts[0][d] = int(2.4 + 0.222 * pow(d * 2 + 0.00, 1.8));
		futileMoveCounts[1][d] = int(3.0 + 0.300 * pow(d * 2 + 0.98, 1.8));
	}
}

Move Ai_Logic::search(bool isWhite) {
	
	//max depth
	int depth = MAX_PLY;

	//generate accurate zobrist key based on bitboards
	zobrist.getZobristHash(newBoard);

	//update color only applied on black turn
	if (!isWhite) zobrist.UpdateColor();

	//decrease history values after a search
	ageHistorys();

	//calc move time for us, send to search driver
	timeM.calcMoveTime(isWhite);

	Move m = iterativeDeep(depth, isWhite);
	
	return m;
}

Move Ai_Logic::iterativeDeep(int depth, bool isWhite)
{
	//reset ply
    int ply = 0;

	//create array of stack objects starting at +2 so we can look two plys back
	//in order to see if a node has improved, as well as prior stats
	searchStack stack[MAX_PLY + 6], *ss = stack + 2;
	std::memset(ss - 2, 0, 5 * sizeof(searchStack));

    //best overall move as calced
    Move bestMove;
    int bestScore, alpha = -INF, beta = INF;
	int delta = 8;
	sd.depth = 1;

    //std::cout << zobrist.zobristKey << std::endl;

    //iterative deepening loop starts at depth 1, iterates up till max depth or time cutoff
    while(sd.depth <= depth){

		if (timeM.timeStopRoot() || timeOver) break;

        //main search
        bestScore = searchRoot(sd.depth, alpha, beta, ss, isWhite);
/*
        if(bestScore <= alpha){ //ISSUES WITH ASPIRATION WINDOWS
			alpha = std::max(bestScore - delta, -INF);			
		}
		else if (bestScore >= beta) {
			beta = std::min(bestScore + delta, INF);
		}

		if (bestScore <= alpha || bestScore >= beta) continue;

		delta += 3 * delta / 8;
*/
        //if the search is not cutoff
        if(!timeOver){

            //grab best move out of PV array ~~ need better method of grabbing best move, not based on "distance"
            //Maybe if statement is a bad fix? sometimes a max of specified depth is not reached near checkmates/possibly checks
            //if statement makes sure the move has been tried before storing it
            if(sd.PV[1].tried) bestMove = sd.PV[1];

			//print data on search 
			print(isWhite, bestScore);
			//std::cout << futileC << std::endl;
        }
		sd.depth++;
        //increment depth 
    }


    //make final move on bitboards + draw board
    newBoard.makeMove(bestMove, zobrist, isWhite);
    newBoard.drawBBA();
	//std::cout << newBoard.sideMaterial[0] << " " << newBoard.sideMaterial[1] <<std::endl;

    //evaluateBB ev; //used for prininting static eval after move
    //std::cout << "Board evalutes to: " << ev.evalBoard(true, newBoard, zobrist) << " for white." << std::endl;

    return bestMove;
}

int Ai_Logic::searchRoot(int depth, int alpha, int beta, searchStack *ss, bool isWhite)
{
    bool flagInCheck;
    int score = 0;
    int best = -INF;
    int legalMoves = 0;
	int hashFlag = TT_ALPHA;

	const HashEntry* ttentry = TT.probe(zobrist.zobristKey);

	sd.nodes++;
	ss->ply = 1;
	(ss + 1)->skipNull = false;

    MoveGen gen_moves;
    //grab bitboards from newBoard object and store color and board to var
    gen_moves.grab_boards(newBoard, isWhite);
    gen_moves.generatePsMoves(false);
	gen_moves.reorderMoves(ss, ttentry);

    //who's our king?
    U64 king;
    if(isWhite) king = newBoard.BBWhiteKing;
    else king = newBoard.BBBlackKing;	

    int moveNum = gen_moves.moveCount;

    for(int i = 0; i < moveNum; ++i){
        //find best move generated
        Move newMove = gen_moves.movegen_sort(ss->ply, &gen_moves.moveAr[0]);
        newBoard.makeMove(newMove, zobrist, isWhite);  

		if (isRepetition(newMove)) { //if position from root move is a two fold repetition, discard that move
			newBoard.unmakeMove(newMove, zobrist, isWhite);
			gen_moves.grab_boards(newBoard, isWhite);
			continue;
		}											   /// !~!~!~!~!~!~!~!~!~! try SWITCHing from a move gen object having a local array to the psuedo movegen funcion
        gen_moves.grab_boards(newBoard, isWhite);      /// returning a vector or a pointer to an array that's stored in the search, so we can stop constructing movegen
                                                       /// objects and instead just have a local global for the duration of the search. sort could return the index of the best move
        //is move legal? if not skip it                ///and take the array or vector as a const refrence argument
        if(gen_moves.isAttacked(king, isWhite, true)){
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }
        legalMoves ++;
        h.cutoffs[isWhite][newMove.from][newMove.to] -= 1;

        //PV search at root
        if(best == -INF){
            //full window PV search
            score = -alphaBeta(depth-1, -beta, -alpha, ss+1, !isWhite, DO_NULL, IS_PV);

       } else {
            //zero window search
            score = -alphaBeta(depth-1, -alpha -1, -alpha, ss + 1, !isWhite, DO_NULL, NO_PV);

            //if we've gained a new alpha we need to do a full window search
            if(score > alpha){
                score = -alphaBeta(depth-1, -beta, -alpha, ss + 1, !isWhite, DO_NULL, IS_PV);
            }
        }

        //undo move on BB's
        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

		if (timeOver) break;

        if(score > best) best = score;

        if(score > alpha){
            //store the principal variation
			sd.PV[ss->ply] = newMove;

            if(score > beta){
				hashFlag = TT_BETA;
				break;
            }
			
            alpha = score;
			hashFlag = TT_ALPHA;
        }

    }

	//save info and move to TT
	TT.save(sd.PV[ss->ply], zobrist.zobristKey, depth, valueToTT(alpha, ss->ply), hashFlag);

    return alpha;
}

int Ai_Logic::alphaBeta(int depth, int alpha, int beta, searchStack *ss, bool isWhite, bool allowNull, bool is_pv)
{
    bool FlagInCheck = false;
    bool raisedAlpha = false; 
	bool futileMoves = false; //have any moves been futile?
	bool singularExtension = false;
	int R = 2;
	int reductionDepth; //how much are we reducing depth in LMR?
	int newDepth;
    //U8 newDepth; //use with futility + other pruning later
    int queitSD = 25, f_prune = 0;
    //int  mateValue = INF - ply; // used for mate distance pruning
	sd.nodes++;
	ss->ply = (ss - 1)->ply + 1; //increment ply
	ss->reduction = 0;
	(ss + 1)->skipNull = false;
	//(ss+1)->excludedMove = false; //dont need?

	//checks if time over is true everytime if( nodes & 4095 ) ///NEED METHOD THAT CHECKS MUCH LESS
	checkInput();

	//mate distance pruning, prevents looking for mates longer than one we've already found
	// NEED to add is draw detection
	alpha = std::max(mated_in(ss->ply), alpha);
	beta = std::min(mate_in(ss->ply+1), beta);
	if (alpha >= beta) return alpha;

	const HashEntry *ttentry;
	bool ttMove;
	int ttValue;

	ttentry = TT.probe(zobrist.zobristKey);
	ttMove = ttentry ? ttentry->move.flag : false; //is there a move stored in transposition table?
	ttValue = ttentry ? valueFromTT(ttentry->eval, ss->ply) : INVALID; //if there is a TT entry, grab its value

	//is the a TT entry? If so, are we in PV? If in PV only accept and exact entry with a depth >= our depth, 
	//accept all if the entry has a equal or greater depth compared to our depth.
	if (ttentry
		&& ttentry->depth >= depth
		&& (is_pv ? ttentry->flag == TT_EXACT
			: ttValue >= beta ? ttentry->flag == TT_BETA
			: ttentry->flag == TT_ALPHA)
		) {

		return ttValue;
	}


    int score;
    if(depth < 1 || timeOver){
        //run capture search to max depth of queitSD
        score = quiescent(alpha, beta, isWhite, ss, queitSD, is_pv);
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

//if in check, or in reduced search extension, skip nulls, statics evals, razoring, etc to moves_loop:
    if(FlagInCheck || sd.excludedMove || sd.skipEarlyPruning) goto moves_loop;

	evaluateBB eval;
	ss->staticEval = eval.evalBoard(isWhite, newBoard, zobrist); //newBoard.sideMaterial[isWhite] - newBoard.sideMaterial[!isWhite];
//eval pruning / static null move
    if(depth < 3 && !is_pv && abs(beta - 1) > -INF + 100){
		//evaluateBB eval;
		//int static_eval = eval.evalBoard(isWhite, newBoard, zobrist);
  
        int eval_margin = 120 * depth;
        if(ss->staticEval - eval_margin >= beta){
            return ss->staticEval - eval_margin;
        }
    }

//Null move heuristics, disabled if in PV, check, or depth is too low
    if(allowNull && !is_pv && depth > R){
        if(depth > 6) R = 3;
        zobrist.UpdateColor();

        score = -alphaBeta(depth -R -1, -beta, -beta +1, ss+1, !isWhite, NO_NULL, NO_PV);
        zobrist.UpdateColor();
        //if after getting a free move the score is too good, prune this branch
        if(score >= beta){
            return score;
        }
    }


//razoring if not PV and is close to leaf and has a low score drop directly into quiescence
    if(!is_pv && allowNull && depth <= 3){
        //evaluateBB eval;
        int threshold = alpha - 300 - (depth - 1) * 60;        
        //if(eval.evalBoard(isWhite, newBoard, zobrist) < threshold){
		if(ss->staticEval < threshold){

            score = quiescent(alpha, beta, isWhite, ss, queitSD, is_pv);

            if(score < threshold) return alpha;
        }
    }

	///* //Still a bit slow?
//do we want to futility prune?
	int fmargin[4] = { 0, 200, 300, 500 };
    //int fmargin[8] = {0, 100, 150, 200, 250, 300, 400, 500};
    if(depth < 4 && !is_pv
		&& abs(alpha) < 9000
		&& ss->staticEval + fmargin[depth] <= alpha){
        f_prune = 1;
    }
	//*/
///*  //Internal iterative deepening search same ply to a shallow depth..
	//and see if we can get a TT entry to speed up search
	if (depth >= 6 && !ttMove
		&& (is_pv || ss->staticEval + 100 >= beta)) {
		int d = (int)(3 * depth / 4 - 2);
		sd.skipEarlyPruning = true;
		alphaBeta(d, alpha, beta, ss, isWhite, NO_NULL, is_pv);
		sd.skipEarlyPruning = false;

		ttentry = TT.probe(zobrist.zobristKey);
		
	}

//*/
moves_loop: //jump to here if in check or in a search extension or skip early pruning is true
/*
	singularExtension = depth >= 7
		&& !sd.excludedMove
		&& ttMove && ttValue != INVALID
		&& ttentry->flag == TT_BETA
		&& ttentry->depth >= depth - 3;
*/

//generate psuedo legal moves (not just captures)
    gen_moves.generatePsMoves(false);

    //add killers scores and hash moves scores to moves if there are any
    gen_moves.reorderMoves(ss, ttentry);

    int hashFlag = TT_ALPHA, movesNum = gen_moves.moveCount, legalMoves = 0;

	bool improving = ss->staticEval >= (ss - 2)->staticEval
		|| ss->staticEval == 0
		|| (ss - 2)->staticEval == 0;

    Move hashMove; //move to store alpha in and pass to TTable
    for(int i = 0; i < movesNum; ++i){
        //grab best scoring move
		Move newMove = gen_moves.movegen_sort(ss->ply, &gen_moves.moveAr[0]); //huge speed decrease if not Move newMove is instatiated above loop!!??

		//if (sd.excludedMove && newMove.score >= SORT_HASH) continue;

		//don't search moves with negative SEE at low depths
		if (depth < 4 && gen_moves.SEE(newMove, newBoard, isWhite) < 0) continue; //NEED to test how useful this is

        //make move on BB's store data to string so move can be undone
        newBoard.makeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        //is move legal? if not skip it
        if(gen_moves.isAttacked(king, isWhite, true)){ ///CHANGE METHOD OF UPDATING BOARDS, TOO INEFFICIENT
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }
        legalMoves ++;
        reductionDepth = 0;
        newDepth = depth - 1;
        h.cutoffs[isWhite][newMove.from][newMove.to] -= 1;
/*
		if (singularExtension && newMove.score >= SORT_HASH) {
			int rBeta = std::max(ttValue - 2 * depth, -mateValue);
			int d = depth / 2;
			sd.excludedMove = true;
			int s = -alphaBeta(d, rBeta - 1, rBeta, isWhite, ply, DO_NULL, NO_PV);
			sd.excludedMove = false;
			if (s < rBeta) {
				newDepth++;
			}
		}		
*/
		///*		
        //futility pruning ~~ is not a promotion or hashmove, is not a capture, and does not give check, and we've tried one move already
        if(f_prune && newMove.score < SORT_HASH
            && newMove.captured == PIECE_EMPTY && legalMoves
            && !gen_moves.isAttacked(eking, !isWhite, true)
			&& newMove.flag != 'Q'){

            newBoard.unmakeMove(newMove, zobrist, isWhite);
			gen_moves.grab_boards(newBoard, isWhite);
			futileMoves = true; //flag so we know we skipped a move/not checkmate
			futileC++;
            continue;
        }
        //*/
		/* //new futility pruning
		if (depth < 10
			&& newMove.score < SORT_HASH
			&& newMove.captured == PIECE_EMPTY && legalMoves
			&& !gen_moves.isAttacked(eking, !isWhite, true)
			&& newMove.flag != 'Q'
			&& i >= futileMoveCounts[improving][depth]) {

			newBoard.unmakeMove(newMove, zobrist, isWhite);
			gen_moves.grab_boards(newBoard, isWhite);
			futileMoves = true; //flag so we know we skipped a move/not checkmate
			futileC++;
			continue;
		}
		*/

        //late move reduction
        if(newDepth > 3
            && legalMoves > 3
			&& !FlagInCheck
			&& h.cutoffs[isWhite][newMove.from][newMove.to] < 50
			&& newMove.captured == PIECE_EMPTY && newMove.flag != 'Q'            
            && (newMove.from != ss->killers[0].from || newMove.to != ss->killers[0].to)
            && (newMove.from != ss->killers[1].from || newMove.to != ss->killers[1].to)
			&& !gen_moves.isAttacked(eking, !isWhite, true)
			&& newMove.score < SORT_HASH){


            h.cutoffs[isWhite][newMove.from][newMove.to] = 50;

			ss->reduction = reductions[is_pv][improving][depth][i]; //i = number of moves

        }

		newDepth -= ss->reduction;

		//load the (most likely) next entry in the TTable into cache near cpu
		_mm_prefetch((char *)TT.first_entry(zobrist.fetchKey(newMove, !isWhite)), _MM_HINT_NTA);

//jump back here if our LMR raises Alpha
re_search:
		

        if(!raisedAlpha){
            //we're in princiapl variation search or full window search
            score = -alphaBeta(newDepth, -beta, -alpha, ss + 1, !isWhite, DO_NULL, is_pv);
        } else {
            //zero window search
            score = -alphaBeta(newDepth, -alpha -1, -alpha, ss + 1, !isWhite, DO_NULL, NO_PV);
            //if our zero window search failed, do a full window search
            if(score > alpha){
                //PV search after failed zero window
                score = -alphaBeta(newDepth, -beta, -alpha, ss + 1, !isWhite, DO_NULL, IS_PV);
            }
        }

        //if a reduced search brings us above alpha, do a full non-reduced search
        if(reductionDepth && score > alpha){
            newDepth += reductionDepth;
			reductionDepth = 0;
			//if(reductionDepth > 2) reductionDepth = 1; //test!!!
            
            goto re_search;
        }


        //undo move on BB's
        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

		if (timeOver) return 0;

        if(score > alpha){            
            h.cutoffs[isWhite][newMove.from][newMove.to] += 6;
			//store the principal variation
			sd.PV[ss->ply] = newMove;
			hashMove = newMove;

            //if move causes a beta cutoff stop searching current branch
            if(score >= beta){

                if(newMove.captured == PIECE_EMPTY && newMove.flag != 'Q'){
                    //add move to killers
                    addKiller(newMove, ss);

                    //add score to historys
                    h.history[isWhite][newMove.from][newMove.to] += depth * depth;
                    //don't want historys to overflow if search is really big
                    if(h.history[isWhite][newMove.from][newMove.to] > SORT_KILL){
                        for(int i = 0; i < 64; i++){
                            for(int i = 0; i < 64; i++){
                                h.history[isWhite][newMove.from][newMove.to] /= 2;
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
        if(FlagInCheck) alpha = mated_in(ss->ply);
        else alpha = contempt(isWhite); 
    }


	if (futileMoves && !raisedAlpha && hashFlag != TT_BETA) {

		if(!legalMoves) alpha = ss->staticEval; //testing needed as well
		hashFlag = TT_EXACT; //NEED TO TEST
	}

	if (!ss->excludedMove) { //only write to TTable if we're not in partial search
		//save info + move to transposition table 
		TT.save(hashMove, zobrist.zobristKey, depth, valueToTT(alpha, ss->ply), hashFlag);
	}


    return alpha;
}

int Ai_Logic::quiescent(int alpha, int beta, bool isWhite, searchStack *ss, int quietDepth, bool is_pv)
{
    //node count, sepperate q count needed?
	sd.nodes++;

	const HashEntry *ttentry;
	bool ttMove;
	int ttValue;
	int oldAlpha, bestScore, score;
	ss->ply = (ss - 1)->ply + 1;

	//if (is_pv) oldAlpha = alpha; //??Need?

	ttentry = TT.probe(zobrist.zobristKey);
	ttMove = ttentry ? ttentry->move.flag : false; //is there a move stored in transposition table?
	ttValue = ttentry ? valueFromTT(ttentry->eval, ss->ply) : INVALID; //if there is a TT entry, grab its value

/*
	if (ttentry
		&& ttentry->depth >= DEPTH_QS
		&& (is_pv ? ttentry->flag == TT_EXACT
			: ttValue >= beta ? ttentry->flag == TT_BETA
			: ttentry->flag == TT_ALPHA)
		) {

		return ttValue;
	}
*/
    evaluateBB eval;
    //evaluate board position
    int standingPat = eval.evalBoard(isWhite, newBoard, zobrist);

    if(quietDepth <= 0 || timeOver){		
        return standingPat;
    }

    if(standingPat >= beta){

		//if (!ttentry) TT.save(zobrist.zobristKey, DEPTH_QS, valueToTT(standingPat, ply), TT_BETA); //save to TT if there's no entry MAJOR PROBLEMS WITH THIS 
		return beta;
    }

    if(alpha < standingPat){
       alpha = standingPat;
    }

    //generate only captures with true capture gen var
    MoveGen gen_moves;
    gen_moves.grab_boards(newBoard, isWhite);
    gen_moves.generatePsMoves(true);
    gen_moves.reorderMoves(ss, ttentry);

    //set hash flag equal to alpha Flag
    int hashFlag = TT_ALPHA, moveNum = gen_moves.moveCount;


    U64 king;
    Move hashMove;
    if(isWhite) king = newBoard.BBWhiteKing;
    else king = newBoard.BBBlackKing;

    for(int i = 0; i < moveNum; ++i)
    {        
        Move newMove = gen_moves.movegen_sort(ss->ply, &gen_moves.moveAr[0]);

		///*
		//also not fast enough yet, though more testing is needed
		//delta pruning
		if ((standingPat + SORT_VALUE[newMove.captured] + 200 < alpha)
			&& (newBoard.sideMaterial[!isWhite] - SORT_VALUE[newMove.captured] > END_GAME_MAT)
			&& newMove.flag != 'Q') continue;
			
		//Don't search losing capture moves if not in PV
		if (!is_pv && gen_moves.SEE(newMove, newBoard, isWhite) < 0) continue;

        newBoard.makeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        //is move legal? if not skip it ~~~~~~~~ possibly remove check later?
        if(gen_moves.isAttacked(king, isWhite, true)){
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }


        score = -quiescent(-beta, -alpha, !isWhite, ss, quietDepth-1, is_pv);

        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        if(score > alpha){

            if(score >= beta){
				hashFlag = TT_BETA;
				hashMove = newMove;
				alpha = beta;
				break;
            }

			hashFlag = TT_EXACT;
            alpha = score;
			hashMove = newMove;
        }

    }

	//TT.save(hashMove, zobrist.zobristKey, DEPTH_QS, valueToTT(alpha, ply), hashFlag);

    return alpha;
}

int Ai_Logic::contempt(bool isWhite)
{
	//used to make engine prefer checkmate over stalemate, escpecially if not in end game
    //NEED TO TEST VARIABLES MIGHT NOT BE ACCURATE

	int value = DRAW_OPENING;

	//if my sides material is below endgame mat, use DRAW_ENDGAME val sideMat[0] is white
	if (newBoard.sideMaterial[!isWhite] < END_GAME_MAT) {
		value = DRAW_ENDGAME;
	}

	if (isWhite) return value;
	else return -value;

	return 0;
}

bool Ai_Logic::isRepetition(const Move& m)
{
	if (m.piece == PAWN) return false;
	else if (m.flag == 'Q') return false;
	//add in castling logic to quit early

	int repCount = 0;

	for (int i = 0; i < h.twoFoldRep.size(); ++i) {
		if (zobrist.zobristKey == h.twoFoldRep[i]) repCount++;

	}

	if (repCount >= 2) {
		return true;
	}

	return false;
}

void Ai_Logic::addKiller(Move move, searchStack *ss)
{
    if(move.captured == PIECE_EMPTY){ //if move isn't a capture save it as a killer
        //make sure killer is different
        if(move.from != ss->killers[0].from
          && move.to != ss->killers[0].to){
            //save primary killer to secondary slot
			ss->killers[1] = ss->killers[0];
        }
        //save primary killer
		ss->killers[0] = move;
    }
}

void Ai_Logic::ageHistorys()
{
    //used to decrease value after a search
    for (int cl = 0; cl < 2; cl++)
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 64; j++) {
                h.history[cl][i][j] = h.history[cl][i][j] / 8;
                h.cutoffs[cl][i][j] = 100;
            }
	sd.nodes = 0;
}


int valueToTT(int val, int ply)
{
	//for adjusting value from "plys to mate from root", to "ply to mate from current ply". - taken from stockfish
	//if value is is a mate value or a mated value, adjust those values to the current ply then return
	//else return the value
	return val >= VALUE_MATE_IN_MAX_PLY ? val + ply
		: val <= VALUE_MATED_IN_MAX_PLY ? val - ply : val;
}

int valueFromTT(int val, int ply)
{
	//does the opposite of valueToTT, adjusts mate score from TTable to the appropriate score and the current ply
	return  val == 0 ? 0
		: val >= VALUE_MATE_IN_MAX_PLY ? val - ply
		: val <= VALUE_MATED_IN_MAX_PLY ? val + ply : val;
}

void Ai_Logic::clearHistorys()
{
	//used to decrease value after a search
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 64; i++)
			for (int j = 0; j < 64; j++) {
				h.history[cl][i][j] = h.history[cl][i][j] = 0;
				h.cutoffs[cl][i][j] = 100;
			}
	sd.nodes = 0;
}

void Ai_Logic::checkInput()
{
	//test for a condition that does less checks
	//way too many atm
	if (!timeOver && (sd.nodes & 4095)) {
		timeOver = timeM.timeStopSearch();

	}
}

void Ai_Logic::print(bool isWhite, int bestScore)
{
	std::stringstream ss;
	
	ss << "info depth " << sd.depth << " nodes " << sd.nodes << " nps " << timeM.getNPS() << " score cp " << bestScore;
/*
	if (sd.depth == 1) { //only eval board once a turn
		evaluateBB ev;
		ss << " score cp " << ev.evalBoard(isWhite, newBoard, zobrist);
	}
*/
	std::cout << ss.str() << std::endl;
}

