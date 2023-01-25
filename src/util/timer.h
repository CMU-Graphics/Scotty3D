
#pragma once

#include <chrono>

class Timer {
public:
	Timer();

	void reset();
	void pause();
	void unpause();

	float ms() const;
	float s() const;

private:
	using Timestamp = std::chrono::high_resolution_clock::time_point;
	using Duration = std::chrono::high_resolution_clock::duration;

	Duration elapsed() const;

	bool is_paused = false;
	Timestamp started, paused;
	Duration lag;
};
