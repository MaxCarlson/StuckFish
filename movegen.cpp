#include "movegen.h"

#include "hashentry.h"
#include "bitboards.h"
#include "slider_attacks.h"
#include "externs.h"


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




static const U64 RankMasks8[8] =/*from rank8 to rank1 ?*/
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


MoveGen::MoveGen()
{

}

void MoveGen::generatePsMoves(bool capturesOnly)
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
        possibleWP(pawns, eking, capturesOnly);

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

    //if we don't want to only generate captures
    U64 capsOnly = full;
    //if we only want to generate captures
    if(capturesOnly) capsOnly = enemys;

    //loop through board an generate ps legal moves for our side
    U64 piece = 0LL;
    for(U8 i = 0; i < 64; i++){
        piece = 0LL;
        piece += 1LL << i;
        if(knights & piece) possibleN(i, friends, enemys, eking, capsOnly);
        else if(bishops & piece) possibleB(i, friends, enemys, eking, capsOnly);
        else if(rooks & piece) possibleR(i, friends, enemys, eking, capsOnly);
        else if(queens & piece) possibleQ(i, friends, enemys, eking, capsOnly);
        else if(king & piece) possibleK(i, friends, enemys, eking, capsOnly);
    }
    return;
}

//pawn moves
void MoveGen::possibleWP(const U64 &wpawns, const U64 &blackking, bool capturesOnly)
{
    char piece = PAWN;
    char captured;

    //forward one
    U64 PAWN_MOVES = northOne(wpawns) & EmptyTiles;
    U64 i = PAWN_MOVES &~ (PAWN_MOVES-1);

    if(!capturesOnly){
        while(i != 0){
            int index = trailingZeros(i);

            movegen_push(piece, PIECE_EMPTY, '0', index+8, index);

            PAWN_MOVES &= ~i;
            i= PAWN_MOVES & ~(PAWN_MOVES-1);
        }


        //forward two
        PAWN_MOVES = (wpawns>>16) & EmptyTiles &(EmptyTiles>>8) & rank4;
        i = PAWN_MOVES &~ (PAWN_MOVES-1);

        while(i != 0){
            int index = trailingZeros(i);

            movegen_push(piece, PIECE_EMPTY, '0', index+16, index);

            PAWN_MOVES &= ~i;
            i= PAWN_MOVES & ~(PAWN_MOVES-1);
        }        
    }

    //capture right
    PAWN_MOVES = noEaOne(wpawns) & BBBlackPieces & ~blackking & ~rank8;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, '0', index+7, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }


    //capture left
    PAWN_MOVES = noWeOne(wpawns) & BBBlackPieces & ~blackking & ~rank8;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece,captured, '0', index+9, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }

//Pawn promotions
    //moving forward one
    PAWN_MOVES = northOne(wpawns) & EmptyTiles & rank8;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);

        movegen_push(piece, PIECE_EMPTY, 'Q', index+8, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }

//pawn capture promotions
    //capture right
    PAWN_MOVES = noEaOne(wpawns) & BBBlackPieces & ~blackking & rank8;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index+7, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }


    //capture left
    PAWN_MOVES = noWeOne(wpawns) & BBBlackPieces & ~blackking & rank8;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);

        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index+9, index);

        PAWN_MOVES &= ~i;
        i = PAWN_MOVES & ~(PAWN_MOVES-1);
    }

}

void MoveGen::possibleBP(const U64 &bpawns, const U64 &whiteking, bool capturesOnly)
{
    char piece = PAWN;
    char captured;
    U64 PAWN_MOVES, i;


    if(!capturesOnly){
        //forward one
        PAWN_MOVES = southOne(bpawns) & EmptyTiles;
        i = PAWN_MOVES &~ (PAWN_MOVES-1);

        while(i != 0){
            int index = trailingZeros(i);

            movegen_push(piece, PIECE_EMPTY, '0', index-8, index);

            PAWN_MOVES &= ~i;
            i= PAWN_MOVES & ~(PAWN_MOVES-1);

        }

        //forward two
        PAWN_MOVES = (bpawns<<16) & EmptyTiles &(EmptyTiles<<8) & rank5;
        i = PAWN_MOVES &~ (PAWN_MOVES-1);

        while(i != 0){
            int index = trailingZeros(i);

            movegen_push(piece, PIECE_EMPTY, '0', index-16, index);

            PAWN_MOVES &= ~i;
            i= PAWN_MOVES & ~(PAWN_MOVES-1);

        }
    }

    //capture right
    PAWN_MOVES = soEaOne(bpawns) & BBWhitePieces & ~whiteking & ~rank1;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, '0', index-9, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }


    //capture left
    PAWN_MOVES = soWeOne(bpawns) & BBWhitePieces & ~whiteking & ~rank1;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, '0', index-7, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }

