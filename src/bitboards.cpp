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
#include "movegen.h"
#include "Thread.h"



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
	int color = board.stm();
	ksq = board.king_square(!color);

	pinned		    = board.pinned_pieces(board.stm());
	dcCandidates    = board.check_candidates();

	checkSq[PAWN]   = board.attacks_from<PAWN>(ksq, !color);
	checkSq[KNIGHT] = board.attacks_from<KNIGHT>(ksq);
	checkSq[BISHOP] = board.attacks_from<BISHOP>(ksq);
	checkSq[ROOK]   = board.attacks_from<ROOK>(ksq);
	checkSq[QUEEN]  = checkSq[ROOK] | checkSq[BISHOP];
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

	// Find and record the max distance between two square on the game board.
	// Also build the BetweenSquares array which holds a U64 of all squares between two squares.
	// LineBB draws all squares that are aligned with two squares, be it diagonally, horizontally, or vertically. 
	for (int s1 = 0; s1 < 64; ++s1) {
		for (int s2 = 0; s2 < 64; ++s2) {
			BetweenSquares[s1][s2] = 0LL;
			LineBB[s1][s2] = 0LL;

			if (s1 != s2) {
				SquareDistance[s1][s2] = std::max(file_distance(s1, s2), rank_distance(s1, s2));
			}

			int pc = (PseudoAttacks[BISHOP][s1] & squareBB[s2]) ? BISHOP
				   : (PseudoAttacks[ROOK  ][s1] & squareBB[s2]) ? ROOK : PIECE_EMPTY;

			if (pc == PIECE_EMPTY)
				continue;

			LineBB[s1][s2]         = (attacks_bb(pc, s1, 0LL) & attacks_bb(pc, s2, 0LL)) | squareBB[s1] | squareBB[s2];

			BetweenSquares[s1][s2] = attacks_bb(pc, s1, squareBB[s2]) & attacks_bb(pc, s2, squareBB[s1]);
		}
	}	
}

U64 BitBoards::nextKey(Move m)
{
	U64 k = st->Key;
	const int color = stm();
	const int to = to_sq(m);
	const int captured = pieceOnSq(to);

	k ^= Zobrist::ZobArray[color][pieceOnSq(from_sq(m))][to];
	k ^= Zobrist::Color;

	if (captured)
		k ^= Zobrist::ZobArray[color][captured][to];

	return k;
}

void BitBoards::constructBoards(const std::string* FEN, Thread * th, StateInfo * si) //replace this with fen string reader
{
	// Set our internal state pointer to that
	// of the first entry in the 
	// StateListPtr deque
	st = si;

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
	//delete FEN; //posibly change this? not much reason for ptr anymore


	set_state(st, th);

	// mark empty tiles opposite of full tiles
  	EmptyTiles = ~FullTiles;	
}

void BitBoards::set_state(StateInfo * si, Thread * th)  ///////////////////////////////////// Remove bInfo and transfer stuff to stateInfo once more stuff is working. Possibly move zobrist key debug like stuff to here, so we can check board isokay better internally
{
	//set the start state
	//si->epSquare = SQ_NONE; // EP square set above in readFenString.!!
	si->MaterialKey = 0LL;
	si->PawnKey = 0LL;
	si->capturedPiece = 0;
	//get zobrist key of current position and store to board
	si->Key = zobrist.getZobristHash(*this);

	si->checkers = attackers_to(king_square(stm()), FullTiles) & pieces(!stm()); 

	//si->Key ^= Zobrist::Castling[st->castlingRights]; //already done in zobrist.gethash above!!

	// Set our internal thread ptr equal
	// to the thread who carrys this board.
	thisThread = th;


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
	int lookup[18] = {0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 2, 0, 1, 5, 4}; // upper case values - 'A'

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

		if (!(attackers_to(st->epSquare, FullTiles) & pieces(stm(), PAWN))
			|| !(pieces(!stm(), PAWN) & square_bb(st->epSquare + pawn_push(!stm()))))
			st->epSquare = SQ_NONE;			
	}
	else
		st->epSquare = SQ_NONE;
	
	ss >> std::skipws >> st->rule50 >> turns;
}

