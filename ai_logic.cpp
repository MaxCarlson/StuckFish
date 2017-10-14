#include "ai_logic.h"

#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>

#include "externs.h"
#include "Evaluate.h"
#include "bitboards.h"
#include "TimeManager.h"
#include "TranspositionT.h"
#include "movegen.h"
#include "Thread.h"


//#include <advisor-annotate.h> // INTEL ADVISOR FOR THREAD ANNOTATION

#define DO_NULL    true //allow null moves or not
#define NO_NULL    false
#define IS_PV      1 //is this search in a principal variation
#define NO_PV      0
#define ASP        50  //aspiration windows size

//master time manager
TimeManager timeM;

namespace Search {
	SearchControls SearchControl;
}

using namespace Search;

const int Tempo = 10;

enum NodeType {
	NonPv,
	PV
};

//mate values are measured in distance from root, so need to be converted to and from TT
inline int valueFromTT(int val, int ply);
inline int valueToTT(int val, int ply);

void update_pv(Move *pv, Move m, Move *childPv) {
	for (*pv++ = m; childPv && *childPv != MOVE_NONE; )
		*pv++ = *childPv++;
	*pv = MOVE_NONE;
}

template <bool isPV> int reduction(bool improving, int depth, int moveCount) {
	return reductions[isPV][improving][std::min(depth, 63)][std::min(moveCount, 63)];
}


//reduction tables: pv, is node improving?, depth, move number
int reductions[2][2][64][64];
//futile move count arrays
int futileMoveCounts[2][32];


int DrawValue[COLOR];

template<NodeType NT>
int alphaBeta(BitBoards& board, int depth, int alpha, int beta, Search::searchStack *ss, bool allowNull);

// Used for applying bonuses and penalties to
// Quiet moves, continuation historys, counter moves, etc. 
inline int stat_bonus(int depth) // This needs to be played with value wise against a different version, right now it hasn't been tested at all
{
	return depth > 14 ? 0 : depth * depth + 2 * depth - 2;
}

// Defined here for easy access to play testing values!
const int razor_margin[] = { 0, 275, 320, 257 };
int futile_margin(int depth) { return 85 * depth; }



void Search::initSearch()
{
	for (int imp = 0; imp <= 1; ++imp)
		for (int d = 1; d < 64; ++d)
			for (int mc = 1; mc < 64; ++mc)
			{
				double r = log(d) * log(mc) / 2.45; // Best value so far, 2.45!

				reductions[NonPv][imp][d][mc] = int(std::round(r));
				reductions[PV][imp][d][mc]    = std::max(reductions[NonPv][imp][d][mc] - 1, 0);

				// Increase reduction for non-PV nodes when eval is not improving
				if (!imp && reductions[NonPv][imp][d][mc] >= 2)
					reductions[NonPv][imp][d][mc]++;
			}

	for (int d = 0; d < 32; ++d)
	{
		futileMoveCounts[0][d] = int(2.4 + 0.222 * pow(d * 2 + 0.00, 1.8));
		futileMoveCounts[1][d] = int(3.0 + 0.300 * pow(d * 2 + 0.98, 1.8));
	}
}

void Search::clear()
{
	TT.clearTable();

	for (Thread * th : Threads)
		th->clear();

	Threads.main()->previousScore = INF;
}

Move MainThread::search() 
{
	const int color = board.stm();

	int contempt = SORT_VALUE[PAWN] / 100;
	DrawValue[ color] = VALUE_DRAW - contempt;
	DrawValue[!color] = VALUE_DRAW + contempt;


	//calc move time for us, send to search driver
	//timeM.calcMoveTime(color, SearchControl); 
	timeM.initTime(color, turns, SearchControl);

	Move m = Thread::search();

	previousScore = rootMoves[0].score;

	return m;
}

