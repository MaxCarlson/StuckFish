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

Ai_Logic::Ai_Logic()
{

}

Move Ai_Logic::search(bool isWhite) {
	
	//max depth
	int depth = 32;

	//generate accurate zobrist key based on bitboards
	zobrist.getZobristHash(newBoard);

	//update color only applied on black turn
	if (!isWhite) zobrist.UpdateColor();

	//decrease history values after a search
	ageHistorys();

	//calc move time for us, send to search driver
	timeM.calcMoveTime(isWhite);

	/*
	U64 king;
	if (isWhite) king = newBoard.BBWhiteKing;
	else king = newBoard.BBBlackKing;
	MoveGen checkcheck;
	checkcheck.grab_boards(newBoard, isWhite);
	//are we in check?
	bool flagInCheck = checkcheck.isAttacked(king, isWhite, false);

	if (flagInCheck) { //extend depth and time if in check ///add more complex methods of time managment
		depth++; 
		sd.moveTime += 2500; 
	} 
	*/

	Move m = iterativeDeep(depth, isWhite);
	
	return m;
}
int futileC = 0;
Move Ai_Logic::iterativeDeep(int depth, bool isWhite)
{
	//reset ply
    int ply = 0;

    //best overall move as calced
    Move bestMove;
    int bestScore, alpha = -INF, beta = INF;
	sd.depth = 0;

    //std::cout << zobrist.zobristKey << std::endl;

    //iterative deepening loop starts at depth 1, iterates up till max depth or time cutoff
    while(sd.depth++ <= depth){

		if (timeM.timeStopRoot() || timeOver) break;

        //main search
        bestScore = searchRoot(sd.depth, alpha, beta, isWhite, ply+1);
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
        if(!timeOver){

            //grab best move out of PV array ~~ need better method of grabbing best move, not based on "distance"
            //Maybe if statement is a bad fix? sometimes a max of specified depth is not reached near checkmates/possibly checks
            //if statement makes sure the move has been tried before storing it
            if(sd.PV[1].tried) bestMove = sd.PV[1];

			//print data on search 
			print(isWhite, bestScore);
			//std::cout << futileC << std::endl;
        }
		
        //increment distance to travel (same as depth at max depth)
    }


    //make final move on bitboards + draw board
    newBoard.makeMove(bestMove, zobrist, isWhite);
    newBoard.drawBBA();
	//std::cout << newBoard.sideMaterial[0] << " " << newBoard.sideMaterial[1] <<std::endl;

    //evaluateBB ev; //used for prininting static eval after move
    //std::cout << "Board evalutes to: " << ev.evalBoard(true, newBoard, zobrist) << " for white." << std::endl;

    return bestMove;
}

int Ai_Logic::searchRoot(U8 depth, int alpha, int beta, bool isWhite, U8 ply)
{
    bool flagInCheck;
    int score = 0;
    int best = -INF;
    int legalMoves = 0;
	int hashFlag = TT_ALPHA;

	sd.nodes++;

    MoveGen gen_moves;
    //grab bitboards from newBoard object and store color and board to var
    gen_moves.grab_boards(newBoard, isWhite);
    gen_moves.generatePsMoves(false);

    //who's our king?
    U64 king;
    if(isWhite) king = newBoard.BBWhiteKing;
    else king = newBoard.BBBlackKing;	

    int moveNum = gen_moves.moveCount;

    for(int i = 0; i < moveNum; ++i){
        //find best move generated
        Move newMove = gen_moves.movegen_sort(ply);
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
        sd.cutoffs[isWhite][newMove.from][newMove.to] -= 1;

        //PV search at root
        if(best == -INF){
            //full window PV search
            score = -alphaBeta(depth-1, -beta, -alpha, !isWhite, ply +1, DO_NULL, IS_PV);

       } else {
            //zero window search
            score = -alphaBeta(depth-1, -alpha -1, -alpha, !isWhite, ply +1, DO_NULL, NO_PV);

            //if we've gained a new alpha we need to do a full window search
            if(score > alpha){
                score = -alphaBeta(depth-1, -beta, -alpha, !isWhite, ply +1, DO_NULL, IS_PV);
            }
        }

        //undo move on BB's
        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

		if (timeOver) break;

        if(score > best) best = score;

        if(score > alpha){
            //store the principal variation
			sd.PV[ply] = newMove;

            if(score > beta){
				hashFlag = TT_BETA;
				break;
            }
			
            alpha = score;
			hashFlag = TT_ALPHA;
        }

    }
	//save info and move to TT
	TT.save(sd.PV[ply], zobrist.zobristKey, depth, score, hashFlag);

    return alpha;
}

