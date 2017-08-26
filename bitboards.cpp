#include "bitboards.h"
#include <cmath>
#include <random>
#include <iostream>
#include <cstdio>
#include "externs.h"
#include "zobristh.h"

class Move;

std::string boardArr[8][8] = {
	{ "r", "n", "b", "q", "k", "b", "n", "r" },
	{ "p", "p", "p", "p", "p", "p", "p", "p", },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ "P", "P", "P", "P", "P", "P", "P", "P" },
	{ "R", "N", "B", "Q", "K", "B", "N", "R" },
};

//used for flipping rank to whites relative rank
const int flipRank[8] = { 8, 7, 6, 5, 4, 3 , 2, 1 };
/*
namespace zobrist {
	//array holding U64 numbers for changing zobrist keys
	U64 pieceKeys[2][7][64]; //color, piece (0 is no piece), square
	//used to denote a castling has taken place for zobrist; wqs, wks, bqs, bks
	U64 zCastle[4];
	U64 zEnPassant[8];
	U64 color;
}

void BitBoards::initializeZobrist(ZobristH zobrist)
{

	//fill zorbist array with random unisgned 64 bit ints
	for (int color = 0; color < 2; color++) {
		for (int pieceType = 0; pieceType < 7; pieceType++) {
			for (int square = 0; square < 64; square++) {
				//fill no pieces with 0LL so when theres no capture master key isn't muddled by XOR
				if (pieceType == 0) zobrist::pieceKeys[color][pieceType][square] = 0LL;

				zobrist::pieceKeys[color][pieceType][square] = zobrist.random64();
			}
		}

	}
	
	//enpassant and castle filling below

	for (int column = 0; column < 8; column++)
	{
	zobrist::zEnPassant[column] = zobrist.random64();
	}
	

	for (int i = 0; i < 4; i++)
	{
		zobrist::zCastle[i] = zobrist.random64(); //white queen side, white king side, black qs, bks
	}

	//random is it blacks turn or not
	zobrist::color = zobrist.random64();
	
}

void BitBoards::UpdateKey(int start, int end, const Move& moveKey, bool isWhite)
{
	//gather piece, capture, and w or b info from movekey
	//normal move
	int piece = moveKey.piece;
	int captured = moveKey.captured;

	//XOR zobristKey with zArray number at piece start end then end location
	//if piece is white..
	if (isWhite) {
		switch (piece) {
		case PAWN:
			//if normal pawn move
			if (moveKey.flag == '0') {
				Key ^= zArray[0][PAWN][start];
				Key ^= zArray[0][PAWN][end];
			}
			else { //if pawn promotion
				Key ^= zArray[0][PAWN][start];
				Key ^= zArray[0][QUEEN][end]; //only handles queen promotions atm
			}
			break;
		default: //default for rest of pieces, castling handled down below
			Key ^= zArray[0][piece][start];
			Key ^= zArray[0][piece][end];
			break;
		}
		//captures (a non capture will be 0, which will XOR key with 0LL)
		Key ^= zArray[0][captured][end];		
	}
	//black
	else {
		switch (piece) {
		case PAWN:
			if (moveKey.flag == '0') {
				Key ^= zArray[1][PAWN][start];
				Key ^= zArray[1][PAWN][end];
			}
			else {
				Key ^= zArray[1][PAWN][start];
				Key ^= zArray[1][QUEEN][end];
			}
			break;
		default:
			Key ^= zArray[1][piece][start];
			Key ^= zArray[1][piece][end];
			break;

		}
		Key ^= zArray[1][captured][end];
	}

	//need caslting code
}

void BitBoards::updateKeyColor()
{
	Key ^= color;
}

U64 BitBoards::fetchKey(const Move & m, bool isWhite)
{
	//get an idea of what most keys will be after moves..
	//so we can prefetch that info
	int color = 0;

	if (!isWhite) color = 1;

	Key ^= zArray[color][m.piece][m.from];
	Key ^= zArray[color][m.piece][m.to];

	Key ^= zArray[!color][m.captured][m.to]; //update if there's a capture

	Key ^= zBlackMove; //update color

	return Key; //return key estimate
}

U64 BitBoards::getZobristHash()
{
	U64 returnZKey = 0LL;
	for (int square = 0; square < 64; square++) {
		//white and black pawns
		//if there is a white pawn on i square
		if (((BBWhitePawns >> square) & 1) == 1)
		{
			//XOR the zkey with the U64 in the white pawns square
			//that was generated from rand64
			returnZKey ^= zArray[0][1][square];
		}
		else if (((BBBlackPawns >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][1][square];
		}
		//white pieces
		else if (((BBWhiteKnights >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][2][square];
		}
		else if (((BBWhiteBishops >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][3][square];
		}
		else if (((BBWhiteRooks >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][4][square];
		}
		else if (((BBWhiteQueens >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][5][square];
		}
		else if (((BBWhiteKing >> square) & 1) == 1)
		{
			returnZKey ^= zArray[0][6][square];
		}

		//black pieces       
		else if (((BBBlackKnights >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][2][square];
		}
		else if (((BBBlackBishops >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][3][square];
		}
		else if (((BBBlackRooks >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][4][square];
		}
		else if (((BBBlackQueens >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][5][square];
		}
		else if (((BBBlackKing >> square) & 1) == 1)
		{
			returnZKey ^= zArray[1][6][square];
		}
	}
	//EnPassant and castling stuff add later

	Key = returnZKey;

	return returnZKey;
}
*/

