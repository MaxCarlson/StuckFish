#include "MovePicker.h"

#include "bitboards.h"
#include "movegen.h"
#include "externs.h"

#include "ai_logic.h" 




// Taken almost directly from Stockfish.
// Our insertion sort, which is guaranteed to be stable
void insertion_sort(SMove* begin, SMove* end)
{
	SMove tmp, *p, *q;

	for (p = begin + 1; p < end; ++p)
	{
		tmp = *p;
		for (q = p; q != begin && *(q - 1) < tmp; --q)
			*q = *(q - 1);
		*q = tmp;
	}
}

// partial_insertion_sort() sorts moves in descending order up to and including
// a given limit. The order of moves smaller than the limit is left unspecified.
void partial_insertion_sort(SMove* begin, SMove* end, int limit) {

	for (SMove *sortedEnd = begin, *p = begin + 1; p < end; ++p)
		if (p->score >= limit)
		{
			SMove tmp = *p, *q;
			*p = *++sortedEnd;
			for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
				*q = *(q - 1);
			*q = tmp;
		}
}


// Unary predicate used by std::partition to split positive values from remaining
// ones so as to sort the two sets separately, with the second sort delayed.
inline bool has_positive_value(const SMove& ms) { return ms.score > 0; }


// Pick the best move in the range begin - end move it to the front and return.
// Avoids sorting more moves than we'd need if we get a cutoff.
inline SMove * pick(SMove * begin, SMove * end) {
	std::swap(*begin, *std::max_element(begin, end));
	return begin;
}

// Main search initilization 
MovePicker::MovePicker(const BitBoards & board, Move ttm, int depth, const ButterflyHistory * hist, const PieceToHistory** ch, Move cm, Move * killers_p)
									   : b(board), Depth(depth), mainHist(hist), contiHistory(ch), counterMove(cm), killers{ killers_p[0], killers_p[1] }
//MovePicker::MovePicker(const BitBoards & board, Move ttm, int depth, const ButterflyHistory * hist,  Move cm, Move * killers_p)
//					           : b(board), Depth(depth), mainHist(hist), counterMove(cm), killers{ killers_p[0], killers_p[1] }
{
	Stage = b.checkers() ? EVASION : MAIN_SEARCH;


	// Check for psuedo legality!!!
	ttMove = (ttm && b.pseudoLegal(ttm) ? ttm : MOVE_NONE);

	Stage += (ttMove == MOVE_NONE);
}

// Qsearch
MovePicker::MovePicker(const BitBoards & board, Move ttm, const ButterflyHistory * hist) : b(board), mainHist(hist)
{
	Stage = Q_SEARCH;

	if (ttm && !b.capture_or_promotion(ttm))
		ttm = MOVE_NONE;


	// Check for psuedo legality!!!
	ttMove = (ttm && b.pseudoLegal(ttm) ? ttm : MOVE_NONE);

	Stage += (ttMove == MOVE_NONE);
}

template<>
void MovePicker::score<CAPTURES>()
{
	Move m;

	for (SMove * i = mList; i != end; ++i) {
		m = i->move;
		i->score = SORT_VALUE[b.pieceOnSq(  to_sq(m))] 
			     - SORT_VALUE[b.pieceOnSq(from_sq(m))];
		//i->score = SORT_VALUE[b.pieceOnSq(to_sq(m))] - 65 * relative_rankSq(b.stm(), to_sq(m)); //WILL have to test this vs. above MVVLVA sometime.

		if (move_type(m) == ENPASSANT)
			i->score += SORT_VALUE[PAWN];

		else if (move_type(m) == PROMOTION)
			i->score += SORT_VALUE[promotion_type(m)] - SORT_VALUE[PAWN];
	}
}

// Score Quiet moves by history score
// Possibly make historys a 2d array instead?
template<>
void MovePicker::score<QUIETS>() 
{
	Move m;

	for (SMove* i = mList; i != end; ++i) {
		
		m = i->move;

		i->score = (*mainHist)[b.stm()][from_sq(m)]
				 + (*contiHistory[0])[b.pieceOnSq(from_sq(m))][to_sq(m)] // These need to be play tested for ELO loss/gain
				 + (*contiHistory[1])[b.pieceOnSq(from_sq(m))][to_sq(m)]
				 + (*contiHistory[3])[b.pieceOnSq(from_sq(m))][to_sq(m)];
	}
}

