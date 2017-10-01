#include "ai_logic.h"

#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>

#include "externs.h"
#include "Evaluate.h"
#include "bitboards.h"
#include "hashentry.h"
#include "TimeManager.h"
#include "TranspositionT.h"
#include "MovePicker.h"


//#include <advisor-annotate.h> // INTEL ADVISOR FOR THREAD ANNOTATION

#define DO_NULL    true //allow null moves or not
#define NO_NULL    false
#define IS_PV      1 //is this search in a principal variation
#define NO_PV      0
#define ASP        50  //aspiration windows size


//holds historys and killers + eventually nodes searched + other data
searchDriver sd;

//value to determine if time for search has run out
bool timeOver;

//master time manager
TimeManager timeM;

//mate values are measured in distance from root, so need to be converted to and from TT
inline int valueFromTT(int val, int ply); 
inline int valueToTT(int val, int ply); 

//reduction tables: pv, is node improving?, depth, move number
int reductions[2][2][64][64];
//futile move count arrays
int futileMoveCounts[2][32];

//holds history and cutoff info for search, global
Historys history;


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

Move Ai_Logic::searchStart(BitBoards& board, bool isWhite) {
	
	//max depth
	int depth = MAX_PLY;

	//if we're only allowed to search to this depth,
	//only search to specified depth.
	if (fixedDepthSearch) depth = fixedDepthSearch;

	//decrease history values and clear gains stats after a search
	ageHistorys();

	//calc move time for us, send to search driver
	timeM.calcMoveTime(isWhite);

	Move m = iterativeDeep(board, depth, isWhite);
	
	return m;
}

Move Ai_Logic::iterativeDeep(BitBoards& board, int depth, bool isWhite)
{
	//reset ply
    int ply = 0;
	int color = !isWhite;

	//create array of stack objects starting at +2 so we can look two plys back
	//in order to see if a node has improved, as well as prior stats
	searchStack stack[MAX_PLY + 6], *ss = stack + 2;
	std::memset(ss - 2, 0, 5 * sizeof(searchStack));

    //best overall move as calced
    Move bestMove;
    int bestScore, alpha = -INF, beta = INF;
	int delta = 8;
	sd.depth = 1;

    //iterative deepening loop starts at depth 1, iterates up till max depth or time cutoff
    while(sd.depth <= depth){

		if (timeM.timeStopRoot() || timeOver) break;

        //main search
        bestScore = searchRoot(board, sd.depth, alpha, beta, ss);
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
            if(sd.PV[1] != MOVE_NONE) bestMove = sd.PV[1];

			//insert_pv(board);

			//print data on search 
			print(isWhite, bestScore);
        }
		//increment depth 
		sd.depth++;    
    }

	StateInfo st;
    //make final move on bitboards + draw board
    board.makeMove(bestMove, st, color);
    board.drawBBA();
	//std::cout << board.sideMaterial[0] << " " << board.sideMaterial[1] <<std::endl;

    //evaluateBB ev; //used for prininting static eval after move
    //std::cout << "Board evalutes to: " << ev.evalBoard(true, board, zobrist) << " for white." << std::endl;

    return bestMove;
}

