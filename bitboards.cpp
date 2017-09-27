#include "bitboards.h"
#include <cmath>
#include <random>
#include <iostream>
#include <cstdio>
#include <sstream>

#include "externs.h"
#include "zobristh.h"
#include "TranspositionT.h"
#include "Evaluate.h"



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


// returns true if squares s0, s1, and s2 are aligned
// either vertically, horizontally, or diagonally
inline bool aligned(int s0, int s1, int s2) { // move this to a new .h
	return LineBB[s0][s1] & (1LL << s2);
}

// holds keys to XOR Transposition, Material, and Pawn hash keys by 
// when board position changes
namespace Zobrist {
	U64 ZobArray[COLOR][PIECES][SQ_ALL];
	U64 EnPassant[8];
	U64 Castling[CASTLING_RIGHTS];
	U64 Color;
}

// Constructor for check info,
// called during make move so we can later exclude
// everything but evasions for move generation if
// we're in check
CheckInfo::CheckInfo(const BitBoards &board)
{
	int color = !board.stm();
	ksq = board.king_square(!color);

	pinned          = board.check_blockers(color,  color);
	dcCandidates    = board.check_blockers(color, !color);

	checkSq[PAWN]   = board.attacks_from<PAWN>(ksq, !color);
	checkSq[KNIGHT] = board.attacks_from<KNIGHT>(ksq);
	checkSq[BISHOP] = board.attacks_from<BISHOP>(ksq);
	checkSq[ROOK]   = board.attacks_from<ROOK>(ksq);
	checkSq[QUEEN]  = board.attacks_from<QUEEN>(ksq);
	checkSq[KING]   = 0LL;
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
	// also build the BetweenSquares array which holds a U64 of all squares between two squares.
	for (int s1 = 0; s1 < 64; ++s1) {
		for (int s2 = 0; s2 < 64; ++s2) {
			BetweenSquares[s1][s2] = 0LL;

			if (s1 != s2) {
				SquareDistance[s1][s2] = std::max(file_distance(s1, s2), rank_distance(s1, s2));


				if (SquareDistance[s1][s2] > 1) {

					if (s1 / 8 == s2 / 8 || s1 % 8 == s2 % 8)
						BetweenSquares[s1][s2] = psuedoAttacks(ROOK, WHITE, s1) & psuedoAttacks(ROOK, WHITE, s2);

					else {
						bool isDiag = false;
						for (int i = 1; i < 8; ++i) {

							if (s1 == s2 + 9 * i)
								isDiag = true;
							if (s1 == s2 - 9 * i)
								isDiag = true;
							if (s1 == s2 + 7 * i)
								isDiag = true;
							if (s1 == s2 - 7 * i)
								isDiag = true;
						}
						if (isDiag) {
							BetweenSquares[s1][s2] = psuedoAttacks(BISHOP, WHITE, s1) & psuedoAttacks(BISHOP, WHITE, s2);

							if (BetweenSquares[s1][s2] == 0LL)
								continue;
						}
						else
							continue;
					}

					int ss1 = s1, ss2 = s2;

					if (s1 > 7) ss1 = s1 % 8;
					if (s2 > 7) ss2 = s2 % 8;

					//squares on vertical
					if (s1 % 8 == s2 % 8) {
						if (s1 > s2) {
							ss1 = s1 / 8;
							ss2 = s2 / 8;
						}
						else {
							ss1 = s2 / 8;
							ss2 = s1 / 8;
						}


						for (int i = ss1; i < 8; ++i) {
							BetweenSquares[s1][s2] &= ~RankMasks8[i ^ 7];
						}

						for (int i = ss2; i >= 0; --i) {
							BetweenSquares[s1][s2] &= ~RankMasks8[i ^ 7];
						}
						continue;
					}

	
					//diagonal and horizontal squares
					if (ss1 > ss2) {
						int tmp = ss1;
						ss1 = ss2;
						ss2 = tmp;
					}

					for (int i = ss2; i < 8; ++i) {
						BetweenSquares[s1][s2] &= ~FileMasks8[i];
					}
					for (int i = ss1; i >= 0; --i) {
						BetweenSquares[s1][s2] &= ~FileMasks8[i];
					}
				}
			}
			
		}
	}	
}

void BitBoards::constructBoards(const std::string* FEN) //replace this with fen string reader
{
	//set the start state
	startState.epSquare = SQ_NONE;
	startState.castlingRights = 0;
	startState.MaterialKey = 0LL;
	startState.PawnKey = 0LL;

	//set state to start state
	//there are probable issues with this, it seems after setting a position
	//once we break into search, there's an infinite list of identical positions to search start
	//in st->previous
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
				castlingPath[h] = 0LL;
			}
		}
	}

	//clear piece locations and indices
	for (int i = 0; i < 64; i++) {
		pieceIndex[i] = SQ_NONE;
		pieceOn[i] = PIECE_EMPTY;
		castlingRightsMasks[i] = 0LL;
	}

	if(FEN) readFenString(*FEN);
	else {
		const std::string startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		readFenString(startpos);
	}
	delete FEN; //posibly change this? not much reason for ptr anymore


	set_state(st);

	// mark empty tiles opposite of full tiles
  	EmptyTiles = ~FullTiles;	
}

