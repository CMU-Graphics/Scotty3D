#pragma once

#include <random>

//wraps a pseudo-random number generator with some convenience functions.

struct RNG {
	//start with a random (random-device-based) seed:
	// (likely different on every run!)
	RNG();

	//start with a specified seed:
	// (same sequence of numbers on every run!)
	RNG(uint32_t seed);

	// Generate random float in the range [0,1)
	float unit();

	// Generate random integer in the range [min,max)
	int32_t integer(int32_t min, int32_t max);

	// Return true with probability p and false with probability 1-p
	bool coin_flip(float p);

	void seed(uint32_t s);
	void random_seed();
	uint32_t get_seed();

	static inline uint32_t fixed_seed = 0; //0 = 'pick a new seed every render', otherwise use as seed

	std::mt19937 mt;
private:
	uint32_t _seed = 0;
};
