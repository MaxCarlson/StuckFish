#include "MovePicker.h"

#include "bitboards.h"
#include "movegen.h"
#include "externs.h"

#include "ai_logic.h" //NEED this enable once unresolved sig is fixed




// Taken almost directly from Stockfish.S
// Our insertion sort, which is guaranteed (and also needed) to be stable
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

// Unary predicate used by std::partition to split positive values from remaining
// ones so as to sort the two sets separately, with the second sort delayed.
inline bool has_positive_value(const SMove& ms) { return ms.score > 0; }


// Pick the best move in the range begin - end move it to the front and return.
// Avoids sorting more moves than we'd need if we get a cutoff.
inline SMove * pick(SMove * begin, SMove * end) {
	std::swap(*begin, *std::max_element(begin, end));
	return begin;
}

template<>
void MovePicker::score<CAPTURES>()
{
	Move m;

	for (SMove * i = mList; i != end; ++i) {
		m = i->move;
		i->score = SORT_VALUE[b.pieceOnSq(  to_sq(m))] 
			     - SORT_VALUE[b.pieceOnSq(from_sq(m))];

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
		i->score = h.history[b.stm()][from_sq(m)][to_sq(m)];
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
			i->score = seeVal - SORT_CAPT;

		else if (b.capture(m))
			i->score = SORT_VALUE[b.pieceOnSq(to_sq(  m))]
			         - SORT_VALUE[b.pieceOnSq(from_sq(m))] + SORT_CAPT;

		else
			i->score = h.history[color][from_sq(m)][to_sq(m)];
	}

}

void MovePicker::generateNextStage()
{
	current = mList;

	switch (++Stage) {

	case CAPTURES_M: case CAPTURES_Q:
		end = generate<CAPTURES>(b, mList);
		score<CAPTURES>();
		return;

	case KILLERS_M:
		current = killers;
		end = current + 2;

		killers[0].move = s->killers[0];
		killers[1].move = s->killers[1]; //NEED TO CHANGE searchStack TO HOLD NEW MOVE TYPE
		return;

	case QUIETS_M:
		endQuiets = end = generate<QUIETS>(b, mList);
		score<QUIETS>();
		std::partition(current, end, has_positive_value);
		insertion_sort(current, end);
		return;

	case QUIETS_M1:
		current = end;
		end     = endQuiets;

		if (Depth >= 3)
			insertion_sort(current, end);
		return;

	case BAD_CAPTURES_M:
		current = mList + 255;
		end = endBadCaptures;
		return;

	case EVASIONS_S1:
		end = generate<EVASIONS>(b, mList);
		if (end > mList + 1)
			score<EVASIONS>();
		return;
	

	case EVASIONS_: case QSEARCH_:		
		Stage = STOP;

	case STOP:
		end = current + 1;
		return;
	}
}

Move MovePicker::nextMove()
{
	Move m;

	while (true) {

		while(current == end)
			generateNextStage();

		switch (Stage) {

		// Return TT move first before generating anything
		case MAIN_M: case QSEARCH_: case EVASIONS_:
			++current;
			return ttMove;
		
		case CAPTURES_M:
			m = pick(current++, end)->move;

			if (m != ttMove) {
				if (b.SEE(m, b.stm(), true) > 0) {
					return m;
				}
				(endBadCaptures--)->move = m;
			}
			break;

		case KILLERS_M:
			m = (current++)->move;
			if (   m != MOVE_NONE
				&& m != ttMove
				&&   !  b.capture(m) 
				&& b.pseudoLegal(m))// && m is psuedoLegal 

				return m;

			break;

		case QUIETS_M: case QUIETS_M1:
			m = (current++)->move;

			if (   m != ttMove
				&& m != killers[0].move
				&& m != killers[1].move)
				return m;

			break;

		case BAD_CAPTURES_M:
			return (current--)->move;


		case EVASIONS_S1: case CAPTURES_Q:
			m = pick(current++, end)->move;
			if (m != ttMove)
				return m;
			break;

		case STOP:
			return MOVE_NONE;

		}
	}
}
