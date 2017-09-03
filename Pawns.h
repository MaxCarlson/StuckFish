#pragma once

#include "defines.h"

class BitBoards;



class Pawns
{
public:

	//holds the key of this pawn entry, used for pawn TT lookups
	U64 Key;

	//holds midgame and end game scores for this entry
	int score[STAGE];

	//mask of passed pawns
	U64 passedPawns[COLOR];
	U64 candidatePawns[COLOR]; //unsure if going to use
	//holds all pawn attacks of a color
	U64 pawnAttacks[COLOR]; 

	int kingSquares[COLOR];
	//king safety scores for each color
	int kingSafety[COLOR];

	int semiOpenFiles[COLOR];
	int pawnSpan[COLOR];
	int pawnsOnSquares[COLOR][COLOR]; //color - white/black squares
};

typedef HashTable<Pawns, 16384> Table;

Pawns* probe(const BitBoards& boards, Table& entries);