// Test if the un made move is legal
bool BitBoards::isLegal(const Move & m, U64 pinned) const
{
	int color =      stm();
	int from  = from_sq(m);

	
	// Move our pawn on occupied board,
	// remove their pawn behind EP square,
	// Test if our king is attacked.
	if (move_type(m) == ENPASSANT) {

		int to    =  to_sq(m);
		int ksq   =  king_square(color);
		int capsq =  to - pawn_push(color);

		U64 occ = (FullTiles ^ squareBB[from] ^ squareBB[capsq]) | squareBB[to];

		return !(attacks_from<ROOK  >(ksq, occ) & pieces(!color, QUEEN,   ROOK))
			&& !(attacks_from<BISHOP>(ksq, occ) & pieces(!color, QUEEN, BISHOP));
	}

	// Castling moves are checked for legality
	// in move generation, otherwise
	// figure out if our king is attacked
	if (pieceOnSq(from) == KING)
		return move_type(m) == CASTLING || ! (attackers_to(to_sq(m), FullTiles) & pieces(!color)); 

	// If the move isn't a king move it's legal
	// if it's not pinned, or it's moving along the pinning ray
	return !pinned 
		|| !(pinned & squareBB[from]) 
		|| aligned(from, to_sq(m), king_square(color));
}

bool BitBoards::pseudoLegal(Move m) const
{
	int color =           stm();
	int to    =        to_sq(m);
	int from  =      from_sq(m);
	int piece = pieceOnSq(from);

	if (move_type(m) != NORMAL) {
		return MoveList<LEGAL>(*this).contains(m);
	}
		
	if (to == from)
		return false;

	// If there's no piece or if the piece is 
	// not of our color it can't be pslegal
	if (piece == PIECE_EMPTY || color_of_pc(from) != color )
		return false;

	// Landing on own color piece
	if (square_bb(to) & pieces(color))
		return false;
		
	// It's not a promotion, so promotion piece must be empty
	if (promotion_type(m) - 2 != PIECE_EMPTY)
		return false;

	// Pawns handled differently due to odd attack
	// and movement patterns
	if (piece == PAWN) {                      

		// Move can not be a promotion
		if (rank_of(to) == relative_rank(color, 7))         
			return false;


		if (!(attacks_from<PAWN>(from, color) & pieces(!color) & squareBB[to]) // Not a capture

				&& !((from + pawn_push(color) == to) && empty(to)) // Not a single push

				&& !((from + 2 * pawn_push(color) == to) // Not a double pawn push
				&& (rank_of(from) == relative_rank(color, 1))
				&& empty(to)
				&& empty(to - pawn_push(color))))
			return false;
	}

	// If the from - to square is not a real piece move, 
	// considering the occupied board, the move isn't legal
	else if (  !(  (  (piece == ROOK || piece == BISHOP || piece == QUEEN) ? attacks_bb(piece, from, FullTiles) : psuedoAttacks(piece, color, from)) & squareBB[to] ) )
		return false;

	// If we're in check, because of the way isLegal checks moves
	// and the way they are generated, we need to make sure 
	// we filter out the moves that generate<EVASIONS> would
	// filter out for us normally.
	if (checkers()) {

		if (piece != KING) {

			// Double check
			if (more_than_one(checkers()))
				return false;

			// If the move doesn't block the check, or capture the checking piece, it's not pslegal
			if (   !(   (BetweenSquares[lsb(checkers())][king_square(color)] | checkers()   ) & squareBB[to]  ))
				return false;
		}

		else if (attackers_to(to, FullTiles ^ squareBB[from])  & pieces(!color))
			return false;
	}

	return true;
}