int Ai_Logic::searchRoot(BitBoards& board, int depth, int alpha, int beta, searchStack *ss)
{
    bool flagInCheck;
    int score = 0;
    int best = -INF;
    int legalMoves = 0;
	int hashFlag = TT_ALPHA;
	Move quiets[64];
	int quietsCount = 0;
	StateInfo st;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;

	const HashEntry* ttentry = TT.probe(board.TTKey());

	sd.nodes++;
	ss->ply = 1;
	(ss + 1)->skipNull = false;
	const int color = board.stm();

    // Are we in check?	
	flagInCheck = board.checkers();


	MovePicker mp(board, MOVE_NONE, depth, history, ss); //CHANGE MOVE NONE TO TTMOVE ONCE IMPLEMENTED

	Move newMove;

    while((newMove = mp.nextMove()) != MOVE_NONE){
        //find best move generated

        board.makeMove(newMove, st, color);  

		/*
		if (isRepetition(board, newMove)) { //if position from root move is a two fold repetition, discard that move
			board.unmakeMove(newMove, color);							 ////////////////////////////////////////////////////////////////////////////////////////////////////////////REENABLE ONCE EVERYTHING IS WORKING AND CONVERT TO NEW MOVE STANDARD
			continue;
		}	
		*/

		if (!board.isLegal(newMove, color)) {
			board.unmakeMove(newMove, color);
			continue;
		}
        legalMoves ++;

        //PV search at root
        if(best == -INF){
            //full window PV search
            score = -alphaBeta(board, depth-1, -beta, -alpha, ss + 1, DO_NULL, IS_PV);

       } else {
            //zero window search
            score = -alphaBeta(board, depth-1, -alpha -1, -alpha, ss + 1, DO_NULL, NO_PV);

            //if we've gained a new alpha we need to do a full window search
            if(score > alpha){
                score = -alphaBeta(board, depth-1, -beta, -alpha, ss + 1, DO_NULL, IS_PV);
            }
        }

        //undo move on BB's
        board.unmakeMove(newMove, color);

		if (board.pieceOnSq(to_sq(newMove)) == PIECE_EMPTY 
			&& move_type(newMove) != PROMOTION 
			&& quietsCount < 64) {

			quiets[quietsCount] = newMove;         
			quietsCount++;
		}

		if (timeOver) break;

        if(score > best) best = score;

        if(score > alpha){
            //store the principal variation
			sd.PV[ss->ply] = newMove;

            if(score > beta){
				hashFlag = TT_BETA;
				alpha = beta;
				break;
            }
			
            alpha = score;
			hashFlag = TT_ALPHA;
        }

    }

	//save info and move to TT
	//TT.save(sd.PV[ss->ply], board.TTKey(), depth, valueToTT(alpha, ss->ply), hashFlag);

	
	if (alpha >= beta && !flagInCheck && !board.capture_or_promotion(sd.PV[1])) {
		updateStats(sd.PV[1], ss, depth, quiets, quietsCount, color);
	}
		
    return alpha;
}