void BitBoards::set_state(StateInfo * si)
{
	//get zobrist key of current position and store to board
	si->Key = zobrist.getZobristHash(*this);

	si->checkers = attackers_to(king_square(stm()), FullTiles) & pieces(!stm()); //IS THIS working as intended????

	//si->Key ^= Zobrist::Castling[st->castlingRights]; //already done in zobrist.gethash above!!

	for (int c = 0; c < COLOR; ++c) {

		bInfo.sideMaterial[c] = 0; //transfer Binfo stuff to st->~!~!~!~!~~!~!!~!~!~!~!~!!~!~!~!~!~!
		bInfo.nonPawnMaterial[c] = 0;

		for (int pt = PAWN; pt < PIECES; ++pt) {

			bInfo.sideMaterial[c] += SORT_VALUE[pt] * pieceCount[c][pt];

			//add up non pawn material 
			if (pt > PAWN)
				bInfo.nonPawnMaterial[c] += SORT_VALUE[pt] * pieceCount[c][pt];

			// XOR the pawn hash key by pawns
			// and their color/locations
			if (pt == PAWN) {
				U64 pawnBoard = pieces(c, PAWN);

				while (pawnBoard) {
					si->PawnKey ^= zobrist.zArray[c][PAWN][pop_lsb(&pawnBoard)];
				}
			}

			// XOR material key with piece count instead of square location
			// so we can have a Material Key that represents what material is on the board.
			for (int count = 0; count <= pieceCount[c][pt]; ++count) {

				si->MaterialKey ^= zobrist.zArray[c][pt][count];
			}
		}
	}
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

	// Side to move
	ss >> token;
	bInfo.sideToMove = (token == 'w' ? WHITE : BLACK);
	ss >> token;

	// figure out which castling rights are applicable
	// and apply them
	while ((ss >> token) && !isspace(token))
	{
		int rookSquare;

		int color = islower(token) ? BLACK : WHITE;

		token = char(toupper(token));

		if (token == 'K') {
			rookSquare = relative_square(color, H1);
		}
		else if (token == 'Q') {
			rookSquare = relative_square(color, A1);
		}
		else continue;

		set_castling_rights(color, rookSquare);
	}

	// En passant square, ignore if no capture possible
	char row, col;

	if (((ss >> col) && (col >= 'a' && col <= 'h')) // TEST AND CHECK if ep square through FEN is working and correct
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

bool BitBoards::gives_check(const Move & m, const CheckInfo & ci)
{

	// direct check?
	if (ci.checkSq[m.piece] & m.to)
		return true;


	if (ci.dcCandidates
		&& (ci.dcCandidates & m.from)
		&& !aligned(m.from, m.to, ci.ksq))
		return true;

	switch (m.flag) {

		// Normal moves
	case '0':
		return false;

		// Promotions
	case 'P':
		return attacks_from<QUEEN>(m.to, FullTiles ^ squareBB[m.from]); // Other promotions not implemented

	// En passants
	case 'E':
	{
		// Find the true capture square for EP since m.to
		// represents destination square.
		int capsq = stm() == WHITE ? m.to + S : m.to + N;

		// "Move" the pieces, so we can see if there is a discoverd check
		// through the captured pawn.
		U64 bb = (FullTiles ^ squareBB[m.from] ^ squareBB[capsq]) | squareBB[m.to];

		return   (attacks_from<ROOK  >(ci.ksq, bb) & pieces(stm(), ROOK,   QUEEN))
			   | (attacks_from<BISHOP>(ci.ksq, bb) & pieces(stm(), BISHOP, QUEEN));
	}

	// Castling
	case 'C':
	{
		// Find rook to and from squares
		int rfrom = relative_square(stm(), (m.to < m.from ? A1 : H1));
		int rto   = relative_square(stm(), (m.to < m.from ? D1 : F1));

		// First we use a psuedo attack lookup table to check if their
		// king would recieve a check if there were no other pieces on the board
		// from this castle. If that's true, then we "move" the pieces and pass
		// attacks_from a occluded full tiles board with the pieces "moved"
		// and generate rook moves to see if the castle truely causes check. 
		return (PseudoAttacks[ROOK][rto] & squareBB[ci.ksq])
			&& (attacks_from<ROOK>(rto, (FullTiles ^ squareBB[m.from] ^ squareBB[rfrom]) | squareBB[rto] | squareBB[m.to]) & squareBB[ci.ksq]); //also test this!!!
	}
	}

	std::cout << "gives check error!!!!!!!!!!!!" << std::endl;
	return false;
}

// returns a bitboard of all pieces of color
// that are blocking checks on king of kingColor
U64 BitBoards::check_blockers(int color, int kingColor) const
{
	U64 b, pinners, result;

	int kingSq = king_square(kingColor);

	// bb of all sliding pieces that give check
	// if we remove the pieces in front of the king 
	pinners = ((piecesByType(ROOK,   QUEEN) & psuedoAttacks(ROOK,   WHITE, kingSq))
		    |  (piecesByType(BISHOP, QUEEN) & psuedoAttacks(BISHOP, WHITE, kingSq))) & pieces(!kingColor);

	while (pinners) {

		b = BetweenSquares[kingSq][pop_lsb(&pinners)] & FullTiles;

		if (!more_than_one(b))
			result |= b & pieces(color);
	}
	return result;
}

void BitBoards::set_castling_rights(int color, int rfrom)
{
	int king = king_square(color);

	int cside = king < rfrom ? KING_SIDE : QUEEN_SIDE;

	// White queenside castle least significant bit is set,
	// White kingside  second least sig bit set
	// Black queenside 3rd    least sig set
	// Black kingside  4th
	int cright = (1LL << ((cside == QUEEN_SIDE) + 2 * color));

	st->castlingRights         |= cright;
	castlingRightsMasks[king]  |= cright;
	castlingRightsMasks[rfrom] |= cright;

	// distance between rook and king,
	// minus their squares
	int d = SquareDistance[rfrom][king] - 2;

	// set cp to king square + or - 1 based on which way we're castling
	castlingPath[cright] |= 1LL << (king + (cside == KING_SIDE ? 1 : -1));

	// bitboard of path of castling, minus the squares
	// our rook and king occupy
	for (int i = 0; i < d; ++i) {
		castlingPath[cright] |= cside == KING_SIDE ? castlingPath[cright] << 1LL 
												   : castlingPath[cright] >> 1LL;
	}
}


void BitBoards::makeMove(const Move& m, StateInfo& newSt, int color)
{
	int them = !color;

	//gives_check(m, CheckInfo(*this));

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

		do_castling<true>(m, color);
	}

	// Update castling rights if a rook is moving for first time,
	// being  captured or if the king is moving 
	if (st->castlingRights && (castlingRightsMasks[m.from] | castlingRightsMasks[m.to])) {

		int cr = castlingRightsMasks[m.from] | castlingRightsMasks[m.to];

		st->Key ^= Zobrist::Castling[st->castlingRights & cr];

		st->castlingRights &= ~cr;
	}

	//update the TT key, capture update done in capture above
	st->Key ^= Zobrist::ZobArray[color][m.piece][m.from]
		    ^  Zobrist::ZobArray[color][m.piece][m.to  ]
		    ^  Zobrist::Color;

	// prefetch TT entry into cache ~THIS IS WAY TOO TIME INTENSIVE? 6.1% on just this call from here?
	// SWITCH TT to single address lookup instead of cluster of two?
	_mm_prefetch((char *)TT.first_entry(st->Key), _MM_HINT_NTA);
	
	assert(posOkay());

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	if (EmptyTiles & FullTiles || allPiecesColorBB[WHITE] & allPiecesColorBB[BLACK]) { //comment out in release
		drawBBA();
	}
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

	//undo the rook movment in castling
	if (m.flag == 'C') {
		do_castling<false>(m, color);
	}


	assert(posOkay());

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

}

template<int make>
inline void BitBoards::do_castling(const Move & m, int color)
{
	bool kingSide = m.to > m.from;

	int rFrom = relative_square(color, kingSide ? H1 : A1);
	int rTo   = relative_square(color, kingSide ? F1 : D1);

	//int rookFrom = relative_square(color, m.to) == C1     ? relative_square(color, A1) : relative_square(color, H1); //delete
	//int rookTo   = rookFrom == relative_square(color, A1) ? relative_square(color, D1) : relative_square(color, F1);

	// move the piece from to if we're making, 
	// and to from for unmake
	movePiece(ROOK, color, (make ? rFrom : rTo), (make ? rTo : rFrom));

	// if we're making the castle, change the keys
	// otherwise they'll be undone by reverting to prior *st
	if(make)
		st->Key ^= Zobrist::ZobArray[color][ROOK][rFrom]
				^  Zobrist::ZobArray[color][ROOK][rTo  ];
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

// Finds all attackers to a square
U64 BitBoards::attackers_to(int square, U64 occupied) const 
{
	return (attacks_from<PAWN>(square, WHITE)			   & pieces(BLACK, PAWN)
		  | attacks_from<PAWN>(square, BLACK)			   & pieces(WHITE, PAWN)
		  | attacks_from<KNIGHT>(square)				   & piecesByType(KNIGHT)
		  | slider_attacks.BishopAttacks(occupied, square) & piecesByType(BISHOP, QUEEN)
		  | slider_attacks.RookAttacks(occupied, square)   & piecesByType(ROOK, QUEEN)
		  | attacks_from<KING>(square)					   & piecesByType(KING));
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


