#ifndef MOVE_H
#define MOVE_H

#include "defines.h"

class Move
{
public:
    const Move();

    U8 from;
    U8 to;
    char piece;
    char captured;
	int score;
    U8 flag;
    bool tried;
};

inline Move::Move()
{
	tried = false;
}

FORCE_INLINE bool operator ==(const Move& m, const Move& m1) {
	return ((m.from == m1.from) && (m.to == m1.to)
		&& (m.piece == m1.piece));
}

#endif // MOVE_H
