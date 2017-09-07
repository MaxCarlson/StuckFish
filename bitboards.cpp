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

	//initialize the forwardBB variable
	//it holds a computer bitboard relative to side to move
	//lines forward from a particual square
	for (int color = 0; color < 2; ++color) {
		//const int sh = color == WHITE ? N : S; ?? doesn't accept as template argument

		for (int sq = 0; sq < 64; ++sq) {

			//create forward bb masks
			forwardBB[color][sq] = 1LL << sq;
			PassedPawnMask[color][sq] = 0LL;
			PawnAttackSpan[color][sq] = 0LL;

			if (color == WHITE) { //unfotunatly cannot find a fix to declaring <x> = color == WHITE ? N : S;
				forwardBB[color][sq] = shift_bb<N>(forwardBB[color][sq]);
				for (int i = 0; i < 8; ++i) { //something to do with non static storage duration cannot be used as non-type argument
					forwardBB[color][sq] |= shift_bb<N>(forwardBB[color][sq]);
					//create passed pawn masks
					PassedPawnMask[color][sq] |= psuedoAttacks(PAWN, WHITE, sq) | forwardBB[color][sq];
					PassedPawnMask[color][sq] |= shift_bb<N>(PassedPawnMask[color][sq]);
					//create pawn attack span masks
					PawnAttackSpan[color][sq] |= psuedoAttacks(PAWN, WHITE, sq);
					PawnAttackSpan[color][sq] |= shift_bb<N>(PawnAttackSpan[color][sq]);
				}
			}
			else {
				forwardBB[color][sq] = shift_bb<S>(forwardBB[color][sq]);
				for (int i = 0; i < 8; ++i) {
					forwardBB[color][sq] |= shift_bb<S>(forwardBB[color][sq]);
					PassedPawnMask[color][sq] |= psuedoAttacks(PAWN, BLACK, sq) | forwardBB[color][sq];
					PassedPawnMask[color][sq] |= shift_bb<S>(PassedPawnMask[color][sq]);
					PawnAttackSpan[color][sq] |= psuedoAttacks(PAWN, BLACK, sq);
					PawnAttackSpan[color][sq] |= shift_bb<S>(PawnAttackSpan[color][sq]);
				}
			}

		}		
	}

	//find and record the max distance between two square on the game board
	for (int s1 = 0; s1 < 64; ++s1) {
		for (int s2 = 0; s2 < 64; ++s2) {
			if (s1 != s2) {
				SquareDistance[s1][s2] = std::max(file_distance(s1, s2), rank_distance(s1, s1));
			}
		}
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
	bInfo.PawnKey = 0LL;
	bInfo.MaterialKey = 0LL;
	bInfo.sideToMove = WHITE;

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
			bInfo.PawnKey ^= zobrist.zArray[WHITE][PAWN][i];
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
			bInfo.PawnKey ^= zobrist.zArray[BLACK][PAWN][i];
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

	for (int color = 0; color < COLOR; ++color) {
		for (int pt = PAWN; pt <= KING; ++pt) {
			for (int count = 0; count <= pieceCount[color][pt]; ++count) {

				//XOR material key with piece count instead of square location
				//so we can have a Material Key that represents what material is on the board.
				bInfo.MaterialKey ^= zobrist.zArray[color][pt][count];
			}
		}
	}

	//add up non pawn material 
	for (int c = 0; c < COLOR; ++c) {
		bInfo.nonPawnMaterial[c] = 0;

		for (int pt = KNIGHT; pt < PIECES; ++pt) {
			bInfo.nonPawnMaterial[c] += SORT_VALUE[pt] * pieceCount[c][pt];
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

	//drawBBA();

	//remove or add captured piece from BB's same as above for moving piece
	if (m.captured) {
		//remove captured piece
		removePiece(m.captured, !color, m.to);
		//update material
		bInfo.sideMaterial[!color] -= SORT_VALUE[m.captured];
		
		//update the pawn hashkey on capture
		if (m.captured == PAWN) bInfo.PawnKey ^= zobrist.zArray[!color][PAWN][m.to];

		else bInfo.nonPawnMaterial[!color] -= SORT_VALUE[m.captured];
		//update material key
		bInfo.MaterialKey ^= zobrist.zArray[!color][m.captured][pieceCount[!color][m.captured]+1];
	}

	//move the piece from - to
	movePiece(m.piece, color, m.from, m.to);	
		
	if (m.piece == PAWN) {
		//pawn promotion
		if (m.flag == 'Q') {
			//remove pawn placed
			removePiece(PAWN, color, m.to);
			//add queen to to square
			addPiece(QUEEN, color, m.to);
			//update material
			bInfo.sideMaterial[color] += SORT_VALUE[QUEEN] - SORT_VALUE[PAWN];
			bInfo.nonPawnMaterial[color] += SORT_VALUE[QUEEN];

			//update pawn key
			bInfo.PawnKey ^= zobrist.zArray[color][PAWN][m.to];
			//update material key
			bInfo.MaterialKey ^= zobrist.zArray[color][QUEEN][pieceCount[color][QUEEN]]
							  ^ zobrist.zArray[color][PAWN][pieceCount[color][PAWN]+1];
		}
		//update the pawn key, if it's a promotion it cancels out and updates correctly
		bInfo.PawnKey ^= zobrist.zArray[color][PAWN][m.to] ^ zobrist.zArray[color][PAWN][m.from];
	}

	//debug catch
	if (allPiecesColorBB[color] & allPiecesColorBB[!color]) {
		drawBBA();
	}

	//update the zobrist key
	zobrist.UpdateKey(m.from, m.to, m, isWhite);
	zobrist.UpdateColor();

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	//prefetch TT entry into cache THIS IS WAY TOO TIME INTENSIVE? 6.1% on just this call from here. SWITCH TT to single address lookup instead of cluster of two?
	//_mm_prefetch((char *)TT.first_entry(zobrist.zobristKey), _MM_HINT_NTA);

}
void BitBoards::unmakeMove(const Move & m, bool isWhite)
{
	
	int color = !isWhite;

	if (m.flag == '0') {
		//move piece
		movePiece(m.piece, color, m.to, m.from);

		if(m.piece == PAWN) bInfo.PawnKey ^= zobrist.zArray[color][PAWN][m.to] ^ zobrist.zArray[color][PAWN][m.from];
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
		bInfo.nonPawnMaterial[color] -= SORT_VALUE[QUEEN];
		//update pawn key
		bInfo.PawnKey ^= zobrist.zArray[color][PAWN][m.from];
		//update material key
		bInfo.MaterialKey ^= zobrist.zArray[color][QUEEN][pieceCount[color][QUEEN]+1] //plus one because we've already decremented the counter
			              ^ zobrist.zArray[color][PAWN][pieceCount[color][PAWN]];
	}

	//add captured piece from BB's similar to above for moving piece
	if (m.captured) {
		//add captured piece to to square
		addPiece(m.captured, !color, m.to);
		bInfo.sideMaterial[!color] += SORT_VALUE[m.captured];

		//if pawn captured undone, update pawn key
		if (m.captured == PAWN) bInfo.PawnKey ^= zobrist.zArray[!color][PAWN][m.to];

		else bInfo.nonPawnMaterial[!color] += SORT_VALUE[m.captured];
		//update material key
		bInfo.MaterialKey ^= zobrist.zArray[!color][m.captured][pieceCount[!color][m.captured]];
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

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	return;
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











