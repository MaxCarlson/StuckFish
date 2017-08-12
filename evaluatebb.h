#ifndef EVALUATEBB_H
#define EVALUATEBB_H
#include "defines.h"

class ZobristH;
class BitBoards;
class MoveGen;


class evaluateBB
{
public:
    evaluateBB();

    //forms total evaluation for baord state
    int evalBoard(bool isWhite, const BitBoards &BBBoard, const ZobristH &zobristE);

    //returns mate or stalemate score
    int returnMateScore(bool isWhite, int depth);

private:
    //gets rudimentry value of piece + square table value
    void getPieceMaterial(int location);


    //generate zone around king ///Up for debate as to how large zone should be, currently encompasses 8 tiles directly around king
    //currently includes blocking pieces in zone
    void generateKingZones(bool isWhite);
        U64 wKingZ;
        U64 bKingZ;
    //king pawn shield info
    int wKingShield();
    int bKingShield();




//piece evaluation for mobility, attacking king squares, etc
    int getPawnScore();
    int pawnEval(bool isWhite, int location);
        int isPawnSupported(bool isWhite, U64 pawn, U64 pawns);

    void evalKnight(bool isWhite, int location);

    void evalBishop(bool isWhite, int location);

    void evalRook(bool isWhite, int location);

    void evalQueen(bool isWhite, int location);

    //gets blocked pieces data
    void blockedPieces(int side, const BitBoards &BBBoard);
        bool isPiece(const U64 &piece, U8 sq);
        int flip(int side, S8 sq);

    void saveTT(bool isWhite, int result, int hash, const ZobristH &zobrist);



};

#endif // EVALUATEBB_H
