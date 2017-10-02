#include "TranspositionT.h"

#include <iostream>

TranspositionT TT; //global transposition table
//TranspositionT TTPawn; //global pawn TTable


void TranspositionT::resize(size_t mbSize)
{
	//create transposition table array of the same size as mbSize in megabytes
	size_t newClusterCount = size_t(1) << msb((mbSize * 1024 * 1024) / sizeof(TTCluster));

	if (newClusterCount == clusterCount)
		return;

	clusterCount = newClusterCount;

	free(mem);

	mem = calloc(clusterCount * sizeof(TTCluster) + CACHE_LINE_SIZE - 1, 1);

	if (!mem)
	{
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}

	table = (TTCluster*)((uintptr_t(mem) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

void TranspositionT::clearTable()
{
	std::memset(table, 0, clusterCount * sizeof(TTCluster));
}

const HashEntry * TranspositionT::probe(const U64 key) const
{
///*
	HashEntry *tte = first_entry(key);

	//is there an entry with the same key inside the cluster?                /////////////////////////////////////////////////////////////////   Obviously Re Enable Once We Want To Test New Scheme
	for (unsigned i = 0; i < TTClusterSize; ++i, ++tte) {
		if (tte->zobrist == key) {

			return tte;
		}
	}
//*/
	return NULL;
}

// Save transposition and best move to the main TTable
void TranspositionT::save(Move m, const U64 zkey, U8 depth, S16 eval, U8 flag)
{
	HashEntry *tte, *replace;

	tte = replace = first_entry(zkey);

	for (unsigned i = 0; i < TTClusterSize; ++i, ++tte) {

		if (!tte->zobrist || tte->zobrist == zkey) {

			if (!m) m = tte->move; //if there's no move in new entry, don't overwrite existing move
			
			replace = tte;
			break;
		}

		if (tte->depth <= depth) replace = tte;	//needs refinement!!!

	}

	//save replace
	replace->depth   = depth;
	replace->eval    =  eval;
	replace->flag    =  flag;
	replace->move    =     m;
	replace->zobrist =  zkey;
}

// Save for when there is no move
void TranspositionT::save(const U64 zkey, U8 depth, S16 eval, U8 flag) 
{
	HashEntry *tte, *replace;

	tte = replace = first_entry(zkey);

	for (unsigned i = 0; i < TTClusterSize; ++i, ++tte) {

		if (!tte->zobrist || tte->zobrist == zkey) {

			replace = tte;
			break;
		}

		if (tte->depth <= depth) replace = tte;	//needs refinement!!!

	}

	//save replace
	replace->depth   = depth;
	replace->eval    =  eval;
	replace->flag    =  flag;
	replace->zobrist =  zkey;
}

/* //pawn hash table?? Not good at the moment
void TranspositionT::resizePawnT(size_t mbSize)
{
	//create transposition table array of the same size as mbSize in megabytes
	size_t newClusterCount = size_t(1) << msb((mbSize * 1024 * 1024) / sizeof(TTClusterPawn));

	if (newClusterCount == pawnClusterCount)
		return;

	pawnClusterCount = newClusterCount;

	free(pawnMem);

	pawnMem = calloc(pawnClusterCount * sizeof(TTClusterPawn) + CACHE_LINE_SIZE - 1, 1);

	if (!pawnMem)
	{
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for pawn transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}

	pawnTable = (TTClusterPawn*)((uintptr_t(pawnMem) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));
}

void TranspositionT::clearPawnTable()
{
	std::memset(pawnTable, 0, pawnClusterCount * sizeof(TTClusterPawn));
}

const PawnEntry * TranspositionT::probePawnT(const U64 key) const
{
	PawnEntry *tte = first_entry_pawn(key);

	//is there an entry with the same key inside the cluster?
	for (unsigned i = 0; i < TTClusterSizePawn; ++i, ++tte) {
		if (tte->pawnKey == key) {

			return tte;
		}
	}

	return NULL;
}

void TranspositionT::savePawnEntry(const U64 key, int eval)
{
	PawnEntry *tte, *replace;

	tte = replace = first_entry_pawn(key);

	for (unsigned i = 0; i < TTClusterSizePawn; ++i, ++tte) {

		if (!tte->pawnKey || tte->pawnKey == key) {

			replace = tte;
			break;
		}
	}
	replace->eval = eval;
	replace->pawnKey = key;
}
*/