Move Thread::search() 
{
	//reset ply
	int ply   = 0;
	int color = board.stm();

	MainThread * mainThread = (this == Threads.main() ? Threads.main() : nullptr);

	//create array of stack objects starting at +2 so we can look two plys back
	//in order to see if a node has improved, as well as prior stats
	Search::searchStack stack[MAX_PLY + 6], *ss = stack + 4;
	std::memset(ss - 4, 0, 7 * sizeof(Search::searchStack));

	for (int i = 4; i > 0; i--)
		(ss - i)->contiHistory = &this->contiHistory[PIECE_EMPTY][0]; // Use this as a start location

	//best overall move as calced
	Move bestMove;
	int bestScore, alpha = -INF, beta = INF;

	int multiPV = rootMoves.size();

	//iterative deepening loop starts at depth 1, iterates up till max depth or time cutoff
	while (  rootDepth < MAX_PLY 
		&& ! Threads.stop
		&& !(SearchControl.depth && mainThread && rootDepth > SearchControl.depth)) 
	{

		// Save previous ID scores to rootMoves previous
		for (RootMove& rm : rootMoves)
			rm.previousScore = rm.score;

		// Age best move changes
		if (mainThread)
			mainThread->bestMoveChanges *= 0.505;
		
		// Sort so we can have gurenteed Pv move first.
		std::stable_sort(rootMoves.begin() , rootMoves.end());

		// Start search from root
		bestScore = searchRoot(board, rootDepth, alpha, beta, ss);

		if (Threads.stop)
			break;

		// Sort so we know what the PV of this iteration was
		std::stable_sort(rootMoves.begin(), rootMoves.end()); 


		// need stop if mate in X code here

		//if the search is not cutoff
		if (!Threads.stop) {

			bestMove = rootMoves[0].pv[0]; 

			//print data on search 
			print(bestScore, rootDepth);


			if (SearchControl.use_time())
			{

				int f1 = bestScore - mainThread->previousScore;

				int improvingFactor = std::max(229, std::min(715, 357 + 119 * -6 * f1));

				double unstablePv = 1 + mainThread->bestMoveChanges;

				if (timeM.elapsed() > timeM.optimum() * unstablePv * improvingFactor / 628)
					Threads.stop = true;
			}


		}
		//increment depth 
		rootDepth++;
	}

	StateInfo st;
	//make final move on bitboards + draw board
	board.makeMove(bestMove, st, color);
	board.drawBBA();

	return bestMove;
}

