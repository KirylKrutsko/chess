#pragma once
#include "Header.h"
#include "TTEntry.h"
#include "SearchResult.h"
#include <array>

struct TranspositionTable
{
    TranspositionTable();
    uint64_t tableSize;
    virtual void store(uint64_t key, int depth, SearchResult result, EntryType type, int age) = 0;
    virtual bool retrieve(uint64_t key, TTEntry& entry) = 0;
};
