#pragma once

#include "defines.h"


// Struct of an individual TT entry, 1 of 3 in a cluster.
struct HashEntry
{
public:
	Move move() const { return (Move) move16; }
	int depth() const { return (int ) depth8; }
	int eval()  const { return (int ) eval16; }
	int flag()  const { return (int )  flag8; }

	void save(U64 key, int d, int e, Move m, int hashflag) {

		if (m || (key >> 48) != zobrist16)
			move16 = (U16)m;

		if ((key >> 48) != zobrist16
			|| d > depth8 - 4)
		{
			depth8    = (U8)           d;
			flag8     = (U8)    hashflag;
			eval16    = (S16)		   e;
			zobrist16 = (U16)(key >> 48);
		}
	}

private:
	friend class TranspositionT;

	//Zobrist key representing board position move was made from
	//U64 zobrist64;
	U16 zobrist16;
	//depth of move
	U8 depth8;
	//move rating
	S16 eval16;
	//move itself
	U16 move16;
	//flag denoting what the hash represents, i.e. alpha 1, beta 2, exact evaluation (between alpha beta) 3
	U8 flag8;
};


class TranspositionT
{
	static const unsigned TTClusterSize = 3;

	// Holds three Hashentrys
	struct TTCluster { //32 Bytes per cluster, 10 bytes per entry

		HashEntry entry[TTClusterSize];
		char padd[2];					// Pad TTCluster to 32 bytes
	};

public:
	~TranspositionT() { free(mem); };

	// Probes TTable for a hit
	HashEntry* probe(const U64 key, bool & hit) const;

	// Grabs pointer to first entry in search TTable
	HashEntry* first_entry(const U64 key) const;

	//void save(Move m, const U64 zkey, U8 depth, S16 eval, U8 flag);

	// Resize TT to a size in megabytes
	void resize(size_t mbSize);

	// Clear the table completely
	void clearTable();

private:	
	size_t clusterCount;

	TTCluster* table;

	void * mem;
};

extern TranspositionT TT;



inline HashEntry* TranspositionT::first_entry(const U64 key) const {
	// Return a pointer to the first entry in the cluster,
	// Indexed by lowest o
	return &table[(size_t)key & (clusterCount - 1)].entry[0];
}