#pragma once

#include "hashentry.h"

const unsigned TTClusterSize = 2;
//const unsigned TTClusterSizePawn = 4;

//main TTable cluster
struct TTCluster { //64 bytes in size, 32 per entry             //////////////////////////////////////////REWORK THIS , MOVE IS NOW ONLY 16Bit

	HashEntry entry[TTClusterSize];
};
/*
//holds pawn boards | with eachother
struct PawnEntry { //16 bytes in size
	U64 pawnKey;
	int eval;
};

//cluster of pawn entrys
struct TTClusterPawn { //32 bytes ?? possibly should change to 4 entrys, 64 bytes?
	PawnEntry entry[TTClusterSizePawn];
};
*/

class TranspositionT
{
public:
	~TranspositionT() { free(mem); };

	//probes TTable for a hit
	const HashEntry* probe(const U64 key) const;
	//probe Pawn table for a hit
	//const PawnEntry* probePawnT(const U64 key) const;

	//grabs pointer to first entry in search TTable
	HashEntry* first_entry(const U64 key) const;
	//grabs pointer to first entry in pawn eval table
	//PawnEntry* first_entry_pawn(const U64 key) const;

	//save TT entry
	void save(Move& m, const U64 zkey, U8 depth, S16 eval, U8 flag);
	void save(const U64 zkey, U8 depth, S16 eval, U8 flag);
	//save pawn entry
	//void savePawnEntry(const U64 key, int eval);

	//resize TT to a size in megabytes
	void resize(size_t mbSize);

	//void resizePawnT(size_t mbSize);

	//clear the table completely
	void clearTable();
	//clear pawn table
	//void clearPawnTable();

private:	
	size_t clusterCount;
	//size_t pawnClusterCount;

	TTCluster* table;
	//TTClusterPawn* pawnTable;

	void * mem;
	//void * pawnMem;


};

extern TranspositionT TT;
extern TranspositionT TTPawn;


inline HashEntry* TranspositionT::first_entry(const U64 key) const {
	//return a pointer to the first entry in the cluster,
	//indexed by key bitwise & number of total clusters - 1 in TTable
	return &table[(size_t)key & (clusterCount - 1)].entry[0];
}
/*
inline PawnEntry* TranspositionT::first_entry_pawn(const U64 key) const {
	//return a pointer to the first entry in the cluster,
	//indexed by key bitwise & number of total clusters - 1 in TTable
	return &pawnTable[(size_t)key & (pawnClusterCount - 1)].entry[0];
}
*/