void BitBoards::constructBoards()
{
    FullTiles = 0LL;

    BBWhitePawns = 0LL;
    BBWhitePieces = 0LL;
    BBWhiteRooks = 0LL;
    BBWhiteKnights = 0LL;
    BBWhiteBishops = 0LL;
    BBWhiteQueens = 0LL;
    BBWhiteKing = 0LL;

    BBBlackPieces = 0LL;
    BBBlackPawns = 0LL;
    BBBlackRooks = 0LL;
    BBBlackKnights = 0LL;
    BBBlackBishops = 0LL;
    BBBlackQueens = 0LL;
    BBBlackKing = 0LL;

	//reset side material values
	sideMaterial[0] = 0; //white
	sideMaterial[1] = 0;

	for (int i = 0; i < 4; ++i) {
		rookMoved[i] = false;
		castled[i] = false;
	}
	

    //seed bitboards
    for(int i = 0; i < 64; i++){
        if(boardArr[i/8][i%8] == "P"){
            BBWhitePawns += 1LL<<i;
            BBWhitePieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[0] += PAWN_VAL;
        } else if(boardArr[i/8][i%8] == "R"){
            BBWhiteRooks += 1LL<<i;
            BBWhitePieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[0] += ROOK_VAL;
        }else if(boardArr[i/8][i%8] == "N"){
            BBWhiteKnights += 1LL<<i;
            BBWhitePieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[0] += KNIGHT_VAL;
        }else if(boardArr[i/8][i%8] == "B"){
            BBWhiteBishops += 1LL<<i;
            BBWhitePieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[0] += BISHOP_VAL;
        }else if(boardArr[i/8][i%8] == "Q"){
            BBWhiteQueens += 1LL<<i;
            BBWhitePieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[0] += QUEEN_VAL;
        }else if(boardArr[i/8][i%8] == "K"){
            BBWhiteKing += 1LL<<i;
            BBWhitePieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[0] += KING_VAL;
        } else if(boardArr[i/8][i%8] == "p"){
            BBBlackPawns += 1LL<<i;
            BBBlackPieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[1] += PAWN_VAL;
        } else if(boardArr[i/8][i%8] == "r"){
            BBBlackRooks += 1LL<<i;
            BBBlackPieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[1] += ROOK_VAL;
        }else if(boardArr[i/8][i%8] == "n"){
            BBBlackKnights += 1LL<<i;
            BBBlackPieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[1] += KNIGHT_VAL;
        }else if(boardArr[i/8][i%8] == "b"){
            BBBlackBishops += 1LL<<i;
            BBBlackPieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[1] += BISHOP_VAL;
        }else if(boardArr[i/8][i%8] == "q"){
            BBBlackQueens += 1LL<<i;
            BBBlackPieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[1] += QUEEN_VAL;
        }else if(boardArr[i/8][i%8] == "k"){
            BBBlackKing += 1LL<<i;
            BBBlackPieces += 1LL<<i;
            FullTiles += 1LL<<i;
			sideMaterial[1] += KING_VAL;
        }
    }

    //mark empty tiles with 1's
    EmptyTiles = ~FullTiles;

}

