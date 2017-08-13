#include "TranspositionT.h"

#include <iostream>

TranspositionT TT; //global transposition table

TranspositionT::TranspositionT()
{
}

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

const HashEntry * TranspositionT::probe(const U64 key) const
{
	HashEntry *tte = first_entry(key);

	for (unsigned i = 0; i < TTClusterSize; ++i, ++tte) {
		if (tte->zobrist == key) {

			return tte;
		}
	}


	return NULL;
}

void TranspositionT::save(Move& m, const U64 zkey, U8 depth, S16 eval, U8 flag)
{
	HashEntry *tte, *replace;

	tte = replace = first_entry(zkey);

	for (unsigned i = 0; i < TTClusterSize; ++i, ++tte) {

		if (!tte->zobrist || tte->zobrist == zkey) {

			if (!m.tried) {
				m = tte->move;
			}
			replace = tte;
			break;
		}
	}
	

}