int Search::searchRoot(BitBoards& board, int depth, int alpha, int beta, searchStack *ss)
{
    bool flagInCheck;
    int score = 0;
    int best = -INF;
    int legalMoves = 0;
	int hashFlag = TT_ALPHA;
	Move pv[MAX_PLY + 1];
	Move quiets[64];
	int quietsCount;
	StateInfo st;
	Thread * thisThread = board.this_thread();

	if (thisThread == Threads.main())
		static_cast<MainThread *>(thisThread)->check_time();

	ss->moveCount = quietsCount = 0;
	ss->statScore = 0;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;
	ss->currentMove = ss->excludedMove = MOVE_NONE;
	ss->contiHistory = &thisThread->contiHistory[PIECE_EMPTY][0];

	const HashEntry* ttentry = TT.probe(board.TTKey());
	//Move ttMove = ttentry ? ttentry->move() : MOVE_NONE;
	Move ttMove = thisThread->rootMoves[0].pv[0];

	ss->ply = 1;
	const int color = board.stm();

    // Are we in check?	
	flagInCheck = board.checkers();

	CheckInfo ci(board);
	const PieceToHistory* contiHist[] = { (ss - 1)->contiHistory, (ss - 2)->contiHistory, nullptr, (ss - 4)->contiHistory };
	MovePicker mp(board, ttMove, depth, &thisThread->mainHistory, contiHist, MOVE_NONE, ss->killers); 

	Move newMove, bestMove = MOVE_NONE;

    while((newMove = mp.nextMove()) != MOVE_NONE){
	//for (RootMove& rm : thisThread->rootMoves){     // Will have to test and see if drawing from the root moves is faster or not!!
		//newMove = rm.pv[0];

		if (!board.isLegal(newMove, ci.pinned)) {
			continue;
		}

        board.makeMove(newMove, st, color);  

        ss->moveCount   = ++legalMoves;
		ss->currentMove = newMove;

        //PV search at root
        if(best == -INF)
		{
			(ss + 1)->pv = pv;
			(ss + 1)->pv[0] = MOVE_NONE;

            //full window PV search
            score = -alphaBeta<PV>(board, depth-1, -beta, -alpha, ss + 1, DO_NULL);

        } 
		else
		{
            //zero window search
            score = -alphaBeta<NonPv>(board, depth-1, -alpha -1, -alpha, ss + 1, DO_NULL);

            //if we've gained a new alpha we need to do a full window search
            if(score > alpha){
                score = -alphaBeta<PV>(board, depth-1, -beta, -alpha, ss + 1, DO_NULL);
            }
        }

        //undo move on BB's
        board.unmakeMove(newMove, color);

		if (!board.capture_or_promotion(newMove)
			&& quietsCount < 64) {

			quiets[quietsCount] = newMove;         
			quietsCount++;
		}

		// Find the move in the list of root moves so we can either
		// increase it's score if it's the new PV or decrease it if it's not.
		RootMove & rm = *std::find(thisThread->rootMoves.begin(), thisThread->rootMoves.end(), newMove);

		if (score > best) 
		{
			best = score;
			bestMove = newMove;

			rm.score = best;
			rm.pv.resize(1);

			// Store the principal variation
			for (Move* m = (ss + 1)->pv; *m != MOVE_NONE; ++m)
				rm.pv.push_back(*m);

			// Record if the best move changes so we can allocate 
			// more time if we have an unstable PV.
			if (legalMoves > 1 && thisThread == Threads.main())
				++static_cast<MainThread*>(thisThread)->bestMoveChanges;
		}
		else 
			rm.score = -INF;

        if(score > alpha){


            if(score > beta){
				hashFlag = TT_BETA;
				alpha = beta;
				break;
            }
			
            alpha    =    score;
			hashFlag = TT_ALPHA;
        }
		
    }

	//save info and move to TT
	TT.save(bestMove, board.TTKey(), depth, valueToTT(alpha, ss->ply), hashFlag);

	
	if (alpha >= beta && !flagInCheck && !board.capture_or_promotion(bestMove)) {
		updateStats(board, bestMove, ss, depth, quiets, quietsCount, stat_bonus(depth));
	}
		
    return alpha;
}


