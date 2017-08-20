#ifndef DEFINES_H
#define DEFINES_H

#include <intrin.h>

#define ENGINE_NAME "StuckFish 0.1"

#define CACHE_LINE_SIZE 64 //taken from stockfish
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#  define CACHE_LINE_ALIGNMENT __declspec(align(CACHE_LINE_SIZE))
#else
#  define CACHE_LINE_ALIGNMENT  __attribute__ ((aligned(CACHE_LINE_SIZE)))
#endif

#ifdef _MSC_VER
#  define FORCE_INLINE  __forceinline
#elif defined(__GNUC__)
#  define FORCE_INLINE  inline __attribute__((always_inline))
#else
#  define FORCE_INLINE  inline
#endif

enum esqare {
    A8=0,  B8, C8, D8, E8, F8, G8, H8,
    A7=8,  B7, C7, D7, E7, F7, G7, H7,
    A6=16, B6, C6, D6, E6, F6, G6, H6,
    A5=24, B5, C5, D5, E5, F5, G5, H5,
    A4=32, B4, C4, D4, E4, F4, G4, H4,
    A3=40, B3, C3, D3, E3, F3, G3, H3,
    A2=48, B2, C2, D2, E2, F2, G2, H2,
    A1=56, B1, C1, D1, E1, F1, G1, H1,
};

enum epiece {
    PIECE_EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

#define PAWN_VAL 100
#define KNIGHT_VAL 325
#define BISHOP_VAL 335
#define ROOK_VAL 500
#define QUEEN_VAL 975
#define KING_VAL 0 //change later??

enum ecolor {
    WHITE,
    BLACK,
    COLOR_EMPTY
};

/*
//used for chronos flags
enum etimef {
	FTIME = 1,
	FINC = 2,
	FMOVESTOGO = 4,
	FDEPTH = 8,
	FNODES = 16,
	FMATE = 32,
	FMOVETIME = 64,
	FINFINITE = 128
};
*/


typedef unsigned char U8;
typedef char S8;
typedef unsigned short int U16;
typedef short int S16;
typedef unsigned int U32;
typedef int S32;
typedef unsigned long long U64;
typedef long long S64;

#define MOVE_NONE Move n //not usable?

#define TT_ALPHA 1
#define TT_BETA 2
#define TT_EXACT 3

#define SORT_KING 400000000
#define SORT_HASH 200000000
#define SORT_CAPT 100000000
#define SORT_PROM  90000000
#define SORT_KILL  80000000

//#define INF 10000

#define MAX_PLY 128

#define DRAW_OPENING -10
#define DRAW_ENDGAME 0
#define END_GAME_MAT 1300

#define DEPTH_QS 0
#define DEPTH_QS_NO_CHECK -1 //add qs checks at somepoint


#define VALUE_MATE 32000
#define INF 32001
#define VALUE_MATE_IN_MAX_PLY 31872
#define VALUE_MATED_IN_MAX_PLY -38172

#define INVALID 32767

enum Value { //taken from stockfish
	/*
	VALUE_ZERO = 0,
	//VALUE_DRAW = 0,

	VALUE_MATE = 32000,
	INF = 32001,
	VALUE_NONE = 32002,

	VALUE_MATE_IN_MAX_PLY = VALUE_MATE - MAX_PLY,
	VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + MAX_PLY,
	*/

	PawnValueMg = 198, PawnValueEg = 258, //NOT USING STOCKFISH VALUES AT THE MOMENT, MAY USE LATER
	KnightValueMg = 817, KnightValueEg = 846,
	BishopValueMg = 836, BishopValueEg = 857,
	RookValueMg = 1270, RookValueEg = 1278,
	QueenValueMg = 2521, QueenValueEg = 2558,

	MidgameLimit = 15581, EndgameLimit = 3998
};

//CACHE_LINE_ALIGNMENT

inline int lsb(unsigned long long b) {
	unsigned long idx;
	_BitScanForward64(&idx, b);
	return (int)idx;
}

inline int msb(unsigned long long b) {
	unsigned long idx;
	_BitScanReverse64(&idx, b);
	return (int)idx;
}

inline int mate_in(int ply) {
	return VALUE_MATE - ply;
}

inline int mated_in(int ply) {
	return -VALUE_MATE + ply;
}



#endif // DEFINES_H
