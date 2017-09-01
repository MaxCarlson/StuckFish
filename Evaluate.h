#pragma once

typedef unsigned long long  U64; // supported by MSC 13.00+ and C99
#define C64(constantU64) constantU64##ULL

class BitBoards;
class EvalInfo;

class Evaluate
{
public:

	int evaluate(const BitBoards & boards, bool isWhite);

	

	



	

private:
	void generateKingZones(const BitBoards & boards, EvalInfo & ev);

	int pawnEval(const BitBoards & boards, int side, int location);

	void saveTT(bool isWhite, int result, int hash, const BitBoards & boards);

	int getPawnScore(const BitBoards & boards, EvalInfo & ev);

	int isPawnSupported(int side, U64 pawn, U64 pawns);

	void blockedPieces(int side, const BitBoards & boards, EvalInfo & ev);

	bool isPiece(const U64 & piece, int sq);

	int flip(int side, int sq);

	//int wKingShield(const BitBoards & boards);

	//int bKingShield(const BitBoards & boards);

};
