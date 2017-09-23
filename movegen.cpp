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

template<int color>
void MoveGen::pawnMoves(const BitBoards& boards, bool capturesOnly) {
	
	const int them     = color == WHITE ?  BLACK : WHITE;
	const int Up       = color == WHITE ?  N     :     S;
	const int Down     = color == WHITE ?  S     :     N;
	const int Right    = color == WHITE ?  NE    :    SE;
	const int Left     = color == WHITE ?  NW    :    SW;
	const int dpush    = color == WHITE ?  16    :   -16;


	const U64 pawns  = boards.pieces(color, PAWN);
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

void MoveGen::generatePsMoves(const BitBoards& boards, bool capturesOnly)
{
    //counts total moves generated
    moveCount = 0;
	int color = boards.stm();

	if(color == WHITE) pawnMoves<WHITE>(boards, capturesOnly);
	else		pawnMoves<BLACK>(boards, capturesOnly);

    //if we don't want to only generate captures
    U64 capsOnly = full;

    //if we only want to generate captures
    if(capturesOnly) capsOnly = boards.pieces(!color);

	// generate moves, looping through a *list of piece
	// locations and generating moves for them,
	// poping the lsb of the moves and pushing it 
	// to the move array stored locally.
	possibleN(boards, color, capsOnly);
	possibleB(boards, color, capsOnly);
	possibleR(boards, color, capsOnly);
	possibleQ(boards, color, capsOnly);
	possibleK(boards, color, capsOnly);

    return;
}

void MoveGen::possibleN(const BitBoards& board, int color, const U64 &capturesOnly)
{

	const int* list = board.pieceLoc[color][KNIGHT];
	U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	U64 eking = board.pieces(!color, KING);

	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = board.psuedoAttacks(KNIGHT, color, square);
		moves &= capturesOnly & ~(friends | eking);

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
	U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	U64 eking = board.pieces(!color, KING);

	int square;
	while ((square = *list++) != SQ_NONE) {
	
		U64 moves = slider_attacks.BishopAttacks(board.FullTiles, square);
		moves &= capturesOnly & ~(friends | eking);

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
	U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	U64 eking = board.pieces(!color, KING);

	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = slider_attacks.RookAttacks(board.FullTiles, square);
		moves &= capturesOnly & ~(friends | eking);
		char flag = '0';
		
		/*
		//flag first movement of rook so we can remove castling rights for that move, as well as put them back if unmakeing move
		if (board.can_castle(color)) {
			if ((board.castlingRights[color] == 0LL || 1LL) && relative_square(color, square) == A1) flag = 'M';
			else if ((board.castlingRights[color] == 0LL || 4LL) && relative_square(color, square) == H1) flag = 'M';
		}
		*/
		
		while (moves) {
			int index = pop_lsb(&moves);

			//U64 landing = board.square_bb(index);
			//int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;
			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, ROOK, captured, flag, square, index);

		}

	}

    

}

void MoveGen::possibleQ(const BitBoards& board, int color, const U64 &capturesOnly)
{
	const int* list = board.pieceLoc[color][QUEEN];
	U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	U64 eking = board.pieces(!color, KING);

	int square;
	while ((square = *list++) != SQ_NONE) {

		//grab moves from magic bitboards
		U64 moves = slider_attacks.QueenAttacks(board.FullTiles, square);

		moves &= capturesOnly & ~(friends | eking);

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
	U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	U64 eking = board.pieces(!color, KING);
	char flag = '0';


	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = board.psuedoAttacks(KING, color, square);
		moves &= capturesOnly & ~(friends | eking);


		//non castling moves
		while (moves) {
			int index = pop_lsb(&moves);

			//U64 landing = board.square_bb(index);
			//int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;
			int	captured = board.pieceOnSq(index);

			movegen_push(board, color, KING, captured, flag, square, index);
		}
	}
}

