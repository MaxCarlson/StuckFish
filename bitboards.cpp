#include "bitboards.h"
#include <cmath>
#include <random>
#include <iostream>
#include <cstdio>
#include <sstream>

#include "externs.h"
#include "zobristh.h"
#include "TranspositionT.h"


/*
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
*/
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

// holds keys to XOR Transposition, Material, and Pawn hash keys by 
// when board position changes
namespace Zobrist {
	U64 ZobArray[COLOR][PIECES][SQ_ALL];
	U64 EnPassant[8];
	U64 Castling[4];
	U64 Color;
}


void BitBoards::initBoards()
{
	//copy values from zobrist object to namespace
	std::memcpy(Zobrist::ZobArray,  zobrist.zArray,     sizeof(zobrist.zArray    ));
	std::memcpy(Zobrist::EnPassant, zobrist.zEnPassant, sizeof(zobrist.zEnPassant));
	std::memcpy(Zobrist::Castling,  zobrist.zCastle,    sizeof(zobrist.zCastle   ));
	Zobrist::Color = zobrist.zBlackMove;

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

		for (int sq = 0; sq < 64; ++sq) {

			//create U64 lookup table for each square
			squareBB[sq] = 1LL << sq;
			//create forward bb masks
			forwardBB[color][sq] = 1LL << sq;
			PassedPawnMask[color][sq] = 0LL;
			PawnAttackSpan[color][sq] = 0LL;

			if (color == WHITE) { 
				forwardBB[color][sq] = shift_bb<N>(forwardBB[color][sq]);
				for (int i = 0; i < 8; ++i) { 
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

void BitBoards::constructBoards(const std::string* FEN) //replace this with fen string reader
{

	//st = new StateInfo; //this is a memory leak currently. Figure out a better way!!!!! Posssibly smart ptr if perfromance isn't effected ?
	//st->previous = NULL;
	startState.epSquare = SQ_NONE;
	startState.castlingRights = 0;
	startState.MaterialKey = 0LL;
	startState.PawnKey = 0LL;

	st = &startState;


	FullTiles = 0LL;

	//reset piece and color BitBoards to 0;
	for (int i = 0; i < 2; ++i) {
		allPiecesColorBB[i] = 0LL;

		for (int j = 0; j < 7; ++j) {
			byColorPiecesBB[i][j] = 0LL;
			byPieceType[j] = 0LL;
			pieceCount[i][j] = 0;

			for (int h = 0; h < 16; ++h) {
				pieceLoc[i][j][h] = SQ_NONE;
			}
		}
	}

	//clear piece locations and indices
	for (int i = 0; i < 64; i++) {
		pieceIndex[i] = SQ_NONE;
		pieceOn[i] = PIECE_EMPTY;
	}

	if(FEN) readFenString(*FEN);
	else {
		const std::string startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		readFenString(startpos);
	}

/*
	//seed bitboards //delete
	for (int i = 0; i < 64; i++) {
		pieceIndex[i] = SQ_NONE;
		pieceOn[i] = PIECE_EMPTY;
*/


	//get zobrist key of current position and store to board
	st->Key = zobrist.getZobristHash(*this);

	for (int c = 0; c < COLOR; ++c) {
		
		bInfo.sideMaterial[c] = 0;
		bInfo.nonPawnMaterial[c] = 0;
		
		for (int pt = PAWN; pt < PIECES; ++pt) {

			bInfo.sideMaterial[c] += SORT_VALUE[pt] * pieceCount[c][pt];

			//add up non pawn material 
			if(pt > PAWN)
				bInfo.nonPawnMaterial[c] += SORT_VALUE[pt] * pieceCount[c][pt];

			// XOR the pawn hash key by pawns
			// and their color/locations
			if (pt == PAWN) {
				U64 pawnBoard = pieces(c, PAWN);

				while (pawnBoard) {
					st->PawnKey ^= zobrist.zArray[c][PAWN][pop_lsb(&pawnBoard)];
				}
			}

			// XOR material key with piece count instead of square location
			// so we can have a Material Key that represents what material is on the board.
			for (int count = 0; count <= pieceCount[c][pt]; ++count) {

				st->MaterialKey ^= zobrist.zArray[c][pt][count];
			}
		}
	}

	// mark empty tiles opposite of full tiles
  	EmptyTiles = ~FullTiles;	
}

void BitBoards::readFenString(const std::string& FEN)
{								
				     //  B                          K        N     P  Q  R
	int lookup[18] = {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 2, 0, 1, 5, 4};

	int spCount = 0;

	std::istringstream ss(FEN);
	char token;

	ss >> std::noskipws;

	int sq = A8;

	// place pieces 
	while((ss >> token) && !isspace(token)) {

		if (isdigit(token)) sq += token - '0';

		if (token == '/') continue; //is this correct?
		
		if (isalpha(token)) {
			if (isupper(token)) {
				addPiece(lookup[token - 'A'], WHITE, sq);
			}
			else {
				addPiece(lookup[char(toupper(token)) - 'A'], BLACK, sq);
			}
			sq++;
		}
	}

	// 2. Active color
	ss >> token;
	bInfo.sideToMove = (token == 'w' ? WHITE : BLACK);
	ss >> token;

	// Need castling code
	while ((ss >> token) && !isspace(token))
	{
		token = char(toupper(token));

		if (token == 'K' || 'Q') continue;

		else continue;
	}

	// En passant square, ignore if no capture possible
	char row, col;

	if (((ss >> col) && (col >= 'a' && col <= 'h'))
		&& ((ss >> row) && (row == '3' || row == '6'))){

		st->epSquare = (col - 'a') * 8 + (row - '1'); //test this 

		if (!(psuedoAttacks(PAWN, bInfo.sideToMove, st->epSquare) //also needs testing
			& pieces(bInfo.sideToMove, PAWN))) { 
			st->epSquare = SQ_NONE;
		}
		
	}
	
	// need to implement half move clock 
	char notUSED;
	ss >> std::skipws >> notUSED >> turns;

}

void BitBoards::makeMove(const Move& m, StateInfo& newSt, int color)
{
	int them = !color;

	assert(posOkay());

	//copy current board state to board state stored on ply
	std::memcpy(&newSt, st, sizeof(StateInfo));
	//set previous state on ply to current state
	newSt.previous = st;
	//use st to modify values on ply
	st = &newSt;

	//remove or add captured piece from BB's same as above for moving piece
	if (m.captured) {
		int capSq = m.to;

		if (m.captured == PAWN) { 
			
			//en passant
			if (m.flag == 'E') {
				//set capture square to correct pos and not ep square
				capSq += pawn_push(them);
			}
			//update the pawn hashkey on capture
			st->PawnKey ^= Zobrist::ZobArray[them][PAWN][capSq];
		}

		else { 
			bInfo.nonPawnMaterial[them] -= SORT_VALUE[m.captured]; 
		}
		//remove captured piece
		removePiece(m.captured, them, capSq);
		//update material
		bInfo.sideMaterial[them] -= SORT_VALUE[m.captured];
		// update TT key
		st->Key ^= Zobrist::ZobArray[them][m.captured][capSq];
		//update material key
		st->MaterialKey ^= Zobrist::ZobArray[them][m.captured][pieceCount[them][m.captured]+1];
	}

	//move the piece from - to
	movePiece(m.piece, color, m.from, m.to);

	
	// if there was an EP square, reset it for the next ply 
	// and remove the ep zobrist XOR
	if (st->epSquare != SQ_NONE) {

		//zobrist.zobristKey ^= zobrist.zEnPassant[file_of(st->epSquare)];
		st->Key ^= Zobrist::EnPassant[file_of(st->epSquare)];
		st->epSquare = SQ_NONE;
	}
	

	if (m.piece == PAWN) {
		
		//if the pawn move is two forward,
		//and there is an enemy pawn positioned to en passant
		if ((m.to ^ m.from) == 16
			&& (psuedoAttacks(PAWN, color, m.from + pawn_push(color)) & pieces(them, PAWN))) {
			
			st->epSquare = (m.from + m.to) / 2;
			st->Key ^= Zobrist::EnPassant[file_of(st->epSquare)];
		} 
		
		// pawn promotion
		if (m.flag == 'Q') {
			//remove pawn placed
			removePiece(PAWN, color, m.to);
			//add queen to to square
			addPiece(QUEEN, color, m.to);
			//update material
			bInfo.sideMaterial[color]    += SORT_VALUE[QUEEN] - SORT_VALUE[PAWN];
			bInfo.nonPawnMaterial[color] += SORT_VALUE[QUEEN];

			st->Key ^= Zobrist::ZobArray[color][QUEEN][m.to] ^ Zobrist::ZobArray[color][PAWN][m.to];
			//update pawn key
			st->PawnKey ^= Zobrist::ZobArray[color][PAWN][m.to];
			//update material key
			st->MaterialKey   ^= Zobrist::ZobArray[color][QUEEN][pieceCount[color][QUEEN]]
							  ^  Zobrist::ZobArray[color ][PAWN][pieceCount[color][PAWN]+1];
		}
		
		//update the pawn key, if it's a promotion it cancels out and updates correctly
		st->PawnKey ^= zobrist.zArray[color][PAWN][m.to] ^ zobrist.zArray[color][PAWN][m.from];
		//_mm_prefetch((char *)pawnsTable[bInfo.PawnKey], _MM_HINT_NTA); // TEST for ELO GAIN, BOTH PLACES WITH PREFETCH HAD ELO LOSS
	}
	// Castling ~~ only implemented to be done by another,
	// engine itself cannot castle yet. Next thing on agenda.
	else if (m.flag == 'C') {
		int rookFrom = relative_square(color, m.to) == C1 ? relative_square(color, A1) : relative_square(color, H1);
		int rookTo = rookFrom == relative_square(color, A1) ? relative_square(color, D1) : relative_square(color, F1);

		movePiece(ROOK, color, rookFrom, rookTo);

		int zCast = color == WHITE ? rookFrom == A1 ? 0 : 1 : rookFrom == A1 ? 2 : 3;
		st->Key ^= Zobrist::Castling[zCast]
			^ Zobrist::ZobArray[color][ROOK][rookFrom]
			^ Zobrist::ZobArray[color][ROOK][rookTo];
	}
	
	assert(posOkay());

	//update the TT key, capture update done in capture above
	st->Key ^= Zobrist::ZobArray[color][m.piece][m.from] 
		    ^  Zobrist::ZobArray[color][m.piece][m.to]
		    ^  Zobrist::Color;

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	if (EmptyTiles & FullTiles || allPiecesColorBB[WHITE] & allPiecesColorBB[BLACK]) { //comment out in release
		drawBBA();
	}

	//prefetch TT entry into cache ~THIS IS WAY TOO TIME INTENSIVE? 6.1% on just this call from here. SWITCH TT to single address lookup instead of cluster of two?
	_mm_prefetch((char *)TT.first_entry(st->Key), _MM_HINT_NTA);

}
void BitBoards::unmakeMove(const Move & m, int color)
{
	//restore state pointer to state before we made this move
	st = st->previous;

	int them = !color;

	assert(posOkay());

	if (m.flag != 'Q') {

		// move piece
		movePiece(m.piece, color, m.to, m.from);
	}
	//pawn promotion
	else if (m.flag == 'Q'){

		addPiece(PAWN, color, m.from);
		//remove queen
		removePiece(QUEEN, color, m.to);

		bInfo.sideMaterial[color]    += SORT_VALUE[PAWN ] - SORT_VALUE[QUEEN];
		bInfo.nonPawnMaterial[color] -= SORT_VALUE[QUEEN];
	}

	//add captured piece from BB's similar to above for moving piece
	if (m.captured) {
		int capSq = m.to;

		// set capture square correctly if 
		// move is an en passant
		if (m.flag == 'E') {
			capSq += pawn_push(them);
		}

		//add captured piece to to square
		addPiece(m.captured, them, capSq);
		bInfo.sideMaterial[them] += SORT_VALUE[m.captured];

		//if pawn captured undone, update pawn key
		if (m.captured != PAWN) bInfo.nonPawnMaterial[them] += SORT_VALUE[m.captured];
	}


	assert(posOkay());

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	return;
}

void BitBoards::makeNullMove(StateInfo& newSt)
{
	//copy current board state to board state stored on ply
	std::memcpy(&newSt, st, sizeof(StateInfo));
	//set previous state on ply to current state
	newSt.previous = st;
	//use st to modify values on ply
	st = &newSt;

	st->Key ^= Zobrist::Color;

	if (st->epSquare != SQ_NONE) {
		st->Key ^= Zobrist::EnPassant[file_of(st->epSquare)];
		st->epSquare = SQ_NONE;
	}

	bInfo.sideToMove = !bInfo.sideToMove;
	_mm_prefetch((char *)TT.first_entry(st->Key), _MM_HINT_NTA);
}

void BitBoards::undoNullMove()
{
	st = st->previous;

	bInfo.sideToMove = !bInfo.sideToMove;
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

	int pcount = 0;

	for (int color = 0; color < COLOR; ++color) {
		U64 tmp = allPiecesColorBB[color];
		while (tmp) {
			pop_lsb(&tmp);
			pcount++;
		}		
	}
	U64 et = EmptyTiles;
	while (et) {
		pop_lsb(&et);
		pcount++;
		if (pcount > 64) goto End;
	}

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
	drawBBA();
	drawBB(EmptyTiles);
	drawBB(FullTiles);
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