void BitBoards::makeMove(const Move &move, bool isWhite)
{
    U8 xyI, xyE;
    //move coordinates, later replace x,y x1,y1 with from/to coordinates to remove math
    //xyI = move.y * 8 + move.x;
    //xyE = move.y1 * 8 + move.x1;
    xyI = move.from;
    xyE = move.to;

    //inital spot piece mask and end spot mask
    U64 pieceMaskI = 1LL << xyI;
    U64 pieceMaskE = 1LL << xyE;

    //find BB that contains correct piece, remove piece from it's starting pos

	if ((EmptyTiles & BBWhitePieces) | (EmptyTiles & BBBlackPieces)) {
		drawBBA();
	}

    //white pieces
    if(isWhite){
        switch(move.piece){
        case PAWN:
            //remove piece from starting loc
            BBWhitePawns &= ~pieceMaskI;
            //remove piece from color BB
            BBWhitePieces &= ~pieceMaskI;
            //remove piece from full tiles
            FullTiles &= ~pieceMaskI;

            if(move.flag != 'Q'){
                //add piece to landing spot
                BBWhitePawns |= pieceMaskE;

            //if it's a pawn promotion
            } else {
                //add queen to landing spot
                BBWhiteQueens |= pieceMaskE;
				//adjust maaterial on board
				sideMaterial[0] -= PAWN_VAL;
				sideMaterial[0] += QUEEN_VAL;
            }
            //add to color pieces then full tiles
            BBWhitePieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

        case KNIGHT:
            BBWhiteKnights &= ~pieceMaskI;
            BBWhitePieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBWhiteKnights |= pieceMaskE;
            //add to color pieces then full tiles
            BBWhitePieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

        case BISHOP:
            BBWhiteBishops &= ~pieceMaskI;
            BBWhitePieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBWhiteBishops |= pieceMaskE;
            //add to color pieces then full tiles
            BBWhitePieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;
		
		case ROOK:
			//remove piece, test/save capture
			BBWhiteRooks &= ~pieceMaskI;
			BBWhitePieces &= ~pieceMaskI;
			FullTiles &= ~pieceMaskI;
			//add piece
			BBWhiteRooks |= pieceMaskE;
			//add to color pieces then full tiles
			BBWhitePieces |= pieceMaskE;
			FullTiles |= pieceMaskE;
			if (move.flag == A1 + 1) rookMoved[0] = true;
			else if (move.flag == H1 + 1) rookMoved[1] = true;
			break;

        case QUEEN:
            BBWhiteQueens &= ~pieceMaskI;
            BBWhitePieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBWhiteQueens |= pieceMaskE;
            //add to color pieces then full tiles
            BBWhitePieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

        case KING:
            BBWhiteKing &= ~pieceMaskI;
            BBWhitePieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBWhiteKing |= pieceMaskE;
            //add to color pieces then full tiles
            BBWhitePieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;
        }
    //black pieces
    } else {
        switch(move.piece){
        case PAWN:
            BBBlackPawns &= ~pieceMaskI;
            BBBlackPieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;

            if(move.flag != 'Q'){
                //add piece to landing spot
                BBBlackPawns |= pieceMaskE;

            //if it's a pawn promotion
            } else {
                //add queen to landing spot
                BBBlackQueens |= pieceMaskE;
				sideMaterial[1] -= PAWN_VAL;
				sideMaterial[1] += QUEEN_VAL;
            }
            //add to color pieces then full tiles
            BBBlackPieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

        case KNIGHT:
            BBBlackKnights &= ~pieceMaskI;
            BBBlackPieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBBlackKnights |= pieceMaskE;
            //add to color pieces then full tiles
            BBBlackPieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

        case BISHOP:
            BBBlackBishops &= ~pieceMaskI;
            BBBlackPieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBBlackBishops |= pieceMaskE;
            //add to color pieces then full tiles
            BBBlackPieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

		case ROOK:
			BBBlackRooks &= ~pieceMaskI;
			BBBlackPieces &= ~pieceMaskI;
			FullTiles &= ~pieceMaskI;
			//add piece
			BBBlackRooks |= pieceMaskE;
			//add to color pieces then full tiles
			BBBlackPieces |= pieceMaskE;
			FullTiles |= pieceMaskE;
			if (move.flag == A8 + 1) rookMoved[2] = true;
			else if (move.flag == H8 + 1) rookMoved[3] = true;

			break;

        case QUEEN:
            BBBlackQueens &= ~pieceMaskI;
            BBBlackPieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBBlackQueens |= pieceMaskE;
            //add to color pieces then full tiles
            BBBlackPieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;

        case KING:
            BBBlackKing &= ~pieceMaskI;
            BBBlackPieces &= ~pieceMaskI;
            FullTiles &= ~pieceMaskI;
            //add piece
            BBBlackKing |= pieceMaskE;
            //add to color pieces then full tiles
            BBBlackPieces |= pieceMaskE;
            FullTiles |= pieceMaskE;
            break;
        }
    }

    //remove captured piece from BB's
    if(move.captured != PIECE_EMPTY) removeCapturedPiece(!isWhite, move.captured, pieceMaskE);


    //correct empty tiles to opposite of full tiles
    EmptyTiles &= ~pieceMaskE;
    EmptyTiles |= pieceMaskI;


    if((EmptyTiles & BBWhitePieces) | (EmptyTiles & BBBlackPieces)){
        drawBBA();
    }

    //Update zobrist hash
    zobrist.UpdateKey(xyI, xyE, move, isWhite);
    //change zobrist color after a move
    zobrist.UpdateColor();
}

