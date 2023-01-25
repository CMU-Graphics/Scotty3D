
#pragma once

#include <vector>

#include "../lib/spectrum.h"
#include "../platform/gl.h"

/*
 *
 * HDR_Image stores an image with a floating-point Spectrum per pixel.
 *
 * The origin is located in the bottom left.
 *
 */
class HDR_Image {
public:
	//(0x0) image:
	HDR_Image() = default;
	//solid-colored image:
	HDR_Image(uint32_t w, uint32_t h, Spectrum color = Spectrum(0.0f, 0.0f, 0.0f));
	//image from pixel array (row-major, bottom-left origin):
	//required: pixels.size() == w*h
	HDR_Image(uint32_t w, uint32_t h, const std::vector<Spectrum>& pixels);

	~HDR_Image() = default;

	//you must copy or move HDR_Image explicitly:
	HDR_Image(const HDR_Image& src) = delete;
	HDR_Image& operator=(const HDR_Image& src) = delete;
	HDR_Image(HDR_Image&& src) = default;
	HDR_Image& operator=(HDR_Image&& src) = default;
	HDR_Image copy() const;

	//direct data access (row-major, bottom-left origin):
	const std::vector<Spectrum>& data() const;

	//range-checked access helpers:
	Spectrum& at(uint32_t x, uint32_t y) {
		assert(x < w && y < h);
		return pixels[y * w + x];
	}
	Spectrum const &at(uint32_t x, uint32_t y) const {
		assert(x < w && y < h);
		return pixels[y * w + x];
	}
	Spectrum& at(uint32_t i) {
		return pixels.at(i);
	}
	Spectrum const &at(uint32_t i) const {
		return pixels.at(i);
	}

	//void clear(Spectrum color);
	//void resize(uint32_t w, uint32_t h);
	std::pair<uint32_t, uint32_t> dimension() const;

	//file I/O:
	static HDR_Image load(const std::string& filename); //load from a file, throws on error
	void save(std::string const &filename) const;

	//memory I/O:
	static HDR_Image decode(uint8_t const *buffer, size_t length); //load from memory buffer, throws on error
	std::vector< uint8_t > encode() const;

	//placeholder for missing images:
	static HDR_Image missing_image();

	GL::Tex2D to_gl(float exposure) const;
	void tonemap_to(std::vector<uint8_t>& data, float exposure) const;

	uint32_t w = 0, h = 0;

	std::string loaded_from = "";
private:
	std::vector<Spectrum> pixels;
};

bool operator!=(const HDR_Image& a, const HDR_Image& b);
