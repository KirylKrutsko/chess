#include "Timer.h"
#include <iostream>

Timer::Timer() {
	timeLimit = 30;
	lastTime = 0;
}

void Timer::start() {
	startPoint = std::chrono::high_resolution_clock::now();
	lastTime = 0;
}

double Timer::runtime() {
	auto duration = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startPoint);
	return duration.count();
}

void Timer::endDepth(long nodes, long moves, int positions) {
	lastTime = runtime() - lastTime;
	lastNodes = nodes;
	lastMoves = moves;
	lastPositions = positions;
}

bool Timer::hasTime() {
	if (runtime() > timeLimit) return false;
	double branchingFactor = (double)lastMoves / lastPositions;
	double timePerNode = lastTime / lastNodes;
	double timeExpected = timePerNode * lastNodes * branchingFactor / 1.4;
	std::cout << "\nExpected to finish in : " << timeExpected + runtime() << "\n";
	return (timeExpected < (timeLimit - runtime()));

	/*return false;
	double time = runtime();
	std::cout << "last : " << lastTimings[nextDepth] << "\nlast-1 : " << lastTimings[nextDepth - 1] << "\nleft : " << (timeLimit - time) << "\n";
	if ((lastTimings[nextDepth] - lastTimings[nextDepth - 1]) < (timeLimit - time)) {
		lastTimings[nextDepth - 1] = time;  
		return true;  
	}
	else {
		lastTimings[nextDepth - 1] = time;
		return false;
	}
	return true;*/
}