#include "AgedTT.h"
#include "TTEntry.h"
#include "TranspositionTable.h"


void AgedTT::store(uint64_t key, int depth, SearchResult result, EntryType type, int age) {
    size_t index = key % tableSize;
    AgedTTEntry& entry = table[index];

    if (entry.key == 0 || entry.age <= age) {
        entry.key = key;
        entry.depth = depth;
        entry.eval = result.eval;
        entry.type = type;
        entry.age = age;
        if (entry.type == EXACT && !result.bestLine.empty()) {
            entry.bestMove = result.bestLine[result.bestLine.size() - 1];
        }
        else entry.bestMove = Move();
    }
}

bool AgedTT::retrieve(uint64_t key, TTEntry& entry) {
    //return false;
    size_t index = key % tableSize;
    const AgedTTEntry& storedEntry = table[index];

    if (storedEntry.key == key) {
        entry = storedEntry;
        return true;
    }
    return false;
}