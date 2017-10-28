#include "TranspositionT.h"

#include <iostream>

// Global transposition table
TranspositionT TT; 

// Create or resize our Transpositon table to the nearest power
// of two megabytes to the mbSize input.
void TranspositionT::resize(size_t mbSize)
{
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

HashEntry * TranspositionT::probe(const U64 key, bool & hit) const
{
	const U16 key16 = key >> 48;

	HashEntry * const tte = first_entry(key);

	// Is there an entry with the same 16 high order bits inside the cluster? 
	// Or an empty entry spot?
	for (int i = 0; i < TTClusterSize; ++i) 
	{
		if (!tte[i].zobrist16 || tte[i].zobrist16 == key16) {

			if ((tte[i].flag8 & 0xFC) != age8 && tte[i].zobrist16)
				 tte[i].flag8 = U8(age8 | tte[i].bound());

			// Only return a ttHit if there is an entry with a key
			return hit = (bool)tte[i].zobrist16, &tte[i];
		}
	}

	HashEntry * replace = tte;
	for (int i = 0; i < TTClusterSize; ++i) {
		//if (replace->depth8 >= tte[i].depth8)
		//	replace = &tte[i];
		if (replace->depth8 - ((259 + age8 - replace->flag8) & 0xFC) * 2
	        > tte[i].depth8 - ((259 + age8 -   tte[i].flag8) & 0xFC) * 2)
			replace = &tte[i];
	}

	return hit = false, replace;
}
