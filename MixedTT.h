#pragma once
#include "TranspositionTable.h"
#include "AgedTTEntry.h"

struct MixedTT : TranspositionTable
{
	int lastIrreversible = 0;
	std::array<AgedTTEntry, TTsize> table;

	void store(uint64_t key, SearchResult result, EntryType type, int depth, int ageOnStarted, int ageCurrent) override;
	bool retrieve(uint64_t key, TTEntry& entry, bool retrieveOnlyExact) override;

};

