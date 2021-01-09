
#include "rand.h"
#include "../lib/mathlib.h"

#include <ctime>
#include <random>
#include <thread>

namespace RNG {

static thread_local std::mt19937 rng;

float unit() {
    std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rng);
}

int integer(int min, int max) {
    return (int)lerp((float)min, (float)max, unit());
}

bool coin_flip(float p) {
    return unit() < p;
}

void seed() {
    std::random_device r;
    std::random_device::result_type seed =
        r() ^
        (std::random_device::result_type)std::hash<std::thread::id>()(std::this_thread::get_id()) ^
        (std::random_device::result_type)std::hash<time_t>()(std::time(nullptr));
    rng.seed(seed);
}

} // namespace RNG
