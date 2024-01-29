#pragma once

/*
 * Aperture Shape represents an aperture shape that a camera can take.
 *
 */
#include <string>
#include <vector>

struct ApertureShape {
	uint32_t id; // unique id for this aperture shape (used for loading/saving!)
	             // (sample id '1' is rectangle)
	std::string name; // human-readable pattern name

	enum : uint32_t {
		CustomBit = 0x80000000, // if you define any custom shapes, make sure this bit is set in the id.
	};

	// you can get sample patterns either by getting a list of all of them:
	static std::vector<ApertureShape> const& all_shapes();
	//...or looking them up by id: (returns nullptr if not found)
	static ApertureShape const* from_id(uint32_t id);

	ApertureShape(uint32_t, std::string const&);
};