template<NodeType NT>
int alphaBeta(BitBoards& board, int depth, int alpha, int beta, Search::searchStack *ss, bool allowNull)
{
	const bool isPV = (NT == PV);
	const int color = board.stm();
	int score;

	bool FlagInCheck;
	bool captureOrPromotion, givesCheck;

	FlagInCheck = false;
	captureOrPromotion = givesCheck  = false;

	Thread * thisThread = board.this_thread();

	if (thisThread == Threads.main())
		static_cast<MainThread *>(thisThread)->check_time();


	// Draw detection by 50 move rule
	// as well as by 3-fold repetition.  
	// Stop search check as well
	if (Threads.stop.load(std::memory_order_relaxed) 
		|| board.isDraw(ss->ply) 
		|| ss->ply >= MAX_PLY)

		return DrawValue[color];

	// Holds state of board on current ply info
	StateInfo st;

	int R = 2, newDepth, predictedDepth = 0, quietsCount;

	Move pv[MAX_PLY + 1];
	// Container holding quiet moves so we can reduce their score 
	// if they're not the ply winning move.
	Move quiets[64]; 

	ss->moveCount = quietsCount = 0;
	ss->ply = (ss - 1)->ply + 1; 
	ss->statScore = 0;
	
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;

	ss->currentMove  = ss->excludedMove = MOVE_NONE;
	ss->contiHistory = &thisThread->contiHistory[PIECE_EMPTY][0];
	int prevSq = to_sq((ss - 1)->currentMove);

	// Mate distance pruning, 
	// prevents looking for mates longer than one we've already found
	alpha = std::max(mated_in(ss->ply),    alpha);
	beta  = std::min(mate_in( ss->ply + 1), beta);
	if (alpha >= beta)
		return alpha;

	const HashEntry *ttentry;
	Move ttMove;
	int ttValue;

	ttentry = TT.probe(board.TTKey());
	ttMove  = ttentry ? ttentry->move() : MOVE_NONE; //is there a move stored in transposition table?
	ttValue = ttentry ? valueFromTT(ttentry->eval(), ss->ply) : 0; //if there is a TT entry, grab its value

	if(!isPV 
		&& ttentry
		&& ttentry->depth() >= depth
		&& (ttValue >= beta
			? ttentry->flag() == TT_BETA
			: ttentry->flag() == TT_ALPHA))
	{
		if (ttMove)
		{
			if (ttValue >= beta)
			{
				if (!board.capture_or_promotion(ttMove))
					updateStats(board, ttMove, ss, depth, nullptr, 0, stat_bonus(depth));

				// Penalty if the TT move from previous ply is quiet and gets refuted
				if ((ss - 1)->moveCount == 1 && !board.captured_piece())
					updateContinuationHistories(ss - 1, board.pieceOnSq(prevSq), prevSq, -stat_bonus(depth + 1));

			}
			else if (!board.capture_or_promotion(ttMove))
			{
				int penalty = -stat_bonus(depth);
				thisThread->mainHistory.update(board.stm(), ttMove, penalty);
				updateContinuationHistories(ss, board.pieceOnSq(from_sq(ttMove)), to_sq(ttMove), penalty);
			}
		}
		return ttValue;
	}

	//are we in check?
	FlagInCheck = board.checkers();

	//if in check, or in reduced search extension: skip nulls, statics evals, razoring, etc to moves_loop:
	if (FlagInCheck) 
	{
		ss->staticEval = 0;
		goto moves_loop;
	}
		

	// Make this into a namespace so we don't need to create this object each time, no reason atm
	Evaluate eval;   
	ss->staticEval =  (ss - 1)->currentMove != MOVE_NULL 
		           ?  eval.evaluate(board) 
		           : -(ss - 1)->staticEval + 2 * Tempo;


	//eval pruning / static null move
	if (depth < 3 && !isPV && abs(beta - 1) > -INF + 100) {

		int eval_margin = 120 * depth;
		if (ss->staticEval - eval_margin >= beta) {
			return ss->staticEval - eval_margin;
		}
	}
///*
	// Razoring 
	if (!isPV 
		&& depth < 4
		&& ss->staticEval + 300 * (depth - 1) * 60 <= alpha
		&& !ttMove
		&& !board.pawnOn7th(color)) {

		if (depth <= 1 && ss->staticEval + 300 * 3 * 60 <= alpha) {
			return quiescent(board, alpha, beta, ss, NO_PV);
		}

		int ralpha = alpha - 300 * (depth - 1) * 60;

		int val = quiescent(board, ralpha, ralpha + 1, ss, NO_PV);

		if (val <= alpha)
			return val;
	}
//*/	
/*	
	// Razoring ///////////////////////////Losing 50-95 ELO at the moment!
	if (!isPV
		&& depth < 4
		&& ss->staticEval + razor_margin[depth] <= alpha)
	{

		if (depth <= 1) {
			return quiescent(board, alpha, alpha + 1, ss, NO_PV);
		}

		int ralpha = alpha - razor_margin[depth];

		int val = quiescent(board, ralpha, ralpha + 1, ss, NO_PV);

		if (val <= alpha) 
			return val;
	}
	*/

	// Futility pruning
	if (depth < 7
		&& ss->staticEval - futile_margin(depth) >= beta // Need to play test futile_margin with different values per margin * depth
		&& ss->staticEval < VALUE_KNOWN_WIN
		&& board.non_pawn_material(board.stm()))
		return ss->staticEval;


	// Null move heuristics, disabled if in PV, check, or depth is too low 
	if (allowNull && !isPV && depth > R) {
		if (depth > 6) R = 3;

		ss->currentMove  = MOVE_NULL;
		ss->contiHistory = &thisThread->contiHistory[PIECE_EMPTY][0];

		board.makeNullMove(st);
		score = -alphaBeta<NonPv>(board, depth - R - 1, -beta, -beta + 1, ss + 1, NO_NULL);
		board.undoNullMove();

		if (score >= beta) {
			return score;
		}
	}

	//Internal iterative deepening search same ply to a shallow depth..
	//and see if we can get a TT entry to speed up search

	if (depth >= 6 && !ttMove
		&& (isPV || ss->staticEval + 100 >= beta)) {
		int d = (int)(3 * depth / 4 - 2);

		alphaBeta<NT>(board, d, alpha, beta, ss, NO_NULL);

		ttentry = TT.probe(board.TTKey());
		ttMove = ttentry ? ttentry->move() : MOVE_NONE;
	}


	moves_loop: //jump to here if in check or in a search extension or skip early pruning is true


		//has this current node variation improved our static_eval ?
	bool improving = ss->staticEval >= (ss - 2)->staticEval
	|| ss->staticEval == 0
	|| (ss - 2)->staticEval == 0;

	int hashFlag = TT_ALPHA, legalMoves = 0, bestScore = -INF;
	score = bestScore;

	bool ttMoveCapture = false, doFullDepthSearch;



	const PieceToHistory* contiHist[] = { (ss - 1)->contiHistory, (ss - 2)->contiHistory, nullptr, (ss - 4)->contiHistory };
	Move counterMove = thisThread->counterMoves[board.pieceOnSq(prevSq)][prevSq];

	CheckInfo ci(board);

	Move newMove, bestMove = MOVE_NONE;
	MovePicker mp(board, ttMove, depth, &thisThread->mainHistory, contiHist, counterMove, ss->killers);

	while ((newMove = mp.nextMove()) != MOVE_NONE) {

		//is move legal? if not skip it
		if (!board.isLegal(newMove, ci.pinned)) {
			continue;
		}

		int movedPiece     = board.moved_piece(newMove);
		captureOrPromotion = board.capture_or_promotion(newMove);
		ttMoveCapture = (ttMove && board.capture_or_promotion(ttMove));

		// Make the move!
		board.makeMove(newMove, st, color);

		ss->moveCount = ++legalMoves;
		newDepth      =    depth - 1;
		givesCheck    =  st.checkers;

		ss->currentMove  = newMove;
		ss->contiHistory = &thisThread->contiHistory[movedPiece][to_sq(newMove)];
///*
		// Early pruning, so far only continuation historys
		// are giving ELO gain with this. 
		if (board.non_pawn_material(board.stm())
			&& bestScore > VALUE_MATED_IN_MAX_PLY) {

			if (    !captureOrPromotion
				&&  !givesCheck
				&& (!board.isMadePawnPush(newMove, color) 
				||   board.non_pawn_material() >= 2100))
			{
				int lmrDepth = std::max(newDepth - reduction<NT>(improving, depth, legalMoves), 0);

				if (lmrDepth < 3
					&& (*contiHist[0])[movedPiece][to_sq(newMove)] < counterMovePruneThreshold
					&& (*contiHist[1])[movedPiece][to_sq(newMove)] < counterMovePruneThreshold)
				{
					board.unmakeMove(newMove, color);
					continue;
				}
								
				//if(lmrDepth < 7
				//&& !FlagInCheck
				//&& ss->staticEval + 128 + 100 * lmrDepth <= alpha)
				//continue;

				
				//if(lmrDepth < 8
				//&& !board.seeGe(newMove, (-17 * lmrDepth * lmrDepth))) // Play test the negative value
				//continue;		
			}
		}
//*/

		if (depth >= 3
			&& legalMoves > 1
			&& !captureOrPromotion)
		{
			int depthR = reduction<NT>(improving, depth, legalMoves);

			if (captureOrPromotion)
				depthR -= depthR ? 1 : 0;

			else 
			{
				// High opponent move count, 
				// reduce reduction
				if ((ss - 1)->moveCount > 15)
					depthR -= 1;

				// If we had an exact PV match, reduce reduction
				if (ttentry && ttentry->flag() == TT_EXACT)
					depthR -= 1;

				// If the ttmove was a capture 
				// increase reduction
				if (ttMoveCapture)
					depthR += 1;
				
				// If the SEE of our move was negative,
				// 
				if (depthR
					&& move_type(newMove) == NORMAL
					&& !board.seeGe(create_move(to_sq(newMove), from_sq(newMove))))
					depthR -= 2;

				ss->statScore = thisThread->mainHistory[color][from_to(newMove)]
					+ (*contiHist[0])[movedPiece][to_sq(newMove)]
					+ (*contiHist[1])[movedPiece][to_sq(newMove)]
					+ (*contiHist[3])[movedPiece][to_sq(newMove)]
					- 4000;

				if (ss->statScore >= 0 && (ss - 1)->statScore < 0)
					depthR -= 1;

				else if ((ss - 1)->statScore >= 0 && ss->statScore < 0)
					depthR += 1;

				depthR = std::max(0, (depthR - ss->statScore / 20000));
			}		

			int d = std::max(newDepth - depthR, 1);

			score = -alphaBeta<NonPv>(board, d, -(alpha + 1), -alpha, ss + 1, DO_NULL);

			doFullDepthSearch = (score > alpha && d != newDepth);
		}
		else
			doFullDepthSearch = !isPV || legalMoves > 1;

		if (isPV)
			(ss + 1)->pv = nullptr;

		if (doFullDepthSearch)
			score = newDepth < 1 ? -quiescent(board, -(alpha + 1), -alpha, ss + 1, NO_PV)
						         : -alphaBeta<NonPv>(board, newDepth, -(alpha + 1), -alpha, ss + 1, DO_NULL);

		if (isPV && (legalMoves == 1 || (score > alpha && score < beta)))
		{
			(ss + 1)->pv = pv;
			(ss + 1)->pv[0] = MOVE_NONE;

			score = newDepth < 1 ? -quiescent(board, -beta, -alpha, ss + 1, IS_PV)
				                 : -alphaBeta<PV>(board, newDepth, -beta, -alpha, ss + 1, DO_NULL);
		}

		// Store queit moves so we can decrease their History value later.
		if (!captureOrPromotion && newMove != bestMove && quietsCount < 64) {
			quiets[quietsCount++] = newMove;                                    
		}
		
		//undo move on BB's
		board.unmakeMove(newMove, color);

		// Search stop has occured.
		if (Threads.stop.load(std::memory_order_relaxed))
			return 0;

		if (score > bestScore) {

			bestScore = score;

			if (score > alpha) {

				bestMove = newMove;

				//store the principal variation
				if(isPV)
					update_pv(ss->pv, bestMove, (ss + 1)->pv);

				//if move causes a beta cutoff stop searching current branch
				if (score >= beta) {
					hashFlag = TT_BETA;
					//stop search and return beta
					alpha = beta;
					break;
				}
				//new best move
				alpha = score;
				//if we've gained a new alpha set hash Flag equal to exact and store best move
				hashFlag = TT_EXACT;
			}
		}
	}

	//if there are no legal moves and we are in checkmate or stalemate this node
	if (!legalMoves) 
		alpha = FlagInCheck ? mated_in(ss->ply) 
		                    : DrawValue[color];

	else if (bestMove) { 

		// Update heuristics for Quiet best move.
		// Decrease score for other quiets, update killers, counters, etc.
		if(!board.capture_or_promotion(bestMove))
			updateStats(board, bestMove, ss, depth, quiets, quietsCount, stat_bonus(depth));

		// Penalty if the TT move from previous ply is quiet and gets refuted
		if ((ss - 1)->moveCount == 1 && !board.captured_piece())
			updateContinuationHistories(ss - 1, board.pieceOnSq(prevSq), prevSq, -stat_bonus(depth + 1)); 
	}

	else if (depth >= 3 
			&& !board.captured_piece() 
			&& is_ok((ss-1)->currentMove))
		updateContinuationHistories(ss - 1, board.pieceOnSq(prevSq), prevSq, stat_bonus(depth));
	

	//save info + move to transposition table 
	TT.save(bestMove, board.TTKey(), depth, valueToTT(alpha, ss->ply), hashFlag);
	
	return alpha;
}

