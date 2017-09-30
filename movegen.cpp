#include "movegen.h"

#include <algorithm> 

#include "hashentry.h"
#include "bitboards.h"
#include "slider_attacks.h"
#include "externs.h"
#include "ai_logic.h"
#include "Pawns.h"


//totally full bitboard
const U64 full  = 0xffffffffffffffffULL;
//files to keep pieces from moving left or right off board
const U64 notAFile = 0x7f7f7f7f7f7f7f7f; // ~0x8080808080808080
const U64 notHFile = 0xfefefefefefefefe; // ~0x0101010101010101
const U64 rank4 = 1095216660480L;
const U64 rank5=4278190080L;
const U64 rank6 = rank5 >> 8;
const U64 rank7 = rank6 >> 8;
const U64 rank8 = rank7 >> 8;
//ugh
const U64 rank3 = rank4 << 8;
const U64 rank2 = rank3 << 8;
const U64 rank1 = rank2 << 8;

//Bitboard of all king movements that can then be shifted
const U64 KING_SPAN = 460039L;
//board for knight moves that can be shifted
const U64 KNIGHT_SPAN = 43234889994L;
const U64 FileABB = 0x0101010101010101ULL;
const U64 FileBBB = FileABB << 1;
const U64 FileCBB = FileABB << 2;
const U64 FileDBB = FileABB << 3;
const U64 FileEBB = FileABB << 4;
const U64 FileFBB = FileABB << 5;
const U64 FileGBB = FileABB << 6;
const U64 FileHBB = FileABB << 7;
//files for keeping knight moves from wrapping
const U64 FILE_AB = FileABB + FileBBB;
const U64 FILE_GH = FileGBB + FileHBB;


template<int color, int genType>
SMoves* pawnMoves(const BitBoards& boards, SMoves *mlist, U64 target) {

	const int them = color == WHITE ? BLACK : WHITE;
	const int Up = color == WHITE ? N : S;
	const int Down = color == WHITE ? S : N;
	const int Right = color == WHITE ? NE : SE;
	const int Left = color == WHITE ? NW : SW;
	const int dpush = color == WHITE ? 16 : -16;

	const U64 thirdRank = color == WHITE ? rank3 : rank6;
	const U64 seventhRank = color == WHITE ? rank7 : rank2;
	const U64 eighthRank = color == WHITE ? rank8 : rank1;


	const U64 pawns = boards.pieces(color, PAWN) & ~seventhRank;
	const U64 candidatePawns = boards.pieces(color, PAWN) &  seventhRank;

	const U64 enemys = (genType == EVASIONS ? boards.pieces(them) ^ boards.pieces(them, KING) & target
		                                    : boards.pieces(them) ^ boards.pieces(them, KING));
	U64 moves, moves1;

	if (genType != CAPTURES) {

		moves = shift_bb<Up>(pawns) & boards.EmptyTiles;
		moves1 = shift_bb<Up>(moves  & thirdRank) & boards.EmptyTiles;

		if (genType == EVASIONS) {
			moves &= target;
			moves1 &= target;
		}

		// up one
		while (moves) {
			int index = pop_lsb(&moves);

			(mlist++)->move = create_move(index + Down, index);
		}

		// up two - If move is made, ep square is flagged in make move
		while (moves1) {
			int index = pop_lsb(&moves1);

			(mlist++)->move = create_move(index + dpush, index);
		}
	}


	//capture right
	moves = shift_bb<Right>(pawns) & enemys;
	while (moves) {
		int index = pop_lsb(&moves);

		(mlist++)->move = create_move(index - Right, index);
	}

	// capture left
	moves = shift_bb<Left>(pawns) & enemys;
	while (moves) {
		int index = pop_lsb(&moves);

		(mlist++)->move = create_move(index - Left, index);
	}

	// en passant
	if (boards.can_enpassant()) {

		int epSq = boards.ep_square();
		// find pawns that can attack ep square if they exist
		U64 epPawns = boards.psuedoAttacks(PAWN, them, epSq) & pawns;

		while (epPawns) {
			// ePawns is our pawn locations in this case
			(mlist++)->move = create_special<ENPASSANT, PIECE_EMPTY>(pop_lsb(&epPawns), epSq);
		}
	}

	// Pawn promotions, if we have pawns on the 7th..
	// generate promotions
	if (candidatePawns) {

		// moving forward one
		moves = shift_bb<Up>(candidatePawns) & boards.EmptyTiles & eighthRank;

		while (moves) {
			int index = pop_lsb(&moves);

			(mlist++)->move = create_special<PROMOTION, QUEEN>(index + Down, index);
		}

		// pawn capture promotions
		// capture right
		moves = shift_bb<Right>(candidatePawns) & enemys & eighthRank;

		while (moves) {
			int index = pop_lsb(&moves);

			(mlist++)->move = create_special<PROMOTION, QUEEN>(index - Right, index);
		}

		// capture left
		moves = shift_bb<Left>(candidatePawns) & enemys & eighthRank;

		while (moves) {
			int index = pop_lsb(&moves);

			(mlist++)->move = create_special<PROMOTION, QUEEN>(index - Left, index);
		}
	}
	return mlist;
}

