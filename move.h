#ifndef MOVE_H
#define MOVE_H

#include "defines.h"

class Move
{
public:
    Move();

    U8 from;
    U8 to;
    char piece;
    char captured;
    int score;
    char flag;
    bool tried;

    /*
    char id;
    char from;
    char to;
    U8 piece_from;
    U8 piece_to;
    U8 piece_cap;
    char flags;
    char castle;
    char ply;
    char ep;
    int score;
    */

};

#endif // MOVE_H
