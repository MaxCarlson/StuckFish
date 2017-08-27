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
        for(int pieceType = 0; pieceType < 6; pieceType ++){
            for(int square = 0; square < 64; square ++){
				//fill the zero spot with 0LL's so we can XOR captures without worrying about if there is one
				if (pieceType == 0) zArray[color][pieceType][square] = 0LL;

				else zArray[color][pieceType][square] = random64();
            }
        }

    }
    /*
    //enpassant and castle filling below

    for (int column = 0; column < 8; column++)
    {
        zEnPassant[column] = random64();
    }
    */

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

void ZobristH::UpdateKey(int start, int end, const Move& moveKey, bool isWhite)
{
    //gather piece, capture, and w or b info from movekey
    //normal move
    int piece = moveKey.piece;
    int captured = moveKey.captured;

    //XOR zobristKey with zArray number at piece start end then end location
    //if piece is white..
    if(isWhite) {
		switch (piece) {
		case PAWN:
			//if normal pawn move
			if (moveKey.flag == '0') {
				zobristKey ^= zArray[0][PAWN][start];
				zobristKey ^= zArray[0][PAWN][end];
			}
			else { //if pawn promotion
				zobristKey ^= zArray[0][PAWN][start];
				zobristKey ^= zArray[0][QUEEN][end]; //only handles queen promotions atm
			}
			break;
		default: //default for rest of pieces, castling handled down below
			zobristKey ^= zArray[0][piece][start];
			zobristKey ^= zArray[0][piece][end];
			break;
		} 
		zobristKey ^= zArray[1][captured][end];
    //black
    } else {
		switch (piece) {
		case PAWN:
			if (moveKey.flag == '0') {
				zobristKey ^= zArray[1][PAWN][start];
				zobristKey ^= zArray[1][PAWN][end];
			}
			else {
				zobristKey ^= zArray[1][PAWN][start];
				zobristKey ^= zArray[1][QUEEN][end];
			}
			break;
		default:
			zobristKey ^= zArray[1][piece][start];
			zobristKey ^= zArray[1][piece][end];
			break;

		} 
		zobristKey ^= zArray[0][captured][end];
    }

	//need caslting code

}

U64 ZobristH::fetchKey(const Move & m, bool isWhite)
{
	//get an idea of what most keys will be after moves..
	//so we can prefetch that info
	int color = 0;
	U64 key = zobristKey;

	if (!isWhite) color = 1;

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
        if(((BBBoard.BBWhitePawns >> square) & 1) == 1)
        {
            //XOR the zkey with the U64 in the white pawns square
            //that was generated from rand64
            returnZKey ^= zArray[0][1][square];
        }
        else if(((BBBoard.BBBlackPawns >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][1][square];
        }
        //white pieces
        else if(((BBBoard.BBWhiteKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][2][square];
        }
        else if(((BBBoard.BBWhiteBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][3][square];
        }
		else if (((BBBoard.BBWhiteRooks >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][4][square];
		}
        else if(((BBBoard.BBWhiteQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][5][square];
        }
        else if(((BBBoard.BBWhiteKing >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][6][square];
        }

        //black pieces       
        else if(((BBBoard.BBBlackKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][2][square];
        }
        else if(((BBBoard.BBBlackBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][3][square];
        }
		else if (((BBBoard.BBBlackRooks >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][4][square];
		}
        else if(((BBBoard.BBBlackQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][5][square];
        }
        else if(((BBBoard.BBBlackKing >> square) & 1) == 1)
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
        if(((BBBoard.BBWhitePawns >> square) & 1) == 1)
        {
            //XOR the zkey with the U64 in the white pawns square
            //that was generated from rand64
            returnZKey ^= zArray[0][1][square];
        }
        else if(((BBBoard.BBBlackPawns >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][1][square];
        }
        //white pieces
        else if(((BBBoard.BBWhiteKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][2][square];
        }
        else if(((BBBoard.BBWhiteBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][3][square];
        }
		else if (((BBBoard.BBWhiteRooks >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][4][square];
		}
        else if(((BBBoard.BBWhiteQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][5][square];
        }
        else if(((BBBoard.BBWhiteKing >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][6][square];
        }

        //black pieces
        else if(((BBBoard.BBBlackKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][2][square];
        }
        else if(((BBBoard.BBBlackBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][3][square];
        }
		else if (((BBBoard.BBBlackRooks >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][4][square];
		}
        else if(((BBBoard.BBBlackQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][5][square];
        }
        else if(((BBBoard.BBBlackKing >> square) & 1) == 1)
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



























