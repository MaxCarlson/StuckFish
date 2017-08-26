#ifndef EVALUATEBB_H
#define EVALUATEBB_H
#include "defines.h"

class ZobristH;
class BitBoards;
class MoveGen;


class evaluateBB
{
public:

    //forms total evaluation for baord state
    int evalBoard(bool isWhite, const BitBoards &boards);


private:
    //gets rudimentry value of piece + square table value
	void evalPieces(const BitBoards & boards);


    //generate zone around king ///Up for debate as to how large zone should be, currently encompasses 8 tiles directly around king
    //currently includes blocking pieces in zone
    void generateKingZones(const BitBoards & boards, bool isWhite);
        U64 wKingZ;
        U64 bKingZ;
    //king pawn shield info
    int wKingShield(const BitBoards & boards);
    int bKingShield(const BitBoards & boards);




//piece evaluation for mobility, attacking king squares, etc
    int getPawnScore(const BitBoards & boards);
    int pawnEval(const BitBoards & boards, bool isWhite, int location);
        int isPawnSupported(bool isWhite, U64 pawn, U64 pawns);

    void evalKnight(const BitBoards & boards, bool isWhite, int location);

    void evalBishop(const BitBoards & boards, bool isWhite, int location);

    void evalRook(const BitBoards & boards, bool isWhite, int location);

    void evalQueen(const BitBoards & boards, bool isWhite, int location);

    //gets blocked pieces data
    void blockedPieces(int side, const BitBoards &BBBoard);
        bool isPiece(const U64 &piece, U8 sq);
        int flip(int side, S8 sq);

    void saveTT(bool isWhite, int result, int hash, const BitBoards &boards); //replace with new tt scheme



};

#endif // EVALUATEBB_H