void BitBoards::unmakeMove(const Move &moveKey, bool isWhite)
{
    //extract from to data
    U8 xyI = moveKey.from;
    U8 xyE = moveKey.to;
    //inital spot piece mask and end spot mask
    U64 pieceMaskI = 1LL<< xyI;
    U64 pieceMaskE = 1LL << xyE;

    if(isWhite){
        switch(moveKey.piece){
            case PAWN:
            //if move not a promotion
            if(moveKey.flag == '0'){
                //remove piece from where it landed
                BBWhitePawns &= ~pieceMaskE;
            //promotion unmake
            } else {
                BBWhiteQueens &= ~pieceMaskE;
				sideMaterial[0] += PAWN_VAL;
				sideMaterial[0] -= QUEEN_VAL;

            }
            //put it back where it started
            BBWhitePawns |= pieceMaskI;
            //change color boards same way
            BBWhitePieces &= ~pieceMaskE;
            BBWhitePieces |= pieceMaskI;
            break;

            case KNIGHT:
            BBWhiteKnights &= ~pieceMaskE;
            BBWhiteKnights |= pieceMaskI;
            BBWhitePieces &= ~pieceMaskE;
            BBWhitePieces |= pieceMaskI;
            break;

            case BISHOP:
            BBWhiteBishops &= ~pieceMaskE;
            BBWhiteBishops |= pieceMaskI;
            BBWhitePieces &= ~pieceMaskE;
            BBWhitePieces |= pieceMaskI;
            break;

			case ROOK:
			BBWhiteRooks &= ~pieceMaskE;
			BBWhiteRooks |= pieceMaskI;
			BBWhitePieces &= ~pieceMaskE;
			BBWhitePieces |= pieceMaskI;
			if (moveKey.flag == A1 + 1) rookMoved[0] = false;
			else if (moveKey.flag == H1 + 1) rookMoved[1] = false;
			break;

            case QUEEN:
            BBWhiteQueens &= ~pieceMaskE;
            BBWhiteQueens |= pieceMaskI;
            BBWhitePieces &= ~pieceMaskE;
            BBWhitePieces |= pieceMaskI;
            break;

            case KING:
            BBWhiteKing &= ~pieceMaskE;
            BBWhiteKing |= pieceMaskI;
            BBWhitePieces &= ~pieceMaskE;
            BBWhitePieces |= pieceMaskI;
            break;
        }
    } else {
        switch(moveKey.piece){
            case PAWN:
            if(moveKey.flag == '0'){
                //remove piece from where it landed
                BBBlackPawns &= ~pieceMaskE;
            //promotion unmake
            } else {
                BBBlackQueens &= ~pieceMaskE;
				sideMaterial[1] += PAWN_VAL;
				sideMaterial[1] -= QUEEN_VAL;
            }
            //put it back where it started
            BBBlackPawns |= pieceMaskI;
            //change color boards same way
            BBBlackPieces &= ~pieceMaskE;
            BBBlackPieces |= pieceMaskI;
            break;

            case ROOK:
            BBBlackRooks &= ~pieceMaskE;
            BBBlackRooks |= pieceMaskI;
            BBBlackPieces &= ~pieceMaskE;
            BBBlackPieces |= pieceMaskI;
            if(moveKey.flag == A8+1) rookMoved[2] = false;
            else if(moveKey.flag == H8+1) rookMoved[3] = false;
            break;

            case KNIGHT:
            BBBlackKnights &= ~pieceMaskE;
            BBBlackKnights |= pieceMaskI;
            BBBlackPieces &= ~pieceMaskE;
            BBBlackPieces |= pieceMaskI;
            break;

            case BISHOP:
            BBBlackBishops &= ~pieceMaskE;
            BBBlackBishops |= pieceMaskI;
            BBBlackPieces &= ~pieceMaskE;
            BBBlackPieces |= pieceMaskI;
            break;

            case QUEEN:
            BBBlackQueens &= ~pieceMaskE;
            BBBlackQueens |= pieceMaskI;
            BBBlackPieces &= ~pieceMaskE;
            BBBlackPieces |= pieceMaskI;
            break;

            case KING:
            BBBlackKing &= ~pieceMaskE;
            BBBlackKing |= pieceMaskI;
            BBBlackPieces &= ~pieceMaskE;
            BBBlackPieces |= pieceMaskI;
            break;

        }
    }

    //correct full tiles and run unmake capture function
    //if a piece has been captured ///Might have errors in calling unmake
    //even when a piece hasn't been captured
    if(isWhite){
        if(moveKey.captured == PIECE_EMPTY){
            FullTiles &= ~pieceMaskE;
            FullTiles |= pieceMaskI;
        } else{
            undoCapture(pieceMaskE, moveKey.captured, !isWhite);
            FullTiles |= pieceMaskI;
        }
    } else {
        if(moveKey.captured == PIECE_EMPTY){
            FullTiles &= ~pieceMaskE;
            FullTiles |= pieceMaskI;
        } else{
            undoCapture(pieceMaskE, moveKey.captured, !isWhite);
            FullTiles |= pieceMaskI;
        }
    }

    //correct empty tiles to opposite of full tiles
    EmptyTiles = ~FullTiles;

    //update zobrist hash
    zobrist.UpdateKey(xyI, xyE, moveKey, isWhite);

    //change cobrist color after a move
    zobrist.UpdateColor();
}

