#include "TranspositionT.h"

#include <iostream>

// Global transposition table
TranspositionT TT; 


void TranspositionT::resize(size_t mbSize)
{
	// Create transposition table array of the same size as mbSize in megabytes
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

	//is there an entry with the same key inside the cluster?               
	for (unsigned i = 0; i < TTClusterSize; ++i, ++tte) {
		if (tte->zobrist64 == key) {

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

		if (!tte->zobrist64 || tte->zobrist64 == zkey) {

			// If there's no move in new entry, don't overwrite existing move
			if (!m) m = tte->move(); 
			
			replace = tte;
			break;
		}

		if (tte->depth8 <= depth) //needs refinement!!!
			replace = tte;	

	}

	replace->save(zkey, depth, eval, m, flag);
}

