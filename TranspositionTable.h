#pragma once
#include "Header.h"
#include "TTEntry.h"
#include<array>

struct TranspositionTable
{
    TranspositionTable();
    size_t tableSize;
    std::array<TTEntry, 1 << 20> table;
    void insert(uint64_t key, uint8_t depth, int16_t eval, EntryType type, Move bestMove);
    void insert(uint64_t key, uint8_t depth, int16_t eval, EntryType type);
    bool retrieve(uint64_t key, TTEntry& entry) const;
};

