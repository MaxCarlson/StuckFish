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


//[white/black][queenside/kingside]
const U64 castlingMasks[COLOR][2] = {
	{ { 0xe00000000000000ULL },{ 0x6000000000000000ULL } },
	{ { 0xeULL               },{ 0x60ULL               } }
};

static const U64 RankMasks8[8] = //from rank 1 - 8 
{
	0xFF00000000000000L, 0xFF000000000000L,  0xFF0000000000L, 0xFF00000000L, 0xFF000000L, 0xFF0000L, 0xFF00L, 0xFFL,
};

static const U64 FileMasks8[8] =/*from fileA to FileH*/
{
    0x101010101010101L, 0x202020202020202L, 0x404040404040404L, 0x808080808080808L,
    0x1010101010101010L, 0x2020202020202020L, 0x4040404040404040L, 0x8080808080808080L
};


/*************************************************
* Values used for sorting captures are the same  *
* as normal piece values, except for a king.     *
*************************************************/
const int SORT_VALUE[7] = { 0, 100, 325, 335, 500, 975, 0 };


template<int genType>
void MoveGen::generate(const BitBoards & board)
{
	//counts total moves generated
	moveCount = 0;
	const int color = board.stm();

	U64 target = genType == CAPTURES
		? (board.pieces(!color) ^ board.pieces(!color, KING))
		: ~(board.pieces(color) | board.pieces(!color, KING));

	color == WHITE ? generateAll<WHITE, genType>(board, target)
		: generateAll<BLACK, genType>(board, target);
}

template void MoveGen::generate<MAIN_GEN>(const BitBoards & board);
template void MoveGen::generate<CAPTURES>(const BitBoards & board);

template<int color, int genType> FORCE_INLINE
void MoveGen::generateAll(const BitBoards & board, const U64 & target)
{
	pawnMoves<color>(board, genType); // once working be sure to add template genType param to pawn moves

	generateMoves<color, KNIGHT>(board, target);
	generateMoves<color, BISHOP>(board, target);
	generateMoves<color, ROOK  >(board, target);
	generateMoves<color, QUEEN >(board, target);
	generateMoves<color, KING  >(board, target);

	if (genType != CAPTURES && board.can_castle(color)) {
		castling<KING_SIDE >(board, color); //move color to template param once working
		castling<QUEEN_SIDE>(board, color);
	}

}

template<int color, int Pt> FORCE_INLINE
void MoveGen::generateMoves(const BitBoards & board, const U64 & target)
{
	const int *pieceList = board.pieceLoc[color][Pt];

	int square;
	while ((square = *pieceList++) != SQ_NONE) {

		U64 moves = board.attacks_from<Pt>(square) & target;

		while (moves) {
			//store moves
			int index = pop_lsb(&moves);

			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, Pt, captured, '0', square, index);
		}
	}
}