bool BitBoards::gives_check(Move m, const CheckInfo & ci)
{
	int from  =		 from_sq(m);
	int to    =        to_sq(m);
	int piece = pieceOnSq(from);
	

	// direct check?
	if (ci.checkSq[piece] & squareBB[to])
		return true;


	if (ci.dcCandidates
		&& (ci.dcCandidates & squareBB[from])
		&& !aligned(from, to, ci.ksq))
		return true;

	switch (move_type(m)) {

		// Normal moves
	case NORMAL:
		return false;

		// Castling
	case CASTLING:
	{
		// Find rook to and from squares
		int rfrom = relative_square(stm(), (to < from ? A1 : H1));
		int rto = relative_square(stm(), (to < from ? D1 : F1));

		// First we use a psuedo attack lookup table to check if their
		// king would recieve a check if there were no other pieces on the board
		// from this castle. If that's true, then we "move" the pieces and pass
		// attacks_from a occluded full tiles board with the pieces "moved"
		// and generate rook moves to see if the castle truely causes check. 
		return (PseudoAttacks[ROOK][rto] & squareBB[ci.ksq])
			&& (attacks_from<ROOK>(rto, (FullTiles ^ squareBB[from] ^ squareBB[rfrom]) | squareBB[rto] | squareBB[to]) & squareBB[ci.ksq]); //also test this!!!
	}

	// En passants
	case ENPASSANT:
	{
		// Find the true capture square for EP since m.to
		// represents destination square.
		int capsq = stm() == WHITE ? to + S : to + N;

		// "Move" the pieces, so we can see if there is a discoverd check
		// through the captured pawn.
		U64 bb = (FullTiles ^ squareBB[from] ^ squareBB[capsq]) | squareBB[to];

		return   (attacks_from<ROOK  >(ci.ksq, bb) & pieces(stm(), ROOK,   QUEEN))
			   | (attacks_from<BISHOP>(ci.ksq, bb) & pieces(stm(), BISHOP, QUEEN));
	}

	// Promotions
	case PROMOTION:
		return attacks_from<QUEEN>(to, FullTiles ^ squareBB[from]); // Other promotions not implemented


	default:
		std::cout << "gives check error!!!!!!!!!!!!" << std::endl;
		return false;
	}
}

