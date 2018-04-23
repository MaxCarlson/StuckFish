#pragma once

#include "defines.h"
#include "bitboards.h"

#include <array>


class Historys;
class searchStack;

enum Stages {
	MAIN_SEARCH, CAPTURES_INIT, GOOD_CAPTURES, KILLERS, COUNTER_MOVE, QUIETS_INIT, QUIET, BAD_CAPTURES,
	Q_SEARCH, CAPTURES_Q_INIT, CAPTURES_Q,
	EVASION, EVASIONS_INIT, EVASIONS_S1,
	STOP
};

template<int index, int index1, typename T = int16_t>
struct StatBoards : public std::array<std::array<T, index1>, index>
{
	void fill(const T& v) {
		T* p = &(*this)[0][0];
		std::fill(p, p + sizeof(*this) / sizeof(*p), v); 
	}

	void update(T& entry, int bonus, const int D) {

		entry += bonus * 32 - entry * abs(bonus) / D;
	}
};

typedef StatBoards<int(COLOR), int(SQ_ALL) * int(SQ_ALL)> ButterflyBoards;

// PieceToBoards are addressed by a move's [piece][to] information
typedef StatBoards<PIECES, SQ_ALL> PieceToBoards;

// Similar to a standard history heuristics,
// but index is color and the numerical representation
// of the move (using only the from to information)
struct ButterflyHistory : public ButterflyBoards 
{
	void update(int color, Move m, int bonus) {
		StatBoards::update( (*this)[color][from_to(m)], bonus, 324); //Bonus value needs to be played with in auto games
	}
};

// History array indexed by piece ID and move to location.  ///TEST IF THIS IS BETTER WITH FROM TO INDEX's instead since we only know 1-6 piece ID
struct PieceToHistory : public PieceToBoards
{
	void update(int piece, int to, int bonus) {
		StatBoards::update( (*this)[piece][to], bonus, 936); //Needs to be value tested
	}
};

// CounterMoveHistory stores counter moves indexed by [piece][to] of the previous
// move, see chessprogramming.wikispaces.com/Countermove+Heuristic
typedef StatBoards<PIECES, SQ_ALL, Move> CounterMoveHistory;

// ContinuationHistory is the history of a given pair of moves, usually the
// current one given a previous one. History table is based on PieceToBoards
// instead of ButterflyBoards.
typedef StatBoards<PIECES, SQ_ALL, PieceToHistory> ContinuationHistory;



class MovePicker
{

public:
	MovePicker(const BitBoards & board, Move ttm, int depth, const ButterflyHistory * hist, const PieceToHistory**, Move cm, Move * killers_p);
	//MovePicker(const BitBoards & board, Move ttm, int depth, const ButterflyHistory * hist, Move cm, Move * killers_p);
	MovePicker(const BitBoards & board, Move ttm, const ButterflyHistory * hist);

	Move nextMove();
	
private:

	template<int genType>
	void score();

	int Stage;

	const BitBoards        & b;
	const ButterflyHistory * mainHist;
	const PieceToHistory   ** contiHistory;
	

	int Depth;
	Move ttMove, counterMove;

	SMove killers[2];
	SMove *current, *end, *endBadCaptures, *endQuiets;
	SMove mList[256];
};
