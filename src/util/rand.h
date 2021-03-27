
#pragma once

#include "../lib/mathlib.h"

namespace RNG {

// Generate random float in the range [0,1]
float unit();

// Generate random integer in the range [min,max)
int integer(int min, int max);

// Return true with probability p and false with probability 1-p
bool coin_flip(float p = 0.5f);

// Seed the current thread's PRNG
void seed();
} // namespace RNG
