#ifndef DEFINES_H
#define DEFINES_H

#include <intrin.h>
#include <vector>

#define ENGINE_NAME "StuckFish 0.2"

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

//holds mid and end game values
struct Scores { int mg = 0, eg = 0; };

inline Scores make_scores(int m, int e) {
	Scores x;
	x.mg = m; x.eg = e;
	return x;
}

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

inline Scores operator+(Scores s1, const int s2) {
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

inline Scores operator*(Scores s1, const int s2) {
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



enum esqare {
    A8=0,  B8, C8, D8, E8, F8, G8, H8,
    A7=8,  B7, C7, D7, E7, F7, G7, H7,
    A6=16, B6, C6, D6, E6, F6, G6, H6,
    A5=24, B5, C5, D5, E5, F5, G5, H5,
    A4=32, B4, C4, D4, E4, F4, G4, H4,
    A3=40, B3, C3, D3, E3, F3, G3, H3,
    A2=48, B2, C2, D2, E2, F2, G2, H2,
    A1=56, B1, C1, D1, E1, F1, G1, H1,
	SQ_NONE
};

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

#define PAWN_VAL 100
#define KNIGHT_VAL 325
#define BISHOP_VAL 335
#define ROOK_VAL 500
#define QUEEN_VAL 975
#define KING_VAL 0 //change later??

enum ecolor {
    WHITE,
    BLACK,
    COLOR
};

//mid and end game indices
enum GameStage {
	mg,
	eg,
	STAGE
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


//used for TTables that aren't our main TT
template<class Entry, int Size>
struct HashTable {
	HashTable() : table(Size, Entry()) {}
	Entry* operator[](U64 k) { return &table[(uint32_t)k & (Size - 1)]; }

private:
	std::vector<Entry> table;
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

#define DRAW_OPENING -10
#define DRAW_ENDGAME 0
#define END_GAME_MAT 1300

#define DEPTH_QS 0
#define DEPTH_QS_NO_CHECK -2 //add qs checks at somepoint, negative values cause errors right now



#define INF 32001
#define VALUE_MATE 32000
#define VALUE_MATE_IN_MAX_PLY 31872
#define VALUE_MATED_IN_MAX_PLY -31872

#define INVALID 32767


//CACHE_LINE_ALIGNMENT


enum Directions {
	N,
	NE,
	NW,
	W,
	S,
	SW,
	SE,
	E
};

//shifts a bitboard in any of the enum directions above. Must use enum values in template
template<int sh> //does not hold west and east shifts, make another that does?
inline U64 shift_bb(U64 b) { //is shift correct? test
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

//returns the correct rank of the square for our bitboard layout.
//with the XOR 7 it returns the reverse correct
inline int rank_of(int sq) {
	return (sq >> 3) ^ 7;
}

inline int file_distance(int s1, int s2) {
	return abs(file_of(s1) - file_of(s2));
}

inline int rank_distance(int s1, int s2) {
	return abs(rank_of(s1) - rank_of(s2));
}

//returns a relative rank from an input rank
inline int relative_rank(int color, int rank) { 
	return (rank ^ (!color * 7));
}
//returns a relative rank from a square location input
inline int relative_rankSq(int color, int square) {
	return relative_rank(color, rank_of(square));
}

inline int pawn_push(int color) {
	return color == WHITE ? -8 : 8;
}

//returns the most advanced piece on the board,
//relative to side to move
inline int frontmost_sq(int color, U64 b) 
{
	return color == WHITE ? lsb(b) : msb(b);
	
}
inline int  backmost_sq(int color, U64 b)
{ 
	return color == WHITE ? msb(b) : lsb(b);
}

//these functions return appropriate mate values for the current ply
//prefering to mate sooner than later
inline int mate_in(int ply) {
	return VALUE_MATE - ply;
}
//^^
inline int mated_in(int ply) {
	return -VALUE_MATE + ply;
}




#endif // DEFINES_H
