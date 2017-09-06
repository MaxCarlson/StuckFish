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

static const U64 RankMasks8[8] =/*from rank 1 to 8 ?*/
{
    0xFFL, 0xFF00L, 0xFF0000L, 0xFF000000L, 0xFF00000000L, 0xFF0000000000L, 0xFF000000000000L, 0xFF00000000000000L
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

/*
MoveGen::MoveGen() 
{
	//memset(&moveAr, 0, sizeof(moveAr)); //~~SLOWER than before
}
*/
void MoveGen::generatePsMoves(const BitBoards& boards, bool capturesOnly)
{

    //counts total moves generated
    moveCount = 0;
    U64 friends, enemys, pawns, knights, rooks, bishops, queens, king, eking;

    if(isWhite){
        friends = BBWhitePieces;
        enemys = BBBlackPieces;

        pawns = BBWhitePawns;
        knights = BBWhiteKnights;
        bishops = BBWhiteBishops;
        rooks = BBWhiteRooks;
        queens = BBWhiteQueens;
        king = BBWhiteKing;
        eking = BBBlackKing;
        //generate pawn moves
        possibleWP(pawns, eking, capturesOnly);   //ADD function to calc attack board or grab it from previous isAttacked in search, so we can generate caslting moves ~!~!~!~!~!~!~!~!~!~!~!

    } else {
        friends = BBBlackPieces;
        enemys = BBWhitePieces;

        pawns = BBBlackPawns;
        knights = BBBlackKnights;
        bishops = BBBlackBishops;
        rooks = BBBlackRooks;
        queens = BBBlackQueens;
        king = BBBlackKing;
        eking = BBWhiteKing;

        possibleBP(pawns, eking, capturesOnly);
    }

	int color = !isWhite;
    //if we don't want to only generate captures
    U64 capsOnly = full;
    //if we only want to generate captures
    if(capturesOnly) capsOnly = enemys;

	//while there is a piece on the board, grab that pieces location and pop it from the board
	//then generate moves for a piece on that location, and push those moves the the move array
/*
	while (knights) {
		int loc = pop_lsb(&knights);
		possibleN(loc, friends, enemys, eking, capsOnly);
	}
	while (bishops) {
		int loc = pop_lsb(&bishops);
		possibleB(loc, friends, enemys, eking, capsOnly);
	}
	while (rooks) {
		int loc = pop_lsb(&rooks);
		possibleR(loc, friends, enemys, eking, capsOnly, boards);
	}
	while (queens) {
		int loc = pop_lsb(&queens);
		possibleQ(loc, friends, enemys, eking, capsOnly);
	}
	while (king) {
		int loc = pop_lsb(&king);
		possibleK(loc, friends, enemys, eking, capsOnly);
	}
*/

	possibleN(boards, color, capsOnly);
	possibleB(boards, color, capsOnly);
	possibleR(boards, color, capsOnly);
	possibleQ(boards, color, capsOnly);
	possibleK(boards, color, capsOnly);

    return;
}

//pawn moves
void MoveGen::possibleWP(const U64 &wpawns, const U64 &blackking, bool capturesOnly)
{
    char piece = PAWN;
    char captured;

    //forward one
    U64 moves = northOne(wpawns) & EmptyTiles;

    if(!capturesOnly){
        while(moves){
            int index = pop_lsb(&moves);

            movegen_push(piece, PIECE_EMPTY, '0', index+8, index);
        }


        //forward two
        moves = (wpawns>>16) & EmptyTiles &(EmptyTiles>>8) & rank4;

        while(moves){
            int index = pop_lsb(&moves);

            movegen_push(piece, PIECE_EMPTY, '0', index+16, index);
        }        
    }

    //capture right
    moves = noEaOne(wpawns) & BBBlackPieces & ~blackking & ~rank8;

    while(moves){
        int index = pop_lsb(&moves);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, '0', index+7, index);
    }


    //capture left
    moves = noWeOne(wpawns) & BBBlackPieces & ~blackking & ~rank8;

    while(moves){
        int index = pop_lsb(&moves);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece,captured, '0', index+9, index);
    }

