#ifndef DEFINES_H
#define DEFINES_H


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

enum ecolor {
    WHITE,
    BLACK,
    COLOR_EMPTY
};

typedef unsigned char U8;
typedef char S8;
typedef unsigned short int U16;
typedef short int S16;
typedef unsigned int U32;
typedef int S32;
typedef unsigned long long U64;
typedef long long S64;


#define TT_ALPHA 1
#define TT_BETA 2
#define TT_EXACT 3

#define SORT_KING 400000000
#define SORT_HASH 200000000
#define SORT_CAPT 100000000
#define SORT_PROM  90000000
#define SORT_KILL  80000000

#define INF 10000
#define INVALID 32767

#endif // DEFINES_H