// returns a bitboard of all pieces of color
// that are blocking checks on king of kingColor
U64 BitBoards::check_blockers(int color, int kingColor) const
{
	U64 b, pinners, result = 0LL;

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
	// No need to enforce sequential consistancy here 
	thisThread->nodes.fetch_add(1, std::memory_order_relaxed);

	int them = !color;

	int to       =        to_sq(m);
	int from     =      from_sq(m);
	int piece    = pieceOnSq(from);

	int captured = move_type(m) == ENPASSANT ? PAWN : pieceOnSq(to);

	CheckInfo ci(*this);
	bool checking = gives_check(m, ci);

	assert(posOkay());

	//copy current board state to board state stored on ply
	std::memcpy(&newSt, st, sizeof(StateInfo));
	//set previous state on ply to current state
	newSt.previous = st;
	//use st to modify values on ply
	st = &newSt;

	++st->rule50;
	++st->plysFromNull;

	//remove or add captured piece from BB's same as above for moving piece
	if (captured) {
		int capSq = to;

		if (captured == PAWN) {
			
			//en passant
			if (move_type(m) == ENPASSANT) {
				//set capture square to correct pos and not ep square 
				capSq += pawn_push(them);
			}
			//update the pawn hashkey on capture
			st->PawnKey ^= Zobrist::ZobArray[them][PAWN][capSq];
		}

		else { 
			bInfo.nonPawnMaterial[them] -= SORT_VALUE[captured];
		}
		//remove captured piece
		removePiece(captured, them, capSq);
		//update material
		bInfo.sideMaterial[them] -= SORT_VALUE[captured];
		// update TT key
		st->Key ^= Zobrist::ZobArray[them][captured][capSq];
		//update material key
		st->MaterialKey ^= Zobrist::ZobArray[them][captured][pieceCount[them][captured]+1];

		// Reset 50 move rule.
		st->rule50 = 0;
	}

	//move the piece from - to
	movePiece(piece, color, from, to);

	st->capturedPiece = captured;

	// if there was an EP square, reset it for the next ply 
	// and remove the ep zobrist XOR
	if (st->epSquare != SQ_NONE) {

		st->Key ^= Zobrist::EnPassant[file_of(st->epSquare)];
		st->epSquare = SQ_NONE;
	}

	if (piece == PAWN) {
		
		//if the pawn move is two forward,
		//and there is an enemy pawn positioned to en passant
		if ((to ^ from) == 16
			&& (psuedoAttacks(PAWN, color, from + pawn_push(color)) & pieces(them, PAWN))) {
			
			st->epSquare = (from + to) / 2;
			st->Key ^= Zobrist::EnPassant[file_of(st->epSquare)];
		} 
		
		// pawn promotion
		if (move_type(m) == PROMOTION) {
			//remove pawn placed
			removePiece(PAWN, color, to);

			int promT = promotion_type(m);

			//add queen to to square
			addPiece(promT, color, to);
			//update material
			bInfo.sideMaterial[color]    += SORT_VALUE[promT] - SORT_VALUE[PAWN];
			bInfo.nonPawnMaterial[color] += SORT_VALUE[promT];

			st->Key ^= Zobrist::ZobArray[color][promT][to] ^ Zobrist::ZobArray[color][PAWN][to];
			//update pawn key
			st->PawnKey ^= Zobrist::ZobArray[color][PAWN][to];
			//update material key
			st->MaterialKey   ^= Zobrist::ZobArray[color][promT][pieceCount[color][promT]]
							  ^  Zobrist::ZobArray[color][PAWN ][pieceCount[color][PAWN]+1];

			// Reset 50 move rule
			st->rule50;
		}
		
		//update the pawn key, if it's a promotion it cancels out and updates correctly
		st->PawnKey ^= zobrist.zArray[color][PAWN][to] ^ zobrist.zArray[color][PAWN][from];
		//_mm_prefetch((char *)pawnsTable[bInfo.PawnKey], _MM_HINT_NTA); // TEST for ELO GAIN, BOTH PLACES WITH PREFETCH HAD ELO LOSS
	}
	// Castling ~~ only implemented to be done by another,
	// engine itself cannot castle yet. Next thing on agenda.
	else if (move_type(m) == CASTLING) {

		do_castling<true>(from, to, color);
	}

	// Update castling rights if a rook is moving for first time,
	// being  captured or if the king is moving 
	if (st->castlingRights && (castlingRightsMasks[from] | castlingRightsMasks[to])) {

		int cr = castlingRightsMasks[from] | castlingRightsMasks[to];

		st->Key ^= Zobrist::Castling[st->castlingRights];

		st->castlingRights &= ~cr;

		st->Key ^= Zobrist::Castling[st->castlingRights];
	}

	//update the TT key, capture update done in capture above
	st->Key ^= Zobrist::ZobArray[color][piece][from]
		    ^  Zobrist::ZobArray[color][piece][to  ]
		    ^  Zobrist::Color;


	// prefetch TT entry into cache ~THIS IS WAY TOO TIME INTENSIVE? 6.1% on just this call from here?
	// SWITCH TT to single address lookup instead of cluster of two?
	prefetch(TT.first_entry(st->Key));

	st->checkers = 0LL;

	if (checking) {

		if (move_type(m) != NORMAL)
			st->checkers = attackers_to(king_square(them), FullTiles) & pieces(color);

		else {

			//Direct checks
			if (ci.checkSq[piece] & squareBB[to])
				st->checkers |= squareBB[to];

			//Discoverd checks
			if (ci.dcCandidates && (ci.dcCandidates & squareBB[from])) {

				if (piece != ROOK)
					st->checkers |= attacks_from<ROOK  >(king_square(them)) & pieces(color, ROOK,   QUEEN);

				if (piece != BISHOP)
					st->checkers |= attacks_from<BISHOP>(king_square(them)) & pieces(color, BISHOP, QUEEN);
			}
		}
	}
	
	assert(posOkay());

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;

	if (EmptyTiles & FullTiles || allPiecesColorBB[WHITE] & allPiecesColorBB[BLACK] || m == MOVE_NONE) { //comment out in release
		drawBBA();
	}
}