template<int color>
void MoveGen::pawnMoves(const BitBoards& boards, bool capturesOnly) {

	const int them = color == WHITE ? BLACK : WHITE;
	const int Up = color == WHITE ? N : S;
	const int Down = color == WHITE ? S : N;
	const int Right = color == WHITE ? NE : SE;
	const int Left = color == WHITE ? NW : SW;
	const int dpush = color == WHITE ? 16 : -16;


	const U64 pawns = boards.pieces(color, PAWN);
	const U64 enemys = boards.pieces(them) ^ boards.pieces(them, KING);
	U64 moves;

	if (!capturesOnly) {
		// up one
		moves = shift_bb<Up>(pawns) & boards.EmptyTiles;
		while (moves) {
			int index = pop_lsb(&moves);

			movegen_push(boards, color, PAWN, PIECE_EMPTY, '0', index + Down, index);
		}

		// up two - If move is made, ep square is flagged in make move
		const int dpushrank = relative_rank(color, 3);
		moves = shift_bb<Up>((shift_bb<Up>(pawns) & boards.EmptyTiles)) & boards.EmptyTiles & RankMasks8[dpushrank];
		while (moves) {
			int index = pop_lsb(&moves);

			movegen_push(boards, color, PAWN, PIECE_EMPTY, '0', index + dpush, index);
		}

	}

	//capture right
	moves = shift_bb<Right>(pawns) & enemys;
	while (moves) {
		int index = pop_lsb(&moves);

		int captured = boards.pieceOnSq(index);

		movegen_push(boards, color, PAWN, captured, '0', index - Right, index);
	}

	// capture left
	moves = shift_bb<Left>(pawns) & enemys;
	while (moves) {
		int index = pop_lsb(&moves);

		int captured = boards.pieceOnSq(index);

		movegen_push(boards, color, PAWN, captured, '0', index - Left, index);
	}

	///*	// en passant
	if (boards.can_enpassant()) {

		int epSq = boards.ep_square();
		// find pawns that can attack ep square if they exist
		U64 epPawns = boards.psuedoAttacks(PAWN, them, epSq) & pawns;

		while (epPawns) {
			// index is our pawn locations in this case,
			// not where the pawn is moving like in rest of template
			int index = pop_lsb(&epPawns);

			movegen_push(boards, color, PAWN, PAWN, 'E', index, epSq);
		}
	}
	//*/

	//rank mask of 7th rank relative side to move
	const U64 seventhRank = RankMasks8[relative_rank(color, 6)];

	// Pawn promotions, if we have pawns on the 7th..
	// generate promotions
	if (pawns & seventhRank) {

		const U64 eighthRank = RankMasks8[relative_rank(color, 7)];

		// moving forward one
		moves = shift_bb<Up>(pawns) & boards.EmptyTiles & eighthRank;

		while (moves) {
			int index = pop_lsb(&moves);

			movegen_push(boards, color, PAWN, PIECE_EMPTY, 'Q', index + Down, index);
		}

		// pawn capture promotions
		// capture right
		moves = shift_bb<Right>(pawns) & enemys & eighthRank;

		while (moves) {
			int index = pop_lsb(&moves);

			int	captured = boards.pieceOnSq(index);

			movegen_push(boards, color, PAWN, captured, 'Q', index - Right, index);
		}


		// capture left
		moves = shift_bb<Left>(pawns) & enemys & eighthRank;

		while (moves) {
			int index = pop_lsb(&moves);

			int	captured = boards.pieceOnSq(index);

			movegen_push(boards, color, PAWN, captured, 'Q', index - Left, index);
		}
	}
}

template<int cs>
void MoveGen::castling(const BitBoards& boards, int color) {

	int cr = cs == KING_SIDE ? color == WHITE ? WHITE_OO : BLACK_OO
		: color == WHITE ? WHITE_OOO : BLACK_OOO;

	// If we don't have the right to this castle
	if (!(cr & boards.castling_rights())) return;

	// If castling is blocked
	if (boards.castling_impeded(cr)) return;

	int ks = boards.king_square(color);

	int kto = relative_square(color, (cs == KING_SIDE ? G1 : C1));

	// Is the king moving left or right?
	int cDelta = (cs == KING_SIDE ? E : W);

	U64 enemys = boards.pieces(!color);

	// if an enemy us attacking any square
	// on the kings path, we don't generate the castling move
	for (int i = ks; i != kto; i += cDelta)
		if (attackersTo(boards, boards.FullTiles, i) & enemys)
			return;

	movegen_push(boards, color, KING, PIECE_EMPTY, 'C', ks, kto);
}


void MoveGen::generatePsMoves(const BitBoards& boards, bool capturesOnly)
{
    //counts total moves generated
    moveCount = 0;
	int color = boards.stm();

	if(color == WHITE) pawnMoves<WHITE>(boards, capturesOnly);
	else		       pawnMoves<BLACK>(boards, capturesOnly);

    //if we don't want to only generate captures
    U64 capsOnly = full;

    //if we only want to generate captures
    if(capturesOnly) capsOnly = boards.FullTiles;

	// remove friends and enemy king 
	// from all possible moves
	capsOnly ^= boards.pieces(!color, KING) ^ boards.pieces(color);

	// generate moves, looping through a *list of piece
	// locations and generating moves for them,
	// poping the lsb of the moves and pushing it 
	// to the move array stored locally.
	possibleN(boards, color, capsOnly);
	possibleB(boards, color, capsOnly);
	possibleR(boards, color, capsOnly);
	possibleQ(boards, color, capsOnly);
	possibleK(boards, color, capsOnly);

	if (boards.can_castle(color) && !capturesOnly) { 
		castling<KING_SIDE >(boards, color);
		castling<QUEEN_SIDE>(boards, color);
	}

    return;
}

