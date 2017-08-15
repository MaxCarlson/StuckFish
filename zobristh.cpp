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



ZobristH::ZobristH()
{

}

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
                zArray[color][pieceType][square] = random64();
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
    zNullMove = random64(); //not needed?
}

void ZobristH::UpdateColor()
{
    zobristKey ^= zBlackMove;
}

void ZobristH::UpdateNull()
{
    zobristKey ^= zNullMove;
}

void ZobristH::UpdateKey(int start, int end, Move moveKey, bool isWhite)
{
    //gather piece, capture, and w or b info from movekey
    //normal move
    char piece = moveKey.piece;
    char captured = moveKey.captured;

    //if a piece was captured XOR that location with randomkey at array location end
    if (isWhite && captured != PIECE_EMPTY){
		switch (captured) {
		case PAWN:
			zobristKey ^= zArray[1][0][end];
			break;
		case KNIGHT:
			zobristKey ^= zArray[1][2][end];
			break;
		case BISHOP:
			zobristKey ^= zArray[1][3][end];
			break;
		case ROOK:
			zobristKey ^= zArray[1][1][end];
			break;
		case QUEEN:
			zobristKey ^= zArray[1][4][end];
			break;
		case KING:
			zobristKey ^= zArray[1][5][end];
			break;
		}        
        
    } else if (!isWhite && captured != PIECE_EMPTY) {
		switch (captured) {
		case PAWN:
			zobristKey ^= zArray[0][0][end];
			break;
		case KNIGHT:
			zobristKey ^= zArray[0][2][end];
			break;
		case BISHOP:
			zobristKey ^= zArray[0][3][end];
			break;
		case ROOK:
			zobristKey ^= zArray[0][1][end];
			break;
		case QUEEN:
			zobristKey ^= zArray[0][4][end];
			break;
		case KING:
			zobristKey ^= zArray[0][5][end];
			break;
		}                
    }

    //XOR zobristKey with zArray number at piece start end then end location
    //if piece is white..
    if(isWhite) {
		switch (piece) {
		case PAWN:
			//if normal pawn move
			if (moveKey.flag == '0') {
				zobristKey ^= zArray[0][0][start];
				zobristKey ^= zArray[0][0][end];				
			}
			else { //if pawn promotion
				zobristKey ^= zArray[0][0][start];
				zobristKey ^= zArray[0][4][end];
			}
			break;
		case KNIGHT:
			zobristKey ^= zArray[0][2][start];
			zobristKey ^= zArray[0][2][end];
			break;
		case BISHOP:
			zobristKey ^= zArray[0][3][start];
			zobristKey ^= zArray[0][3][end];
			break;
		case ROOK:
			zobristKey ^= zArray[0][1][start];
			zobristKey ^= zArray[0][1][end];
			break;
		case QUEEN:
			zobristKey ^= zArray[0][4][start];
			zobristKey ^= zArray[0][4][end];
			break;
		case KING:
			zobristKey ^= zArray[0][5][start];
			zobristKey ^= zArray[0][5][end];
			break;
		}      
    //black
    } else {
		switch (piece) {
		case PAWN:
			if (moveKey.flag == '0') {
				zobristKey ^= zArray[1][0][start];
				zobristKey ^= zArray[1][0][end];
			}
			else {
				zobristKey ^= zArray[1][0][start];
				zobristKey ^= zArray[1][4][end];
			}
			break;
		case KNIGHT:
			zobristKey ^= zArray[1][2][start];
			zobristKey ^= zArray[1][2][end];
			break;
		case BISHOP:
			zobristKey ^= zArray[1][3][start];
			zobristKey ^= zArray[1][3][end];
			break;
		case ROOK:
			zobristKey ^= zArray[1][1][start];
			zobristKey ^= zArray[1][1][end];
			break;
		case QUEEN:
			zobristKey ^= zArray[1][4][start];
			zobristKey ^= zArray[1][4][end];
			break;
		case KING:
			zobristKey ^= zArray[1][5][start];
			zobristKey ^= zArray[1][5][end];
			break;
		}     
    }
}

U64 ZobristH::getZobristHash(BitBoards BBBoard)
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
            returnZKey ^= zArray[0][0][square];
        }
        else if(((BBBoard.BBBlackPawns >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][0][square];
        }
        //white pieces
        else if(((BBBoard.BBWhiteRooks >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][1][square];
        }
        else if(((BBBoard.BBWhiteKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][2][square];
        }
        else if(((BBBoard.BBWhiteBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][3][square];
        }
        else if(((BBBoard.BBWhiteQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][4][square];
        }
        else if(((BBBoard.BBWhiteKing >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][5][square];
        }

        //black pieces
        else if(((BBBoard.BBBlackRooks >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][1][square];
        }
        else if(((BBBoard.BBBlackKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][2][square];
        }
        else if(((BBBoard.BBBlackBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][3][square];
        }
        else if(((BBBoard.BBBlackQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][4][square];
        }
        else if(((BBBoard.BBBlackKing >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][5][square];
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

U64 ZobristH::debugKey(bool isWhite, BitBoards BBBoard)
{
    U64 returnZKey = 0LL;
    for (int square = 0; square < 64; square++){

        //if there is a white pawn on i square
        if(((BBBoard.BBWhitePawns >> square) & 1) == 1)
        {
            //XOR the zkey with the U64 in the white pawns square
            //that was generated from rand64
            returnZKey ^= zArray[0][0][square];
        }
        else if(((BBBoard.BBBlackPawns >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][0][square];
        }
        //white pieces
        else if(((BBBoard.BBWhiteRooks >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][1][square];
        }
        else if(((BBBoard.BBWhiteKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][2][square];
        }
        else if(((BBBoard.BBWhiteBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][3][square];
        }
        else if(((BBBoard.BBWhiteQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][4][square];
        }
        else if(((BBBoard.BBWhiteKing >> square) & 1) == 1)
        {
            returnZKey ^= zArray[0][5][square];
        }

        //black pieces
        else if(((BBBoard.BBBlackRooks >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][1][square];
        }
        else if(((BBBoard.BBBlackKnights >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][2][square];
        }
        else if(((BBBoard.BBBlackBishops >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][3][square];
        }
        else if(((BBBoard.BBBlackQueens >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][4][square];
        }
        else if(((BBBoard.BBBlackKing >> square) & 1) == 1)
        {
            returnZKey ^= zArray[1][5][square];
        }
    }
    //EnPassant and castling stuff add later


    //if it isn't whites turn, XOR test zobrist key with black move U64
    if(isWhite == false){
        returnZKey ^= zBlackMove;
    }

    return returnZKey;
}



























