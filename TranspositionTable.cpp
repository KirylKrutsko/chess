#include "TranspositionTable.h"
#include "TTEntry.h"
#include<iostream>
#include<array>

TranspositionTable::TranspositionTable() {
    tableSize = 1 << 20;
};
    void TranspositionTable::insert(uint64_t key, uint8_t depth, int16_t eval, EntryType type, Move bestMove) {
        size_t index = key % tableSize;
        TTEntry& entry = table[index];

        // Replace only if the new entry is deeper or the slot is empty
        if (entry.key == 0 || entry.depth <= depth) {
            entry.key = key;
            entry.depth = depth;
            entry.eval = eval;
            entry.type = type;
            entry.bestMove = bestMove;
        }
    }
    void TranspositionTable::insert(uint64_t key, uint8_t depth, int16_t eval, EntryType type) {
        size_t index = key % tableSize;
        TTEntry& entry = table[index];

        if (entry.key == 0 || entry.depth <= depth) {
            entry.key = key;
            entry.depth = depth;
            entry.eval = eval;
            entry.type = type;
        }
    }

    bool TranspositionTable::retrieve(uint64_t key, TTEntry& entry) const {
        size_t index = key % tableSize;
        const TTEntry& storedEntry = table[index];

        if (storedEntry.key == key) {
            entry = storedEntry;
            return true;
        }
        return false;
    }