int MoveGen::whichPieceCaptured(const BitBoards & board, int location, int color)
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


    /**************************************************************************
    * Quiet moves are sorted by history score.                                *
    **************************************************************************/
	moveAr[moveCount].score = history.history[color][from][to]; //+ history.gains[isWhite][piece][to] * 10; //find a way to pass historys here instead of using global???

    //scoring capture moves
    if(captured){

		//higher id value for lower value piece,
		//captures are better if from lower (small bonus for both kinds of captures)
		//insuring identical captures, lower piece capturing always scores higher.
		int idAr[7] = { 0, 5, 4, 3, 2, 1, 0 }; 
		

        //Good captures are scored higher, based on BLIND better lower if not defended
        //need to add Static Exchange at somepoint
		if (blind(board, to, color, SORT_VALUE[piece], SORT_VALUE[captured])) moveAr[moveCount].score = SORT_CAPT + SORT_VALUE[captured] + idAr[piece];

		else  moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece];

		/*
		U64 occ = board.FullTiles ^ board.square_bb(from);
		U64 attackers = attackersTo(board, occ, to) & board.pieces(color) ^ board.square_bb(from);

		int def = min_attacker<PAWN>(board, color, to, attackers, occ, attackers);

		if (blind(board, to, color, SORT_VALUE[piece], SORT_VALUE[captured])) {

			if (def) moveAr[moveCount].score = SORT_CAPT + 10000 + SORT_VALUE[captured] + idAr[piece];

			else moveAr[moveCount].score = SORT_CAPT + SORT_VALUE[captured]  + idAr[piece];

		}

        //captures of defended pieces or pieces we know nothing about ~~ better if lower still, by id
		else {

			if(def) moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece] - idAr[def]*3;
			else  moveAr[moveCount].score = SORT_VALUE[captured] - SORT_VALUE[piece];
			
		}
		

		//int value = SEE(moveAr[moveCount], board, !color, true);

		//if(value > 0) moveAr[moveCount].score = value*12 + SORT_CAPT; //possibility, will have to test for ELO Gain
		//else moveAr[moveCount].score = value;
		//*/
    }

    //pawn promotions
    if(moveAr[moveCount].flag == 'Q') moveAr[moveCount].score += SORT_PROM;

	//increment move counter so we know how many
	//moves we have to search and sort through
    moveCount ++;
}
/*
bool MoveGen::blind(const BitBoards & board, const Move &move, int pieceVal, int captureVal)
{
    //approx static eval, Better Lower if not Defended
    int piece = move.piece;
    //get bitboards containing start and landing spot
    U64 sLoc = 1LL << move.from;
    U64 eLoc = 1LL << move.to;

    //captures from pawns don't lose material
    if(move.piece == PAWN) return true;

    //capture lower takes higher
    if(captureVal >= pieceVal - 50) return true;

    //move the piece only on the color BB's and full/emptys
    if(isWhite){
        BBWhitePieces &= ~sLoc;
        BBWhitePieces |= eLoc;
        BBBlackPieces &= ~eLoc;
    } else {
        BBBlackPieces &= ~sLoc;
        BBBlackPieces |= eLoc;
        BBWhitePieces &= ~eLoc;
    }
    FullTiles &= ~ sLoc;
    FullTiles |= eLoc;
    EmptyTiles = ~FullTiles;

    bool defended = false;

    //in order to test if capture location is attacked
    if(isAttacked(eLoc, isWhite, false)){
        defended = true;
    }
    //correct BB's
    if(isWhite){
        BBWhitePieces |= sLoc;
        BBWhitePieces &= ~ eLoc;
        BBBlackPieces |= eLoc;
    } else {
        BBBlackPieces |= sLoc;
        BBBlackPieces &= ~eLoc;
        BBWhitePieces |= eLoc;
    }
    FullTiles |= sLoc;
    EmptyTiles = ~FullTiles;

    if(!defended) return true;

    return 0; //of other captures we know not
}
*/
bool MoveGen::blind(const BitBoards & board, int to, int color, int pieceVal, int captureVal) { //REplace move with just to square? use piece val to see if it equals PAWN val
	
	//better lower if not defeneded 

	//captures from pawns don't lose material
	if (pieceVal == SORT_VALUE[PAWN]) return true;

	//capture lower takes higher
	if (captureVal >= pieceVal - 50) return true;

	//is our capture attacked by enemys?
	//bool defended = isSquareAttacked(board, move.to, !color);

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

	swapList[0] = SORT_VALUE[m.captured];

	occupied = b.FullTiles ^ b.square_bb(m.from); //remove capturing piece from occupied bb 

	//need castling and enpassant logic once implmented

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

bool MoveGen::isLegal(const BitBoards & b, const Move & m, int color) 
{
	const int them = !color;
	const int kingLoc = b.king_square(color);

	return !isSquareAttacked(b, kingLoc, color);

	// if move is not a castling move and ///CASTLING LEGALITY CHECKS REMOVED WHILE TESTING FOR BUGS
	// has passed all legality checks 
	// move is legal
	//if (m.flag != 'C') return !kAttacked;
	//return !kAttacked;
/*	
	U64 rookSq = color == WHITE ? m.to == C1  ? (1LL << A1) : (1LL << H1)
							    : m.to == C8  ? (1LL << A8) : (1LL << H8);

	//if we don't have our rook in the right spot, 
	//castling isn't legal
	if (!(rookSq & b.pieces(color, ROOK))) return false;
	


	//if we don't have the appropriate castling rights, 
	//then castling isn't legal
	int relRook = relative_square(color, pop_lsb(&rookSq));
	U64 index = relRook == A1 ? 3LL : 6LL;

	if (index & b.castlingRights[color]) return false;

	//set the appropriate squares that we need to
	//test for legality depending on castling color, qs/ks
	U64 castleSqs = color == WHITE ? m.to == C1 ? 0xc00000000000000ULL : 0x6000000000000000ULL
		                           : m.to == C8 ? 0xcULL : 0x60ULL;

	for (int i = 0; i < 2; ++i) {
		int loc = pop_lsb(&castleSqs);

		if (isSquareAttacked(b, loc, color)) return false;
	}

	return true;
	*/
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


/* //TEST new min_attacker function for ELO loss/gain before deleting this!!!!!!!!!!!!!!!!!!!!!!!!!!
inline int MoveGen::min_attacker(bool isWhite, const BitBoards & b, const int & to, const U64 & stmAttackers, U64 & occupied, U64 & attackers)
{
U64 pawns, knights, rooks, bishops, queens, king, loc;

if (isWhite) { //NEED TO CHANGE to make recursive and use piece type to return and search again
pawns = b.byColorPiecesBB[0][1];
knights = b.byColorPiecesBB[0][2];
bishops = b.byColorPiecesBB[0][3];
rooks = b.byColorPiecesBB[0][4];
queens = b.byColorPiecesBB[0][5];
king = b.byColorPiecesBB[0][6];
}
else {
pawns = b.byColorPiecesBB[1][1];
knights = b.byColorPiecesBB[1][2];
bishops = b.byColorPiecesBB[1][3];
rooks = b.byColorPiecesBB[1][4];
queens = b.byColorPiecesBB[1][5];
king = b.byColorPiecesBB[1][6];
}

//drawBB(stmAttackers);

int piece;
//find smallest attacker
if (pawns & stmAttackers) {
piece = PAWN; loc = pawns & stmAttackers;
}
else if (knights & stmAttackers) {
piece = KNIGHT; loc = knights & stmAttackers;
}
else if (bishops & stmAttackers) {
piece = BISHOP; loc = bishops & stmAttackers;
}
else if (rooks & stmAttackers) {
piece = ROOK; loc = rooks & stmAttackers;
}
else if (queens & stmAttackers) {
piece = QUEEN; loc = queens & stmAttackers;
}
else if (king & stmAttackers) {
piece = KING; loc = king & stmAttackers;
}
else { //early cutoff if no stm attackers
return PIECE_EMPTY;
}

//remove piece from occupied board
occupied ^= loc & ~(loc - 1);
//drawBB(occupied);

//find xray attackers behind piece once it's been removed and add to attackers
if (piece == PAWN || piece == BISHOP || piece == QUEEN) {                       //ADD BY TYPE BOARD REPRESENTATION TO BITBOARDS
attackers |= slider_attacks.BishopAttacks(occupied, to) & (b.byPieceType[3] | b.byPieceType[5]);
}

if (piece == ROOK || piece == QUEEN) {
attackers |= slider_attacks.RookAttacks(occupied, to) & (b.byPieceType[4] | b.byPieceType[5]);
}
//add new attackers to board
attackers &= occupied;
//drawBB(attackers);


return piece;
}

//returns a bitboard of all attackers of sq locations
U64 MoveGen::attackersTo(int sq, const BitBoards& b, const U64 occ) const
{
U64 attackers = 0LL, loc = 1LL << sq, kingA;

//drawBB(occ);

//knights
if (sq > 18) {
attackers = KNIGHT_SPAN << (sq - 18);
}
else {
attackers = KNIGHT_SPAN >> (18 - sq);
}

if (sq % 8 < 4) {
attackers &= ~FILE_GH;
}
else {
attackers &= ~FILE_AB;
}
attackers &= b.byPieceType[KNIGHT]; //knights

//drawBB(attackers);

//pawns
attackers |= (loc << 9) & notHFile & b.byColorPiecesBB[WHITE][PAWN];
attackers |= (loc << 7) & notAFile & b.byColorPiecesBB[WHITE][PAWN];
//drawBB(attackers);
attackers |= (loc >> 9) & notHFile & b.byColorPiecesBB[BLACK][PAWN];
attackers |= (loc >> 7) &  notAFile & b.byColorPiecesBB[BLACK][PAWN];
//drawBB(attackers);

//bishop and queens
attackers |= slider_attacks.BishopAttacks(occ, sq) & (b.byPieceType[BISHOP] | b.byPieceType[QUEEN]);
//drawBB(attackers);

//rooks and queens
attackers |= slider_attacks.RookAttacks(occ, sq) & (b.byPieceType[ROOK] | b.byPieceType[QUEEN]);
//drawBB(attackers);

if (sq > 9) {
kingA = KING_SPAN << (sq - 9);
}
else {
kingA = KING_SPAN >> (9 - sq);
}
if (sq % 8 < 4) {
kingA &= ~FILE_GH;

}
else {
kingA &= ~FILE_AB;
}
attackers |= kingA & (b.byPieceType[KING]);

return attackers;
}



*/
