
#include "hdr_image.h"
#include "../lib/log.h"

#include <sf_libs/stb_image.h>
#include <sf_libs/tinyexr.h>

#include <cstring>

HDR_Image::HDR_Image(uint32_t w, uint32_t h, Spectrum color) : w(w), h(h) {
	pixels.resize(w * h, color);
}

HDR_Image::HDR_Image(uint32_t w, uint32_t h, const std::vector<Spectrum>& pixels)
	: w(w), h(h), pixels(pixels) {
	assert(pixels.size() == w * h);
}

HDR_Image HDR_Image::copy() const {
	HDR_Image ret(w, h, pixels);
	return ret;
}

const std::vector<Spectrum>& HDR_Image::data() const {
	return pixels;
}

std::pair<uint32_t, uint32_t> HDR_Image::dimension() const {
	return {w, h};
}

HDR_Image HDR_Image::load(const std::string& file) {
	HDR_Image image;

	if (IsEXR(file.c_str()) == TINYEXR_SUCCESS) {
		int32_t n_w, n_h;
		float* data;
		const char* err = nullptr;

		int32_t ret = LoadEXR(&data, &n_w, &n_h, file.c_str(), &err);

		if (ret != TINYEXR_SUCCESS) {
			if (err) {
				std::string err_s(err);
				FreeEXRErrorMessage(err);
				throw std::runtime_error("Failed to load EXR from " + file + ": " + err_s);
			} else {
				throw std::runtime_error("Failed to load EXR from " + file + ": Unknown failure.");
			}
		}

		image = HDR_Image(n_w, n_h);
		std::vector< Spectrum > &pixels = image.pixels;

		//EXR is top-left origin, so flip vertically during load to put origin in bottom left:
		for (uint32_t j = 0; j < image.h; j++) {
			for (uint32_t i = 0; i < image.w; i++) {
				uint32_t didx = 4 * ((image.h - 1 - j) * image.w + i);
				uint32_t pidx = j * image.w + i;
				pixels[pidx] = Spectrum(data[didx], data[didx + 1], data[didx + 2]);
				if (!pixels[pidx].valid()) pixels[pidx] = {};
			}
		}

		free(data);

	} else {
		//set first pixel to bottom left:
		stbi_set_flip_vertically_on_load(true);

		int32_t n_w, n_h, channels;
		uint8_t* data = stbi_load(file.c_str(), &n_w, &n_h, &channels, 0);

		if (!data) throw std::runtime_error("Failed to load image from " + file + ": " + std::string(stbi_failure_reason()));
		if (channels < 3) throw std::runtime_error("Image loaded from " + file + " has fewer than 3 color channels.");

		image = HDR_Image(n_w, n_h);
		std::vector< Spectrum > &pixels = image.pixels;

		for (uint32_t i = 0; i < image.w * image.h * channels; i += channels) {
			float r = data[i] / 255.0f;
			float g = data[i + 1] / 255.0f;
			float b = data[i + 2] / 255.0f;
			pixels[i / channels] = Spectrum(r, g, b);
		}

		stbi_image_free(data);

		//TODO: not always correct to assume loaded image is in sRGB colorspace
		for (uint32_t i = 0; i < pixels.size(); i++) {
			if (!pixels[i].valid()) pixels[i] = {};
			pixels[i] = pixels[i].to_linear();
		}
	}

	//remember where the image came from:
	image.loaded_from = file;

	return image;
}

void HDR_Image::save(std::string const &filename) const {
	warn("TODO: write image saving!");
}

constexpr char Raw_Float_format[4] = {'r','a','w','f'};

HDR_Image HDR_Image::decode(uint8_t const *buffer, size_t length) {
	struct {
		char format[4];
		uint32_t width;
		uint32_t height;
	} header;
	static_assert(sizeof(header) == 12, "header is packed.");
	if (length < sizeof(header)) throw std::runtime_error("Buffer isn't large enough for header.");
	//copy and advance:
	std::memcpy(&header, buffer, sizeof(header));
	buffer += sizeof(header);
	length -= sizeof(header);

	if (std::memcmp(header.format, Raw_Float_format, 4) == 0) {
		//raw!
		if (length != header.width * header.height * 3 * 4) throw std::runtime_error("Buffer doesn't have the right number of bytes for a raw 32-bit floating point image.");
		std::vector< Spectrum > pixels(header.width * header.height);
		static_assert(sizeof(Spectrum) == 12, "Spectrum is packed");
		static_assert(offsetof(Spectrum, r) == 0, "Spectrum r is first.");
		static_assert(offsetof(Spectrum, g) == 4, "Spectrum g is second.");
		static_assert(offsetof(Spectrum, b) == 8, "Spectrum b is third.");
		std::memcpy(pixels.data(), buffer, sizeof(Spectrum) * pixels.size());
		return HDR_Image(header.width, header.height, pixels);
	} else {
		throw std::runtime_error("Unrecognized format for image storage.");
	}
}

