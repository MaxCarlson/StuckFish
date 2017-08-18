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
    U8 flag;
    bool tried;
};

#endif // MOVE_H
