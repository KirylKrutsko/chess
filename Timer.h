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
	bool hasTime(int forDepth);
	void endDepth(int depth, long moves, int positions);
	std::array<double, 30> lastTime{ 0 };
	std::array<long, 30> lastMoves{ 1 };
	std::array<int, 30> lastPositions{ 1 }; // moves/positions = branching factor
};

