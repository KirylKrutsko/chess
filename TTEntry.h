#pragma once
#include<iostream>
#include "Header.h"
#include "Move.h"

struct TTEntry {
    TTEntry();
    uint64_t key;
    uint8_t depth;
    int16_t eval;
    EntryType type;
    Move bestMove;
};