int Search::quiescent(BitBoards& board, int alpha, int beta, searchStack *ss, int isPV)
{
	//node count, sepperate q count needed?
	const int color = board.stm();
	const HashEntry *ttentry;
	int ttValue;
	int oldAlpha, bestScore, score;

	StateInfo st;
	Move ttMove, bestMove, newMove;
	ss->ply = (ss - 1)->ply + 1;
	ss->currentMove = bestMove = MOVE_NONE;

	if (board.isDraw(ss->ply) || ss->ply == MAX_PLY)
		return DrawValue[color];

	ttentry = TT.probe(board.TTKey());
	ttMove  = ttentry ? ttentry->move() : MOVE_NONE; //is there a move stored in transposition table?
	ttValue = ttentry ? valueFromTT(ttentry->eval(), ss->ply) : INVALID; //if there is a TT entry, grab its value

	if (ttentry
		&& ttentry->depth() >= DEPTH_QS
		&& (isPV ? ttentry->flag() == TT_EXACT
			: ttValue >= beta ? ttentry->flag() == TT_BETA
			: ttentry->flag() == TT_ALPHA)
		) {

		return ttValue;
	}

	Thread * thisThread = board.this_thread();

	Evaluate eval;
	int standingPat = eval.evaluate(board);

	if (standingPat >= beta) {

		if (!ttentry) TT.save(MOVE_NONE, board.TTKey(), DEPTH_QS, valueToTT(standingPat, ss->ply), TT_BETA); //save to TT if there's no entry 
		return beta;
	}

	if (alpha < standingPat) {
		alpha = standingPat;
	}

	//set hash flag equal to alpha Flag
	int hashFlag = TT_ALPHA;

	CheckInfo ci(board);

	MovePicker mp(board, ttMove, &thisThread->mainHistory); 

	while((newMove = mp.nextMove()) != MOVE_NONE)
	{

		if (!board.isLegal(newMove, ci.pinned)) {
			continue;
		}
		
		// If the move is gurenteed to be below alpha, don't search it
		// unless we're in endgame, where that's not safe ~~ possibly use more advanced board.game_phase()?
		if ((standingPat + SORT_VALUE[board.pieceOnSq(to_sq(newMove))] + 200 < alpha)
				&& (board.bInfo.sideMaterial[!color] - SORT_VALUE[board.pieceOnSq(to_sq(newMove))] > END_GAME_MAT)  
				&& move_type(newMove) != PROMOTION) 
			continue;
		

		//Don't search losing capture moves if not in PV
		if (!isPV && board.SEE(newMove, color, true) < 0) 
			continue; 
		

		board.makeMove(newMove, st, color);

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

void Search::updateStats(const BitBoards & board, Move move, searchStack * ss, int depth, Move * quiets, int qCount, int bonus)
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

	const int color = board.stm();

	//update historys, increasing the cutoffs score, decreasing every other moves score
	int val = 4 * depth * depth;  //Possibly change how this bonus is handled??

	Thread * thisThread = board.this_thread();

	thisThread->mainHistory.update(color, move, val);
	updateContinuationHistories(ss, board.pieceOnSq(from_sq(move)), to_sq(move), val);
	
	// Update counter moves
	if (is_ok((ss - 1)->currentMove)) {

		int prevSq = to_sq((ss - 1)->currentMove);
		thisThread->counterMoves[board.pieceOnSq(prevSq)][prevSq] = move;
	}

	for(int i = 0; i < qCount; ++i){
		thisThread->mainHistory.update(color, quiets[i], -val);
		updateContinuationHistories(ss, board.pieceOnSq(from_sq(quiets[i])), to_sq(move), -val);
	}
}
///*
void Search::updateContinuationHistories(searchStack * ss, int piece, int to, int bonus)
{
	for (int i : {1, 2, 4})
		if (is_ok((ss - i)->currentMove))
			(ss - i)->contiHistory->update(piece, to, bonus);
}
//*/
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
		: val >= VALUE_MATE_IN_MAX_PLY  ? val - ply
		: val <= VALUE_MATED_IN_MAX_PLY ? val + ply : val;
}

