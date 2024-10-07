#pragma once
#include "Header.h"
#include "TTEntry.h"
#include "SearchResult.h"
#include <array>

struct TranspositionTable
{
	uint64_t stored = 0;
	uint64_t overriten = 0;
	uint64_t overwritenWithDiff = 0;
	uint64_t refused = 0;
	uint64_t retrieved = 0;
	uint64_t not_retrieved = 0;

    virtual void store(uint64_t key, int depth, SearchResult result, EntryType type, int age) = 0;
    virtual bool retrieve(uint64_t key, TTEntry& entry, bool retrieveOnlyExact) = 0;
};
