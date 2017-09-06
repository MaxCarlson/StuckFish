#include "material.h"
#include "bitboards.h"
#include "defines.h"


Material::Table MaterialTable;

const int Linear[6] = { 1852, -162, -1122, -183,  249, -154 };

const int QuadraticSameSide[][PIECES] = {
	//            OUR PIECES
	// pair  pawn  knight bishop rook queen
	{    0                                 }, // Bishop pair
	{   39,     2                          }, // Pawn
	{   35,   271,   -4                    }, // knight      OUR PIECES
	{    0,   105,    4,     0             }, // Bishop
	{  -27,    -2,   46,   100,  -141      }, // Rook
	{ -177,    25,  129,   142,  -137,   0 }  // Queen
};

const int QuadraticOppositeSide[][PIECES] = {
	//           THEIR PIECES
	// pair pawn knight bishop rook queen
	{ 0                                }, // Bishop pair
	{ 37,    0                         }, // Pawn
	{ 10,   62,   0                    }, // Knight      OUR PIECES
	{ 57,   64,  39,     0             }, // Bishop
	{ 50,   40,  23,   -22,    0	   }, // Rook
	{ 98,  105, -39,   141,  274,    0 }  // Queen
};

template<int color>
int materialImbalence(const int pieceCount[][PIECES]) {
	
	const int them = color == WHITE ? BLACK : WHITE;

	int bonus = 0;

	// Second-degree polynomial material imbalance by Tord Romstad
	for (int pt1 = 0; pt1 <= QUEEN; ++pt1) {

		if (!pieceCount[color][pt1]) continue;

		int v = Linear[pt1];

		for (int pt2 = 0; pt2 <= pt1; ++pt2) {

			v += QuadraticSameSide[pt1][pt2] * pieceCount[color][pt2]
				+ QuadraticOppositeSide[pt1][pt2] * pieceCount[them][pt2];

		}

		bonus += pieceCount[color][pt1] * v;
	}

	return bonus;
}

namespace Material{

Entry * probe(const BitBoards& boards, Table& entries)
{
	U64 key = boards.material_key();

	Entry* e = entries[key];

	//if we get a Material hash table hit,
	//ie the two Material keys match
	//return that entry
	if (e->Key == key) return e;

	std::memset(e, 0, sizeof(Entry));
	e->Key = key;
	e->factor[WHITE] = e->factor[BLACK] = (U8)SF_NORMAL;

	e->gamePhase = boards.game_phase();

	//probe end game for material config


	//probe end game for a specific scaling factor




	e->value = (U16)(())
	
}


} //namespace Material