//Pawn promotions
    //moving forward one
    moves = northOne(wpawns) & EmptyTiles & rank8;

    while(moves){
        int index = pop_lsb(&moves);

        movegen_push(piece, PIECE_EMPTY, 'Q', index+8, index);
    }

//pawn capture promotions
    //capture right
    moves = noEaOne(wpawns) & BBBlackPieces & ~blackking & rank8;

    while(moves){
        int index = pop_lsb(&moves);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index+7, index);
    }


    //capture left
    moves = noWeOne(wpawns) & BBBlackPieces & ~blackking & rank8;

    while(moves){
        int index = pop_lsb(&moves);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index+9, index);
    }

}

void MoveGen::possibleBP(const U64 &bpawns, const U64 &whiteking, bool capturesOnly)
{
    char piece = PAWN;
    char captured;
    U64 moves;


    if(!capturesOnly){
        //forward one
        moves = southOne(bpawns) & EmptyTiles;

        while(moves){
            int index = pop_lsb(&moves);

            movegen_push(piece, PIECE_EMPTY, '0', index-8, index);
        }

        //forward two
        moves = (bpawns<<16) & EmptyTiles &(EmptyTiles<<8) & rank5;

        while(moves){
            int index = pop_lsb(&moves);

            movegen_push(piece, PIECE_EMPTY, '0', index-16, index);
        }
    }

    //capture right
    moves = soEaOne(bpawns) & BBWhitePieces & ~whiteking & ~rank1;

    while(moves){
        int index = pop_lsb(&moves);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, '0', index-9, index);
    }


    //capture left
    moves = soWeOne(bpawns) & BBWhitePieces & ~whiteking & ~rank1;

    while(moves){
        int index = pop_lsb(&moves);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, '0', index-7, index);
    }

//promotions
    //forward promotions
    moves = southOne(bpawns) & EmptyTiles & rank1;

    while(moves){
        int index = pop_lsb(&moves);

        movegen_push(piece, PIECE_EMPTY, 'Q', index-8, index);
    }

//capture promotions
    //capture right
    moves = soEaOne(bpawns) & BBWhitePieces & ~whiteking & rank1;

    while(moves){
        int index = pop_lsb(&moves);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index-9, index);
    }


    //capture left
    moves = soWeOne(bpawns) & BBWhitePieces & ~whiteking & rank1;

    while(moves){
        int index = pop_lsb(&moves);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index-7, index);
    }

}

//other piece moves
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

			U64 landing = board.square_bb(index);

			int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;

			movegen_push(KNIGHT, captured, '0', square, index);
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

			U64 landing = board.square_bb(index);

			int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;

			movegen_push(BISHOP, captured, '0', square, index);

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

		char captured;
		while (moves) {
			int index = pop_lsb(&moves);

			U64 landing = board.square_bb(index);

			int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;

			movegen_push(ROOK, captured, '0', square, index);

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

		int captured;
		while (moves) {
			int index = pop_lsb(&moves);

			U64 landing = board.square_bb(index);

			int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;

			movegen_push(QUEEN, captured, '0', square, index);
		}
	}
}

void MoveGen::possibleK(const BitBoards& board, int color, const U64 &capturesOnly)
{
	const int* list = board.pieceLoc[color][KING];
	U64 friends = board.pieces(color);
	U64 enemys = board.pieces(!color);
	U64 eking = board.pieces(!color, KING);

	int square;
	while ((square = *list++) != SQ_NONE) {

		U64 moves = board.psuedoAttacks(KING, color, square);
		moves &= capturesOnly & ~(friends | eking);

		while (moves) {
			int index = pop_lsb(&moves);

			U64 landing = board.square_bb(index);

			int captured = (landing & enemys) ? whichPieceCaptured(landing) : PIECE_EMPTY;

			movegen_push(KING, captured, '0', square, index);
		}
	}   
}

