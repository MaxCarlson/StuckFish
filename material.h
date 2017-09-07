#pragma once

#include "defines.h"

class BitBoards;

namespace Material {

class Entry
{
public:

	Scores material_value() { return make_scores(value, value); };

	//material hash key
	U64 Key;

	S16 value;
	U8 factor[COLOR];

	//just how important is center control
	//during this phase?
	Scores centerWeight;


	int gamePhase; //change to Phase struct??
};

typedef HashTable<Entry, 8192> Table; //2000 bug testing?

Entry* probe(const BitBoards & boards, Table& entries); //add endgame bases

}// namespace Material