//promotions
    //forward promotions
    PAWN_MOVES = southOne(bpawns) & EmptyTiles & rank1;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);

        movegen_push(piece, PIECE_EMPTY, 'Q', index-8, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }

//capture promotions
    //capture right
    PAWN_MOVES = soEaOne(bpawns) & BBWhitePieces & ~whiteking & rank1;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index-9, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }


    //capture left
    PAWN_MOVES = soWeOne(bpawns) & BBWhitePieces & ~whiteking & rank1;
    i = PAWN_MOVES &~ (PAWN_MOVES-1);

    while(i != 0){
        int index = trailingZeros(i);
        U64 landing = 0LL;
        landing += 1LL << index;
        captured = whichPieceCaptured(landing);

        movegen_push(piece, captured, 'Q', index-7, index);

        PAWN_MOVES &= ~i;
        i= PAWN_MOVES & ~(PAWN_MOVES-1);
    }

}

//other piece moves
void MoveGen::possibleN(U8 location, const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly)
{
    char piece = KNIGHT;
    U64 moves;

    //use the knight span board which holds possible knight moves
    //and apply a shift to the knights current pos
    if(location > 18){
        moves = KNIGHT_SPAN<<(location-18);
    } else {
        moves = KNIGHT_SPAN>>(18-location);
    }

    //making sure the moves don't wrap around to other side once shifter
    //as well as friendly and illegal king capture check
    if(location % 8 < 4){
        moves &= ~FILE_GH & ~friends & ~oppositeking & capturesOnly;
    } else {
        moves &= ~FILE_AB & ~friends & ~oppositeking & capturesOnly;
    }

    U64 j = moves & ~(moves-1);
    char captured;
    while(j != 0){
        //store moves
        int index = trailingZeros(j);
        captured = PIECE_EMPTY;
        U64 landing = 0LL;
        landing += 1LL << index;
        if(landing & enemys){
            captured = whichPieceCaptured(landing);
        }

        movegen_push(piece, captured, '0', location, index);

        moves &= ~j;
        j = moves & ~(moves-1);
    }
}

void MoveGen::possibleB(U8 location,  const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly)
{
    char piece = BISHOP;

    U64 moves = slider_attacks.BishopAttacks(FullTiles, location);
    moves &= ~friends & capturesOnly & ~oppositeking;

    U64 j = moves & ~ (moves-1);

    char captured;
    while(j != 0){
        int index = trailingZeros(j);
        captured = PIECE_EMPTY;
        U64 landing = 0LL;
        landing += 1LL << index;
        if(landing & enemys){
            captured = whichPieceCaptured(landing);
        }

        movegen_push(piece, captured, '0', location, index);

        moves &= ~j;
        j = moves & ~(moves-1);
    }
}

void MoveGen::possibleR(U8 location,  const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly)
{
    char piece = ROOK;
    char flag = '0';

	if (isWhite) {
		//has rook moved flags
		//is rook on initial location and rook hasn't moved yet
		if (location == A1 && !rookMoved[0]) flag = A1 + 1;
		else if (location == H1 && !rookMoved[1]) flag = H1 + 1;
	}
	else {
		if (location == A8 && !rookMoved[2]) flag = A8 + 1;
		else if (location == H8 && !rookMoved[3]) flag = H8 + 1;
	}
    

    U64 moves = slider_attacks.RookAttacks(FullTiles, location);
    moves &= ~friends & capturesOnly & ~oppositeking;

    U64 j = moves & ~ (moves-1);

    char captured;
    while(j != 0){
        int index = trailingZeros(j);
        captured = PIECE_EMPTY;
        U64 landing = 0LL;
        landing += 1LL << index;
        if(landing & enemys){
            captured = whichPieceCaptured(landing);
        }

        movegen_push(piece, captured, flag, location, index);

        moves &= ~j;
        j = moves & ~(moves-1);
    }

}

