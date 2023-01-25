
#include "timer.h"

Timer::Timer() {
	reset();
}

void Timer::reset() {
	started = std::chrono::high_resolution_clock::now();
	paused = {};
	lag = {};
	is_paused = false;
}

void Timer::pause() {
	if (is_paused) return;
	paused = std::chrono::high_resolution_clock::now();
	is_paused = true;
}

void Timer::unpause() {
	if (!is_paused) return;
	lag += std::chrono::high_resolution_clock::now() - paused;
	is_paused = false;
}

Timer::Duration Timer::elapsed() const {
	if (is_paused) return paused - started;
	auto now = std::chrono::high_resolution_clock::now();
	return (now - started) - lag;
}

float Timer::s() const {
	auto dur = elapsed();
	return std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(dur).count();
}

float Timer::ms() const {
	auto dur = elapsed();
	return std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(dur).count();
}
