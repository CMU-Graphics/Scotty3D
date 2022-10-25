
#include "rand.h"
#include "../lib/mathlib.h"

#include <ctime>
#include <random>
#include <thread>

RNG::RNG() {
	random_seed();
}

RNG::RNG(uint32_t seed) {
	this->seed(seed);
}

float RNG::unit() {
	//not using std::uniform_real_distribution because it has different behavior on different standard libraries
	static_assert(decltype(mt)::min() == 0 && decltype(mt)::max() == 0xffffffff, "Mersenne Twister has the expected range.");
	return std::scalbn(float(mt()), -32);
}

int32_t RNG::integer(int32_t min, int32_t max) {
	//not using std::uniform_int_distribution because it has different behavior on different standard libraries
	static_assert(decltype(mt)::min() == 0 && decltype(mt)::max() == 0xffffffff, "Mersenne Twister has the expected range.");
	uint64_t size = int64_t(max) - int64_t(min);
	//true, but for readability will not do it: static_assert(int64_t(std::numeric_limits< int32_t >::max()) - int64_t(std::numeric_limits< int32_t >::min()) == std::numeric_limits< uint32_t >::max(), "range size fits into uint32_t");
	//maximum value such that (max_val + 1) is a multiple of size:
	uint32_t max_val = static_cast<uint32_t>(0x100000000ull / size * size - 1ull);
	uint32_t val;
	//rejection sample a value less than max_val:
	do {
		val = mt();
	} while (val > max_val);
	return int32_t(int64_t(uint64_t(val) % size) + int64_t(min));
}

bool RNG::coin_flip(float p) {
	return unit() < p;
}

void RNG::seed(uint32_t s) {
	_seed = s;
	mt.seed(_seed);
}

void RNG::random_seed() {
	std::random_device r;
	std::random_device::result_type s =
		r() +
		static_cast<std::random_device::result_type>(
			std::hash<std::thread::id>()(std::this_thread::get_id())) +
		static_cast<std::random_device::result_type>(std::hash<std::time_t>()(std::time(nullptr)));
	_seed = static_cast<uint32_t>(s);
	mt.seed(_seed);
}

uint32_t RNG::get_seed() {
	return _seed;
}
