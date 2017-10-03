#include "Evaluate.h"

#include <iostream>

#include "defines.h"
#include "bitboards.h"
#include "externs.h"
#include "psqTables.h"
#include "Pawns.h"




int SquareDistance[64][64]; //distance between squares // MOVE SOMEWERE ELSE, new .h file for inline functions??

inline int square_distance(int s1, int s2) {
	return SquareDistance[s1][s2];
}

//for flipping squares in blockers if black
static const int bSQ[64] = {
	56,57,58,59,60,61,62,63,
	48,49,50,51,52,53,54,55,
	40,41,42,43,44,45,46,47,
	32,33,34,35,36,37,38,39,
	24,25,26,27,28,29,30,31,
	16,17,18,19,20,21,22,23,
	8, 9, 10,11,12,13,14,15,
	0, 1,  2, 3, 4, 5, 6, 7
};

//masks for determining which side king is on
U64 KingSide = FileMasks8[5] | FileMasks8[6] | FileMasks8[7];
U64 QueenSide = FileMasks8[2] | FileMasks8[1] | FileMasks8[0];

//used for minor bonus to mid game scoring. for not 
//attacked area in center of the board that we control
U64 center[STAGE] = {
	(FileCBB | FileDBB | FileEBB | FileFBB) & (RankMasks8[1] | RankMasks8[2] | RankMasks8[3]),
	(FileCBB | FileDBB | FileEBB | FileFBB) & (RankMasks8[6] | RankMasks8[5] | RankMasks8[4])
};

const U64 full = 0xffffffffffffffffULL;

//Scores used for ease of use calcuating and 
//weighting mid and end game scores at the same time.
//Idea taken from StockFish.
#define S(mg, eg) make_scores(mg, eg)


const Scores pawnThreat[PIECES]{
	S(0, 0), S(0 , 0), S(24, 32), S(25, 32), S(35, 57), S(40, 65)
};

const Scores threats[2][PIECES]{
	{ S(0 , 0), S(2, 8), S(6, 10), S(6, 10), S(12, 20), S(12, 20) }, //Minor
	{ S(0,  0), S(4, 8), S(4, 10), S(4, 10), S(4,  10), S(6,  11) }  //Major
};

//Bonuses and penalties
const Scores Hanging		 = S( 9,  7); //all values need to be tested against changes for ELO gain
const Scores KingOnPawn      = S( 0, 30);
const Scores KingOnMPawn     = S( 0, 64);
const Scores RookOnPawn		 = S( 4, 13);
const Scores RookHalfOpen    = S( 5,  3);
const Scores RookOpen	     = S(10,  5);
const Scores Unstoppable     = S( 0,  6); //might need to play with value since not weighted could easily be worth more
const Scores MinorBehindPawn = S( 7,  0);
const Scores BishopPawns     = S( 3,  6);

//used to value higher value pieces more
//if they are attacking zone around the king
const int KingAttackerWeights[6] = { 0, 0, 2, 2, 3, 5 };

//numbers used in king safety, representing enemy safe checks.
//Multiplied by the number of 
//squares in king zone/adjacent kz
const int QueenContactCheck = 7; //values probably should be changed to more accurate values and WEIGHTING adjusted instead
const int RookContactCheck = 5;
const int QueenCheck = 3;
const int RookCheck = 2;
const int BishopCheck = 1;
const int KnightCheck = 1;

//score of evaluate king uses this as a lookup table
//and subtracts from score for white, adds for black 
				//Possibly create larger range of values, and a logrithmic amp up when evaluating/weighting king safety scoring
