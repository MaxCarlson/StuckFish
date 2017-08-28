#ifndef EVALUATEBB_H
#define EVALUATEBB_H
#include "defines.h"

class ZobristH;
class BitBoards;
class MoveGen;

struct evalVect {
	int gamePhase = 0;
	int pieceMaterial[2] = { 0 };
	int midGMobility[2] = { 0 };
	int endGMobility[2] = { 0 };
	int attCount[2] = { 0 };
	int attWeight[2] = { 0 };
	int mgTropism[2] = { 0 };// still need to add
	int egTropism[2] = { 0 }; // still need to add
	int kingShield[2] = { 0 };
	int adjustMaterial[2] = { 0 };
	int blockages[2] = { 0 };
	int pawnCount[2] = { 0 };
	int pawnMaterial[2] = { 0 };
	int knightCount[2] = { 0 };
	int bishopCount[2] = { 0 };
	int rookCount[2] = { 0 };
	int queenCount[2] = { 0 };
	int psqTabMat[2][2] = { { 0 } }; //holds psq table scores, 0 white, 1 black, mid game = 0, end game 1
}; //object to hold values incase we want to print

class evaluateBB
{
public:

    //forms total evaluation for baord state
    int evalBoard(bool isWhite, const BitBoards &boards);


private:
    //gets rudimentry value of piece + square table value
	void evalPieces(const BitBoards & boards, evalVect & ev);


    //generate zone around king ///Up for debate as to how large zone should be, currently encompasses 8 tiles directly around king
    //currently includes blocking pieces in zone
    void generateKingZones(const BitBoards & boards);
    
	U64 kingZones[2]; //0 == white
    //king pawn shield info
    int wKingShield(const BitBoards & boards);
    int bKingShield(const BitBoards & boards);




//piece evaluation for mobility, attacking king squares, etc
    int getPawnScore(const BitBoards & boards);
    int pawnEval(const BitBoards & boards, int side, int location);
        int isPawnSupported(int side, U64 pawn, U64 pawns);

    void evalKnight(const BitBoards & boards, evalVect & ev, int side, int location);

    void evalBishop(const BitBoards & boards, evalVect & ev, int side, int location);

    void evalRook(const BitBoards & boards, evalVect & ev, int side, int location);

    void evalQueen(const BitBoards & boards, evalVect & ev, int side, int location);

    //gets blocked pieces data
    void blockedPieces(int side, const BitBoards &BBBoard, evalVect & ev);
        bool isPiece(const U64 &piece, U8 sq);
        int flip(int side, S8 sq);

    void saveTT(bool isWhite, int result, int hash, const BitBoards &boards); //replace with new tt scheme



};

#endif // EVALUATEBB_H
