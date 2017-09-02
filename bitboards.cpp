#include "bitboards.h"
#include <cmath>
#include <random>
#include <iostream>
#include <cstdio>
#include "externs.h"
#include "zobristh.h"
#include "TranspositionT.h"

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

//zero index contains white pawn attacks, 1 black pawns
 //move this somewhere else, preferably not an extern. Maybe into bb object?

void BitBoards::initBoards()
{
	//initilize all Pseudo legal attacks into BB
	//move this somewhere else.
	for (int i = 0; i < 64; ++i) {
		U64 moves, p, wp, bp;
		p = 1LL << i;
		wp = p >> 7;
		wp |= p >> 9;
		bp = p << 9;
		bp |= p << 7;

		if (i % 8 < 4) {
			wp &= ~FileHBB;
			bp &= ~FileHBB;
		}
		else {
			wp &= ~FileABB;
			bp &= ~FileABB;
		}
		PseudoAttacks[PAWN - 1][i] = wp;
		PseudoAttacks[PAWN][i] = bp;
		

		if (i > 18) {
			moves = KNIGHT_SPAN << (i - 18);
		}
		else {
			moves = KNIGHT_SPAN >> (18 - i);
		}

		if (i % 8 < 4) {
			moves &= ~FILE_GH;
		}
		else {
			moves &= ~FILE_AB;
		}

		PseudoAttacks[KNIGHT][i] = moves;

		PseudoAttacks[QUEEN][i] = PseudoAttacks[BISHOP][i] = slider_attacks.BishopAttacks(0LL, i);

		PseudoAttacks[QUEEN][i] |= PseudoAttacks[ROOK][i] = slider_attacks.RookAttacks(0LL, i);

		if (i > 9) {
			moves = KING_SPAN << (i - 9);
		}
		else {
			moves = KING_SPAN >> (9 - i);
		}
		if (i % 8 < 4) {
			moves &= ~FILE_GH;
		}
		else {
			moves &= ~FILE_AB;
		}

		PseudoAttacks[KING][i] = moves;
	}
}

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
		bInfo.sideMaterial[i] = 0; 

		for (int j = 0; j < 7; ++j) {
			byColorPiecesBB[i][j] = 0LL;
			byPieceType[j] = 0LL;
			pieceCount[i][j] = 0;	
			
			for (int h = 0; h < 16; ++h) {
				pieceLoc[i][j][h] = SQ_NONE;
			}
		}
	}
	
	FullTiles = 0LL;

	//seed bitboards
	for (int i = 0; i < 64; i++) {
		squareBB[i] = 1LL << i;
		pieceIndex[i] = SQ_NONE;
		pieceOn[i] = PIECE_EMPTY;

		if (boardArr[i / 8][i % 8] == "P") {
			byColorPiecesBB[0][1] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[1] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[0] += PAWN_VAL;

			//record square piece is located at and increment the piece count
			pieceLoc[WHITE][PAWN][pieceCount[WHITE][PAWN]] = i;
			pieceIndex[i] = pieceCount[WHITE][PAWN]; //store the piece loc list index of this particular piece, indexed by square it's on
			pieceCount[WHITE][PAWN] ++;
			
		}
		else if (boardArr[i / 8][i % 8] == "N") {
			byColorPiecesBB[0][2] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[2] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[0] += KNIGHT_VAL;

			pieceLoc[WHITE][KNIGHT][pieceCount[WHITE][KNIGHT]] = i;
			pieceIndex[i] = pieceCount[WHITE][KNIGHT];
			pieceCount[WHITE][KNIGHT]++;
		}
		else if (boardArr[i / 8][i % 8] == "B") {
			byColorPiecesBB[0][3] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[3] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[0] += BISHOP_VAL;

			pieceLoc[WHITE][BISHOP][pieceCount[WHITE][BISHOP]] = i;
			pieceIndex[i] = pieceCount[WHITE][BISHOP];
			pieceCount[WHITE][BISHOP]++;
		}
		else if (boardArr[i / 8][i % 8] == "R") {
			byColorPiecesBB[0][4] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[4] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[0] += ROOK_VAL;
			pieceLoc[WHITE][ROOK][pieceCount[WHITE][ROOK]] = i;
			pieceIndex[i] = pieceCount[WHITE][ROOK];
			pieceCount[WHITE][ROOK]++;
		}
		else if (boardArr[i / 8][i % 8] == "Q") {
			byColorPiecesBB[0][5] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[5] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[0] += QUEEN_VAL;

			pieceLoc[WHITE][QUEEN][pieceCount[WHITE][QUEEN]] = i;
			pieceIndex[i] = pieceCount[WHITE][QUEEN];
			pieceCount[WHITE][QUEEN]++;
		}
		else if (boardArr[i / 8][i % 8] == "K") {
			byColorPiecesBB[0][6] += 1LL << i;
			allPiecesColorBB[0] += 1LL << i;
			byPieceType[6] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[0] += KING_VAL;

			pieceLoc[WHITE][KING][pieceCount[WHITE][KING]] = i;
			pieceIndex[i] = pieceCount[WHITE][KING];
			pieceCount[WHITE][KING]++;
		}
		//black pieces
		else if (boardArr[i / 8][i % 8] == "p") {
			byColorPiecesBB[1][1] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[1] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[1] += PAWN_VAL;

			pieceLoc[BLACK][PAWN][pieceCount[BLACK][PAWN]] = i;
			pieceIndex[i] = pieceCount[BLACK][PAWN];
			pieceCount[BLACK][PAWN]++;
		}
		else if (boardArr[i / 8][i % 8] == "n") {
			byColorPiecesBB[1][2] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[2] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[1] += KNIGHT_VAL;

			pieceLoc[BLACK][KNIGHT][pieceCount[BLACK][KNIGHT]] = i;
			pieceIndex[i] = pieceCount[BLACK][KNIGHT];
			pieceCount[BLACK][KNIGHT]++;
		}
		else if (boardArr[i / 8][i % 8] == "b") {
			byColorPiecesBB[1][3] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[3] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[1] += BISHOP_VAL;

			pieceLoc[BLACK][BISHOP][pieceCount[BLACK][BISHOP]] = i;
			pieceIndex[i] = pieceCount[BLACK][BISHOP];
			pieceCount[BLACK][BISHOP]++;
		}
		else if (boardArr[i / 8][i % 8] == "r") {
			byColorPiecesBB[1][4] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[4] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[1] += ROOK_VAL;

			pieceLoc[BLACK][ROOK][pieceCount[BLACK][ROOK]] = i;
			pieceIndex[i] = pieceCount[BLACK][ROOK];
			pieceCount[BLACK][ROOK]++;
		}
		else if (boardArr[i / 8][i % 8] == "q") {
			byColorPiecesBB[1][5] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[5] += 1LL << i;

			FullTiles += 1LL << i;
			bInfo.sideMaterial[1] += QUEEN_VAL;

			pieceLoc[BLACK][QUEEN][pieceCount[BLACK][QUEEN]] = i;
			pieceIndex[i] = pieceCount[BLACK][QUEEN];
			pieceCount[BLACK][QUEEN]++;
		}
		else if (boardArr[i / 8][i % 8] == "k") {
			byColorPiecesBB[1][6] += 1LL << i;
			allPiecesColorBB[1] += 1LL << i;
			byPieceType[6] += 1LL << i;
			
			FullTiles += 1LL << i;
			bInfo.sideMaterial[1] += KING_VAL;

			pieceLoc[BLACK][KING][pieceCount[BLACK][KING]] = i;
			pieceIndex[i] = pieceCount[BLACK][KING];
			pieceCount[BLACK][KING]++;
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

	//remove or add captured piece from BB's same as above for moving piece
	if (m.captured) {
		//remove captured piece
		removePiece(m.captured, !color, m.to);
		//update material
		bInfo.sideMaterial[!color] -= SORT_VALUE[m.captured];
	}

	//move the piece from - to
	movePiece(m.piece, color, m.from, m.to);	
		

	//pawn promotion
	if (m.flag == 'Q') {
		//remove pawn placed
		removePiece(PAWN, color, m.to);
		//add queen to to square
		addPiece(QUEEN, color, m.to);
		//update material
		bInfo.sideMaterial[color] += SORT_VALUE[QUEEN] - SORT_VALUE[PAWN];
	}

	//debug catch
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

	if (m.flag == '0') {
		//move piece
		movePiece(m.piece, color, m.to, m.from);
	}
	//pawn promotion
	else if (m.flag == 'Q'){
		//remove pawn from to square,
		//we added two pawns above so here we remove one
		//removePiece(PAWN, color, m.to);
		addPiece(PAWN, color, m.from);
		//remove queen
		removePiece(QUEEN, color, m.to);

		bInfo.sideMaterial[color] += SORT_VALUE[PAWN] - SORT_VALUE[QUEEN];
	}

	//add captured piece from BB's similar to above for moving piece
	if (m.captured) {
		//add captured piece to to square
		addPiece(m.captured, !color, m.to);
		bInfo.sideMaterial[!color] += SORT_VALUE[m.captured];
	}

	//debug catch
	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
		drawBB(allPiecesColorBB[color]);
		drawBB(allPiecesColorBB[!color]);
	}

	//update the zobrist key
	zobrist.UpdateKey(m.from, m.to, m, isWhite);
	zobrist.UpdateColor();

	//prefetch TT entry into cache
	_mm_prefetch((char *)TT.first_entry(zobrist.zobristKey), _MM_HINT_NTA);
}

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











