#ifndef AI_LOGIC_H
#define AI_LOGIC_H
#include <string>
#include <thread>
#include "slider_attacks.h"
#include "defines.h"
#include "MovePicker.h"


class BitBoards;
class evaluateBB;
class HashEntry;


namespace Search {

	const int counterMovePruneThreshold = 0;

	// Holds info about past and current plys of
	// current search.
	struct searchStack {
		int ply;
		int moveCount;
		Move killers[2];
		Move currentMove;
		Move excludedMove;

		PieceToHistory * contiHistory;

		int reduction;
		int staticEval;
		int statScore;
	};

	// Struct holds info about how to search,
	// when we started, and sometimes when to stop.
	struct SearchControls {

		// Should be be using time managment?
		bool use_time() const {
			return !(depth | infinite);
		}

		SearchControls() {
			time[WHITE] = time[BLACK] = inc[WHITE] = inc[BLACK] = depth = movesToGo = infinite = 0;
		}

		long time[COLOR];
		int  inc[COLOR], depth, movesToGo, infinite;
		TimePoint startTime;
	};

	extern SearchControls SearchControl;

	void initSearch();
	void clear();
	int searchRoot(BitBoards & board, int depth, int alpha, int beta, searchStack * ss);

	int quiescent(BitBoards & board, int alpha, int beta, searchStack * ss, int isPV);


	void updateContinuationHistories(searchStack * ss, int piece, int to, int bonus);

	void updateStats(const BitBoards & board, Move move, searchStack * ss, int depth, Move * quiets, int qCount, int bonus);
	void checkInput();
	void print(int bestScore, int depth);

	template<bool Root>
	U64 perft(BitBoards & board, int depth);

	template<bool Root>
	U64 perftDivide(BitBoards & board, int depth);

}// namespace

#endif // AI_LOGIC_H

