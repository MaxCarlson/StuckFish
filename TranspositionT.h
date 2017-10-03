#pragma once

#include "defines.h"


// Struct holds all TT entries. Size is 16 bytes
struct HashEntry
{
public:
	Move move() const { return (Move) move16; }
	int depth() const { return (int ) depth8; }
	int eval()  const { return (int ) eval16; }
	int flag()  const { return (int )  flag8; }

private:
	friend class TranspositionT;

	void save(U64 key, int d, int e, Move m, int hashflag) {
		zobrist64 =           key;
		depth8    = (U8)        d;
		eval16    = (S16)       e;
		move16    = (U16)       m;
		flag8     = (U8) hashflag;

	}

	//Zobrist key representing board position move was made from
	U64 zobrist64;
	//depth of move
	U8 depth8;
	//move rating
	S16 eval16;
	//move itself
	U16 move16;
	//flag denoting what the hash represents, i.e. alpha 1, beta 2, exact evaluation (between alpha beta) 3
	U8 flag8;
};

const unsigned TTClusterSize = 2;

//main TTable cluster
struct TTCluster { //32 Bytes per cluster, 16 bytes per entry

	HashEntry entry[TTClusterSize];
};


class TranspositionT
{
public:
	~TranspositionT() { free(mem); };

	//probes TTable for a hit
	const HashEntry* probe(const U64 key) const;

	//grabs pointer to first entry in search TTable
	HashEntry* first_entry(const U64 key) const;

	//save TT entry
	void save(Move m, const U64 zkey, U8 depth, S16 eval, U8 flag);

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