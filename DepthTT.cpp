#include "DepthTT.h"

void DepthTT::store(uint64_t key, int depth, SearchResult result, EntryType type, int age) {
    if (result.eval > 30000 || result.eval < -30000) return;

    //size_t index = key % TTsize;
    size_t index = key & (TTsize - 1);
    TTEntry& entry = table[index];

    if ((entry.key == 0 || entry.depth <= depth) && !result.bestLine.empty()) {
        if (entry.key == 0) stored++;
        else {
            overriten++;
            if (entry.key == key) overwritenWithDiff++;
        }
        entry.key = key;
        entry.depth = depth;
        entry.eval = result.eval;
        entry.type = type;
        entry.bestMove = result.bestLine[result.bestLine.size() - 1];
    }
    else {
        refused++;
    }
}

bool DepthTT::retrieve(uint64_t key, TTEntry& entry, bool retrieveOnlyExact) {
    //size_t index = key % TTsize;
    size_t index = key & (TTsize - 1);
    const TTEntry& storedEntry = table[index];

    if (storedEntry.key == key && (!retrieveOnlyExact || storedEntry.type == EXACT)) {
        retrieved++;
        entry = storedEntry;
        return true;
    }
    not_retrieved++;
    return false;
}