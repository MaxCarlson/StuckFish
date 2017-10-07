#pragma once

#include <intrin.h>
#include <vector>


#include <iostream>


#define NDEBUG

#ifdef NDEBUG
#define assert(EXPRESSION) ((void)0)
#else
#define assert(EXPRESSION) ((EXPRESSION) ? (void)0 : _assert(#EXPRESSION, __FILE__, __LINE__))
#endif

#include <assert.h>

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

/*
void prefetch(char* addr) {

#  if defined(__INTEL_COMPILER)
	// This hack prevents prefetches from being optimized away by
	// Intel compiler. Both MSVC and gcc seem not be affected by this.
	__asm__(""); //this isn't recognized??
#  endif

#  if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	_mm_prefetch(addr, _MM_HINT_T0);
#  else
	__builtin_prefetch(addr);
#  endif
}
*/

// Move represented in 16 bits, idea taken from Stockfish
// 1-6 from square
// 6-12 desitination square
// 13-14 move flag, 1 Castle, 2 EP, 3 Promotion
// 15-16 promotion piece type
enum Move { //later change to move once entire move class has been removed.
	MOVE_NONE
};

// Struct that holds moves and their scores
struct SMove {
	Move move;
	int score;
};

// Removes error from std::algorithm about std::less no overload found.
// Is also usefull in move picking.
inline bool operator<(const SMove& f, const SMove& s) {
	return f.score < s.score;
}

enum MoveType {
	NORMAL,
	CASTLING  = 1 << 12,
	ENPASSANT = 2 << 12,
	PROMOTION = 3 << 12
};



inline Move create_move(int from, int to) {
	return Move(from | (to << 6));
}

inline int from_sq(Move m) {
	return (m & 0x3f);
}

inline int to_sq(Move m) {
	return (m >> 6) & 0x3f;
}

// Returns 0 if the move is normal, 
// and enum values above it is an EP, Castle, or promotion
inline MoveType move_type(Move m) {
	return MoveType(m & (3 << 12));
}

// What piece does the move promote to?
inline int promotion_type(Move m) {
	return ((m >> 15) & 3) + 2;
}

// Create EP, Castle, or promotion move
// Pt is equal to zero for all moves aside from promotions
// where it designates piece type move is promoting to. 
template<MoveType T, int Pt>
inline Move create_special(int from, int to) {
	return Move((from | (to << 6) | T | (Pt ? (Pt - KNIGHT) << 15 : 0LL)));
}




// holds mid and end game values
// for ease of use in evaluations
struct Scores { int mg = 0, eg = 0; };

inline Scores make_scores(int m, int e) {
	Scores x;
	x.mg = m; x.eg = e;
	return x;
}

#define SCORE_ZERO make_scores(0, 0)

//overloaded Scores operators
inline Scores operator-(Scores s1, const Scores s2) {
	s1.mg -= s2.mg;
	s1.eg -= s2.eg;
	return s1;
};

inline void operator-=(Scores& s1, const Scores s2) {
	s1.mg -= s2.mg;
	s1.eg -= s2.eg;
};

inline Scores operator+(Scores s1, const Scores s2) {
	s1.mg += s2.mg;
	s1.eg += s2.eg;
	return s1;
};

inline Scores operator+(Scores s1, int s2) {
	s1.mg += s2;
	s1.eg += s2;
	return s1;
};

inline void operator+=(Scores& s1, const Scores s2) {
	s1.mg += s2.mg;
	s1.eg += s2.eg;
};

inline Scores operator*(Scores s1, const Scores s2) {
	s1.mg *= s2.mg;
	s1.eg *= s2.eg;
	return s1;
}

inline Scores operator*(Scores s1, int s2) {
	s1.mg *= s2;
	s1.eg *= s2;
	return s1;
}

inline void operator*=(Scores& s1, const Scores s2) {
	s1.mg *= s2.mg;
	s1.eg *= s2.eg;
}

inline Scores operator/(Scores s1, const Scores s2) {
	s1.mg /= s2.mg;
	s1.eg /= s2.eg;
	return s1;
}

