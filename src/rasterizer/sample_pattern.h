#pragma once
// clang-format off
/*
 * SamplePattern represents an arrangement of sample locations and blending weights.
 *
 */
#include <string>
#include <vector>

#include "../lib/vec3.h"
struct SamplePattern {
	uint32_t id; // unique id for this sample pattern (used for loading/saving!) (sample id '1' is
	             // single sample per pixel, centered)
	std::string name; // human-readable pattern name
	std::vector<Vec3> centers_and_weights; // center positions (x,y) relative to [0,1]^2 pixels and
	                                       // blending weights (z)

	enum : uint32_t {
		CustomBit = 0x80000000, // if you define any custom patterns, make sure this bit is set in the id.
	};

	// you can get sample patterns either by getting a list of all of them:
	static std::vector<SamplePattern> const& all_patterns();
	//...or looking them up by id: (returns nullptr if not found)
	static SamplePattern const* from_id(uint32_t id);

	// TODO: actually do something elegant to allow vector of patterns to exist while still
	// 		 preventing new patterns from being declared outside of all_patterns().
// private:
	// you really shouldn't be constructing 'em on your own, but...
	SamplePattern(uint32_t, std::string const&, std::vector<Vec3> const&);
	// SamplePattern(SamplePattern const &) = delete; //these are global constants and shouldn't be copied
};
