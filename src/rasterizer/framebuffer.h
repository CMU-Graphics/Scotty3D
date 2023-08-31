#pragma once

#include <vector>

#include "../lib/spectrum.h"

class HDR_Image;
struct SamplePattern;

struct Framebuffer {
	static constexpr uint32_t MaxWidth = 4096;
	static constexpr uint32_t MaxHeight = 4096;

	// Construct a new framebuffer:
	//  width must be <= MaxWidth
	//  height must be <= MaxHeight
	//  both width and height must be even
	// these restrictions exist because:
	//  - having a limited size makes it easier to write rasterization functions with fixed-point
	//    math (if you want)
	//  - having an even size avoids some corner cases if you choose to rasterize with quadfrags
	Framebuffer(uint32_t width, uint32_t height, SamplePattern const& sample_pattern);

	const uint32_t width, height;
	SamplePattern const& sample_pattern;

	// storage for color and depth samples:
	std::vector<Spectrum> colors;
	std::vector<float> depths;

	// return storage index for sample s of pixel (x,y):
	uint32_t index(uint32_t x, uint32_t y, uint32_t s) const {
		// A1T7: index
		// TODO: update to provide different storage locations for different samples
		return y * width + x;
	}

	// helpers that look up colors and depths for sample s of pixel (x,y):
	Spectrum& color_at(uint32_t x, uint32_t y, uint32_t s) {
		return colors[index(x, y, s)];
	}
	Spectrum const& color_at(uint32_t x, uint32_t y, uint32_t s) const {
		return colors[index(x, y, s)];
	}
	float& depth_at(uint32_t x, uint32_t y, uint32_t s) {
		return depths[index(x, y, s)];
	}
	float const& depth_at(uint32_t x, uint32_t y, uint32_t s) const {
		return depths[index(x, y, s)];
	}

	// resolve_colors creates a weighted average of the color samples:
	HDR_Image resolve_colors() const;
};