inline Scores operator/(Scores s1, int s2) {
	s1.mg /= s2;
	s1.eg /= s2;
	return s1;
}

inline void operator/=(Scores& s1, const Scores s2) {
	s1.mg /= s2.mg;
	s1.eg /= s2.eg;
}


//squares from whites POV
enum esqare {
	A8 = 0,  B8, C8, D8, E8, F8, G8, H8,
	A7 = 8,  B7, C7, D7, E7, F7, G7, H7,
	A6 = 16, B6, C6, D6, E6, F6, G6, H6,
	A5 = 24, B5, C5, D5, E5, F5, G5, H5,
	A4 = 32, B4, C4, D4, E4, F4, G4, H4,
	A3 = 40, B3, C3, D3, E3, F3, G3, H3,
	A2 = 48, B2, C2, D2, E2, F2, G2, H2,
	A1 = 56, B1, C1, D1, E1, F1, G1, H1,
	SQ_NONE,
	SQ_ALL = 64
};

//piece types
enum epiece {
    PIECE_EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
	PIECES
};

/*************************************************
* Values used for sorting captures are the same  *
* as normal piece values, except for a king.     *
*************************************************/
const int SORT_VALUE[7] = { 0, 100, 325, 335, 500, 975, 0 };

// Used to determine which type of 
// move generation we want
#define MAIN_GEN 0
#define CAPTURES 1
#define EVASIONS 2
#define QUIETS   3
#define LEGAL    4
#define PERFT_TESTING 5

#define PAWN_VAL 100 //should pieces have a mid and end game value??
#define KNIGHT_VAL 325
#define BISHOP_VAL 335
#define ROOK_VAL 500
#define QUEEN_VAL 975
#define KING_VAL 0 //change later??


#define DRAW_OPENING -10 //re define these? //DELETE??
#define DRAW_ENDGAME 0
#define END_GAME_MAT 1300

#define MIDGAME_LIMIT 6090 //values subject to serious changes!!!!!!!!!!
#define ENDGAME_LIMIT 1475


//game colors.
//COLOR used for arrays containing
//both colors
enum ecolor {
    WHITE,
    BLACK,
    COLOR
};


#define KING_SIDE  0
#define QUEEN_SIDE 1

// castling sematics
// taken from Stockfish
enum Castling {
	NO_CASTLING,
	WHITE_OO,
	WHITE_OOO    = WHITE_OO << 1,
	BLACK_OO     = WHITE_OO << 2,
	BLACK_OOO    = WHITE_OO << 3,
	ANY_CASTLING = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO,
	CASTLING_RIGHTS = 16
};

//mid and end game indices
enum GameStage {
	mg,
	eg,
	STAGE
};

enum ScaleFactor {
	SF_DRAW = 0,
	SF_ONEPAWN = 48,
	SF_NORMAL = 64,
	SF_MAX = 128,
	SF_NONE = 255
};

typedef unsigned char U8;
typedef char S8;
typedef unsigned short int U16;
typedef short int S16;
typedef unsigned int U32;
typedef int S32;
typedef unsigned long long U64;
typedef long long S64;


//used for TTables that aren't our main TT
template<class Entry, int Size>
struct HashTable {
	HashTable() : table(Size, Entry()) {}
	Entry* operator[](U64 k) { return &table[(uint32_t)k & (Size - 1)]; }

private:
	std::vector<Entry> table;
};

const U64 RankMasks8[8] = //from rank 1 - 8 
{
	0xFF00000000000000L, 0xFF000000000000L,  0xFF0000000000L, 0xFF00000000L, 0xFF000000L, 0xFF0000L, 0xFF00L, 0xFFL,
};


const U64 FileMasks8[8] =/*from fileA to FileH*/
{
	0x101010101010101L, 0x202020202020202L, 0x404040404040404L, 0x808080808080808L,
	0x1010101010101010L, 0x2020202020202020L, 0x4040404040404040L, 0x8080808080808080L
};



