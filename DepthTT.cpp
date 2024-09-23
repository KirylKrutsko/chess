#include "DepthTT.h"

void DepthTT::store(uint64_t key, int depth, SearchResult result, EntryType type, int age) {
    size_t index = key % tableSize;
    TTEntry& entry = table[index];

    if (entry.key == 0 || entry.depth <= depth) {
        entry.key = key;
        entry.depth = depth;
        entry.eval = result.eval;
        entry.type = type;
        if (entry.type == EXACT && !result.bestLine.empty()) {
            entry.bestMove = result.bestLine[result.bestLine.size() - 1];
        }
        else entry.bestMove = Move();
    }
}

bool DepthTT::retrieve(uint64_t key, TTEntry& entry) {
    //return false;
    size_t index = key % tableSize;
    const TTEntry& storedEntry = table[index];

    if (storedEntry.key == key) {
        entry = storedEntry;
        return true;
    }
    return false;
}