#include "MixedTT.h"


void MixedTT::store(uint64_t key, int depth, SearchResult result, EntryType type, int age) {
    if (result.eval > 30000 || result.eval < -30000) return;

    //size_t index = key % TTsize;
    size_t index = key & (TTsize - 1);
    AgedTTEntry& entry = table[index];

    if (!result.bestLine.empty() && (entry.key == 0 || entry.age <= lastIrreversible || entry.depth < depth)) {
        if (entry.key == 0) stored++;
        else {
            overriten++;
            if (entry.key == key) overwritenWithDiff++;
        }
        entry.key = key;
        entry.depth = depth;
        entry.eval = result.eval;
        entry.type = type;
        entry.age = age;
        entry.bestMove = result.bestLine[result.bestLine.size() - 1];
    }
    else {
        refused++;
    }
}

bool MixedTT::retrieve(uint64_t key, TTEntry& entry, bool retrieveOnlyExact) {
    //return false;
    //size_t index = key % TTsize;
    size_t index = key & (TTsize - 1);
    const AgedTTEntry& storedEntry = table[index];

    if (storedEntry.key == key && (!retrieveOnlyExact || storedEntry.type == EXACT)) {
        retrieved++;
        entry = storedEntry;
        return true;
    }
    not_retrieved++;
    return false;
}