void MoveGen::movegen_push(int piece, int captured, char flag, int from, int to) //change flags to int eventually
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
	moveAr[moveCount].score = history.history[isWhite][from][to]; //+ history.gains[isWhite][piece][to] * 10; //find a way to pass historys here instead of using global???

    //scoring capture moves
    if(captured){

		//higher id value for lower value piece,
		//captures are better if from lower (small bonus for both kinds of captures)
		//insuring identical captures, lower piece capturing always scores higher.
		int idAr[7] = { 0, 5, 4, 3, 2, 1, 0 }; 
		

        //Good captures are scored higher, based on BLIND better lower if not defended
        //need to add Static Exchange at somepoint
        if(blind(moveAr[moveCount], SORT_VALUE[piece], SORT_VALUE[captured])) moveAr[moveCount].score = SORT_CAPT + SORT_VALUE[captured] + idAr[piece];

        //captures of defended pieces or pieces we know nothing about ~~ better if lower still, by id
        else moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece]; 
    }

    //pawn promotions
    if(moveAr[moveCount].flag == 'Q') moveAr[moveCount].score += SORT_PROM;

	//increment move counter so we know how many
	//moves we have to search and sort through
    moveCount ++;
}

bool MoveGen::blind(const Move &move, int pieceVal, int captureVal)
{
    //approx static eval, Better Lower if not Defended
    char piece = move.piece;
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

int MoveGen::SEE(const Move& m, const BitBoards& b, bool isWhite, bool isCapture)
{
	U64 attackers, occupied, stmAttackers;
	int swapList[32], index = 1; //play with swap val for speed?

	//early return, SEE can't be a losing capture
	//is capture flag is used for when we're checking to see if the move is escaping capture
	//we don't want an early return if that's the case
	if (SORT_VALUE[m.piece] <= SORT_VALUE[m.captured] && isCapture) return INF;

	swapList[0] = SORT_VALUE[m.captured];

	occupied = b.FullTiles ^ b.square_bb(m.from); //remove capturing piece from occupied bb 

	//need castling and enpassant logic once implmented

	//finds all attackers to the square
	attackers = attackersTo(m.to, b, occupied) & occupied;

	//if there are no attacking pieces, return
	int color = !isWhite;
	if(!(attackers & b.allPiecesColorBB[color])) return swapList[0];


	//if (isWhite && !(attackers & b.BBBlackPieces)) return swapList[0];
	//else if (!isWhite && !(attackers & b.BBWhitePieces))  return swapList[0];

	//switch sides
	isWhite = !isWhite;
	color = ~color;
	stmAttackers = attackers & b.allPiecesColorBB[color];
	//if(isWhite) stmAttackers = attackers & b.BBWhitePieces; 
	//else  stmAttackers = attackers & b.BBBlackPieces;

	if (!stmAttackers) return swapList[0];
	
	int captured = m.piece; //if there are attackers, our piece being moved will be captured

	//loop through and add pieces that can capture on the square to swapList
	do {
		if (index > 31) break;

		//add entry to swap list
		swapList[index] = -swapList[index - 1] + SORT_VALUE[captured];

		captured = min_attacker(isWhite, b, m.to, stmAttackers, occupied, attackers);

		if (captured == KING) {
			if (stmAttackers == attackers) {
				index++;
			}
			break;
		}

		isWhite = !isWhite;

		color = ~color;
		stmAttackers = attackers & b.allPiecesColorBB[color];
		//if (isWhite) stmAttackers = attackers & b.BBWhitePieces;
		//else stmAttackers = attackers & b.BBBlackPieces;

		index++;
	} while (stmAttackers);
	
	//negamax through swap list finding best possible outcome for starting side to move
	while (--index) {
		swapList[index - 1] = std::min(-swapList[index], swapList[index - 1]);
	}

	return swapList[0];
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

FORCE_INLINE int MoveGen::min_attacker(bool isWhite, const BitBoards & b, const int & to, const U64 & stmAttackers, U64 & occupied, U64 & attackers)
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

int MoveGen::whichPieceCaptured(U64 landing)
{
    if(isWhite){
        if(landing & BBBlackPawns) return PAWN;
        else if(landing & BBBlackKnights) return KNIGHT;
		else if(landing & BBBlackBishops) return BISHOP;
		else if(landing & BBBlackRooks) return ROOK;
		else if(landing & BBBlackQueens) return QUEEN;
		else if(landing & BBBlackKing) return KING;
    } else {
		if(landing & BBWhitePawns) return PAWN;
		else if(landing & BBWhiteKnights) return KNIGHT;
		else if(landing & BBWhiteBishops) return BISHOP;
		else if(landing & BBWhiteRooks) return ROOK;
		else if(landing & BBWhiteQueens) return QUEEN;
		else if(landing & BBWhiteKing) return KING;
    }
    drawBBA();

	std::cout << "which piece captured error" << std::endl;
    return '0';
}

void MoveGen::grab_boards(const BitBoards &BBBoard, bool wOrB)
{
	
    isWhite = wOrB;
    FullTiles = BBBoard.FullTiles;
    EmptyTiles = BBBoard.EmptyTiles;
	/*
    BBWhitePieces = BBBoard.BBWhitePieces;
    BBWhitePawns = BBBoard.BBWhitePawns;
    BBWhiteKnights = BBBoard.BBWhiteKnights;
    BBWhiteBishops = BBBoard.BBWhiteBishops;
    BBWhiteRooks = BBBoard.BBWhiteRooks;
    BBWhiteQueens = BBBoard.BBWhiteQueens;
    BBWhiteKing = BBBoard.BBWhiteKing;

    BBBlackPieces = BBBoard.BBBlackPieces;
    BBBlackPawns = BBBoard.BBBlackPawns;
    BBBlackKnights = BBBoard.BBBlackKnights;
    BBBlackBishops = BBBoard.BBBlackBishops;
    BBBlackRooks = BBBoard.BBBlackRooks;
    BBBlackQueens = BBBoard.BBBlackQueens;
    BBBlackKing = BBBoard.BBBlackKing;
	*/
	BBWhitePieces = BBBoard.allPiecesColorBB[0];
	BBWhitePawns = BBBoard.byColorPiecesBB[0][1];
	BBWhiteKnights = BBBoard.byColorPiecesBB[0][2];
	BBWhiteBishops = BBBoard.byColorPiecesBB[0][3];
	BBWhiteRooks = BBBoard.byColorPiecesBB[0][4];
	BBWhiteQueens = BBBoard.byColorPiecesBB[0][5];
	BBWhiteKing = BBBoard.byColorPiecesBB[0][6];

	BBBlackPieces = BBBoard.allPiecesColorBB[1];
	BBBlackPawns = BBBoard.byColorPiecesBB[1][1];
	BBBlackKnights = BBBoard.byColorPiecesBB[1][2];
	BBBlackBishops = BBBoard.byColorPiecesBB[1][3];
	BBBlackRooks = BBBoard.byColorPiecesBB[1][4];
	BBBlackQueens = BBBoard.byColorPiecesBB[1][5];
	BBBlackKing = BBBoard.byColorPiecesBB[1][6];

}

bool MoveGen::isAttacked(U64 pieceLoc, bool wOrB, bool isSearchKingCheck)
{
    U64 attacks = 0LL, friends, enemys, pawns, knights, rooks, bishops, queens, king, ourking;
    //int x, y, x1, y1;

    if(wOrB){
        friends = BBWhitePieces;
        enemys = BBBlackPieces;
        pawns = BBBlackPawns;
        knights = BBBlackKnights;
        bishops = BBBlackBishops;
        rooks = BBBlackRooks;
        queens = BBBlackQueens;
        king = BBBlackKing;
        ourking = BBWhiteKing;

        //pawns
        //capture right
        attacks = soEaOne(pawns);
        //capture left
        attacks |= soWeOne(pawns);

    } else {
        friends = BBBlackPieces;
        enemys = BBWhitePieces;
        pawns = BBWhitePawns;
        knights = BBWhiteKnights;
        bishops = BBWhiteBishops;
        rooks = BBWhiteRooks;
        queens = BBWhiteQueens;
        king = BBWhiteKing;
        ourking = BBBlackKing;

        //capture right
        attacks = noEaOne(pawns);
        //capture left
        attacks |= noWeOne(pawns);
    }

    if(isSearchKingCheck) pieceLoc = ourking; //used for search king in check checking, so kings pos is not out of date

    if(attacks & pieceLoc) return true;

    //int is piece/square attacked location on board
    int location = lsb(pieceLoc);

//very similar to move generation code just ending with generated bitboards of attacks
    //knight attacks
    if(location > 18){
        attacks = KNIGHT_SPAN<<(location-18);
    } else {
        attacks = KNIGHT_SPAN>>(18-location);
    }

    if(location % 8 < 4){
        attacks &= ~FILE_GH & ~friends;
    } else {
        attacks &= ~FILE_AB & ~friends;
    }

    if(attacks & knights) return true;

    //diagonal of bishops and queens attack check
    U64 BQ = bishops | queens;    

    attacks = slider_attacks.BishopAttacks(FullTiles, location);

    if(attacks & BQ) return true;

    //horizontal of rooks and queens attack check
    U64 RQ = rooks | queens;

    attacks = slider_attacks.RookAttacks(FullTiles, location);

    if(attacks & RQ) return true;

    //king attack checks
	if (location > 9) {
		attacks = KING_SPAN << (location - 9);
	}
	else {
		attacks = KING_SPAN >> (9 - location);
	}
	if (location % 8 < 4) {
		attacks &= ~FILE_GH;

	}
	else {
		attacks &= ~FILE_AB;
	}

    if(attacks & king) return true;

return false;
}

void MoveGen::drawBBA() const
{
    char flips[8] = {'8', '7', '6', '5', '4', '3', '2', '1'};
    char flipsL[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    int c = 0;

    for(int i = 0; i < 64; i++){
        if((i)%8 == 0){
            std::cout<< std::endl;
            std::cout << flips[c] << " | ";
            c++;
        }

        if(BBWhitePawns & (1ULL<<i)){
            std::cout << "P" << ", ";
        }
        if(BBWhiteRooks & (1ULL<<i)){
            std::cout << "R" << ", ";
        }
        if(BBWhiteKnights & (1ULL<<i)){
            std::cout << "N" << ", ";
        }
        if(BBWhiteBishops & (1ULL<<i)){
            std::cout << "B" << ", ";
        }
        if(BBWhiteQueens & (1ULL<<i)){
            std::cout << "Q" << ", ";
        }
        if(BBWhiteKing & (1ULL<<i)){
            std::cout << "K" << ", ";
        }
        if(BBBlackPawns & (1ULL<<i)){
            std::cout << "p" << ", ";
        }
        if(BBBlackRooks & (1ULL<<i)){
            std::cout << "r" << ", ";
        }
        if(BBBlackKnights & (1ULL<<i)){
            std::cout << "n" << ", ";
        }
        if(BBBlackBishops & (1ULL<<i)){
            std::cout << "b" << ", ";
        }
        if(BBBlackQueens & (1ULL<<i)){
            std::cout << "q" << ", ";
        }
        if(BBBlackKing & (1ULL<<i)){
            std::cout << "k" << ", ";
        }
        if(EmptyTiles & (1ULL<<i)){
            std::cout << " " << ", ";
        }

        //if(i % 8 == 7){
        //    std::cout << "| " << flips[c] ;
       //     c++;
       // }
    }

    std::cout << std::endl << "    ";
    for(int i = 0; i < 8; i++){
        std::cout << flipsL[i] << "  ";
    }

    std::cout << std::endl << std::endl;;
}

void MoveGen::drawBB(U64 board) const
{
    for(int i = 0; i < 64; i++){
        if(board & (1ULL << i)){
            std::cout<< 1 <<", ";
        } else {
            std::cout << 0 << ", ";
        }
        if((i+1)%8 == 0){
            std::cout<< std::endl;
        }
    }
    std::cout<< std::endl;
}

U64 MoveGen::northOne(U64 b)
{
    return b >> 8;
}

U64 MoveGen::southOne(U64 b)
{
    return b << 8;
}

U64 MoveGen::eastOne (U64 b)
{
    return (b << 1) & notHFile;
}

U64 MoveGen::noEaOne(U64 b)
{
    return (b >> 7) & notHFile;;
}

U64 MoveGen::soEaOne(U64 b)
{
    return (b << 9) & notHFile;
}

U64 MoveGen::westOne(U64 b)
{
    return (b >> 1) & notAFile;
}

U64 MoveGen::soWeOne(U64 b)
{

    return (b << 7) & notAFile;
}

U64 MoveGen::noWeOne(U64 b)
{
    return (b >> 9) & notAFile;
}