void MainThread::check_time()
{
	//test for a condition that does less checks
	//way too many atm
	//if (SearchControl.use_time() && !timeOver && (Threads.nodes_searched() & 4095)) {
	//	timeOver = timeM.timeStopSearch();
	//}

	if (SearchControl.use_time() && timeM.elapsed() > timeM.maximum())
		Threads.stop = true;
}

void Search::print(int bestScore, int depth)
{
	std::stringstream ss;
	
	int nodes = Threads.nodes_searched();

	ss << "info depth " << depth << " nodes " << nodes << " nps " << timeM.getNPS(nodes) << " score ";

	if (bestScore < VALUE_MATE - MAX_PLY)
		ss << "cp " << bestScore;
	else
		ss << "mate " << (bestScore > 0 ? VALUE_MATE - bestScore + 1 : -VALUE_MATE - bestScore) / 2;

	std::cout << ss.str() << std::endl;
}



template<bool Root>
U64 Search::perft(BitBoards & board, int depth)
{

	StateInfo st;
	U64 count, nodes = 0;
	SMove mlist[256];
	SMove * end = generate<PERFT_TESTING>(board, mlist);

	for (SMove * i = mlist; i != end; ++i)
	{
		if (depth == 1)
			nodes++;

		else {

			board.makeMove(i->move, st, board.stm());

			count = perft<false>(board, depth - 1);

			nodes += count;

			board.unmakeMove(i->move, !board.stm());
		}
	}

	return nodes;
}

