#pragma once
#include "TranspositionTable.h"
#include "TTEntry.h"

struct DepthTT : TranspositionTable
{
	std::array<TTEntry, TTsize> table;

	void store(uint64_t key, int depth, SearchResult result, EntryType type, int age) override;
	bool retrieve(uint64_t key, TTEntry& entry, bool retrieveOnlyExact) override;
};

