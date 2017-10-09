#ifndef MOVEGEN_H
#define MOVEGEN_H

typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

#include <iostream>
#include <string>
#include "defines.h"


class BitBoards;
class Historys;
class HashEntry;
class searchStack;

template<int genType>
SMove* generate(const BitBoards & board, SMove *mlist);


// Wrapper for move generation,
// Not used in MovePicker.
template<int genType>
struct MoveList {
	
	explicit MoveList(const BitBoards & board) : last(generate<genType>(board, mList)) {}
	const SMove * begin() const { return mList; }
	const SMove *   end() const { return  last; }
	size_t size() const { return last - mList; }
	bool contains(Move m) const {
		return std::find(begin(), end(), m) != end();
	}

private:
	SMove mList[256], *last;
};

#endif // MOVEGEN_H
