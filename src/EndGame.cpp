#include "EndGame.h"

// Table used to drive the king towards the edge of the board
// in KX vs K and KQ vs KR endgames.
const int PushToEdges[64] = {
	100, 90, 80, 70, 70, 80, 90, 100,
	90, 70, 60, 50, 50, 60, 70,  90,
	80, 60, 40, 30, 30, 40, 60,  80,
	70, 50, 30, 20, 20, 30, 50,  70,
	70, 50, 30, 20, 20, 30, 50,  70,
	80, 60, 40, 30, 30, 40, 60,  80,
	90, 70, 60, 50, 50, 60, 70,  90,
	100, 90, 80, 70, 70, 80, 90, 100,
};

// Table used to drive the king towards a corner square of the
// right color in KBN vs K endgames.
const int PushToCorners[64] = {
	200, 190, 180, 170, 160, 150, 140, 130,
	190, 180, 170, 160, 150, 140, 130, 140,
	180, 170, 155, 140, 140, 125, 140, 150,
	170, 160, 140, 120, 110, 140, 150, 160,
	160, 150, 140, 110, 120, 140, 160, 170,
	150, 140, 125, 140, 140, 155, 170, 180,
	140, 130, 140, 150, 160, 170, 180, 190,
	130, 140, 150, 160, 170, 180, 190, 200
};

EndGame::EndGame()
{
}


EndGame::~EndGame()
{
}