template<int color, int cs>
SMoves* castling(const BitBoards& boards, SMoves *mlist) {

	int cr = cs == KING_SIDE ? color == WHITE ? WHITE_OO : BLACK_OO
		: color == WHITE ? WHITE_OOO : BLACK_OOO;

	// If we don't have the right to this castle
	if (!(cr & boards.castling_rights())) 
		return mlist;

	// If castling is blocked
	if (boards.castling_impeded(cr)) 
		return mlist;

	int ks = boards.king_square(color);

	int kto = relative_square(color, (cs == KING_SIDE ? G1 : C1));

	// Is the king moving left or right?
	int cDelta = (cs == KING_SIDE ? E : W);

	U64 enemys = boards.pieces(!color);

	// if an enemy us attacking any square
	// on the kings path, we don't generate the castling move
	for (int i = ks; i != kto; i += cDelta)
		if (boards.attackers_to(i, boards.FullTiles) & enemys)
			return mlist;

	(mlist++)->move = create_special<CASTLING, PIECE_EMPTY>(ks, kto);
	return mlist;
}

template<int color, int Pt>
SMoves* generateMoves(const BitBoards & board, SMoves *mlist, const U64 & target)
{
	// Grab the list of all pieces of a type
	const int *pieceList = board.pieceLoc[color][Pt];

	// Find the first piece and generate 
	// its moves
	int square;
	while ((square = *pieceList++) != SQ_NONE) {

		U64 moves = board.attacks_from<Pt>(square) & target;

		// Loop through those moves, removing them
		// from the board and pushing them to the 
		// move array + scoring
		while (moves) {
			//store moves
			(mlist++)->move = create_move(square, pop_lsb(&moves));
		}
	}
	return mlist;
}

template<int color, int genType> FORCE_INLINE
SMoves* generateAll(const BitBoards & board, SMoves *mlist, const U64 & target)
{
	mlist = pawnMoves<color, genType>(board, mlist, target); // once working be sure to add template genType param to pawn moves

	mlist = generateMoves<color, KNIGHT>(board, mlist, target);
	mlist = generateMoves<color, BISHOP>(board, mlist, target);
	mlist = generateMoves<color, ROOK  >(board, mlist, target);
	mlist = generateMoves<color, QUEEN >(board, mlist, target);

	if (genType != EVASIONS)
		mlist = generateMoves<color, KING>(board, mlist, target);


	if (genType != CAPTURES && genType != EVASIONS && board.can_castle(color)) {
		mlist = castling<color, KING_SIDE >(board, mlist); //move color to template param once working
		mlist = castling<color, QUEEN_SIDE>(board, mlist);
	}

	return mlist;
}

template<int genType>
SMoves* generate(const BitBoards & board, SMoves *mlist)
{
	U64 target;
	const int color = board.stm();


	// If only generating captures our target is only enemy pieces,
	// aside from E king. Else our target is all squares that are not
	// occupied by our friends or E king.
	target = genType == CAPTURES ?  (board.pieces(!color) ^ board.pieces(!color, KING))
		   : genType == MAIN_GEN ? ~(board.pieces(color)  | board.pieces(!color, KING))
		   : genType == QUIETS   ?   board.EmptyTiles     : 0;                                  ////////////////////////////////////////////////////////////////////////////////////////////////NEED TO ADD THIS TO OTHER TEMPLATES TO MAKE SURE ONLY QUIETS ARE GENERATED

	return color == WHITE ? generateAll<WHITE, genType>(board, mlist, target)
			              : generateAll<BLACK, genType>(board, mlist, target);
}

