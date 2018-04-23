#include "material.h"
#include "bitboards.h"
#include "defines.h"
#include "Thread.h"


//Material::Table MaterialTable;

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
	for (int pt1 = PIECE_EMPTY; pt1 <= QUEEN; ++pt1) {

		if (!pieceCount[color][pt1]) continue;

		int v = Linear[pt1];

		for (int pt2 = PIECE_EMPTY; pt2 <= pt1; ++pt2) {

			v += QuadraticSameSide[pt1][pt2]     * pieceCount[color][pt2]
			  +  QuadraticOppositeSide[pt1][pt2] *  pieceCount[them][pt2];

		}

		bonus += pieceCount[color][pt1] * v;
	}

	return bonus;
}

namespace Material{

Entry * probe(const BitBoards& boards)
{
	U64 key = boards.material_key();

	Entry* e = boards.this_thread()->materialTable[key];

	//if we get a Material hash table hit,
	//ie the two Material keys match
	//return that entry
	if (e->Key == key) return e;

	std::memset(e, 0, sizeof(Entry));
	e->Key = key;
	e->factor[WHITE] = e->factor[BLACK] = (U8)SF_NORMAL;
	e->centerWeight = SCORE_ZERO;
	e->gamePhase = boards.game_phase();

	//material value of side, excluding queen
	int whiteNonPawnMat = boards.non_pawn_material(WHITE);
	int blackNonPawnMat = boards.non_pawn_material(BLACK);

	//probe end game for material config


	//probe end game for a specific scaling factor


	//compute center control weight, based on total number and type of pieces.
	if (whiteNonPawnMat + blackNonPawnMat >= 2 * QUEEN_VAL + 4 * ROOK_VAL + 2 * KNIGHT_VAL) {

		int minorPieceCount = boards.count<KNIGHT>(WHITE) + boards.count<BISHOP>(WHITE)
						    + boards.count<KNIGHT>(BLACK) + boards.count<BISHOP>(BLACK);
		//midgame score only
		e->centerWeight = make_scores(minorPieceCount * minorPieceCount, 0);
	}


	// Evaluate the material imbalance. We use PIECE_EMPTY(0) as a place holder
	// for the bishop pair "extended piece", which allows us to be more flexible
	// in defining bishop pair bonuses.

	const int pieceCount[COLOR][PIECES] = {

	   { boards.count<BISHOP>(WHITE) > 1, boards.count<PAWN>(WHITE), boards.count<KNIGHT>(WHITE),
		 boards.count<BISHOP>(WHITE)    , boards.count<ROOK>(WHITE), boards.count<QUEEN>(WHITE)   },
	   { boards.count<BISHOP>(BLACK) > 1, boards.count<PAWN>(BLACK), boards.count<KNIGHT>(BLACK),
		 boards.count<BISHOP>(BLACK)    , boards.count<ROOK>(BLACK), boards.count<QUEEN >(BLACK)  } 
	};

	e->value = (S16)((materialImbalence<WHITE>(pieceCount) - materialImbalence<BLACK>(pieceCount)) / 16);
	
	return e;
}


} //namespace Material