void MoveGen::possibleN(const BitBoards& board, int color, const U64 &capturesOnly)
{

	const int* list = board.pieceLoc[color][KNIGHT];
	//U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	//U64 eking = board.pieces(!color, KING);

	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = board.psuedoAttacks(KNIGHT, color, square) & capturesOnly;
		//moves &= capturesOnly & ~(friends | eking);

		while (moves) {
			//store moves
			int index = pop_lsb(&moves);

			//U64 landing = board.square_bb(index);
			//int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;
			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, KNIGHT, captured, '0', square, index);
		}
	}


}

void MoveGen::possibleB(const BitBoards& board, int color, const U64 &capturesOnly)
{

	const int* list = board.pieceLoc[color][BISHOP];

	U64 enemys = board.pieces(!color);

	int square;
	while ((square = *list++) != SQ_NONE) {
	
		U64 moves = slider_attacks.BishopAttacks(board.FullTiles, square)  & capturesOnly;


		while (moves) {
			int index = pop_lsb(&moves);

			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, BISHOP, captured, '0', square, index);

		}

	}

 
}

void MoveGen::possibleR(const BitBoards& board, int color, const U64 &capturesOnly)
{

	const int* list = board.pieceLoc[color][ROOK];

	U64 enemys = board.pieces(!color);


	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = slider_attacks.RookAttacks(board.FullTiles, square)  & capturesOnly;
		
		while (moves) {
			int index = pop_lsb(&moves);

			//U64 landing = board.square_bb(index);
			//int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;
			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, ROOK, captured, '0', square, index);

		}

	}

    

}

void MoveGen::possibleQ(const BitBoards& board, int color, const U64 &capturesOnly)
{
	const int* list = board.pieceLoc[color][QUEEN];

	U64 enemys = board.pieces(!color);

	int square;
	while ((square = *list++) != SQ_NONE) {

		//grab moves from magic bitboards
		U64 moves = slider_attacks.QueenAttacks(board.FullTiles, square)  & capturesOnly;

		while (moves) {
			int index = pop_lsb(&moves);

			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, QUEEN, captured, '0', square, index);
		}
	}
}

void MoveGen::possibleK(const BitBoards& board, int color, const U64 &capturesOnly)
{
	const int* list = board.pieceLoc[color][KING];
	U64 enemys = board.pieces(!color);

	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = board.psuedoAttacks(KING, color, square) & capturesOnly;

		//non castling moves
		while (moves) {
			int index = pop_lsb(&moves);

			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, KING, captured, '0', square, index);
		}
	}
}

int MoveGen::whichPieceCaptured(const BitBoards & board, int location, int color) // no longer needed??
{
	for (int i = PAWN; i < KING; ++i) {
		if (board.pieces(!color, i) & board.square_bb(location)) return i;
	}
	return PIECE_EMPTY;
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
		if (blind(board, to, color, SORT_VALUE[piece], SORT_VALUE[captured])) moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece] + SORT_CAPT; //

		else  moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece];

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

	// if square is attacked return false
	return !isSquareAttacked(board, to, color);
}

int MoveGen::SEE(const Move& m, const BitBoards& b, int color, bool isCapture)
{
	U64 attackers, occupied, stmAttackers;
	int swapList[32], index = 1; 

	//early return, SEE can't be a losing capture
	//is capture flag is used for when we're checking to see if the move is escaping capture
	//we don't want an early return if that's the case
	if (SORT_VALUE[m.piece] <= SORT_VALUE[m.captured] && isCapture) return INF;

	// return in case of a castling move
	if (m.flag == 'C') return 0;

	swapList[0] = SORT_VALUE[m.captured];

	occupied = b.FullTiles ^ b.square_bb(m.from); //remove capturing piece from occupied bb 

	//remove captured pawn
	if (m.flag == 'E') {
		occupied ^= m.to - pawn_push(!color);
	}

	//finds all attackers to the square
	//attackers = attackersTo(m.to, b, occupied) & occupied;
	attackers = attackersTo(b, occupied, m.to) & occupied;

	//if there are no attacking pieces, return
	if(!(attackers & b.allPiecesColorBB[color])) return swapList[0];


	//switch sides
	color = !color;
	stmAttackers = attackers & b.allPiecesColorBB[color];


	if (!stmAttackers) return swapList[0];
	
	int captured = m.piece; //if there are attackers, our piece being moved will be captured

	//loop through and add pieces that can capture on the square to swapList
	do {
		if (index > 31) break;

		//add entry to swap list
		swapList[index] = -swapList[index - 1] + SORT_VALUE[captured];

	
		captured = min_attacker<PAWN>(b, color, m.to, stmAttackers, occupied, attackers);

		if (captured == KING) {
			if (stmAttackers == attackers) {
				index++;
			}
			break;
		}

		//isWhite = !isWhite; remove if new min_attackers function is solid
		//captured = min_attacker(isWhite, b, m.to, stmAttackers, occupied, attackers);

		color = !color;
		stmAttackers = attackers & b.allPiecesColorBB[color];

		index++;
	} while (stmAttackers);
	
	//negamax through swap list finding best possible outcome for starting side to move
	while (--index) {
		swapList[index - 1] = std::min(-swapList[index], swapList[index - 1]);
	}

	return swapList[0];
}