void BitBoards::unmakeMove(const Move & m, int color)
{
	int them = !color;

	int to    =  to_sq(m);
	int from  =  from_sq(m);
	int type  =  move_type(m);
	int piece =  pieceOnSq(to);

	assert(posOkay());

	if (type != PROMOTION) {

		// move piece
		movePiece(piece, color, to, from);
	}
	//pawn promotion
	else if (type == PROMOTION){

		int promT = promotion_type(m); //////////////////// USE THIS TO DO OTHER PROMOTIONS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		addPiece(PAWN, color, from);
		//remove queen
		removePiece(promT, color, to);

		bInfo.sideMaterial[color]    += SORT_VALUE[PAWN ] - SORT_VALUE[promT];
		bInfo.nonPawnMaterial[color] -= SORT_VALUE[promT];
	}

	//add captured piece from BB's similar to above for moving piece
	if (st->capturedPiece) {
		int capSq = to;
		int captured = st->capturedPiece;

		// set capture square correctly if 
		// move is an en passant
		if (type == ENPASSANT) {
			capSq += pawn_push(them);
		}

		//add captured piece to to square
		addPiece(captured, them, capSq);
		bInfo.sideMaterial[them] += SORT_VALUE[captured];

		//if pawn captured undone, update pawn key
		if (captured != PAWN) bInfo.nonPawnMaterial[them] += SORT_VALUE[captured];
	}

	//undo the rook movment in castling
	if (type == CASTLING) {
		do_castling<false>(from, to, color);
	}

	assert(posOkay());

	//restore state pointer to state before we made this move
	st = st->previous;

	//flip internal side to move
	bInfo.sideToMove = !bInfo.sideToMove;
}

