#pragma once
#include<vector>
#include"Move.h"

struct SearchResult {
    int eval;
    std::vector<Move> bestLine;
    long nodesSearched;
    long ttHit;
    double runtime;
};