#pragma once

#include "defines.h"

class BitBoards;

namespace Pawns {
	const U64 DarkSquares = 0x55aa55aa55aa55aaULL;
	const U64 LightSquares = 0xAA55AA55AA55AA55ULL;

	struct PawnsEntry
	{
	public:

		int pawns_on_same_color_squares(int color, int sq) const {
			return pawnsOnSquares[color][!!(DarkSquares & (1LL << sq))];
		}
		int semi_open_file(int color, int file) const {
			return semiOpenFiles[color] & (1LL << file);
		}

		//holds the key of this pawn entry, used for pawn TT lookups
		U64 Key;

		//holds midgame and end game scores for this entry
		Scores score; 
		//pawn piece square table values by color
		Scores pieceSqTabScores[COLOR];
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

	typedef HashTable<PawnsEntry, 16384> Table;

	PawnsEntry* probe(const BitBoards& boards);
}