#define TT_ALPHA 1
#define TT_BETA 2
#define TT_EXACT 3

#define SORT_KING 400000000
#define SORT_HASH 200000000
#define SORT_CAPT 100000000
#define SORT_PROM  90000000
#define SORT_KILL  80000000


#define MAX_PLY 128

#define DEPTH_QS 0
#define DEPTH_QS_NO_CHECK -2 //add qs checks at somepoint, negative values cause errors right now



#define INF 32001
#define VALUE_MATE 32000
#define VALUE_MATE_IN_MAX_PLY 31872
#define VALUE_MATED_IN_MAX_PLY -31872

#define INVALID 32767


//CACHE_LINE_ALIGNMENT

//also values correspond to actual squares changed when
// wanting to shift a board 1 over in the listed direction
enum Directions {
	N  = - 8,
	NE = - 7,
	NW = - 9,
	W  = - 1,
	S  =   8,
	SW =   7,
	SE =   9,
	E  =   1 
};

//shifts a bitboard in any of the enum directions above. Must use enum values in template
template<int sh> //does not hold west and east shifts, make another that does?
inline U64 shift_bb(U64 b) { 
	return  sh == N ? b >> 8 : sh == S ? b << 8
		: sh == NE ? (b & ~FileHBB) >> 7 : sh == SE ? (b & ~FileHBB) << 9
		: sh == NW ? (b & ~FileABB) >> 9 : sh == SW ? (b & ~FileABB) << 7
		: 0LL;
}

//return least significant bit location
inline int lsb(unsigned long long b) {
	unsigned long idx;
	_BitScanForward64(&idx, b);
	return (int)idx;
}
//most significant bit location
inline int msb(unsigned long long b) {
	unsigned long idx;
	_BitScanReverse64(&idx, b);
	return (int)idx;
}

//finds and pops the least significant bit from the board, pass it a refrence
inline int pop_lsb(unsigned long long* b) {
	const int s = lsb(*b);
	*b &= *b - 1;
	return s;
}

//counts number of non zero bits in a bitboard
inline int bit_count(unsigned long long b) {
	return _mm_popcnt_u64(b);
}

//is there more than one bit set on the board?
inline bool more_than_one(unsigned long long b) {
	return b & (b - 1);
}

//returns file of square
inline int file_of(int sq) {
	return sq & 7;
}

// Returns the correct rank of the square for our bitboard layout.
// Ranking starts from a ZERO index!!!
inline int rank_of(int sq) {
	return (sq >> 3) ^ 7;
}

//distance from one file/rank to another
inline int file_distance(int s1, int s2) {
	return abs(file_of(s1) - file_of(s2));
}

inline int rank_distance(int s1, int s2) {
	return abs(rank_of(s1) - rank_of(s2));
}

//returns a relative rank from an input rank
inline int relative_rank(int color, int rank) { 
	return (rank ^ (color * 7));
}
//returns a relative rank from a square location input
inline int relative_rankSq(int color, int square) {
	return relative_rank(color, rank_of(square));
}

//returns same square if color == white,
//if black it returns the relative square number.
//e.g: relative_square(BLACK, H1(63)) == H8(7)
inline int relative_square(int color, int square) {
	return (square ^ (color * 56));
}

//return what a square + pawn_push,
//would equal the correct square to move for a pawn
//of that color. e.g: sq 48 + pawn_push(WHITE) = sq 40;
inline int pawn_push(int color) {
	return color == WHITE ? -8 : 8;
}

//returns the most advanced piece on the board,
//relative to side to move
inline int frontmost_sq(int color, U64 b) 
{
	return color == WHITE ? lsb(b) : msb(b);
	
}
inline int backmost_sq(int color, U64 b)
{ 
	return color == WHITE ? msb(b) : lsb(b);
}

//these functions return appropriate mate values for the current ply
//prefering to mate sooner than later
inline int mate_in(int ply) {
	return VALUE_MATE - ply;
}
//^^ prefering to be mated later than sooner
inline int mated_in(int ply) {
	return -VALUE_MATE + ply;
}




