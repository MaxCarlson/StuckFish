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
/*
std::string boardArr[8][8] = {
	{ "r", " ", " ", " ", "k", " ", " ", "r" },
	{ "p", "p", "p", "p", "p", "p", "p", "p", },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ " ", " ", " ", " ", " ", " ", " ", " " },
	{ "P", "P", "P", "P", "P", "P", "P", "P" },
	{ "R", " ", " ", " ", "K", " ", " ", "R" },
};
*/

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

	//reset piece and color BitBoards to 0;
	for (int i = 0; i < 2; ++i) {
		allPiecesColorBB[i] = 0LL;
		//reset side material values
		bInfo.sideMaterial[i] = 0; 
		//reset castling rights
		castlingRights[i] = 0LL;

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

			addPiece(PAWN, WHITE, i);
			bInfo.sideMaterial[0] += PAWN_VAL;
			bInfo.PawnKey ^= zobrist.zArray[WHITE][PAWN][i];
		}
		else if (boardArr[i / 8][i % 8] == "N") {
			addPiece(KNIGHT, WHITE, i);

			bInfo.sideMaterial[0] += KNIGHT_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "B") {

			addPiece(BISHOP, WHITE, i);
			bInfo.sideMaterial[0] += BISHOP_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "R") {

			addPiece(ROOK, WHITE, i);
			bInfo.sideMaterial[0] += ROOK_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "Q") {

			addPiece(QUEEN, WHITE, i);
			bInfo.sideMaterial[0] += QUEEN_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "K") {

			addPiece(KING, WHITE, i);
			bInfo.sideMaterial[0] += KING_VAL;
		}
		//black pieces
		else if (boardArr[i / 8][i % 8] == "p") {

			addPiece(PAWN, BLACK, i);
			bInfo.sideMaterial[1] += PAWN_VAL;
			bInfo.PawnKey ^= zobrist.zArray[BLACK][PAWN][i];
		}
		else if (boardArr[i / 8][i % 8] == "n") {

			addPiece(KNIGHT, BLACK, i);
			bInfo.sideMaterial[1] += KNIGHT_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "b") {

			addPiece(BISHOP, BLACK, i);
			bInfo.sideMaterial[1] += BISHOP_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "r") {

			addPiece(ROOK, BLACK, i);
			bInfo.sideMaterial[1] += ROOK_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "q") {

			addPiece(QUEEN, BLACK, i);
			bInfo.sideMaterial[1] += QUEEN_VAL;
		}
		else if (boardArr[i / 8][i % 8] == "k") {

			addPiece(KING, BLACK, i);
			bInfo.sideMaterial[1] += KING_VAL;
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

void BitBoards::makeMove(const Move& m, int color)
{
	//color is stupid because I started the program with a bool is white, which (int)'s to 1
	//need to change it to an int to remove this sort of thing
	int them = !color;

	assert(posOkay());

	//remove or add captured piece from BB's same as above for moving piece
	if (m.captured) {
		//remove captured piece
		removePiece(m.captured, them, m.to);
		//update material
		bInfo.sideMaterial[them] -= SORT_VALUE[m.captured];
		
		//update the pawn hashkey on capture
		if (m.captured == PAWN)  bInfo.PawnKey ^= zobrist.zArray[them][PAWN][m.to]; 

		else bInfo.nonPawnMaterial[them] -= SORT_VALUE[m.captured];
		//update material key
		bInfo.MaterialKey ^= zobrist.zArray[them][m.captured][pieceCount[them][m.captured]+1];
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
							  ^ zobrist.zArray[color ][PAWN][pieceCount[color][PAWN]+1];
		}
		//update the pawn key, if it's a promotion it cancels out and updates correctly
		bInfo.PawnKey ^= zobrist.zArray[color][PAWN][m.to] ^ zobrist.zArray[color][PAWN][m.from];
		//_mm_prefetch((char *)pawnsTable[bInfo.PawnKey], _MM_HINT_NTA); // TEST for ELO GAIN, BOTH PLACES WITH PREFETCH HAD ELO LOSS
	}
	else if (m.flag == 'C') {
		int rookFrom = relative_square(color, m.to) == C1 ? relative_square(color, A1) : relative_square(color, H1);
		int rookTo = rookFrom == relative_square(color, A1) ? relative_square(color, D1) : relative_square(color, F1);

		movePiece(ROOK, color, rookFrom, rookTo);

		int zCast = color == WHITE ? rookFrom == A1 ? 0 : 1 : rookFrom == A1 ? 2 : 3;
		zobrist.zobristKey ^= zobrist.zCastle[zCast]
			^ zobrist.zArray[color][ROOK][rookFrom]
			^ zobrist.zArray[color][ROOK][rookTo];
	}
	
	assert(posOkay());
	
	//update the zobrist key
	zobrist.UpdateKey(m.from, m.to, m, !color);
	zobrist.UpdateColor();

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	//prefetch TT entry into cache THIS IS WAY TOO TIME INTENSIVE? 6.1% on just this call from here. SWITCH TT to single address lookup instead of cluster of two?
	_mm_prefetch((char *)TT.first_entry(zobrist.zobristKey), _MM_HINT_NTA);

}
void BitBoards::unmakeMove(const Move & m, int color)
{
	int them = !color;

	assert(posOkay());

	if (m.flag != 'Q') {
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

		bInfo.sideMaterial[color]    += SORT_VALUE[PAWN ] - SORT_VALUE[QUEEN];
		bInfo.nonPawnMaterial[color] -= SORT_VALUE[QUEEN];
		//update pawn key
		bInfo.PawnKey ^= zobrist.zArray[color][PAWN][m.from];
		//update material key
		bInfo.MaterialKey ^= zobrist.zArray[color][QUEEN][pieceCount[color][QUEEN]+1] //plus one because we've already decremented the counter
			              ^  zobrist.zArray[color][PAWN ][pieceCount[color][PAWN ]  ];
	}
	else if (m.flag == 'C') {
		
	}

	//add captured piece from BB's similar to above for moving piece
	if (m.captured) {
		//add captured piece to to square
		addPiece(m.captured, them, m.to);
		bInfo.sideMaterial[them] += SORT_VALUE[m.captured];

		//if pawn captured undone, update pawn key
		if (m.captured == PAWN) bInfo.PawnKey ^= zobrist.zArray[them][PAWN][m.to];

		else bInfo.nonPawnMaterial[them] += SORT_VALUE[m.captured];
		//update material key
		bInfo.MaterialKey ^= zobrist.zArray[them][m.captured][pieceCount[them][m.captured]];
	}

	assert(posOkay());

	//update the zobrist key
	zobrist.UpdateKey(m.from, m.to, m, !color);
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

//method used for checking if
//everything is okay within the boards
//will need more added to it
bool BitBoards::posOkay()
{


	for (int i = PAWN; i < PIECES; ++i) {
		for (int j = PAWN; j < PIECES; ++j) {
			if (pieces(WHITE, i) & pieces(BLACK, j)) {
				drawBB(pieces(WHITE, i));
				drawBB(pieces(BLACK, j));
				goto End;
			}
		}
	}

	if (EmptyTiles & FullTiles) goto End;


	return true;

End:
	drawBB(EmptyTiles);
	drawBB(FullTiles);
	drawBBA();
	return false;
}

/*
//make move castling stuff

//remove castling right for rook or king if it's its first movement
/*	else if (m.flag == 'M') {
castlingRights[color] |= m.piece == ROOK ? relative_square(color, m.from) == A1 ? 1LL : 4LL : 2LL;
}
//do castling. We've already moved the king, so we need to
//deal with the rook, castling rights, and the zobrist keys
else if (m.flag == 'C') {
//BitBoards * a = this;
//a->drawBBA();

castlingRights[color] |= 2LL;

int rookFrom = relative_square(color, m.to)     == C1  ? relative_square(color, A1) : relative_square(color, H1);
int rookTo   = rookFrom == relative_square(color,  A1) ? relative_square(color, D1) : relative_square(color, F1);

movePiece(ROOK, color, rookFrom, rookTo);

int zCast = color == WHITE ? rookFrom == A1 ? 0 : 1 : rookFrom == A1 ? 2 : 3;
zobrist.zobristKey ^= zobrist.zCastle[zCast]
^  zobrist.zArray[color][ROOK][rookFrom]
^  zobrist.zArray[color][ROOK][rookTo];
//a = this;
//a->drawBBA();
}

//unmake castling stuff

//restore castling rights if we're unmaking the rooks first move and the king hasn't moved.
//else if (m.flag == 'M') {
//	castlingRights[color] ^= m.piece == ROOK ? relative_square(color, m.from) == A1 ? 1LL : 4LL : 2LL;
//}
//unmake castling
if (m.flag == 'C') {
//BitBoards * a = this;
//a->drawBBA();

castlingRights[color] ^= 2;

int rookFrom = relative_square(color, m.to)    == C1  ? relative_square(color, A1) : relative_square(color, H1);
int rookTo   = rookFrom == relative_square(color, A1) ? relative_square(color, D1) : relative_square(color, F1);

movePiece(ROOK, color, rookTo, rookFrom);

int zCast = color == WHITE ? rookFrom == A1 ? 0 : 1 : rookFrom == A1 ? 2 : 3;
zobrist.zobristKey ^= zobrist.zCastle[zCast]
^  zobrist.zArray[ color][ROOK][rookFrom]
^  zobrist.zArray[ color][ROOK][rookTo];

//a = this;
//a->drawBBA();
//a->drawBBA();
}
*/





