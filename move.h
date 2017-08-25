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
		&& (m.piece == m1.piece) && (m.flag == m1.flag));
}

FORCE_INLINE bool operator !=(const Move& m, const Move& m1) {
	//if all the conditions are equal, return false (or they are equal),
	//else return true they are not equal.
	return ((m.from == m1.from) && (m.to == m1.to)
		&& (m.piece == m1.piece) && (m.flag == m1.flag)) ? false : true;
}

#endif // MOVE_H