static const int SafetyTable[100] = {
	0,     0,   1,   2,   3,   5,   7,   9,  12,  15,
	18,   22,  26,  30,  35,  39,  44,  50,  56,  62,
	68,   75,  82,  85,  89,  97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

//bonus for mid and end game mobility, 
//by pieces and by squares of mobility
const Scores mobilityBonus[PIECES][30]{ //S(midgame, endgame)
	{}, {}, //no piece and pawn, no mobility bonus

	{ S(-20, -17), S(-16, -14), S(-2, -3),  S(3, 0),  S(  6, 3 ), S(10, 6),  S(15, 9),  S(17, 10), S(19, 11), }, //Knights

	{ S(-17, -15), S(-10, -7),  S( 1,  1),  S( 4,  6), S( 7,  9), S(11, 12), S(14, 14), //Bishops
	  S( 17,  15), S( 19, 17),  S(20, 17),  S(21, 17), S(22, 19), S(22, 19), S(23, 19), },

	{ S(-12, -14), S( -8, -7),  S( -3,  0), S( 1,  4), S( 4,  9), S( 6, 13), S( 9, 17), S(11, 24), //Rooks
	  S( 12,  30), S( 13, 35),  S( 14, 40), S(15, 42), S(15, 43), S(15, 45), S(16, 45), },

	{ S(-14, -14), S(-9,  -8),  S(-3,  -4), S( 0,  0), S( 2,  3), S( 4,  5), S( 5, 10), S( 6, 15), 
	  S(  7,  16), S( 7,  17),  S( 7,  17), S( 7, 17), S( 8, 17), S( 8, 17), S( 8, 17), S( 8, 17), S(8, 17), S(8, 17), //Queens
	  S(  9,  17), S( 9,  17),  S( 9,  17), S( 9, 17), S( 9, 17), S( 9, 17), S( 9, 17), S( 9, 17), S(9, 17), S(9, 17) }
}; 

struct EvalInfo {

	Scores psqScores[COLOR]; //piece square table scores

	U64 kingZone[COLOR] = { 0LL }; //zone around king, by color
	int kingAttackers[COLOR] = { 0 }; //number of pieces attacking opponent king
	int kingAttWeights[COLOR] = { 0 }; //added weight of king attackers
	int adjacentKingAttckers[COLOR] = { 0 }; //num pieces attacking sqs adjacent to king

	//bitboards of attacked squares by color and piece type, 
	U64 attackedBy[COLOR][7] = { { 0LL } }; //0 for second index is all pieces of that color

	//holds bb of pinned pieces of color
	U64 pinnedPieces[COLOR] = { 0LL };

	Scores mobility[COLOR]; 

	int blockages[COLOR] = { 0 };

	int adjustMaterial[COLOR] = { 0 };

	//pawns and material
	//hash table * entries
	Pawns::PawnsEntry * pe;

	Material::Entry * me;
};


void Evaluate::generateKingZones(const BitBoards & boards, EvalInfo & ev)
{
	//draw zone around king all 8 tiles around him, plus three in front -- north = front for white, south black
	
	for (int side = 0; side < 2; ++side) {
		U64 kZone, tkz;

		if (side) kZone = boards.byColorPiecesBB[BLACK][KING];
		else kZone = boards.byColorPiecesBB[WHITE][KING];

		int location = boards.pieceLoc[side][KING][0];

		//piece square table flip if black
		int psqLoc = side == WHITE ? location : bSQ[location];

		//psq table scoring for king
		ev.psqScores[side].mg += pieceSqTab[KING][mg][psqLoc]; //change piece square tables to Scores
		ev.psqScores[side].eg += pieceSqTab[KING][eg][psqLoc];

		//grab pseudo attacks for king on square
		kZone = boards.psuedoAttacks(KING, side, location);

		//add king attacks to attacked by and attacked by all
		ev.attackedBy[side][0] |= ev.attackedBy[side][KING] = kZone;

		//add a zone three tiles across in front of the 8 squares surrounding the king
		if (side)  kZone |= kZone << 8;
		else kZone |= kZone >> 8;

		if (location % 8 < 4) {
			kZone &= ~FILE_GH;

		}
		else {
			kZone &= ~FILE_AB;
		}
		//remove square king is on, maybe delete this in future?
		if (side) ev.kingZone[BLACK] = kZone & ~boards.byColorPiecesBB[BLACK][KING];
		else ev.kingZone[WHITE] = kZone & ~boards.byColorPiecesBB[WHITE][KING];
	}
}

template<int pT, int color>
Scores evaluatePieces(const BitBoards & boards, EvalInfo & ev, U64 * mobilityArea) {

	Scores score;

	const int* piece = boards.pieceLoc[color][pT];
	const int them = (color == WHITE ? BLACK : WHITE);

	//increment piece type if we're on a black piece
	const int nextPiece = (color == WHITE ? pT : pT + 1);

	int square;

	while (( square = *piece++ ) != SQ_NONE) {

		//adjust square table lookup for black if need be
		int adjSq = color == WHITE ? square : bSQ[square];

		//add piece square table scores 
		ev.psqScores[color] += make_scores(pieceSqTab[pT][mg][adjSq], pieceSqTab[pT][eg][adjSq]);
			

		//find attack squares, including xray bishop and rook attacks blocked by queens / rooks/queens
		U64 bb = pT == BISHOP ? slider_attacks.BishopAttacks(boards.FullTiles ^ boards.pieces(color, QUEEN), square)
			   : pT == ROOK   ? slider_attacks.RookAttacks(  boards.FullTiles ^ boards.pieces(color, QUEEN, ROOK), square)
			   : pT == QUEEN  ? slider_attacks.QueenAttacks( boards.FullTiles, square)
			   : boards.PseudoAttacks[KNIGHT][square]; 

		// If piece is pinned, be sure that we only look at
		// Mobility and move data for it moving along it's pinned line ///////////////////////////////////////THIS NEEDS SERIOUS TESTING, SLOWS DOWN SEARCH SIGNIFICANTLY
		if (ev.pinnedPieces[color] & boards.square_bb(square))
			bb &= LineBB[boards.king_square(color)][square];

		//add attack squares to both piece attacks by color, and all piece attacks by color
		//very important order, if all pieces comes first the pT will grab all attack boards generated.
		ev.attackedBy[color][0] |= ev.attackedBy[color][pT] |= bb;

		//if pieces is attacking zone around king..
		if (bb & ev.kingZone[them]) {

			ev.kingAttackers[color]++;
			ev.kingAttWeights[color] += KingAttackerWeights[pT];		
			U64 adjacent = bb & ev.attackedBy[them][KING]; 

			//if piece is attacking squares defended by the enemy king..
			if (adjacent) ev.adjacentKingAttckers[color] += bit_count(adjacent);
		}

		//if piece is queen, don't count squares that are attacked by 
		//knight, bishop, rook towards mobility
		if (pT == QUEEN)
			bb &= ~(ev.attackedBy[them][KNIGHT]
			   |    ev.attackedBy[them][BISHOP]
			   |    ev.attackedBy[them][ROOK  ]);

		//gather mobility count and record mob scores for mid and end game
		//don't count squares occupied by our pawns or king, or attacked by their pawns
		//towards mobility. Additional restrictions for queen mob above.
		int mobility = bit_count(bb & mobilityArea[color]);

		ev.mobility[color] += mobilityBonus[pT][mobility];

		//if piece is threatened by a pawn, apply penalty
		//increasing penalty in threatened piece value
		if (ev.attackedBy[them][PAWN] & square) 
			score -= pawnThreat[pT];
		

		//need knight and bishop evals
		if (pT == KNIGHT || pT == BISHOP) {

			//penalty for bishop with our pawns on same color squares
			if (pT == BISHOP) score -= BishopPawns * ev.pe->pawns_on_same_color_squares(color, square); //Very High Cost? What's with this?
			
			if (relative_rankSq(color, square) < 5 
				&& boards.piecesByType(PAWN) & (1LL << (square + pawn_push(color))))
				score += MinorBehindPawn;
		}

		if (pT == ROOK) {

			// is our rook on a semi open file?
			// if so, is it semi open from enemys pov as well? then it's an open file,
			// else if it's open from our POV only it's semi open.
			if (ev.pe->semi_open_file(color, file_of(square)))
				score += ev.pe->semi_open_file(them, file_of(square)) ? RookOpen : RookHalfOpen;

			// if our rook is past the 5th rank, relative to stm
			// and our rook is attacking(or just could attack if a piece moved)
			// a pawn, or multiple pawns increase score.
			if (relative_rankSq(color, square) >= 5) {
				U64 pawns = boards.pieces(them, PAWN) & boards.psuedoAttacks(ROOK, color, square);

				if (pawns) score += RookOnPawn * bit_count(pawns);
			}

			//move rook trapped by king to here, out of cumbersome blocked pieces check
			//add penalty if rook is trapped AND can't caslte
		}
	}

	return score - evaluatePieces<nextPiece, them>(boards, ev, mobilityArea);
}

template<> //get rid of recursive function too complex error, also return when piece type hits king
Scores evaluatePieces<KING, WHITE>(const BitBoards & boards, EvalInfo & ev, U64 * mobility) { return S(0, 0); };

template<int color>
int centerControl(const BitBoards & boards, const EvalInfo & ev) {

	const int them = color == WHITE ? BLACK : WHITE;

	//find the safe tiles, safe if not attacked by enemy pawn,
	//or if it is attacked by an enemy piece and not defended
	U64 safe = center[color]
			 & ~boards.pieces(color, PAWN)
			 & ~ev.attackedBy[them][PAWN]
			 & (ev.attackedBy[color][0] | ev.attackedBy[them][0]);


	//find squares which are between one and three squares behind friendly pawns
	U64 behind = boards.pieces(color, PAWN);

	behind |= (color == WHITE ? behind <<  8 : behind >>  8);
	behind |= (color == WHITE ? behind << 16 : behind >> 16);

	//return the count of number of safe squares behind, relative to stm,
	//that are within the center. Very minor bonus to mid game
	return bit_count((color == WHITE ? safe >> 32 : safe << 32) | (behind & safe));
}

template<int color>
int evaluateKing(const BitBoards & boards, EvalInfo & ev, int mgScore) {

	const int them = color == WHITE ? BLACK : WHITE;
	int attackUnits = 0;
	int score = 0;

	//our king location
	const int square = boards.king_square(color);

	U64 undefended, safe;

	if (ev.kingAttackers[them]) {
				
		//find attacked squares around king which have no defenders apart from king
		undefended = ev.attackedBy[them][0] //attacked by squares for all pieces of color
				& ev.attackedBy[color][KING]
				& ~( ev.attackedBy[color][PAWN] | ev.attackedBy[color][KNIGHT]
				| ev.attackedBy[color][BISHOP] | ev.attackedBy[color][ROOK]
				| ev.attackedBy[color][QUEEN]);

		attackUnits = std::min(17, (ev.kingAttackers[them] * ev.kingAttWeights[them]) / 2)
					+ 2 * (ev.adjacentKingAttckers[them] + bit_count(undefended))
					- mgScore / 32
					- !boards.pieceCount[them][QUEEN] * 15;

		//queen enemy safe contact checks, where enemy queen is next to king 
		U64 bb = undefended & ev.attackedBy[them][QUEEN] & ~boards.pieces(them);

		if (bb) {
				//remove squares defended by enemy pieces
			bb &= (ev.attackedBy[them][PAWN] | ev.attackedBy[them][KNIGHT]
				| ev.attackedBy[them][BISHOP] | ev.attackedBy[them][ROOK]);

			if (bb) attackUnits += QueenContactCheck * bit_count(bb);
		}

		//rook safe enemy contact checks
		bb = undefended & ev.attackedBy[them][ROOK] & ~boards.pieces(them);

		//only look at undefended checking squares, & with rook attacks from king location
		bb &= boards.psuedoAttacks(ROOK, color, square);

		if (bb) {
				//remove squares defended by enemy pieces
			bb &= (ev.attackedBy[them][PAWN] | ev.attackedBy[them][KNIGHT]
				| ev.attackedBy[them][BISHOP] | ev.attackedBy[them][ROOK]);

			if (bb) attackUnits += RookContactCheck * bit_count(bb);
		}

		//look at enemy safe checks from sliders and knights
		safe = ~(boards.pieces(them) | ev.attackedBy[color][0]);

		//any enemy safe slider, non contact checks??
		U64 b1 = slider_attacks.RookAttacks(boards.FullTiles, square) & safe;
		U64 b2 = slider_attacks.BishopAttacks(boards.FullTiles, square) & safe;

		bb = (b1 | b2) & ev.attackedBy[them][QUEEN];

		//enemy safe queen checks
		if (bb) attackUnits += QueenCheck * bit_count(bb);

		bb = b1 & ev.attackedBy[them][ROOK];

		if(bb) attackUnits += RookCheck * bit_count(bb);

		//enemy safe bishop checks
		bb = b2 & ev.attackedBy[them][BISHOP];

		if(bb) attackUnits += BishopCheck * bit_count(bb);

		//enemy safe knight checks
		bb = boards.psuedoAttacks(KNIGHT, them, square) & ev.attackedBy[them][KNIGHT] & safe;

		if(bb) attackUnits += KnightCheck * bit_count(bb);

		attackUnits = std::min(99, attackUnits);

		score -= SafetyTable[attackUnits];
	}

	return score;
}

template<int color>
Scores evalThreats(const BitBoards & boards, EvalInfo & ev) {

	int them = (color == WHITE ? BLACK : WHITE);

	U64 bb, weakEnemys, protectedEnemys;

	Scores score;

	enum {Minor, Major};

	//enemys (that are not their pawns) which are protected by their knights or bishops
	protectedEnemys = (boards.pieces(them) ^ boards.pieces(them, PAWN))
					& ev.attackedBy[them][PAWN]
					& (ev.attackedBy[color][KNIGHT] | ev.attackedBy[color][BISHOP]);


	//if an enemy is protected by a pawn, and attacked by our bishop or knight
	if (protectedEnemys) {
		int p = boards.pieceOnSq(lsb(protectedEnemys));

		score += threats[Minor][p];
	}

	weakEnemys = boards.pieces(them)
			   & ~ev.attackedBy[them][0] //not supported by their pieces
			   & ev.attackedBy[color][0]; //and attacked by any of our pieces

	//add bonus for attacking undefended pieces
	if (weakEnemys) {

		//enemy piece is attacked by our knight or bishop
		bb = weakEnemys & (ev.attackedBy[color][KNIGHT] | ev.attackedBy[color][BISHOP]);

		if (bb) {
			int p = boards.pieceOnSq(lsb(bb));

			score += threats[Minor][p];
		}

		//enemy piece is attacked by our rook/queen
		bb = weakEnemys & (ev.attackedBy[color][ROOK] | ev.attackedBy[color][QUEEN]);

		if (bb) {
			int p = boards.pieceOnSq(lsb(bb));

			score += threats[Major][p];
		}

		//enemy piece/s aren't supported
		bb = weakEnemys & ~ev.attackedBy[them][0]; 

		if (bb) {
			int pop = bit_count(bb);

			score += Hanging * pop;
		}

		//enemy pawn/s are attacked by our king
		bb = weakEnemys & boards.pieces(them, PAWN) & ev.attackedBy[color][KING];

		if (bb) {

			//one or many pawns?
			//no mid game value for this threat
			score += more_than_one(bb) ? KingOnMPawn : KingOnPawn;
		}
	}
	return score;
}

template<int color>
Scores evaluatePassedPawns(const BitBoards& board, const EvalInfo& ev) {
	
	const int them = color == WHITE ? BLACK : WHITE;

	U64 b, squaresToQueen, defendedSquares, unsafeSquares;

	Scores score;

	//COMMENT OUT WHEN NOT TESTING!!!!!!!!!!!!!!!!!!!!!!!!!
	//BitBoards a = board;

	b = ev.pe->passedPawns[color];

	while (b) {

		//assert pawn_passed check to test here

		//find, remove, record pos of least sig bit
		int sq = pop_lsb(&b);

		int r = relative_rankSq(color, sq) - 1;
		int rr = r * (r - 1);

		//need to modify bonus values for vals that are appropriate to this engine
		int mBonus = 17 * rr, ebonus = 7 * (rr + r + 1);

		if (rr) {

			//square in front of passed pawn, relative our stm
			int blockSq = sq + pawn_push(color);

			//adjust bonus passed on both kings proximity to block square 
			ebonus += square_distance(board.king_square(them), blockSq) * 5 * rr
				   - square_distance(board.king_square(color), blockSq) * 2 * rr;

			// If blockSq is not the queening square then consider also a second push
			if (relative_rankSq(color, blockSq) != 7) {
				ebonus -= square_distance(board.king_square(color), blockSq + pawn_push(color)) * rr;
			}

			// If the pawn is free to advance, then increase the bonus
			if (board.empty(blockSq)) {

				// If there is a rook or queen attacking/defending the pawn from behind,
				// consider all the squaresToQueen. Otherwise consider only the squares
				// in the pawn's path attacked or occupied by the enemy.

				defendedSquares = unsafeSquares = squaresToQueen = forwardBB[color][sq];

				U64 bb = forwardBB[them][sq] & board.piecesByType(ROOK, QUEEN) & slider_attacks.RookAttacks(board.FullTiles, sq); 

				if (!(board.pieces(color) & bb))
					defendedSquares &= ev.attackedBy[color][0]; //attacked by all pieces


				if (!(board.pieces(them) & bb))
					unsafeSquares &= ev.attackedBy[them][0] | board.pieces(them);


				// If there aren't any enemy attacks, assign a big bonus. Otherwise
				// assign a smaller bonus if the block square isn't attacked.

				int k = !unsafeSquares ? 15 : !(unsafeSquares & (1LL << blockSq)) ? 9 : 0;

				// If the path to queen is fully defended, assign a big bonus.
				// Otherwise assign a smaller bonus if the block square is defended.

				if (defendedSquares == squaresToQueen)
					k += 6;

				else if (defendedSquares & blockSq)
					k += 4;

				mBonus += k * rr, ebonus += k * rr;

			}

			else if (board.pieces(color) & (1LL << blockSq)) {

				mBonus += rr * 3 + r * 2 + 3;
				ebonus += rr + r * 2;
			}
		}

		if (board.count<PAWN>(color) < board.count<PAWN>(them))
			ebonus += ebonus / 4;


		score += make_scores(mBonus, ebonus);
	}

	return applyWeights(score, Weights[PassedPawns]);
}

template<int color>
Scores unstoppablePawns(const EvalInfo& ev) {

	U64 bb = ev.pe->passedPawns[color] | ev.pe->candidatePawns[color];

	return bb ? Unstoppable * relative_rank(color, frontmost_sq(color, bb)) : SCORE_ZERO;
}

//Weights are used to modify values in a particular section 
//compared to those of other sections in overall scoring
enum {Mobility,  PawnStructure,  PassedPawns, Center,  KingSafety, Imbalence};

const struct Weight { int mg, eg; } Weights[] = { //Test Weights
	{ 289, 344 }, { 205, 188 }, { 65, 86 }, { 25, 0 }, { 318, 0 }, { 40, 40 } //new weights testing (217, 200)~~testPawnStructureWeight
};
/*
const struct Weight { int mg, eg; } Weights[] = { //LatestStuck Weights Current weights With best ELO SO FAR.
{ 289, 344 }, { 205, 188 }, { 65, 86 }, { 25, 0 }, { 318, 0 }, { 40, 40} //CenterControlWeight Needs adjusting!!!!
}; ^^mob should be ^^pawn adjusted down

const struct Weight { int mg, eg; } Weights[] = {//StockFish Weights.
{289, 344}, {233, 201}, {221, 273}, {46, 0}, {318, 0}
};
*/
Scores applyWeights(Scores s, const Weight & w) {
	s.mg = s.mg * w.mg / 256;
	s.eg = s.eg * w.eg / 256;
	return s;
}

int Evaluate::evaluate(const BitBoards & boards)
{
	const int color = boards.stm();

	/*
	 //is this needed with TT lookup in quiet??
	int hash = boards.TTKey() & 5021982; //REPLACE THIS!!
	HashEntry entry = transpositionEval[hash];
	//if we get a hash-table hit, return the evaluation
	if (entry.zobrist == boards.TTKey()) {
		if (color == WHITE) {
			//if eval was from blacks POV, return -eval
			if (entry.flag) return -entry.eval;
			else return entry.eval; //if eval was from our side, return normally
		}
		else {

			if (entry.flag) return entry.eval;
			else return -entry.eval;
		}

	}
	*/

	EvalInfo ev;
	Scores score;	
	int result = 0;

	//initilize king zones and king attacks for both kings
	generateKingZones(boards, ev);

	//find pinned pieces of color
	ev.pinnedPieces[WHITE] = boards.pinned_pieces(WHITE);
	ev.pinnedPieces[BLACK] = boards.pinned_pieces(WHITE);

	//probe the material hash table for an end game scenario,
	//a factor to scale to evaluation score by, and/or
	//any other bonuses or penalties from material config
	ev.me = Material::probe(boards, MaterialTable);

	score += applyWeights(ev.me->material_value(), Weights[Imbalence]);

	//probe the pawn hash table for a hit,
	//if we don't get a hit do full pawn eval and return
	ev.pe = Pawns::probe(boards, pawnsTable); 

	//Weights are used to modify the scoring of a section of the 
	//evaluation, without modifying the values inside the subsection
	score += applyWeights(ev.pe->score, Weights[PawnStructure]);

	//merge pawn attacks with our attack boards
	ev.attackedBy[WHITE][0] |= ev.attackedBy[WHITE][PAWN] = ev.pe->pawnAttacks[WHITE];
	ev.attackedBy[BLACK][0] |= ev.attackedBy[BLACK][PAWN] = ev.pe->pawnAttacks[BLACK];

	//grab pawn piece square table values
	ev.psqScores[WHITE] += ev.pe->pieceSqTabScores[WHITE];
	ev.psqScores[BLACK] += ev.pe->pieceSqTabScores[BLACK];

	
	// remove our king + our pawns as well as places attacked 
	// by enemy pawns  from our color mobility areas
	U64 mobilityArea[COLOR]; 
	mobilityArea[WHITE] = ~(ev.attackedBy[BLACK][PAWN] | boards.pieces(WHITE, PAWN, KING)); 
	mobilityArea[BLACK] = ~(ev.attackedBy[WHITE][PAWN] | boards.pieces(BLACK, PAWN, KING)); 
															
	//loop through all pieces from Knight on, subtracting the first score with that of 
	//the next pieces scores of a different color. Return when we reach king.
	score += evaluatePieces<KNIGHT, WHITE>(boards, ev, mobilityArea);


	//can only evaluate threats after piece attacked by boards have been generated
	score += evalThreats<WHITE>(boards, ev) - evalThreats<BLACK>(boards, ev);

	//add piece square table scores as well as material scores
	score += (ev.psqScores[WHITE] + boards.bInfo.sideMaterial[WHITE]) - (ev.psqScores[BLACK] + boards.bInfo.sideMaterial[BLACK]);

	//evaluate passed pawns, Weights are applied in function itself 
	score += evaluatePassedPawns<WHITE>(boards, ev) - evaluatePassedPawns<BLACK>(boards, ev);

	//get center control data and apply the weights. Only 
	//minor effect on midgame score
	if (ev.me->centerWeight.mg) {
		int ctrl = centerControl<WHITE>(boards, ev) - centerControl<BLACK>(boards, ev);
		
		score += applyWeights(ev.me->centerWeight * ctrl, Weights[Center]);
	}

	//score pieces in bad to horrible positions and pieces 
	//that are blocked into those bad positons by other pieces
	blockedPieces(WHITE, boards, ev);
	blockedPieces(BLACK, boards, ev);

	//add king shield bonus to mid game score, also REPLACE this in pawn eval so we can hash it
	score.mg += wKingShield(boards) - bKingShield(boards);


	//evaluate both kings. Function returns a score taken from the king safety array
	Scores ksf;
	ksf.mg = evaluateKing<WHITE>(boards, ev, score.mg) - evaluateKing<BLACK>(boards, ev, score.mg);

	score += applyWeights(ksf, Weights[KingSafety]);

	//add weight adjusted mobility score to score
	score += applyWeights(ev.mobility[WHITE] - ev.mobility[BLACK], Weights[Mobility]);

	//if both sides only have pawns, score for unstoppable pawns
	if (!boards.non_pawn_material(WHITE) && !boards.non_pawn_material(BLACK)) {

		score += unstoppablePawns<WHITE>(ev) - unstoppablePawns<BLACK>(ev);
	}

	//Interpolate between a mid and end game score based on 
	//total material held as well as predetermined values for
	//finding game phase.
	result = score.mg * (ev.me->gamePhase)	   //replace with a calcualted scale factor from mat/endgames
		   + score.eg * (64 - ev.me->gamePhase) * SF_NORMAL / SF_NORMAL; //change scale factor norm to 32

	result /= 64;

	result += (ev.blockages[WHITE] - ev.blockages[BLACK]);
	result += (ev.adjustMaterial[WHITE] - ev.adjustMaterial[BLACK]);

	//low material adjustment scoring here
	int strong = result > 0 ? WHITE : BLACK;
	int weak = !strong;

	// NEED more intensive filters for low material
	if (boards.pieceCount[strong][PAWN] == 0) { 

		if (boards.bInfo.sideMaterial[strong] < 400) return 0;

		if (boards.pieceCount[weak][PAWN] == 0 && boards.bInfo.sideMaterial[strong] == 2 * SORT_VALUE[KNIGHT])  return 0;//two knights

		if (boards.bInfo.sideMaterial[strong] == SORT_VALUE[ROOK] && boards.bInfo.sideMaterial[weak] == SORT_VALUE[BISHOP]) result /= 2;

		if (boards.bInfo.sideMaterial[strong] == SORT_VALUE[ROOK] + SORT_VALUE[BISHOP]
			&& boards.bInfo.sideMaterial[weak] == SORT_VALUE[ROOK]) result /= 2;

		if (boards.bInfo.sideMaterial[strong] == SORT_VALUE[ROOK] + SORT_VALUE[KNIGHT]
			&& boards.bInfo.sideMaterial[weak] == SORT_VALUE[ROOK]) result /= 2;
	}


	//switch score for color
	result = color == WHITE ? result : -result;

	//save to TT eval table
	//saveTT(color, result, hash, boards);

	return result;
}

int Evaluate::wKingShield(const BitBoards & boards) ///MOVE SHIELDS TO PAWN EVAL, NO REASON TO DO HERE. CAN BE HASHED
{
	//gather info on defending pawns
	int result = 0;
	U64 pawns = boards.byColorPiecesBB[WHITE][PAWN];
	U64 king = boards.byColorPiecesBB[WHITE][KING];

	U64 location = 1LL;

	//king on kingside
	if (KingSide & king) {
		if (pawns & (location << F2)) result += 10;
		else if (pawns & (location << F3)) result += 5;

		if (pawns & (location << G2)) result += 10;
		else if (pawns & (location << G3)) result += 5;

		if (pawns & (location << H2)) result += 10;
		else if (pawns & (location << H3)) result += 5;
	}
	else if (QueenSide & king) {
		if (pawns & (location << A2)) result += 10;
		else if (pawns & (location << A3)) result += 5;

		if (pawns & (location << B2)) result += 10;
		else if (pawns & (location << B3)) result += 5;

		if (pawns & (location << C2)) result += 10;
		else if (pawns & (location << C3)) result += 5;
	}
	return result;
}  

int Evaluate::bKingShield(const BitBoards & boards)
{
	int result = 0;
	U64 pawns = boards.byColorPiecesBB[BLACK][PAWN];
	U64 king = boards.byColorPiecesBB[BLACK][KING];

	U64 location = 1LL;

	//king on kingside
	if (QueenSide & king) {
		if (pawns & (location << F7)) result += 10;
		else if (pawns & (location << F6)) result += 5;

		if (pawns & (location << G7)) result += 10;
		else if (pawns & (location << G6)) result += 5;

		if (pawns & (location << H7)) result += 10;
		else if (pawns & (location << H6)) result += 5;
	}
	//queen side
	else if (KingSide & king) {
		if (pawns & (location << A7)) result += 10;
		else if (pawns & (location << A6)) result += 5;

		if (pawns & (location << B7)) result += 10;
		else if (pawns & (location << B6)) result += 5;

		if (pawns & (location << C7)) result += 10;
		else if (pawns & (location << C6)) result += 5;
	}
	return result;
}

void Evaluate::saveTT(int color, int result, int hash, const BitBoards &boards) //replace this scheme
{
	/*
	//store eval into eval hash table
	transpositionEval[hash].eval = result;
	transpositionEval[hash].zobrist = boards.TTKey();

	//set flag for TT so we can switch value if we get a TT hit but
	//the color of the eval was opposite
	if (color == WHITE) transpositionEval[hash].flag = 0;
	else transpositionEval[hash].flag = 1;
	*/
}

void Evaluate::blockedPieces(int side, const BitBoards& boards, EvalInfo & ev) //REPLACE THIS SOON, DEF BETTER WAY
{
	U64 pawn, epawn, knight, bishop, rook, king;
	U64 empty = boards.EmptyTiles;
	U64 emptyLoc = 1LL;
	int oppo = !side;


	pawn = boards.byColorPiecesBB[side][PAWN];
	knight = boards.byColorPiecesBB[side][KNIGHT];
	bishop = boards.byColorPiecesBB[side][BISHOP];
	rook = boards.byColorPiecesBB[side][ROOK];
	king = boards.byColorPiecesBB[side][KING];

	epawn = boards.byColorPiecesBB[oppo][PAWN];
	
	//central pawn block bishop blocked
	if (isPiece(bishop, boards, relative_square(side, C1))
		&& isPiece(pawn, boards, relative_square(side, D2))
		&& emptyLoc << relative_square(side, D3))
		ev.blockages[side] -= 24;

	if (isPiece(bishop, boards, relative_square(side, F1))
		&& isPiece(pawn, boards, relative_square(side, E2))
		&& emptyLoc << relative_square(side, E3))
		ev.blockages[side] -= 24;

	//trapped knights
	if (isPiece(knight, boards, relative_square(side, A8))
		&& isPiece(epawn, boards, relative_square(oppo, A7))
		|| isPiece(epawn, boards, relative_square(oppo, C7)))
		ev.blockages[side] -= 150;

	//trapped knights
	if (isPiece(knight, boards, relative_square(side, H8))
		&& isPiece(epawn, boards, relative_square(oppo, H7))
		|| isPiece(epawn, boards, relative_square(oppo, F7)))
		ev.blockages[side] -= 150;

	if (isPiece(knight, boards, relative_square(side, A7))
		&& isPiece(epawn, boards, relative_square(oppo, A6))
		&& isPiece(epawn, boards, relative_square(oppo, B7)))
		ev.blockages[side] -= 150;

	if (isPiece(knight, boards, relative_square(side, H7))
		&& isPiece(epawn, boards, relative_square(oppo, H6))
		&& isPiece(epawn, boards, relative_square(oppo, G7)))
		ev.blockages[side] -= 150;

	//knight blocking queenside pawn
	if (isPiece(knight, boards, relative_square(side, C3))
		&& isPiece(knight, boards, relative_square(side, C2))
		&& isPiece(knight, boards, relative_square(side, D4))
		&& !isPiece(knight, boards, relative_square(side, E4)))
		ev.blockages[side] -= 5;

	//trapped bishop
	if (isPiece(bishop, boards, relative_square(side, A7))
		&& isPiece(epawn, boards, relative_square(oppo, B6)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, boards, relative_square(side, H7))
		&& isPiece(epawn, boards, relative_square(oppo, G6)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, boards, relative_square(side, B8))
		&& isPiece(epawn, boards, relative_square(oppo, C7)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, boards, relative_square(side, G8))
		&& isPiece(epawn, boards, relative_square(oppo, F7)))
		ev.blockages[side] -= 150;

	if (isPiece(bishop, boards, relative_square(side, A6))
		&& isPiece(epawn, boards, relative_square(oppo, B5)))
		ev.blockages[side] -= 50;

	if (isPiece(bishop, boards, relative_square(side, H6))
		&& isPiece(epawn, boards, relative_square(oppo, G5)))
		ev.blockages[side] -= 50;

	// bishop on initial square supporting castled king
	//if(isPiece(bishop, relative_square(side, F1))
	//&& isPiece(king, relative_square(side, B1)))
	//need positional themes

	//uncastled king blocking own rook
	if ((isPiece(king, boards, relative_square(side, F1)) || isPiece(king, boards, relative_square(side, G1)))
		&& (isPiece(rook, boards, relative_square(side, H1)) || isPiece(rook, boards, relative_square(side, G1))))
		ev.blockages[side] -= 24;

	if ((isPiece(king, boards, relative_square(side, C1)) || isPiece(king, boards, relative_square(side, B1)))
		&& (isPiece(rook, boards, relative_square(side, A1)) || isPiece(rook, boards, relative_square(side, B1))))
		ev.blockages[side] -= 24;
}

bool Evaluate::isPiece(const U64 & piece, const BitBoards & boards, int sq)
{
	return (piece & boards.square_bb(sq));
}

