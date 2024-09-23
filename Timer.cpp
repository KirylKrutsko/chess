#include "Timer.h"
#include <iostream>

Timer::Timer() {
	timeLimit = 30;
}

void Timer::start() {
	startPoint = std::chrono::high_resolution_clock::now();
}

double Timer::runtime() {
	auto duration = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startPoint);
	return duration.count();
}

void Timer::endDepth(int depth, long moves, int positions) {
	lastTime[depth + 1] = runtime() - lastTime[depth];
	lastMoves[depth + 1] = moves;
	lastPositions[depth + 1] = positions;
}

bool Timer::hasTime(int forDepth) {
	if (runtime() > timeLimit) return false;
	// The way to estimate time from previous depth (is essentially used just for empty TT cases) :
	// time/nodes = timePerNode
	// nodes * BF = currentNodes
	// timePerNode * currentNodes = expectedTime
	// Note that nodes are actually canceling out
	// Devision by 1.4 is just estimated effect of filled TT
	// Also compared to the previous search up to current depth 
	// Because of extention out of TT last depth search can be sugnificantly different from described estimation
	double branchingFactor = (double)lastMoves[forDepth] / lastPositions[forDepth];
	double timeExpected = lastTime[forDepth] * branchingFactor / 1.4; 
	timeExpected = std::max(timeExpected, lastTime[forDepth + 1]);
	//std::cout << "Expected : " << timeExpected + runtime() << "\n";
	return ((timeExpected + runtime()) < timeLimit);
}