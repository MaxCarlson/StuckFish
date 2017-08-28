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

void BitBoards::constructBoards()
{

	for (int i = 0; i < 4; ++i) {
		rookMoved[i] = false;
		castled[i] = false;
	}

	//reset piece and color BitBoards to 0;
	for (int i = 0; i < 2; ++i) {
		allPiecesColorBB[i] = 0LL;
		//reset side material values
		sideMaterial[i] = 0; 

		for (int j = 0; j < 7; ++j) {
			byColorPiecesBB[i][j] = 0LL;
			byPieceType[j] = 0LL;
		}
	}

	FullTiles = 0LL;

	//seed bitboards
	for (int i = 0; i < 64; i++) {
		squareBB[i] = 1LL << i;

		if (boardArr[i / 8][i % 8] == "P") {
			byColorPiecesBB[0][1] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[1] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[0] += PAWN_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "N") {
			byColorPiecesBB[0][2] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[2] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[0] += KNIGHT_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "B") {
			byColorPiecesBB[0][3] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[3] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[0] += BISHOP_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "R") {
			byColorPiecesBB[0][4] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[4] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[0] += ROOK_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "Q") {
			byColorPiecesBB[0][5] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[5] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[0] += QUEEN_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "K") {
			byColorPiecesBB[0][6] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[6] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[0] += KING_VAL;
		}
		//black pieces
		else if (boardArr[i / 8][i % 8] == "p") {
			byColorPiecesBB[1][1] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[1] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[1] += PAWN_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "n") {
			byColorPiecesBB[1][2] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[2] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[1] += KNIGHT_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "b") {
			byColorPiecesBB[1][3] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[3] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[1] += BISHOP_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "r") {
			byColorPiecesBB[1][4] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[4] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[1] += ROOK_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "q") {
			byColorPiecesBB[1][5] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[5] += 1LL << i;

			FullTiles += 1LL << i;
			sideMaterial[1] += QUEEN_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "k") {
			byColorPiecesBB[1][6] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[6] += 1LL << i;
			
			FullTiles += 1LL << i;
			sideMaterial[1] += KING_VAL;
		}
	}

	//mark empty tiles opposite of full tiles
  	EmptyTiles = ~FullTiles;

}

void BitBoards::makeMove(const Move& m, bool isWhite) 
{
	//color is stupid because I started the program with a bool is white, which (int)'s to 1
	//need to change it to an int to remove this sort of thing
	int color = !isWhite;


	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
	}

	//drawBBA();
	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
		drawBB(allPiecesColorBB[color]);
		drawBB(allPiecesColorBB[!color]);
	}

	//remove or add captured piece from BB's same as above for moving piece
	if (m.captured) {

		addOrRemovePiece(m.captured, !color, m.to);
		//update material
		sideMaterial[!color] -= SORT_VALUE[m.captured];
	}

	if (m.flag == '0') {
		movePiece(m.piece, color, m.from, m.to);	
		
	}
	//pawn promotion
	else if (m.flag == 'Q') {

		addOrRemovePiece(PAWN, color, m.from);
		addOrRemovePiece(QUEEN, color, m.to);

		sideMaterial[color] += SORT_VALUE[QUEEN] - SORT_VALUE[PAWN];
	}


	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
	}

	//update the zobrist key
	zobrist.UpdateKey(m.from, m.to, m, isWhite);
	zobrist.UpdateColor();
}
void BitBoards::unmakeMove(const Move & m, bool isWhite)
{
	int color = !isWhite;


	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
		drawBB(allPiecesColorBB[color]);
		drawBB(allPiecesColorBB[!color]);
	}


	if (m.flag == '0') {
		movePiece(m.piece, color, m.to, m.from);
	}
	//pawn promotion
	else if (m.flag == 'Q'){

		addOrRemovePiece(PAWN, color, m.from);
		addOrRemovePiece(QUEEN, color, m.to);

		sideMaterial[color] += SORT_VALUE[PAWN] - SORT_VALUE[QUEEN];
	}

	//add captured piece from BB's similar to above for moving piece
	if (m.captured) {

		addOrRemovePiece(m.captured, !color, m.to);
		sideMaterial[!color] += SORT_VALUE[m.captured];
	}


	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
		drawBB(allPiecesColorBB[color]);
		drawBB(allPiecesColorBB[!color]);
	}

	//update the zobrist key
	zobrist.UpdateKey(m.from, m.to, m, isWhite);
	zobrist.UpdateColor();
}
/*
void BitBoards::makeMove(const Move &move, bool isWhite)
{
    int xyI, xyE;
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
    int xyI = moveKey.from;
    int xyE = moveKey.to;
    //inital spot piece mask and end spot mask
    U64 pieceMaskI = 1LL << xyI;
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
*/
//returns the rank of a square relative to side specified
int BitBoards::relativeRank(int sq, bool isWhite)
{ //return reletive rank for side to move.
	sq /= 8;
	return isWhite ? flipRank[sq] : sq + 1;
}

//used for drawing a singular bitboard
void BitBoards::drawBB(U64 board)
{
	for (int i = 0; i < 64; i++) {
		if (board & (1ULL << i)) {
			std::cout << 1 << ", ";
		}
		else {
			std::cout << 0 << ", ";
		}
		if ((i + 1) % 8 == 0) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
}
//draws all bitboards as a chess board in chars
void BitBoards::drawBBA()
{
	char flips[8] = { '8', '7', '6', '5', '4', '3', '2', '1' };
	char flipsL[8] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H' };
	int c = 0;

	for (int i = 0; i < 64; i++) {
		if ((i) % 8 == 0) {
			std::cout << std::endl;
			std::cout << flips[c] << " | ";
			c++;
		}

		if (byColorPiecesBB[0][1] & (1ULL << i)) {
			std::cout << "P" << ", ";
		}
		if (byColorPiecesBB[0][2] & (1ULL << i)) {
			std::cout << "N" << ", ";
		}
		if (byColorPiecesBB[0][3] & (1ULL << i)) {
			std::cout << "B" << ", ";
		}
		if (byColorPiecesBB[0][4] & (1ULL << i)) {
			std::cout << "R" << ", ";
		}
		if (byColorPiecesBB[0][5] & (1ULL << i)) {
			std::cout << "Q" << ", ";
		}
		if (byColorPiecesBB[0][6] & (1ULL << i)) {
			std::cout << "K" << ", ";
		}
		if (byColorPiecesBB[1][1] & (1ULL << i)) {
			std::cout << "p" << ", ";
		}
		if (byColorPiecesBB[1][2] & (1ULL << i)) {
			std::cout << "n" << ", ";
		}
		if (byColorPiecesBB[1][3] & (1ULL << i)) {
			std::cout << "b" << ", ";
		}
		if (byColorPiecesBB[1][4] & (1ULL << i)) {
			std::cout << "r" << ", ";
		}
		if (byColorPiecesBB[1][5] & (1ULL << i)) {
			std::cout << "q" << ", ";
		}
		if (byColorPiecesBB[1][6] & (1ULL << i)) {
			std::cout << "k" << ", ";
		}
		if (EmptyTiles & (1ULL << i)) {
			std::cout << " " << ", ";
		}

		//if(i % 8 == 7){
		//    std::cout << "| " << flips[c] ;
		//     c++;
		// }
	}

	std::cout << std::endl << "    ";
	for (int i = 0; i < 8; i++) {
		std::cout << flipsL[i] << "  ";
	}

	std::cout << std::endl << std::endl;;
}