int Ai_Logic::alphaBeta(U8 depth, int alpha, int beta, bool isWhite, U8 ply, bool allowNull, bool is_pv)
{
    bool FlagInCheck = false;
    bool raisedAlpha = false; 
	bool futileMoves = false; //have any moves been futile?
	bool singularExtension = false;
    U8 R = 2;
    U8 reductionDepth; //how much are we reducing depth in LMR?
    U8 newDepth;
    //U8 newDepth; //use with futility + other pruning later
    int queitSD = 25, f_prune = 0;
    int  mateValue = INF - ply; // used for mate distance pruning
	sd.nodes++;

	//checks if time over is true everytime ( nodes & 4095 ) = true ///NEED METHOD THAT CHECKS MUCH LESS
	checkInput();

	//mate distance pruning, prevents looking for mates longer than one we've already found
	if (alpha < -mateValue) alpha = -mateValue;
	if (beta > mateValue - 1) beta = mateValue - 1;
	if (alpha >= beta) return alpha;

	const  HashEntry *ttentry;
	bool ttMove;
	int ttValue;

	ttentry = TT.probe(zobrist.zobristKey);
	ttMove = ttentry ? ttentry->move.flag : false; //is there a move stored in transposition table?
	ttValue = ttentry ? ttentry->eval : INVALID; //if there is a TT entry, grab its value
/*
	if (ttentry && ttentry->depth >= depth) {
		hashHits++;
		if (!is_pv || (ttentry->eval > alpha && ttentry->eval < beta)) {
			switch (ttentry->flag) {
			case TT_ALPHA:
				if (ttentry->eval <= alpha) {
					alphas++;
					return alpha;
				}
				break;
			case TT_BETA:
				if (ttentry->eval >= beta) {
					betas++;
					return beta;
				}
				break;
			case TT_EXACT:
				exacts++;
				return ttentry->eval;
			}
		}
	}
*/

///*
	if (ttentry
		&& ttentry->depth >= depth
		&& (is_pv ? ttentry->flag == TT_EXACT
			: ttValue >= beta ? ttentry->flag == TT_BETA
			: ttentry->flag == TT_ALPHA)
		) {

		return ttValue;
	}
//*/


    int score;
    if(depth < 1 || timeOver){
        //run capture search to max depth of queitSD
        score = quiescent(alpha, beta, isWhite, ply, queitSD);
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
    if(FlagInCheck || sd.excludedMove) goto moves_loop;

	evaluateBB eval;
	int static_eval = eval.evalBoard(isWhite, newBoard, zobrist); //newBoard.sideMaterial[isWhite] - newBoard.sideMaterial[!isWhite];
//eval pruning / static null move
    if(depth < 3 && !is_pv && abs(beta - 1) > -INF + 100){
		//evaluateBB eval;
		//int static_eval = eval.evalBoard(isWhite, newBoard, zobrist);
  
        int eval_margin = 120 * depth;
        if(static_eval - eval_margin >= beta){
            return static_eval - eval_margin;
        }
    }

//Null move heuristics, disabled if in PV, check, or depth is too low
    if(allowNull && !is_pv && depth > R){
        if(depth > 6) R = 3;
        zobrist.UpdateColor();

        score = -alphaBeta(depth -R -1, -beta, -beta +1, !isWhite, ply +1, NO_NULL, NO_PV);
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
		if(static_eval < threshold){

            score = quiescent(alpha, beta, isWhite, ply, queitSD);

            if(score < threshold) return alpha;
        }
    }

	///* //TOO slow
//do we want to futility prune?
	int fmargin[4] = { 0, 200, 300, 500 };
    //int fmargin[8] = {0, 100, 150, 200, 250, 300, 400, 500};
    if(depth < 4 && !is_pv
		&& abs(alpha) < 9000
		&& static_eval + fmargin[depth] <= alpha){
        f_prune = 1;
    }
	//*/


moves_loop: //jump to here if in check or in a search extension
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
    gen_moves.reorderMoves(ply, ttentry);

    int hashFlag = TT_ALPHA, movesNum = gen_moves.moveCount, legalMoves = 0;

    Move hashMove; //move to store alpha in and pass to TTable
    for(int i = 0; i < movesNum; ++i){
        //grab best scoring move
		Move newMove = gen_moves.movegen_sort(ply); //huge speed decrease if not Move newMove is instatiated above loop!!??

		//if (sd.excludedMove && newMove.score >= SORT_HASH) continue;

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
        sd.cutoffs[isWhite][newMove.from][newMove.to] -= 1;
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
            && !gen_moves.isAttacked(eking, !isWhite, true)){

            newBoard.unmakeMove(newMove, zobrist, isWhite);
			gen_moves.grab_boards(newBoard, isWhite);
			futileMoves = true; //flag so we know we skipped a move/not checkmate
			futileC++;
            continue;
			//break;
        }
        //*/

        //late move reduction
        if(i > 1 && newDepth > 3
            && legalMoves > 3
            && !gen_moves.isAttacked(eking, !isWhite, true)
            && !FlagInCheck
            && sd.cutoffs[isWhite][newMove.from][newMove.to] < 50
            && (newMove.from != sd.killers[0][ply].from || newMove.to != sd.killers[0][ply].to)
            && (newMove.from != sd.killers[1][ply].from || newMove.to != sd.killers[1][ply].to)
            && newMove.captured == PIECE_EMPTY && newMove.flag != 'Q'
			&& newMove.score < SORT_HASH){


            sd.cutoffs[isWhite][newMove.from][newMove.to] = 50;
            reductionDepth = 1;
            if(legalMoves > 6) reductionDepth++;
			if (legalMoves > 14) reductionDepth++; //maybe? need more lmr traits

            newDepth -= reductionDepth;
        }
		//load the (most likely) next entry in the TTable into cache near cpu
		_mm_prefetch((char *)TT.first_entry(zobrist.fetchKey(newMove, isWhite)), _MM_HINT_NTA);

//jump back here if our LMR raises Alpha
re_search:
		

        if(!raisedAlpha){
            //we're in princiapl variation search or full window search
            score = -alphaBeta(newDepth, -beta, -alpha, !isWhite, ply +1, DO_NULL, is_pv);
        } else {
            //zero window search
            score = -alphaBeta(newDepth, -alpha -1, -alpha, !isWhite, ply +1, DO_NULL, NO_PV);
            //if our zero window search failed, do a full window search
            if(score > alpha){
                //PV search after failed zero window
                score = -alphaBeta(newDepth, -beta, -alpha,  !isWhite, ply +1, DO_NULL, IS_PV);
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
            sd.cutoffs[isWhite][newMove.from][newMove.to] += 6;
			//store the principal variation
			sd.PV[ply] = newMove;
			hashMove = newMove;

            //if move causes a beta cutoff stop searching current branch
            if(score >= beta){

                if(newMove.captured == PIECE_EMPTY && newMove.flag != 'Q'){
                    //add move to killers
                    addKiller(newMove, ply);

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
        else alpha = contempt(isWhite); 
    }
	if (futileMoves && !raisedAlpha && hashFlag != TT_BETA) {

		if(!legalMoves) alpha = static_eval; //testing needed as well
		hashFlag = TT_EXACT; //NEED TO TEST
	}

	if (!sd.excludedMove) { //only write to TTable if we're not in partial search
		//save info + move to transposition table ///MAYBE add an if(legalMoves) check? will have to test
		TT.save(hashMove, zobrist.zobristKey, depth, alpha, hashFlag);
	}


    return alpha;
}


int Ai_Logic::quiescent(int alpha, int beta, bool isWhite, int ply, int quietDepth)
{
    //iterative deeping timer stuff
	sd.nodes++;

	HashEntry *entry = NULL;

    evaluateBB eval;
    //evaluate board position
    int standingPat = eval.evalBoard(isWhite, newBoard, zobrist);

    if(quietDepth <= 0 || timeOver){
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

		///*
		//also not fast enough yet, though more testing is needed
		//delta pruning
		if ((standingPat + SORT_VALUE[newMove.captured] + 200 < alpha)
			&& (newBoard.sideMaterial[!isWhite] - SORT_VALUE[newMove.captured] > END_GAME_MAT)
			&& newMove.flag != 'Q') continue;
			
		/*
		U64 f = 1LL << newMove.from; //SEE eval cutoff
		U64 t = 1LL << newMove.to;
		if (newMove.score <= 0) continue; //or equal to zero add once bug is found
		*/

        newBoard.makeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        //is move legal? if not skip it ~~~~~~~~ possibly remove check later?
        if(gen_moves.isAttacked(king, isWhite, true)){
            newBoard.unmakeMove(newMove, zobrist, isWhite);
            gen_moves.grab_boards(newBoard, isWhite);
            continue;
        }

        /*
        //bad capture pruning
        if(badCapture(tempMove, isWhite, newBoard) && tempMove[3] != 'Q'){
            continue;
        }
        */
        score = -quiescent(-beta, -alpha, !isWhite, ply+1, quietDepth-1);

        newBoard.unmakeMove(newMove, zobrist, isWhite);
        gen_moves.grab_boards(newBoard, isWhite);

        if(score > alpha){
            if(score >= beta){

                return beta;
            }

           alpha = score;
        }
    }

    return alpha;
}

int Ai_Logic::contempt(bool isWhite)
{
    //need some way to check board material before implementing

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

	for (int i = 0; i < sd.twoFoldRep.size(); ++i) {
		if (zobrist.zobristKey == sd.twoFoldRep[i]) repCount++;

	}

	if (repCount >= 2) {
		return true;
	}

	return false;
}

void Ai_Logic::addKiller(Move move, int ply)
{
    if(move.captured == PIECE_EMPTY){ //if move isn't a capture save it as a killer
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
	sd.nodes = 0;
}

void Ai_Logic::clearHistorys()
{
	//used to decrease value after a search
	for (int cl = 0; cl < 2; cl++)
		for (int i = 0; i < 64; i++)
			for (int j = 0; j < 64; j++) {
				sd.history[cl][i][j] = sd.history[cl][i][j] = 0;
				sd.cutoffs[cl][i][j] = 100;
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