void MoveGen::possibleQ(U8 location,  const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly)
{
    char piece = QUEEN;

    //grab moves from magic bitboards
    U64 moves = slider_attacks.QueenAttacks(FullTiles, location);

    //and against friends and a full board if normal move gen, or enemy board if captures only
    moves &= ~friends & capturesOnly & ~oppositeking;

    U64 j = moves & ~ (moves-1);

    char captured;
    while(j != 0){
        int index = trailingZeros(j);

        captured = PIECE_EMPTY;
        U64 landing = 0LL;
        landing += 1LL << index;

        if(landing & enemys){
            captured = whichPieceCaptured(landing);
        }

        movegen_push(piece, captured, '0', location, index);

        moves &= ~j;
        j = moves & ~(moves-1);
    }

}

void MoveGen::possibleK(U8 location,  const U64 &friends, const U64 &enemys, const U64 &oppositeking, const U64 &capturesOnly)
{
    U64 moves;
    char piece = KING;

    if(location > 9){
        moves = KING_SPAN << (location-9);

    } else {
        moves = KING_SPAN >> (9-location);
    }

    if(location % 8 < 4){
        moves &= ~FILE_GH & ~friends & capturesOnly & ~oppositeking;

    } else {
        moves &= ~FILE_AB & ~friends & capturesOnly& ~oppositeking;
    }

    U64 j = moves &~(moves-1);

    char captured;
    while(j != 0){
        int index = trailingZeros(j);
        captured = PIECE_EMPTY;
        U64 landing = 0LL;
        landing += 1LL << index;

        if(landing & enemys){
            captured = whichPieceCaptured(landing);
        }

        movegen_push(piece, captured, '0', location, index);

        moves &= ~j;
        j = moves &~ (moves-1);
    }

}

