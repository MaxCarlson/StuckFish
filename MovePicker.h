#pragma once

#include "defines.h"
#include "bitboards.h"

#include <array>


class Historys;
class searchStack;

enum Stages {
	MAIN_M,    CAPTURES_M, KILLERS_M, QUIETS_M, QUIETS_M1, BAD_CAPTURES_M,
	QSEARCH_,  CAPTURES_Q, 
	EVASIONS_, EVASIONS_S1,
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
		StatBoards::update( (*this)[color][from_sq(m)], bonus, 200);
	}
};

// History array indexed by piece ID and move to location.  ///TEST IF THIS IS BETTER WITH FROM TO INDEX's instead since we only know 1-6 piece ID
struct PieceToHistory : public PieceToBoards
{
	void update(int piece, int to, int bonus) {
		StatBoards::update( (*this)[piece][to], bonus, 800);
	}
};

typedef StatBoards<PIECES, SQ_ALL, Move> CounterMoveHistory;



class MovePicker
{

public:
	MovePicker(const BitBoards & board, Move ttm, int depth, const Historys & hist, Move * killers_p);
	MovePicker(const BitBoards & board, Move ttm, const Historys & hist);

	Move nextMove();
	
private:

	template<int genType>
	void score();

	int Stage;

	void generateNextStage();

	const BitBoards   & b;
	const Historys    & h;
	const searchStack * s;

	int Depth;
	Move ttMove;

	SMove killers[2];
	SMove *current, *end, *endBadCaptures, *endQuiets;
	SMove mList[256];
};
						