void BitBoards::undoCapture(U64 location, char piece, bool isNotWhite)
{

    if(isNotWhite){
        switch(piece){
            case PAWN:
                //restore piece to both piece board and color board
                //no need to change FullTiles as captured piece was already there
                BBWhitePawns |= location;
                BBWhitePieces |= location;
				//adjust side material balence
				sideMaterial[0] += PAWN_VAL;
                break;            
            case KNIGHT:
                BBWhiteKnights |= location;
                BBWhitePieces |= location;
				sideMaterial[0] += KNIGHT_VAL;
                break;
            case BISHOP:
                BBWhiteBishops |= location;
                BBWhitePieces |= location;
				sideMaterial[0] += BISHOP_VAL;
                break;
			case ROOK:
				BBWhiteRooks |= location;
				BBWhitePieces |= location;
				sideMaterial[0] += ROOK_VAL;
				break;
            case QUEEN:
                BBWhiteQueens |= location;
                BBWhitePieces |= location;
				sideMaterial[0] += QUEEN_VAL;
                break;
            case KING:
                BBWhiteKing |= location;
                BBWhitePieces |= location;
				sideMaterial[0] += KING_VAL;
                break;
            default:
                std::cout << "UNDO CAPTURE ERROR" << std::endl;
        }
    } else {
        switch(piece){
            case PAWN:
                //restore piece to both piece board and color board
                //no need to change FullTiles as captured piece was already there
                BBBlackPawns |= location;
                BBBlackPieces |= location;
				sideMaterial[1] += PAWN_VAL;
                break;
            case KNIGHT:
                BBBlackKnights |= location;
                BBBlackPieces |= location;
				sideMaterial[1] += KNIGHT_VAL;
                break;
            case BISHOP:
                BBBlackBishops |= location;
                BBBlackPieces |= location;
				sideMaterial[1] += BISHOP_VAL;
                break;
			case ROOK:
				BBBlackRooks |= location;
				BBBlackPieces |= location;
				sideMaterial[1] += ROOK_VAL;
				break;
            case QUEEN:
                BBBlackQueens |= location;
                BBBlackPieces |= location;
				sideMaterial[1] += QUEEN_VAL;
                break;
            case KING:
                BBBlackKing |= location;
                BBBlackPieces |= location;
				sideMaterial[1] += KING_VAL;
                break;
            default:
                drawBBA();
                std::cout << "UNDO CAPTURE ERROR" << std::endl;
        }
    }

}

void BitBoards::drawBB(U64 board)
{
    for(int i = 0; i < 64; i++){
        if(board & (1ULL << i)){
            std::cout<< 1 <<", ";
        } else {
            std::cout << 0 << ", ";
        }
        if((i+1)%8 == 0){
            std::cout<< std::endl;
        }
    }
    std::cout<< std::endl;
}

