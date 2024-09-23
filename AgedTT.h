#pragma once
#include "TranspositionTable.h"
#include "TTEntry.h"

struct AgedTT : TranspositionTable
{
	struct AgedTTEntry : TTEntry {
		int age;
	};

	std::array<AgedTTEntry, (1 << 22) - 1> table;

	void store(uint64_t key, int depth, SearchResult result, EntryType type, int age) override;
	bool retrieve(uint64_t key, TTEntry& entry) override;

};