template SMoves* generate<MAIN_GEN>(const BitBoards & board, SMoves *mlist);
template SMoves* generate<CAPTURES>(const BitBoards & board, SMoves *mlist);
template SMoves* generate<QUIETS  >(const BitBoards & board, SMoves *mlist);

template<>
SMoves* generate<EVASIONS>(const BitBoards & board, SMoves *mlist) {

	U64 target;
	const int color = board.stm();

	int ksq = board.king_square(color);
	U64 sliderAtt = 0LL;
	U64 sliders = board.checkers() & ~board.piecesByType(PAWN, KNIGHT);

	// Find all squares attacked by slider checks
	// we remove them in order not to generate useless illegal moves
	while (sliders) {
		int checksq = pop_lsb(&sliders);
		sliderAtt |= LineBB[checksq][ksq] ^ board.square_bb(checksq);
	}

	//Generate all possible check evasions for king
	U64 bb = board.attacks_from<KING>(ksq) & ~board.pieces(color) & ~sliderAtt;

	while (bb) 
		(mlist++)->move = create_move(ksq, pop_lsb(&bb));
	
	// If double check, we can only use king moves
	if (more_than_one(board.checkers()))
		return mlist;

	// Target blocking evasions or capturing the checking piece
	int checksq = lsb(board.checkers());

	target = BetweenSquares[checksq][ksq] | board.square_bb(checksq)
		   & ~(board.pieces(color)        | board.pieces(!color, KING));

	return color == WHITE ? generateAll<WHITE, EVASIONS>(board, mlist, target)
				          : generateAll<BLACK, EVASIONS>(board, mlist, target);
}

void MoveGen::movegen_push(const BitBoards & board, int color, int piece, int captured, char flag, int from, int to) //change flags to int eventually
{
    //store move data to move object array
    moveAr[moveCount].from = from;
    moveAr[moveCount].to = to;
    moveAr[moveCount].piece = piece;
    moveAr[moveCount].captured = captured;
    moveAr[moveCount].flag = flag;

    //scoring capture moves
    if(captured){

		//higher id value for lower value piece,
		//captures are better if from lower (small bonus for both kinds of captures)
		//insuring identical captures, lower piece capturing always scores higher.
		int idAr[7] = { 0, 5, 4, 3, 2, 1, 0 }; 
		

        //Good captures are scored higher, based on BLIND better lower if not defended
        //need to add Static Exchange at somepoint
		if (blind(board, to, color, SORT_VALUE[piece], SORT_VALUE[captured])) moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece] + SORT_CAPT;

		else moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece];

		//else  moveAr[moveCount].score = SEE(moveAr[moveCount], board, color, true);
    }
	else {

		// Quiet moves are sorted by history score.                                
		moveAr[moveCount].score = history.history[color][from][to]; 
	}

    //pawn promotions
    if(moveAr[moveCount].flag == 'Q') moveAr[moveCount].score += SORT_PROM;

	//increment move counter so we know how many
	//moves we have to search and sort through
    moveCount ++;
}

bool MoveGen::blind(const BitBoards & board, int to, int color, int pieceVal, int captureVal) { //REplace move with just to square? use piece val to see if it equals PAWN val
	
	//better lower if not defeneded 

	//captures from pawns don't lose material
	if (pieceVal == SORT_VALUE[PAWN]) return true;

	//capture lower takes higher
	if (captureVal >= pieceVal - 50) return true;

	//return false; //currently in testing for ELO loss/gain
	// if square is attacked return false
	return !board.isSquareAttacked(to, color);
}

void MoveGen::reorderMoves(searchStack *ss, const HashEntry *entry)
{
	
    for(int i = 0; i < moveCount; ++i){
        //add killer moves score to move if there is a from - to match in killers
		if (moveAr[i] == ss->killers[0] //operator == overloaded in move.h
			&& moveAr[i].score < SORT_KILL) {
			moveAr[i].score = SORT_KILL;
		}

        if(moveAr[i] == ss->killers[1]
        && moveAr[i].score < SORT_KILL - 1){
            moveAr[i].score = SORT_KILL - 1;
        }

        if( entry && moveAr[i] == entry->move){
            moveAr[i].score = SORT_HASH;
        }
    }

}