int Ai_Logic::alphaBeta(BitBoards& board, int depth, int alpha, int beta, searchStack *ss, bool allowNull, int isPV)
{
	bool FlagInCheck, raisedAlpha, doFullDepthSearch, futileMoves;
	bool singularExtension, captureOrPromotion, givesCheck;

	FlagInCheck = raisedAlpha = doFullDepthSearch = futileMoves = false;
	singularExtension = captureOrPromotion = givesCheck = false;

	(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;

	//holds state of board on current ply info
	StateInfo st;

	int R = 2, newDepth, predictedDepth = 0, f_prune = 0, quietsC = 0;

	Move quiets[64]; //container holding quiet moves so we can reduce their score 

	sd.nodes++;
	ss->ply = (ss - 1)->ply + 1; //increment ply
	ss->reduction = 0;
	ss->excludedMove = MOVE_NONE;
	(ss + 1)->skipNull = false;

	const int color = board.stm();

	//checks if time over is true everytime if( nodes & 4095 ) ///NEED METHOD THAT CHECKS MUCH LESS
	checkInput();

	//mate distance pruning, prevents looking for mates longer than one we've already found
	// NEED to add is draw detection
	alpha = std::max(mated_in(ss->ply), alpha);
	beta = std::min(mate_in(ss->ply + 1), beta);
	if (alpha >= beta) return alpha;

	const HashEntry *ttentry;
	Move ttMove;
	int ttValue;

	ttentry = TT.probe(board.TTKey());
	ttMove  = ttentry ? ttentry->move : MOVE_NONE; //is there a move stored in transposition table?
	ttValue = ttentry ? valueFromTT(ttentry->eval, ss->ply) : INVALID; //if there is a TT entry, grab its value


	//is the a TT entry? If so, are we in PV? If in PV only accept and exact entry with a depth >= our depth, 
	//accept all if the entry has a equal or greater depth compared to our depth.
	if (ttentry
		&& ttentry->depth >= depth
		&& (isPV ? ttentry->flag == TT_EXACT
			: ttValue >= beta ? ttentry->flag == TT_BETA
			: ttentry->flag == TT_ALPHA)
		) {

		return ttValue;
	}

	int score;
	if (depth < 1 || timeOver) {
		//run capture search to max depth of queitSD
		score = quiescent(board, alpha, beta, ss, isPV);
		return score;
	}

	//are we in check?
	FlagInCheck = board.isSquareAttacked(board.king_square(color), color);

	//if in check, or in reduced search extension: skip nulls, statics evals, razoring, etc to moves_loop:
	if (FlagInCheck || (ss - 1)->excludedMove || sd.skipEarlyPruning) goto moves_loop;

	//evaluateBB eval;
	Evaluate eval;
	ss->staticEval = eval.evaluate(board);

	/*
	//update gain from previous ply stats for previous move
	if ((ss - 1)->currentMove.captured == PIECE_EMPTY
		&& (ss - 1)->currentMove.flag != 'Q'
		&& ss->staticEval != 0 && (ss - 1)->staticEval != 0
		&& (ss - 1)->currentMove.tried) {

		history.updateGain((ss - 1)->currentMove, -(ss - 1)->staticEval - ss->staticEval, !color);          //RE ENABLE ONCE DONE EP TESTING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}
	*/


	//eval pruning / static null move
	if (depth < 3 && !isPV && abs(beta - 1) > -INF + 100) {

		int eval_margin = 120 * depth;
		if (ss->staticEval - eval_margin >= beta) {
			return ss->staticEval - eval_margin;
		}
	}

	//Null move heuristics, disabled if in PV, check, or depth is too low //RE ENABLE ONCE DONE EP TESTING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
///*
	if (allowNull && !isPV && depth > R) {
		if (depth > 6) R = 3;

		board.makeNullMove(st);
		score = -alphaBeta(board, depth - R - 1, -beta, -beta + 1, ss + 1, NO_NULL, NO_PV);
		board.undoNullMove();

		//if after getting a free move the score is too good, prune this branch
		if (score >= beta) {
			return score;
		}
	}

	if (!isPV ///TESTTESTTESTTEST
		&& depth < 4
		&& ss->staticEval + 300 * (depth - 1) * 60 <= alpha
		&& !ttMove
		&& !board.pawnOn7th(color)) { //need to add no pawn on 7th rank

		if (depth <= 1 && ss->staticEval + 300 * 3 * 60 <= alpha) {
			return quiescent(board, alpha, beta, ss, NO_PV);
		}

		int ralpha = alpha - 300 * (depth - 1) * 60;

		int val = quiescent(board, ralpha, ralpha + 1, ss, NO_PV);

		if (val <= alpha) return val;
	}

	/* //Still a bit slow?
//do we want to futility prune?
	int fmargin[4] = { 0, 200, 300, 500 };
	//int fmargin[8] = {0, 100, 150, 200, 250, 300, 400, 500};
	if (depth < 4 && !isPV
		&& abs(alpha) < 9000
		&& ss->staticEval + fmargin[depth] <= alpha) {
		f_prune = 1;
	}
	//*/

///*  //Internal iterative deepening search same ply to a shallow depth..
	//and see if we can get a TT entry to speed up search

	if (depth >= 6 && !ttMove
		&& (isPV || ss->staticEval + 100 >= beta)) {
		int d = (int)(3 * depth / 4 - 2);
		sd.skipEarlyPruning = true;

		alphaBeta(board, d, alpha, beta, ss, NO_NULL, isPV);
		sd.skipEarlyPruning = false;

		ttentry = TT.probe(board.TTKey());
	}

	//*/
moves_loop: //jump to here if in check or in a search extension or skip early pruning is true
/*
	singularExtension = depth >= 8
		&& !sd.excludedMove
		&& ttMove && ttValue != INVALID
		&& ttentry->flag == TT_BETA
		&& ttentry->depth >= depth - 3;
*/

	//has this current node variation improved our static_eval ?
	bool improving =        ss->staticEval >= (ss - 2)->staticEval
		           ||       ss->staticEval == 0
		           || (ss - 2)->staticEval == 0;
	
	int hashFlag = TT_ALPHA, legalMoves = 0, bestScore = -INF;

	CheckInfo ci(board);

	Move newMove, bestMove = MOVE_NONE;
	MovePicker mp(board, MOVE_NONE, depth, history, ss); //CHANGE MOVE NONE TO TTMOVE ONCE IMPLEMENTED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	while((newMove = mp.nextMove()) != MOVE_NONE){

		//if (sd.excludedMove && newMove.score >= SORT_HASH) continue;

		captureOrPromotion = board.capture_or_promotion(newMove);

		//make move on BB's store data to string so move can be undone
		board.makeMove(newMove, st, color);

		//is move legal? if not skip it
		if (!board.isLegal(newMove, color)) {
			board.unmakeMove(newMove, color);
			continue;
		}

		legalMoves++;
		newDepth   = depth - 1;
		givesCheck = st.checkers;

		/* //ELO loss with singular extensions so far
		if (singularExtension && newMove.score >= SORT_HASH) {
			int rBeta = std::max(ttValue - 2 * depth, -mateValue);
			int d = depth / 2;
			sd.excludedMove = true;
			int s = -alphaBeta(d, rBeta - 1, rBeta, ply, DO_NULL, NO_PV);
			sd.excludedMove = false;
			if (s < rBeta) {
				newDepth++;
			}
		}
		*/
		/*
		//new futility pruning really looks like it's pruning way to much right now, maybe adjust futile move counts until we have better move ordering!!!!
		if (!isPV
			&& newMove.score <= SORT_HASH
			&& !captureOrPromotion
			&& !FlagInCheck
			&& !givesCheck
			&& !board.isPawnPush(newMove, color)
			&& bestScore > VALUE_MATED_IN_MAX_PLY) {

			bool shouldSkip = false;

			if (depth < 10 && legalMoves >= futileMoveCounts[improving][depth]+7) {
				shouldSkip = true;
			}

			predictedDepth = newDepth - reductions[isPV][improving][depth][legalMoves];

			int futileVal;
			if (!shouldSkip && predictedDepth < 7) {
				//int a = history.gains[color][newMove.piece][newMove.to];
				//if (predictedDepth < 0) predictedDepth = 0;
				//use predicted depth? Need to play with numbers!!
				futileVal = ss->staticEval + history.gains[color][newMove.piece][newMove.to] + (predictedDepth * 200) + 140;

				if (futileVal <= alpha) {
					//bestScore = std::max(futileVal, bestScore);
					shouldSkip = true;
				}
			}

			//don't search moves with negative SEE at low depths
			//if (!shouldSkip && depth < 4 && gen_moves.SEE(newMove, board, color, true) < 0) shouldSkip = true;

			if (shouldSkip) {
				board.unmakeMove(newMove, color);
				futileMoves = true; //flag so we know we skipped a move/not checkmate
				continue;
			}
		}
		//*/

		/* //This futility pruning works in conjuction with futile conditions above move loop, slight speed boost, unsure on ELO gain
		//futility pruning ~~ is not a promotion or hashmove, is not a capture, and does not give check, and we've tried one move already
		if(f_prune && newMove.score < SORT_HASH
			&& !captureOrPromotion && legalMoves
			&& !givesCheck){

			board.unmakeMove(newMove, color);
			futileMoves = true; //flag so we know we skipped a move/not checkmate
			continue;
		}
		//*/
///*
		//late move reductions, reduce the depth of the search in non dangerous situations. 
		if (newDepth > 3
			&& legalMoves > 3
			&& !FlagInCheck
			&& !captureOrPromotion
			&& !givesCheck	
			&& newMove != ttMove
			&& newMove != ss->killers[0]
			&& newMove != ss->killers[1]) {


			ss->reduction = reductions[isPV][improving][depth][legalMoves];

			//reduce reductions for moves that escape capture
			if (ss->reduction
				&& move_type(newMove) == NORMAL
				&& board.pieceOnSq(to_sq(newMove)) != PAWN
				&& board.SEE(create_move(to_sq(newMove), from_sq(newMove)), color, false) < 0){ //////////////////////////////////////////////////////////////////////////TEST THIS TO TRY AND MAKE SURE SEE IS WORKING CORRECTLY HERE!!!!

					ss->reduction = std::max(1, ss->reduction - 2); //play with reduction value
			}

			int d1 =  std::max(newDepth - ss->reduction, 1);

			int val = -alphaBeta(board, d1, -(alpha + 1), -alpha, ss + 1, DO_NULL, NO_PV);

			//if reduction is very high, and we fail high above, re-search with a lower reduction
			if (val > alpha && ss->reduction >= 4) {

				int d2 = std::max(newDepth - 2, 1);
				val = -alphaBeta(board, d2, -(alpha + 1), -alpha, ss + 1, DO_NULL, NO_PV);
			}
			//if we still fail high, do a full depth search
			if (val > alpha && ss->reduction != 0) {
				ss->reduction = 0;
			}
		}
//*/
		newDepth -= ss->reduction;

		ss->currentMove = newMove; 

		//jump back here if our LMR raises Alpha ~~ NOT IN USE
	//re_search:
		if (!raisedAlpha) {
			//we're in principal variation search or full window search

			score = -alphaBeta(board, newDepth, -beta, -alpha, ss + 1, DO_NULL, isPV);
		}
		else {
			//zero window search
			score = -alphaBeta(board, newDepth, -alpha - 1, -alpha, ss + 1, DO_NULL, NO_PV);
			//if our zero window search failed, do a full window search
			if (score > alpha) {
				//PV search after failed zero window
				score = -alphaBeta(board, newDepth, -beta, -alpha, ss + 1, DO_NULL, IS_PV);
			}
		}

		//store queit moves so we can decrease their value later, find a more effecient way or storing than a huge move array?, possibly 2D array by ply?
		if (!captureOrPromotion && quietsC < 64) {
			quiets[quietsC] = newMove;
			quietsC++;                                     //////////////////////////////////////////////////////////////////////////////////////////////////REENABLE ONCE EVERYTHING IS WOKRING
		}
		
		//undo move on BB's
		board.unmakeMove(newMove, color);

		if (timeOver) return 0;

		if (score > bestScore) {
			bestScore = score;

			if (score > alpha) {
				//store the principal variation
				sd.PV[ss->ply] = bestMove = newMove; //NEED TO DELETE AFTER A SEARCH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				//if move causes a beta cutoff stop searching current branch
				if (score >= beta) {
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
	}


	if (!legalMoves) {
		//if there are no legal moves and we are in checkmate this node
		if (FlagInCheck) alpha = mated_in(ss->ply);
		//else it's a stalemate, return contempt score - prefering mate to draw
		else alpha = contempt(board, color);

	}
	
	else if (alpha >= beta && !FlagInCheck && !board.capture_or_promotion(sd.PV[ss->ply])) { ///////////////////////////////////////////////////////////////////////TEST THAT CAPTURE_OR_PROMOTION IS WORKING APPROPRIATLY
		updateStats(sd.PV[ss->ply], ss, depth, quiets, quietsC, color);
	}
	

	if (futileMoves && !raisedAlpha && hashFlag != TT_BETA) {

		if (!legalMoves) alpha = ss->staticEval; //testing needed as well
		hashFlag = TT_EXACT; //NEED TO TEST
	}

	
	if (!ss->excludedMove) { //only write to TTable if we're not in partial search 
		//save info + move to transposition table 
		TT.save(bestMove, board.TTKey(), depth, valueToTT(alpha, ss->ply), hashFlag);
	}
	

	return alpha;
}

int Ai_Logic::quiescent(BitBoards& board, int alpha, int beta, searchStack *ss, int isPV)
{
	//node count, sepperate q count needed?
	sd.nodes++;
	const int color = board.stm();
	const HashEntry *ttentry;
	int ttValue;
	int oldAlpha, bestScore, score;
	ss->ply = (ss - 1)->ply + 1;
	StateInfo st;

	ttentry     = TT.probe(board.TTKey());
	Move ttMove = ttentry ? ttentry->move : MOVE_NONE; //is there a move stored in transposition table?
	ttValue     = ttentry ? valueFromTT(ttentry->eval, ss->ply) : INVALID; //if there is a TT entry, grab its value

	if (ttentry
		&& ttentry->depth >= DEPTH_QS
		&& (isPV ? ttentry->flag == TT_EXACT
			: ttValue >= beta ? ttentry->flag == TT_BETA
			: ttentry->flag == TT_ALPHA)
		) {

		return ttValue;
	}

	Evaluate eval;
	int standingPat = eval.evaluate(board);

	if (timeOver) {
		return standingPat;
	}

	if (standingPat >= beta) {

		if (!ttentry) TT.save(board.TTKey(), DEPTH_QS, valueToTT(standingPat, ss->ply), TT_BETA); //save to TT if there's no entry MAJOR PROBLEMS WITH THIS  ////////////////////////////////////////////////////////////REENABLE ONCE EVERYTHING IS WOKRING
		return beta;
	}

	if (alpha < standingPat) {
		alpha = standingPat;
	}

	MovePicker mp(board, ttMove, history, ss); //CHANGE MOVE NONE TO TTMOVE ONCE IMPLEMENTED

	//set hash flag equal to alpha Flag
	int hashFlag = TT_ALPHA;

	Move newMove, bestMove = MOVE_NONE;
	while((newMove = mp.nextMove()) != MOVE_NONE)
	{
		
		// If the move is gurenteed to be below alpha, don't search it
		// unless we're in endgame, where that's not safe ~~ possibly use more advanced board.game_phase()?
		if ((standingPat + SORT_VALUE[board.pieceOnSq(to_sq(newMove))] + 200 < alpha)
			&& (board.bInfo.sideMaterial[!color] - SORT_VALUE[board.pieceOnSq(to_sq(newMove))] > END_GAME_MAT)  //////////////////////////////////////////////////////////////////////////////////////////////////REENABLE ONCE EVERYTHING IS WOKRING
			&& move_type(newMove) != PROMOTION) continue;
		

		//Don't search losing capture moves if not in PV
		if (!isPV && board.SEE(newMove, color, true) < 0) continue;
		

		board.makeMove(newMove, st, color);

		if (!board.isLegal(newMove, color)) {
			board.unmakeMove(newMove, color);
			continue;
		}

		score = -quiescent(board, -beta, -alpha, ss, isPV);

		board.unmakeMove(newMove, color);

		if (score > alpha) {
			bestMove = newMove;

			if (score >= beta) {
				hashFlag = TT_BETA;
				alpha = beta;
				break;
			}

			hashFlag = TT_EXACT;
			alpha = score;
		}
	}

	TT.save(bestMove, board.TTKey(), DEPTH_QS, valueToTT(alpha, ss->ply), hashFlag);

	return alpha;
}

int Ai_Logic::contempt(const BitBoards& board, int color)
{
	//used to make engine prefer checkmate over stalemate, escpecially if not in end game
    //NEED TO TEST VARIABLES MIGHT NOT BE ACCURATE

	int value = DRAW_OPENING;

	//if my sides material is below endgame mat, use DRAW_ENDGAME val sideMat[0] is white
	if (board.bInfo.sideMaterial[color] < END_GAME_MAT) {
		value = DRAW_ENDGAME;
	}

	if (color == WHITE) return value;
	else return -value;

	return 0;
}

bool Ai_Logic::isRepetition(const BitBoards& board, const Move& m) //Ai_Logic::
{
	if (board.pieceOnSq(m) == PAWN) return false;
	//add in castling logic to quit early

	int repCount = 0;

	for (int i = 0; i < history.twoFoldRep.size(); ++i) {
		if (board.TTKey() == history.twoFoldRep[i]) repCount++;
	}

	if (repCount >= 2) {
		return true;
	}

	return false;
}

/*
void Ai_Logic::insert_pv(BitBoards & board)
{
	StateInfo state[MAX_PLY + 6], *st = state;
	const HashEntry * tte;
	int id = 1;

	do {
		tte = TT.probe(board.TTKey());

		if (!tte || tte->move != sd.PV[id])
			TT.save(sd.PV[id], board.TTKey(), 0, 0, TT_ALPHA); //////////////////////////////////////////////////////////////////////////////////////////////////REENABLE ONCE EVERYTHING IS WOKRING

		board.makeMove(sd.PV[id++], *st++, board.stm());

		board.drawBBA();

	} while (sd.PV[id].tried);

	while (id > 1) {
		board.drawBBA();
		board.unmakeMove(sd.PV[--id], !board.stm());
	}
	board.drawBBA();
}
*/

void Ai_Logic::updateStats(Move move, searchStack * ss, int depth, Move * quiets, int qCount, int color)
{	
	static const int limit = SORT_KILL;
	//update Killers for ply
	//make sure killer is different
	if (move != ss->killers[0]) {
		//save primary killer to secondary slot
		ss->killers[1] = ss->killers[0];
		//save primary killer
		ss->killers[0] = move;
	}

	//update historys, increasing the cutoffs score, decreasing every other moves score
	int val = 4 * depth * depth;
	history.updateHist(move, val, color); //CHange to index by ply? or index just by piece type and to location>? TEST

	for(int i = 0; i < qCount; ++i){
		history.updateHist(quiets[i], -val, color);
	}
}

inline int valueToTT(int val, int ply) 
{
	//for adjusting value from "plys to mate from root", to "ply to mate from current ply". - taken from stockfish
	//if value is is a mate value or a mated value, adjust those values to the current ply then return
	//else return the value
	return val >= VALUE_MATE_IN_MAX_PLY ? val + ply
		: val <= VALUE_MATED_IN_MAX_PLY ? val - ply : val;
}

inline int valueFromTT(int val, int ply) 
{
	//does the opposite of valueToTT, adjusts mate score from TTable to the appropriate score and the current ply
	return  val == 0 ? 0
		: val >= VALUE_MATE_IN_MAX_PLY ? val - ply
		: val <= VALUE_MATED_IN_MAX_PLY ? val + ply : val;
}

void Ai_Logic::ageHistorys()
{
	//used to decrease value after a search
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 64; i++)
			for (int j = 0; j < 64; j++) {
				history.history[cl][i][j] = history.history[cl][i][j] / 8;
				history.cutoffs[cl][i][j] = 100;
			}
	sd.nodes = 0;

	//clears gains stats
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 7; i++)
			for (int j = 0; j < 64; j++) {
				history.cutoffs[cl][i][j] = 0;
			}
}

void Ai_Logic::clearHistorys()
{
	//used to decrease value after a search
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 64; i++)
			for (int j = 0; j < 64; j++) {
				history.history[cl][i][j] = history.history[cl][i][j] = 0;
				history.cutoffs[cl][i][j] = 100;
			}
	sd.nodes = 0;

	//clears gains stats
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 7; i++)
			for (int j = 0; j < 64; j++) {
				history.cutoffs[cl][i][j] = 0;
			}
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
		ss << " score cp " << ev.evalBoard(isWhite, board, zobrist);
	}
*/
	std::cout << ss.str() << std::endl;
}



