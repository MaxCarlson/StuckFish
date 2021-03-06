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

		Move * pv;
		Move killers[2];
		Move currentMove;
		Move excludedMove;

		PieceToHistory * contiHistory;

		int ply;
		int moveCount;
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

	// Each RootMove holds a pv and
	// is used as a base for the *ss->pv.
	// Also used for sorting root moves
	// and deciding on search order. 
	struct RootMove {
		explicit RootMove(Move m) : pv(1, m) {}
		bool operator==(const Move& m) const { return pv[0] == m;  }

		bool operator<(const RootMove & m) const {
			return m.score != score 
				           ? m.score < score 
				           : m.previousScore < previousScore;
		}

		int score = -INF;
		int previousScore = -INF;

		std::vector<Move> pv;
	};
	typedef std::vector<RootMove> RootMoves;



	void initSearch();
	void clear();
	int searchRoot(BitBoards & board, int depth, int alpha, int beta, searchStack * ss);

	void updateContinuationHistories(searchStack * ss, int piece, int to, int bonus);

	void updateStats(const BitBoards & board, Move move, searchStack * ss,  Move * quiets, int qCount, int bonus);

	void print(int bestScore, int depth, const BitBoards &board);

	void perftInit(BitBoards & board, bool isDivide, int depth);
}// namespace

#endif // AI_LOGIC_H

