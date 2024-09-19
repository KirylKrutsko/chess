#pragma once
#include<chrono>
#include<array>

struct Timer
{
	int timeLimit;
	std::chrono::steady_clock::time_point startPoint;
	void start();
	double runtime();
	Timer();
	bool hasTime();
	void endDepth(long nodes, long moves, int positions);
	double lastTime;
	long lastNodes;
	long lastMoves;
	int lastPositions; // moves/positions = branching factor
};