void MoveGen::movegen_push(char piece, char captured, char flag, U8 from, U8 to)
{
    //store move data to move object array
    moveAr[moveCount].from = from;
    moveAr[moveCount].to = to;

    moveAr[moveCount].piece = piece;
    moveAr[moveCount].captured = captured;
    moveAr[moveCount].flag = flag;
    moveAr[moveCount].score = 0;

    /**************************************************************************
    * Quiet moves are sorted by history score.                                *
    **************************************************************************/
    moveAr[moveCount].score = sd.history[isWhite][from][to];

    //scoring capture moves
    if(captured != PIECE_EMPTY){

		int idAr[7] = { 0, 5, 4, 3, 2, 1, 0 }; //higher id value for lower value piece - captures are better if from lower

        //Good captures are scored higher, based on BLIND better lower if not defended
        //need to add Static Exchange at somepoint
        if(blind(moveAr[moveCount], SORT_VALUE[piece], SORT_VALUE[captured])) moveAr[moveCount].score = SORT_CAPT + SORT_VALUE[captured] + idAr[piece];

        //captures of defended pieces or pieces we know nothing about ~~ better if lower still, by id
        else moveAr[moveCount].score = SORT_VALUE[captured] + idAr[piece]; // + SORT_CAPT/4 ???

    }

    //pawn promotions
    if(moveAr[moveCount].flag == 'Q') moveAr[moveCount].score += SORT_PROM;

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

#include <algorithm> 
int MoveGen::SEE(const U64 &sq, int piece, bool isWhite) {
	int val = 0;

	U64 loc = 1LL << trailingZeros(sq);

	std::vector<U64> defenders = getSmallestAttacker(loc, isWhite);

	U64 piece = defenders[0];
	int defLoc = defenders[1];

	if (piece) {
		
		Move m = makeCaptureSEE(piece, sq, piece, isWhite);

		val = std::max(0, SORT_VALUE[piece] - SEE(sq, m.piece, !isWhite));

		unmakeCaptureSEE(m, isWhite);
	}

	return val;
}

std::vector<U64> MoveGen::getSmallestAttacker(const U64 &sq, bool isWhite) {
	U64 attacks = 0LL, piece = 0LL, enemys, pawns, knights, rooks, bishops, queens, king, ourking;
	//int x, y, x1, y1;
	std::vector<U64> att;

	if (isWhite) {
		pawns = BBBlackPawns;
		knights = BBBlackKnights;
		bishops = BBBlackBishops;
		rooks = BBBlackRooks;
		queens = BBBlackQueens;
		king = BBBlackKing;
		//pawns
		//capture right
		attacks = soEaOne(pawns);
		//capture left
		attacks |= soWeOne(pawns);
	}
	else {
		pawns = BBWhitePawns;
		knights = BBWhiteKnights;
		bishops = BBWhiteBishops;
		rooks = BBWhiteRooks;
		queens = BBWhiteQueens;
		king = BBWhiteKing;
		//capture right
		attacks = noEaOne(pawns);
		//capture left
		attacks |= noWeOne(pawns);
	}

	//pawn/s defend/s
	if (attacks & sq) {
		att.push_back(attacks);
		att.push_back(1LL);
		return att;
	}

	//int is piece/square attacked location on board
	int location = trailingZeros(sq);

	//very similar to move generation code just ending with generated bitboards of attacks
	//knight attacks
	if (location > 18) {
		attacks = KNIGHT_SPAN << (location - 18);
	}
	else {
		attacks = KNIGHT_SPAN >> (18 - location);
	}

	if (location % 8 < 4) {
		attacks &= ~FILE_GH;
	}
	else {
		attacks &= ~FILE_AB;
	}

	if (attacks & knights) {
		att.push_back(attacks);
		att.push_back(2LL);
		return att;
	}

	
	attacks = slider_attacks.BishopAttacks(FullTiles, location);
	//bishop/s defend/s
	if (attacks & bishops) {
		att.push_back(attacks);
		att.push_back(3LL);
		return att;
	}
	
	attacks = slider_attacks.RookAttacks(FullTiles, location);
	//rook/s defend/s
	if (attacks & rooks) {
		att.push_back(attacks);
		att.push_back(4LL);
		return att;
	}
	
	attacks = slider_attacks.QueenAttacks(FullTiles, location);

	//queen/s defend/s
	if (attacks & queens) {
		att.push_back(attacks);
		att.push_back(5LL);
		return att;
	}

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

	//king defend/s
	if (attacks & king) {
		att.push_back(attacks);
		att.push_back(6LL);
		return att;
	}

	//no defenders
	att.push_back(0LL);
	att.push_back(0LL);
	return att;
}

Move MoveGen::makeCaptureSEE(const U64 & from, const U64 & to, int captured, bool isWhite)
{
	//create bitboard of only one attacker if there are multiple
	U64 piece = 1LL << trailingZeros(from);
	Move m;
	m.captured = captured;
	m.from = piece;
	m.to = trailingZeros(to);

	if (isWhite) {
		if (piece & BBBlackPawns) {
			BBBlackPawns &= ~piece;
			BBBlackPawns |= to;
			m.piece = PAWN;

		} else if (piece & BBBlackKnights) {
			BBBlackKnights &= ~piece;
			BBBlackKnights |= to;
			m.piece = KNIGHT;

		}else if (piece & BBBlackBishops) {
			BBBlackBishops &= ~piece;
			BBBlackBishops |= to;
			m.piece = BISHOP;

		}else if (piece & BBBlackRooks) {
			BBBlackRooks &= ~piece;
			BBBlackRooks |= to;
			m.piece = ROOK;

		}else if (piece & BBBlackQueens) {
			BBBlackQueens &= ~piece;
			BBBlackQueens |= to;
			m.piece = QUEEN;

		}else if (piece & BBBlackKing) {
			BBBlackKing &= ~piece;
			BBBlackKing |= to;
			m.piece = KING;
		}
		BBBlackPieces &= ~piece;
		BBBlackPieces |= to;
	}
	else {
		if (piece & BBWhitePawns) {
			BBWhitePawns &= ~piece;
			BBWhitePawns |= to;
			m.piece = PAWN;

		}else if (piece & BBWhiteKnights) {
			BBWhiteKnights &= ~piece;
			BBWhiteKnights |= to;
			m.piece = KNIGHT;

		}else if (piece & BBWhiteBishops) {
			BBWhiteBishops &= ~piece;
			BBWhiteBishops |= to;
			m.piece = BISHOP;

		}else if (piece & BBWhiteRooks) {
			BBWhiteRooks &= ~piece;
			BBWhiteRooks |= to;
			m.piece = ROOK;

		}else if (piece & BBWhiteQueens) {
			BBWhiteQueens &= ~piece;
			BBWhiteQueens |= to;
			m.piece = QUEEN;

		}else if (piece & BBWhiteKing) {
			BBWhiteKing &= ~piece;
			BBWhiteKing |= to;
			m.piece = KING;
		}
		BBWhitePieces &= ~piece;
		BBWhitePieces |= to;
	}
	FullTiles &= ~piece;
	EmptyTiles = FullTiles;
}

void MoveGen::unmakeCaptureSEE(const Move & m, bool isWhite)
{
}

Move MoveGen::movegen_sort(int ply)
{
    int best = -INF;
    int high = 0;
    //find best scoring move
    for(U8 i = 0; i < moveCount; ++i){
        if(moveAr[i].score > best && !moveAr[i].tried){
            high = i;
            best = moveAr[i].score;
        }
    }
    //mark best scoring move tried since we're about to try it
    //~~~ change later if we don't always try move on return
    moveAr[high].tried = true;

    return moveAr[high];
}

void MoveGen::reorderMoves(int ply, const HashEntry &entry)
{

    for(int i = 0; i < moveCount; ++i){
        //add killer moves score to move if there is a from - to match in killers
        if(moveAr[i].piece == sd.killers[ply][1].piece
        && moveAr[i].from == sd.killers[ply][1].from
        && moveAr[i].to == sd.killers[ply][1].to
        && moveAr[i].score < SORT_KILL - 1){
            moveAr[i].score = SORT_KILL - 1;
        }
        if(moveAr[i].piece == sd.killers[ply][0].piece //maybe don't need piece varifier?
        && moveAr[i].from == sd.killers[ply][0].from
        && moveAr[i].to == sd.killers[ply][0].to
        && moveAr[i].score < SORT_KILL){
            moveAr[i].score = SORT_KILL;
        }

        if(moveAr[i].from == entry.move.from
        && moveAr[i].to == entry.move.to
        && moveAr[i].piece == entry.move.piece){
            moveAr[i].score = SORT_HASH;
        }


    }
}

char MoveGen::whichPieceCaptured(U64 landing)
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

int MoveGen::trailingZeros(U64 i)
{
    //find the first one and number of zeros after it
    if (i == 0) return 64;
    U64 x = i;
    U64 y;
    int n = 63;
    y = x << 32; if (y != 0) { n -= 32; x = y; }
    y = x << 16; if (y != 0) { n -= 16; x = y; }
    y = x <<  8; if (y != 0) { n -=  8; x = y; }
    y = x <<  4; if (y != 0) { n -=  4; x = y; }
    y = x <<  2; if (y != 0) { n -=  2; x = y; }
    return (int) ( n - ((x << 1) >> 63));
}

U64 MoveGen::ReverseBits(U64 input)
{
    //literally reverse bits in U64's
    U64 output = input;
    for (int i = sizeof(input) * 8-1; i; --i)
    {
        output <<= 1;
        input  >>= 1;
        output |=  input & 1;
    }
    return output;
}


void MoveGen::grab_boards(const BitBoards &BBBoard, bool wOrB)
{
    isWhite = wOrB;
    FullTiles = BBBoard.FullTiles;
    EmptyTiles = BBBoard.EmptyTiles;

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
}

void MoveGen::updateBoards(const Move &move, const BitBoards &board)
{

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
    int location = trailingZeros(pieceLoc);

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

void MoveGen::drawBBA()
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

void MoveGen::drawBB(U64 board)
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