template<>
void MovePicker::score<EVASIONS>()
{
	Move m;
	int seeVal;
	int color = b.stm();

	for (SMove* i = mList; i != end; ++i) {

		m = i->move;
		if (seeVal = b.SEE(m, color, true) < 0)
			i->score = seeVal;

		else if (b.capture(m))
			i->score = SORT_VALUE[b.pieceOnSq(to_sq(  m))]
			         - SORT_VALUE[b.pieceOnSq(from_sq(m))];

		else
			i->score = (*mainHist)[b.stm()][from_sq(m)]; 
	}

}

// This function finds the ( hopefully ) best move 
// with the least amount of work and returns it to the search.
// Generation is sepparated into stages.
Move MovePicker::nextMove()
{
	Move m;

	switch (Stage) {

	// Return TT move if it exists first before generating anything
	case MAIN_SEARCH: case Q_SEARCH: case EVASION:
		++Stage;
		return ttMove;

	case CAPTURES_INIT:
		endBadCaptures = current = mList;
		end = generate<CAPTURES>(b, current);
		score<CAPTURES>();
		Stage++;

		// Fall Through....

	case GOOD_CAPTURES:
		while (current < end) {
			m = pick(current++, end)->move;

			if (m != ttMove) {
				if (b.SEE(m, b.stm(), true) > 0) {   //////////////////////////////////////////////////////////TEST THESE TO BE SURE SEE IS WORKING CORRECTLY!!!
					return m;
				}
				(endBadCaptures++)->move = m;
			}
		}

		++Stage;

		// First Killer
		m = killers[0].move;

		if (    m != MOVE_NONE  // Check that the move isn't a repeat, 				                   
			&&  m != ttMove     // and is a reasonable move for board position
			&& !b.capture(m)
			&&  b.pseudoLegal(m))
			return m;

	// Fall Through...

	case KILLERS:
		++Stage;

		// Second Killer
		m = killers[1].move;

		if (    m != MOVE_NONE   				                   
			&&  m != ttMove     
			&& !b.capture(m)
			&&  b.pseudoLegal(m))
			return m;

	// Fall Through...

	case COUNTER_MOVE:
		++Stage;
		m = counterMove;
		if (   m != MOVE_NONE
			&& m != ttMove
			&& m != killers[0].move
			&& m != killers[1].move
			&&  b.pseudoLegal(m)
			&& !b.capture(m))
			return m;
	// Fall Through...

	case QUIETS_INIT:
		current = endBadCaptures;
		end = generate<QUIETS>(b, current);
		score<QUIETS>();

		partial_insertion_sort(current, end, -2200 * Depth); ///Really Need to play test with changes to this value. !!!!!!!!!
		++Stage;

	// Fall Through...

	case QUIET:

		while (current < end) {

			m = (current++)->move;

			if (   m != ttMove
				&& m != killers[0].move
				&& m != killers[1].move
				&& m != counterMove)
				return m;
		}
		++Stage;
		current = mList; // Point to front of badCaptures

	// Fall Through...

	case BAD_CAPTURES:
		if(current < endBadCaptures)
			return (current++)->move;
		break;

			
	case CAPTURES_Q_INIT:
		current = mList;
		end = generate<CAPTURES>(b, current);
		score<CAPTURES>();
		++Stage;

	// Fall Through...

	case CAPTURES_Q:

		while (current < end) 
		{
			m = pick(current++, end)->move;
			if (m != ttMove)
				return m;
		}
		break;


	case EVASIONS_INIT:
		current = mList;
		end = generate<EVASIONS>(b, mList); //replace all these with current?
		score<EVASIONS>();
		++Stage;

	// Fall Through...

	case EVASIONS_S1:
		while (current < end) {

			m = pick(current++, end)->move;
			return m;
		}
		break;
	}
	
	return MOVE_NONE;
}
