
#pragma once

#include "../lib/mathlib.h"

#include <random>

namespace RNG {

// The following functions use a static thread-local RNG object. Only use them if you don't care
// about a particular seed/sequence. If you do, create your own RNG object.

// Generate random float in the range [0,1)
float unit();

// Generate random integer in the range [min,max)
int32_t integer(int32_t min, int32_t max);

// Return true with probability p and false with probability 1-p
bool coin_flip(float p = 0.5f);

class RNG {
public:
	RNG();
	RNG(uint32_t seed);

	float unit(); //random float in [0,1)
	int32_t integer(int32_t min, int32_t max); //random int in [min,max)
	bool coin_flip(float p); //true with probability p and false with probability 1-p

	void seed(uint32_t s);
	void random_seed();
	uint32_t get_seed();

private:
	uint32_t _seed = 0;
	std::mt19937 mt;
};

} // namespace RNG