//finds all attackers to a location
U64 MoveGen::attackersTo(const BitBoards& b, const U64 occ, int square) {

	U64 attackers;

	// Note. Color does not matter on b.psuedoAttacks(KING) / KNIGHT

	attackers  = b.psuedoAttacks(PAWN, BLACK, square)      & b.pieces(WHITE, PAWN);
	attackers |= b.psuedoAttacks(PAWN, WHITE, square)      & b.pieces(BLACK, PAWN);

	attackers |= b.psuedoAttacks(KNIGHT, WHITE, square)    & b.piecesByType(KNIGHT);

	attackers |= slider_attacks.BishopAttacks(occ, square) & b.piecesByType(BISHOP, QUEEN);

	attackers |= slider_attacks.RookAttacks(occ, square)   & b.piecesByType(ROOK, QUEEN);

	attackers |= b.psuedoAttacks(KING, WHITE, square)      & b.piecesByType(KING);

	return attackers;
}
// Find the smallest attacker to a square
template<int Pt> FORCE_INLINE
int MoveGen::min_attacker(const BitBoards & b, int color, const int & to, const U64 & stmAttackers, U64 & occupied, U64 & attackers) {

	// is there a piece of this Pt that we've previously found
	// attacking the square?
	U64 bb = stmAttackers & b.pieces(color, Pt);

	// if not, increment piece type and re-search
	if (!bb)
		return min_attacker<Pt + 1>(b, color, to, stmAttackers, occupied, attackers);

	// We've found a attackerr!
	// remove piece from occupied board
	occupied ^= bb & ~(bb - 1);

	//find xray attackers behind piece once it's been removed and add to attackers
	if (Pt == PAWN || Pt == BISHOP || Pt == QUEEN) {                       
		attackers |= slider_attacks.BishopAttacks(occupied, to) & (b.pieces(color, BISHOP, QUEEN));
	}

	if (Pt == ROOK || Pt == QUEEN) {
		attackers |= slider_attacks.RookAttacks(occupied, to) & (b.pieces(color, ROOK, QUEEN));
	}

	//add new attackers to board
	attackers &= occupied;

	//return attacker
	return Pt;
}

template<> FORCE_INLINE // If we've checked all the pieces, return KING and stop SEE search
int MoveGen::min_attacker<KING>(const BitBoards & b, int color, const int & to, const U64 & stmAttackers, U64 & occupied, U64 & attackers) {
	return KING;
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

// Just test to see if our king is attacked by any enemy pieces
// castling moves are checked for legality durning move gen, aside from
// this king in check test. En passants are done the same way
bool MoveGen::isLegal(const BitBoards & b, const Move & m, int color) 
{
	const int them = !color;
	const int kingLoc = b.king_square(color);

	return !isSquareAttacked(b, kingLoc, color);
}

bool MoveGen::isSquareAttacked(const BitBoards & b, const int square, const int color) {

	const int them = !color;

	U64 attacks = b.psuedoAttacks(PAWN, color, square); //reverse color pawn attacks so we can find enemy pawns

	if (attacks & b.pieces(them, PAWN)) return true;

	attacks = b.psuedoAttacks(KNIGHT, them, square);

	if (attacks & b.pieces(them, KNIGHT)) return true;

	attacks = slider_attacks.BishopAttacks(b.FullTiles, square);

	if (attacks & b.pieces(them, BISHOP, QUEEN)) return true;

	attacks = slider_attacks.RookAttacks(b.FullTiles, square);

	if (attacks & b.pieces(them, ROOK, QUEEN)) return true;

	attacks = b.psuedoAttacks(KING, them, square);

	if (attacks & b.pieces(them, KING)) return true;

	return false;
}
