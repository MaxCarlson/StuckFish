#pragma once

#include "hashentry.h"

const unsigned TTClusterSize = 2;

struct TTCluster {

	HashEntry entry[TTClusterSize];
};


class TranspositionT
{
public:
	TranspositionT();
	~TranspositionT() { free(mem); };

	const HashEntry* probe(const U64 key) const;
	HashEntry* first_entry(const U64 key) const;

	//save TT entry
	void save(Move& m, const U64 zkey, U8 depth, S16 eval, U8 flag);

	//resize TT to a size in megabytes
	void resize(size_t mbSize);

	//clear the table completely
	void clearTable();

private:	
	size_t clusterCount;
	TTCluster* table;
	void * mem;


};

extern TranspositionT TT;


inline HashEntry* TranspositionT::first_entry(const U64 key) const {
	//return a pointer to the first entry in the cluster,
	//indexed by key bitwise & number of total clusters - 1 in TTable
	return &table[(size_t)key & (clusterCount - 1)].entry[0];
}