void BitBoards::drawBBA()
{
    char flips[8] = {'8', '7', '6', '5', '4', '3', '2', '1'};
    char flipsL[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    int c = 0;

    for(int i = 0; i < 64; i++){
        if((i)%8 == 0){
            std::cout<< std::endl;
            std::cout << flips[c] << " | ";
            c++;
        }

        if(BBWhitePawns & (1ULL<<i)){
            std::cout << "P" << ", ";
        }
        if(BBWhiteRooks & (1ULL<<i)){
            std::cout << "R" << ", ";
        }
        if(BBWhiteKnights & (1ULL<<i)){
            std::cout << "N" << ", ";
        }
        if(BBWhiteBishops & (1ULL<<i)){
            std::cout << "B" << ", ";
        }
        if(BBWhiteQueens & (1ULL<<i)){
            std::cout << "Q" << ", ";
        }
        if(BBWhiteKing & (1ULL<<i)){
            std::cout << "K" << ", ";
        }
        if(BBBlackPawns & (1ULL<<i)){
            std::cout << "p" << ", ";
        }
        if(BBBlackRooks & (1ULL<<i)){
            std::cout << "r" << ", ";
        }
        if(BBBlackKnights & (1ULL<<i)){
            std::cout << "n" << ", ";
        }
        if(BBBlackBishops & (1ULL<<i)){
            std::cout << "b" << ", ";
        }
        if(BBBlackQueens & (1ULL<<i)){
            std::cout << "q" << ", ";
        }
        if(BBBlackKing & (1ULL<<i)){
            std::cout << "k" << ", ";
        }
        if(EmptyTiles & (1ULL<<i)){
            std::cout << " " << ", ";
        }

        //if(i % 8 == 7){
        //    std::cout << "| " << flips[c] ;
       //     c++;
       // }
    }

    std::cout << std::endl << "    ";
    for(int i = 0; i < 8; i++){
        std::cout << flipsL[i] << "  ";
    }

    std::cout << std::endl << std::endl;;
}

void BitBoards::removeCapturedPiece(bool isWhite, char captured, U64 location)
{
	if (isWhite) {
		switch (captured) {
		case PAWN:
			BBWhitePawns &= ~location;
			BBWhitePieces &= ~location;
			sideMaterial[0] -= PAWN_VAL;
			break;
		case KNIGHT:
			BBWhiteKnights &= ~location;
			BBWhitePieces &= ~location;
			sideMaterial[0] -= KNIGHT_VAL;
			break;
		case BISHOP:
			BBWhiteBishops &= ~location;
			BBWhitePieces &= ~location;
			sideMaterial[0] -= BISHOP_VAL;
			break;
		case ROOK:
			BBWhiteRooks &= ~location;
			BBWhitePieces &= ~location;
			sideMaterial[0] -= ROOK_VAL;
			break;
		case QUEEN:
			BBWhiteQueens &= ~location;
			BBWhitePieces &= ~location;
			sideMaterial[0] -= QUEEN_VAL;
			break;
		case KING:
			BBWhiteKing &= ~location;
			BBWhitePieces &= ~location;
			sideMaterial[0] -= KING_VAL;
			break;
		}
	}
	else {
		switch(captured){
		case PAWN:
			BBBlackPawns &= ~location;
			BBBlackPieces &= ~location;
			sideMaterial[1] -= PAWN_VAL;
			break;
		case KNIGHT:
			BBBlackKnights &= ~location;
			BBBlackPieces &= ~location;
			sideMaterial[1] -= KNIGHT_VAL;
			break;
		case BISHOP:
			BBBlackBishops &= ~location;
			BBBlackPieces &= ~location;
			sideMaterial[1] -= BISHOP_VAL;
			break;
		case ROOK:
			BBBlackRooks &= ~location;
			BBBlackPieces &= ~location;
			sideMaterial[1] -= ROOK_VAL;
			break;
		case QUEEN:
			BBBlackQueens &= ~location;
			BBBlackPieces &= ~location;
			sideMaterial[1] -= QUEEN_VAL;
			break;
		case KING:
			BBBlackKing &= ~location;
			BBBlackPieces &= ~location;
			sideMaterial[1] -= KING_VAL;
			break;
		default:
			std::cout << "Remove Captured Piece on make move ERROR" << std::endl;
		}
	}
}

int BitBoards::relativeRank(int sq, bool isWhite)
{ //return reletive rank for side to move.
	sq /= 8;
	return isWhite ? flipRank[sq] : sq + 1;
}