template U64 Search::perft<true>(BitBoards & board, int depth);

std::string flipsL[8] = { "a", "b", "c", "d", "e", "f", "g", "h" };
int flipsN[8]		  = {   8,   7,   6,   5,   4,   3,   2,   1 };

template<bool Root>
U64 Search::perftDivide(BitBoards & board, int depth)
{
	StateInfo st;
	U64 count, nodes = 0, tNodes = 0;
	SMove mlist[256];
	SMove * end = generate<PERFT_TESTING>(board, mlist);
	int RootMoves = 0;

	const bool leaf = (depth == 1);

	if (depth == 0)
		return 0;

	for (SMove * i = mlist; i != end; ++i)
	{
		Move m = i->move;

		if (depth == 1)
			nodes++;


		if (Root) {
			

			int x = file_of(from_sq(m));
			int y = rank_of(from_sq(m)) ^ 7; // Note: Move scheme was changed and too lazy to change array
			int x1 = file_of(to_sq(m));
			int y1 = rank_of(to_sq(m)) ^ 7;

			board.makeMove(m, st, board.stm());

			RootMoves++;

			U64 nodesFromMove = perftDivide<false>(board, depth - 1);

			tNodes += nodesFromMove;

			std::cout << flipsL[x] << flipsN[y] << flipsL[x1] << flipsN[y1] << "   " << nodesFromMove << std::endl;

			board.unmakeMove(m, !board.stm());
		}

		if(!Root && depth != 1)
		{
			board.makeMove(m, st, board.stm());

			count = perftDivide<false>(board, depth - 1);

			nodes += count;

			board.unmakeMove(m, !board.stm());
		}
	}

	if (Root)
	{
		std::cout << std::endl << "Nodes: " << tNodes << std::endl;
		std::cout << "Moves: " << RootMoves << std::endl;
	}

	return nodes;
}

template U64 Search::perftDivide<true>(BitBoards & board, int depth);
