#pragma once

#include "defines.h"


// Struct of an individual TT entry, 1 of 4 in a cluster.
// 8 bytes per entry
struct HashEntry
{
public:
	Move move()  const { return (Move)       move16; }
	int depth()  const { return (int )       depth8; }
	int eval()   const { return (int )       eval16; }
	int bound()  const { return (Flag)  flag8 & 0x3; }

	void save(U64 key, int d, int e, Move m, Flag hashFlag, U8 age) {

		if (m || (key >> 48) != zobrist16)
			move16 = (U16)m;

		if ((key >> 48) != zobrist16
			|| d > depth8 - 4
			|| hashFlag == EXACT)
		{
			depth8    = (U8)              d;
			flag8     = (U8) hashFlag | age;
			eval16    = (S16)		      e;
			zobrist16 = (U16)   (key >> 48);
		}
	}

private:
	friend class TranspositionT;

	U8      flag8; // 8  bits
	U8     depth8; // 8  bits
	S16    eval16; // 16 bits
	U16    move16; // 16 bits
	U16 zobrist16; // 16 bits
};


class TranspositionT
{
	static const int TTClusterSize = 4;

	// Holds four Hashentrys
	struct TTCluster { //32 Bytes per cluster, 8 bytes per entry

		HashEntry entry[TTClusterSize];
	};

public:
	~TranspositionT() { free(mem); };

	HashEntry* probe(const U64 key, bool & hit) const;

	// Grabs pointer to first entry in search TTable
	HashEntry* first_entry(const U64 key) const;

	void resize(size_t mbSize);

	void clearTable();

	U8 age() { return age8; }
	void newSearch() { age8 += 4; } // Avoids overwriting the bound flag when or'd with flag8

private:
	size_t clusterCount;

	TTCluster* table;

	void * mem;

	U8 age8;
};

extern TranspositionT TT;



inline HashEntry* TranspositionT::first_entry(const U64 key) const {
	// Return a pointer to the first entry in the cluster,
	// Indexed by low order bits of key for cluster entry, and higher order bits
	// of key for key inside cluster.
	return &table[(size_t)key & (clusterCount - 1)].entry[0];
}