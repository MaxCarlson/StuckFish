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
		std::fill(p, p + sizeof(*this) / sizeof(*p), v); // Use memset here instad? We don't need to fill
	}

	void update(T& entry, int bonus, const int D) {

		assert(abs(bonus) <= D); // Ensure range is [-32 * D, 32 * D]
		assert(abs(32 * D) < INT16_MAX); // Ensure we don't overflow

		entry += bonus * 32 - entry * abs(bonus) / D;
	}
};

typedef StatBoards<int(COLOR), int(SQ_ALL) * int(SQ_ALL)> ButterflyBoards;

// PieceToBoards are addressed by a move's [piece][to] information
typedef StatBoards<PIECES, SQ_ALL> PieceToBoards;


struct ButterflyHistory : public ButterflyBoards 
{
	void update(int color, Move m, int bonus) {
		StatBoards::update( (*this)[color][from_sq(m)], bonus, 300); ///Bonus value needs to be played with in auto games
	}
};

// History array indexed by piece ID and move to location.  ///TEST IF THIS IS BETTER WITH FROM TO INDEX's instead since we only know 1-6 piece ID
struct PieceToHistory : public PieceToBoards
{
	void update(int piece, int to, int bonus) {
		StatBoards::update( (*this)[piece][to], bonus, 715); //Needs to be value tested
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
