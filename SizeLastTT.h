#pragma once
#include "TranspositionTable.h"
#include "SizedExtTTEntry.h"

struct SizeLastTT : TranspositionTable
{
	std::array<SizedExtTTEntry, TTsize> table; // here ext acts like ageCurrent that is compared to last irreversible

	void store(uint64_t key, SearchResult result, EntryType type, int depth, int ageOnStarted, int ageCurrent, uint64_t subtreeSize, int irreversibleNum) override;
	bool retrieve(uint64_t key, TTEntry& entry, bool retrieveOnlyExact) override;
};
