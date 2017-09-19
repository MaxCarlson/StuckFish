#include "zobristh.h"
#include "externs.h"
#include "movegen.h"

#include "bitboards.h"

#include <cmath>
#include <random>
#include <iostream>


//random 64 bit number generation
std::random_device rd;
std::mt19937_64 mt(rd());
std::uniform_int_distribution<U64> dist(std::llround(std::pow(2,61)), std::llround(std::pow(2,62)));



U64 ZobristH::random64()
{
    //get random 64 bit integer to seed zorbist arrays
    U64 ranUI = dist(mt);
    return ranUI;
}

void ZobristH::zobristFill()
{

    //fill zorbist array with random unisgned 64 bit ints
    for(int color = 0; color < 2; color++){
        for(int pieceType = 0; pieceType <= KING; pieceType ++){
            for(int square = 0; square < 64; square ++){
				//fill the zero spot with 0LL's so we can XOR captures without worrying about if there is one
				if (pieceType == 0) zArray[color][pieceType][square] = 0LL;

				else zArray[color][pieceType][square] = random64();
            }
        }

    }
    
    //enpassant and castle filling below
    for (int column = 0; column < 8; column++)
    {
        zEnPassant[column] = random64();
    }
    

    for (int i = 0; i < 4; i++)
    {
        zCastle[i] = random64(); //white queen side, white king side, black qs, bks
    }

    //random is it blacks turn or not
    zBlackMove = random64();
}

void ZobristH::UpdateColor()
{
    zobristKey ^= zBlackMove;
}

U64 ZobristH::fetchKey(const Move & m, int color)
{
	//get an idea of what most keys will be after moves..
	//so we can prefetch that info

	U64 key = zobristKey;

	key ^= zArray[color][m.piece][m.from];
	key ^= zArray[color][m.piece][m.to];

	key ^= zArray[!color][m.captured][m.to]; //update if there's a capture

	key ^= zBlackMove; //update color

	return key; //return key estimate
}

U64 ZobristH::getZobristHash(const BitBoards& BBBoard)
{
    U64 returnZKey = 0LL;
    for (int square = 0; square < 64; square++){
        //if tile is empty skip to next i
        /*
        if(((EmptyTiles >> square) & 1) == 1){
            continue;
        }
        */
        //white and black pawns
        //if there is a white pawn on i square
        if(((BBBoard.byColorPiecesBB[0][1] >> square) & 1) == 1)
        {
            //XOR the zkey with the U64 in the white pawns square
            //that was generated from rand64
            returnZKey ^= zArray[0][1][square];
        }
        else if(((BBBoard.byColorPiecesBB[1][1] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][1][square];
        }
        //white pieces
        else if(((BBBoard.byColorPiecesBB[0][2] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][2][square];
        }
        else if(((BBBoard.byColorPiecesBB[0][3] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][3][square];
        }
		else if (((BBBoard.byColorPiecesBB[0][4] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][4][square];
		}
        else if(((BBBoard.byColorPiecesBB[0][5] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][5][square];
        }
        else if(((BBBoard.byColorPiecesBB[0][6] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][6][square];
        }

        //black pieces       
        else if(((BBBoard.byColorPiecesBB[1][2] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][2][square];
        }
        else if(((BBBoard.byColorPiecesBB[1][3] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][3][square];
        }
		else if (((BBBoard.byColorPiecesBB[1][4] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][4][square];
		}
        else if(((BBBoard.byColorPiecesBB[1][5] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][5][square];
        }
        else if(((BBBoard.byColorPiecesBB[1][6] >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][6][square];
        }
    }
    //EnPassant and castling stuff add later

    zobristKey = returnZKey;

    return returnZKey;
}

void ZobristH::testDistibution()
{
    const int sampleSize = 2000;
    int distArray[sampleSize] = {};
    int t = 0;
    while (t < 1500)
    {
       for (int i = 0; i < 2000; i++)
       {
           distArray[(int)(random64() % sampleSize)]++;
       }
       t++;
    }
    for (int i = 0; i < sampleSize; i++)
    {
        std::cout << distArray[i] << std::endl;
    }

}

U64 ZobristH::debugKey(bool isWhite, const BitBoards& BBBoard)
{
    U64 returnZKey = 0LL;
    for (int square = 0; square < 64; square++){

        //if there is a white pawn on i square
		if (((BBBoard.byColorPiecesBB[0][1] >> square) & 1) == 1)
		{
			//XOR the zkey with the U64 in the white pawns square
			//that was generated from rand64
			returnZKey ^= zArray[0][1][square];
		}
		else if (((BBBoard.byColorPiecesBB[1][1] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][1][square];
		}
		//white pieces
		else if (((BBBoard.byColorPiecesBB[0][2] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][2][square];
		}
		else if (((BBBoard.byColorPiecesBB[0][3] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][3][square];
		}
		else if (((BBBoard.byColorPiecesBB[0][4] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][4][square];
		}
		else if (((BBBoard.byColorPiecesBB[0][5] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][5][square];
		}
		else if (((BBBoard.byColorPiecesBB[0][6] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][6][square];
		}

		//black pieces       
		else if (((BBBoard.byColorPiecesBB[1][2] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][2][square];
		}
		else if (((BBBoard.byColorPiecesBB[1][3] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][3][square];
		}
		else if (((BBBoard.byColorPiecesBB[1][4] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][4][square];
		}
		else if (((BBBoard.byColorPiecesBB[1][5] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][5][square];
		}
		else if (((BBBoard.byColorPiecesBB[1][6] >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][6][square];
		}
    }
    //EnPassant and castling stuff add later


    //if it isn't whites turn, XOR test zobrist key with black move U64
    if(isWhite == false){
        returnZKey ^= zBlackMove;
    }

    return returnZKey;
}

U64 ZobristH::debugPawnKey(const BitBoards & BBBoard)
{
	U64 pkey = 0LL;
	for (int s = 0; s < 64; ++s) {
		if (((BBBoard.byColorPiecesBB[WHITE][PAWN] >> s ) & 1) == 1) {
			pkey ^= zArray[WHITE][PAWN][s];
		}

		if (((BBBoard.byColorPiecesBB[BLACK][PAWN] >> s) & 1) == 1 ) {
			pkey ^= zArray[BLACK][PAWN][s];
		}
	}
	return pkey;
}

U64 ZobristH::debugMaterialKey(const BitBoards & BBBoard)
{
	U64 mkey = 0LL;
	for (int color = 0; color < COLOR; ++color) {
		for (int pt = PAWN; pt <= KING; ++pt) {
			for (int count = 0; count <= BBBoard.pieceCount[color][pt]; ++count) {

				//XOR material key with piece count instead of square location
				//so we can have a Material Key that represents what material is on the board.
				mkey ^= zArray[color][pt][count];
			}
		}
	}
	return mkey;
}



























