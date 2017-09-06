#pragma once

#include "defines.h"

class BitBoards;

namespace Material {

class Entry
{
public:



	//material hash key
	U64 Key;

	U16 value;
	U8 factor[COLOR];

	//weight we're going to be using for space weight?
	//or just a bool check?
	Scores spaceWeight;


	int gamePhase; //change to Phase struct??
};

typedef HashTable<Entry, 8192> Table; //2000 bug testing?

Entry* probe(const BitBoards & boards, Table& entries); //add endgame bases

}// namespace Material