std::vector< uint8_t > HDR_Image::encode() const {
	std::vector< uint8_t > data( 12 + w * h * 12 );

	static_assert(sizeof(w) == 4, "four-byte width");
	static_assert(sizeof(h) == 4, "four-byte height");

	uint8_t *buffer = data.data();

	//header:
	std::memcpy(buffer, Raw_Float_format, 4); buffer += 4;
	std::memcpy(buffer, reinterpret_cast< const char * >(&w), 4); buffer += 4;
	std::memcpy(buffer, reinterpret_cast< const char * >(&h), 4); buffer += 4;

	//data:
	std::memcpy(buffer, reinterpret_cast< const char * >(pixels.data()), 12 * pixels.size());
	buffer += 12 * pixels.size();
	assert(buffer == data.data() + data.size());

	return data;
}

HDR_Image HDR_Image::missing_image() {
	static const uint64_t img[16] = {
		0xaabaabaabaabaaba,
		0xabaabaabaabaabaa,
		0xbaeeeeeeeeeeeaab,
		0xaaefffffffff1aba,
		0xabefffffffff1baa,
		0xbaeff9fff9ff1aab,
		0xaaeff89f98ff1aba,
		0xabefff898fff1baa,
		0xbaefff989fff1aab,
		0xaaeff98f89ff1aba,
		0xabeff8fff8ff1baa,
		0xbaefffffffff1aab,
		0xaaefffffffff1aba,
		0xab11111111110baa,
		0xbaabaabaabaabaab,
		0xaabaabaabaabaaba
	};
	std::vector< Spectrum > pixels;
	pixels.reserve(16*16);
	for (uint32_t row = 15; row < 16; --row) {
		uint64_t data = img[row];
		for (uint32_t col = 0; col < 16; ++col) {
			pixels.emplace_back(
				((data & 8) / 4  + (data & 1)) / 3.0f,
				((data & 4) / 2  + (data & 1)) / 3.0f,
				(row & 3) / 3.0f
			);
			data >>= 4;
		}
	}
	return HDR_Image(16, 16, pixels);
}

//TODO: should support HDR (i.e. floating point) textures in GL::Tex2D to avoid tonemapping
GL::Tex2D HDR_Image::to_gl(float e) const {
	std::vector<uint8_t> data(w * h);
	tonemap_to(data, e);
	GL::Tex2D tex;
	tex.image(w, h, data.data());
	return tex;
}

void HDR_Image::tonemap_to(std::vector<uint8_t>& data, float e) const {

	if (data.size() != w * h * 4) data.resize(w * h * 4);

	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {

			uint32_t pidx = j * w + i;
			const Spectrum& sample = pixels[pidx];

			float r = 1.0f - std::exp(-sample.r * e);
			float g = 1.0f - std::exp(-sample.g * e);
			float b = 1.0f - std::exp(-sample.b * e);

			Spectrum out(r, g, b);
			out = out.to_srgb();

			uint32_t didx = 4 * (j * w + i);
			data[didx] = static_cast<uint8_t>(std::round(out.r * 255.0f));
			data[didx + 1] = static_cast<uint8_t>(std::round(out.g * 255.0f));
			data[didx + 2] = static_cast<uint8_t>(std::round(out.b * 255.0f));
			data[didx + 3] = 255;
		}
	}
}

bool operator!=(const HDR_Image& a, const HDR_Image& b) {
	auto [aw, ah] = a.dimension();
	auto [bw, bh] = b.dimension();
	if (aw != bw || ah != bh) return true;
	for (uint32_t i = 0; i < aw * ah; i++) {
		if (a.at(i) != b.at(i)) return true;
	}
	return false;
}
