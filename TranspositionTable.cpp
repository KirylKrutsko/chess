#include "TranspositionTable.h"
#include "TTEntry.h"
#include<iostream>
#include<array>

TranspositionTable::TranspositionTable() {
    tableSize = (1 << 22) - 1;
};