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

	//holds info about past and current searches, passed to search
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
	void initSearch();
	void clear();
	int searchRoot(BitBoards & board, int depth, int alpha, int beta, searchStack * ss);

	int quiescent(BitBoards & board, int alpha, int beta, searchStack * ss, int isPV);


	void updateContinuationHistories(searchStack * ss, int piece, int to, int bonus);

	void updateStats(const BitBoards & board, Move move, searchStack * ss, int depth, Move * quiets, int qCount, int bonus);
	void checkInput();
	void print(bool isWhite, int bestScore);

	template<bool Root>
	U64 perft(BitBoards & board, int depth);

	template<bool Root>
	U64 perftDivide(BitBoards & board, int depth);
}

#endif // AI_LOGIC_H