// Makes or unmakes a castling move depending on template true, false.
template<int make>
inline void BitBoards::do_castling(int from, int to, int color)
{
	bool kingSide = to > from;

	int rFrom = relative_square(color, kingSide ? H1 : A1);
	int rTo   = relative_square(color, kingSide ? F1 : D1);

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

	++st->rule50;
	st->plysFromNull = 0;

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

bool BitBoards::isDraw(int ply)
{
	// If we've reached 100 half moves and we either don't have checkers,
	// or we have atleast one move it's a draw by the 50 move rule.
	if (st->rule50 > 99 && (!checkers() || MoveList<LEGAL>(*this).size() ))
		return true;

	int end = std::min(st->rule50, st->plysFromNull);

	if (end < 4)
		return false;

	StateInfo * stp = st->previous->previous;
	int count = 0;

	for (int i = 4; i <= end; i += 2) {

		stp = stp->previous->previous;

		// Return a draw score if a position repeats once earlier but strictly
		// after the root, or repeats twice before or at the root.

		if (stp->Key == st->Key
			&& ++count + (ply > i) == 2)
			return true;
	}
	return false;
}

int BitBoards::SEE(const Move& m, int color, bool isCapture) const
{
	U64 attackers, occupied, stmAttackers;
	int swapList[32], index = 1;

	int from     =      from_sq(m);
	int to       =		  to_sq(m);
	int piece    = pieceOnSq(from);
	int captured =   pieceOnSq(to);
	int type     =    move_type(m);


	//early return, SEE can't be a losing capture
	//is capture flag is used for when we're checking to see if the move is escaping capture
	//we don't want an early return if that's the case
	if (SORT_VALUE[piece] <= SORT_VALUE[captured] && isCapture) return INF;

	// return in case of a castling move
	if (type == CASTLING) return 0;

	swapList[0] = SORT_VALUE[captured];

	occupied = FullTiles ^ square_bb(from); //remove capturing piece from occupied bb 

												  //remove captured pawn
	if (type == ENPASSANT) {
		occupied   ^= square_bb(to - pawn_push(!color));
		swapList[0] = SORT_VALUE[PAWN];
	}

	//finds all attackers to the square
	attackers = attackers_to(to, occupied) & occupied;

	//switch sides
	color = !color;
	stmAttackers = attackers & allPiecesColorBB[color];


	if (!stmAttackers) return swapList[0];

	captured = piece; //if there are attackers, our piece being moved will be captured

	//loop through and add pieces that can capture on the square to swapList
	do {

		//add entry to swap list
		swapList[index] = -swapList[index - 1] + SORT_VALUE[captured];


		captured = min_attacker<PAWN>(color, to, stmAttackers, occupied, attackers);

		if (captured == KING) {

			if (stmAttackers == attackers) {
				index++;
			}
			break;
		}

		color = !color;
		stmAttackers = attackers & allPiecesColorBB[color];

		index++;
	} while (stmAttackers);

	//negamax through swap list finding best possible outcome for starting side to move
	while (--index) {
		swapList[index - 1] = std::min(-swapList[index], swapList[index - 1]);
	}

	return swapList[0];
}

bool BitBoards::seeGe(Move m, int Threshold)
{
	if (move_type(m) != NORMAL)
		return 0 >= Threshold;

	 
	int to     =	   	   to_sq(m);
	int from   =         from_sq(m);
	int victim =    pieceOnSq(from); 
	int stm    = !color_of_pc(from); // Look at opponenets move first
	int balance;					 // Value of pieces captured by us, minus opponents captures.
	U64 occupied, stmAttackers;

	balance = SORT_VALUE[pieceOnSq(to)];

	if (balance < Threshold)
		return false;

	balance -= SORT_VALUE[victim];

	if (balance >= Threshold)
		return true;

	bool relativeStm = true;
	occupied = FullTiles ^ squareBB[from] ^ squareBB[to];

	U64 attackers = attackers_to(to, occupied);

	while (true)
	{
		stmAttackers = attackers & pieces(stm);

		if (!stmAttackers)
			return relativeStm;

		victim = min_attacker<PAWN>(stm, to, stmAttackers, occupied, attackers);

		if (victim == KING)
			return relativeStm = bool(attackers & pieces(!stm));

		balance += relativeStm ?  SORT_VALUE[victim]
							   : -SORT_VALUE[victim];

		relativeStm = !relativeStm;

		if (relativeStm == (balance >= Threshold))
			return relativeStm;

		stm = !stm;
	}

}

// Find the smallest attacker to a square
template<int Pt> FORCE_INLINE
int BitBoards::min_attacker(int color, int to, const U64 & stmAttackers, U64 & occupied, U64 & attackers) const
{

	// is there a piece of this Pt that we've previously found
	// attacking the square?
	U64 bb = stmAttackers & pieces(color, Pt);

	// if not, increment piece type and re-search
	if (!bb)
		return min_attacker<Pt + 1>(color, to, stmAttackers, occupied, attackers);

	// We've found a attackerr!
	// remove piece from occupied board
	occupied ^= bb & ~(bb - 1);

	//find xray attackers behind piece once it's been removed and add to attackers
	if (Pt == PAWN || Pt == BISHOP || Pt == QUEEN) {
		attackers |= slider_attacks.BishopAttacks(occupied, to) & (pieces(color, BISHOP, QUEEN));
	}

	if (Pt == ROOK || Pt == QUEEN) {
		attackers |= slider_attacks.RookAttacks(occupied, to) & (pieces(color, ROOK, QUEEN));
	}

	//add new attackers to board
	attackers &= occupied;

	//return attacker
	return Pt;
}

template<> FORCE_INLINE // If we've checked all the pieces, return KING and stop SEE search
int BitBoards::min_attacker<KING>(int , int , const U64 & , U64 & , U64 & ) const{
	return KING;
}

// Checks if square input is attacked by color opposite of input color
bool BitBoards::isSquareAttacked(const int square, const int color) const {

	const int them = !color;

	U64 attacks = psuedoAttacks(PAWN, color, square); //reverse color pawn attacks so we can find enemy pawns

	if (attacks & pieces(them, PAWN)) return true;

	attacks = psuedoAttacks(KNIGHT, them, square);

	if (attacks & pieces(them, KNIGHT)) return true;

	attacks = slider_attacks.BishopAttacks(FullTiles, square);

	if (attacks & pieces(them, BISHOP, QUEEN)) return true;

	attacks = slider_attacks.RookAttacks(FullTiles, square);

	if (attacks & pieces(them, ROOK, QUEEN)) return true;

	attacks = psuedoAttacks(KING, them, square);

	if (attacks & pieces(them, KING)) return true;

	return false;
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
void BitBoards::drawBB(U64 board) const
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
void BitBoards::drawBBA() const
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


