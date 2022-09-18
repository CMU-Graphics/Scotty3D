#include "test.h"

#include "rasterizer/framebuffer.h"
#include "rasterizer/sample_pattern.h"
#include "util/hdr_image.h"

//-------------------------------------------------
// framebuffer indexing:

Test test_a1_task7_index("a1.task7.index", []() {

	//this is a terrible sample pattern, but it does have an odd number of sample locations:
	SamplePattern pattern(SamplePattern::CustomBit | 1234, "just some test sample pattern", std::vector< Vec3 >{
		Vec3(0.1f, 0.1f, 0.1f),
		Vec3(0.2f, 0.2f, 0.1f),
		Vec3(0.3f, 0.3f, 0.1f),
		Vec3(0.4f, 0.4f, 0.1f),
		Vec3(0.5f, 0.5f, 0.1f),
		Vec3(0.6f, 0.6f, 0.1f),
		Vec3(0.7f, 0.7f, 0.3f)
	});

	Framebuffer fb(14, 22, pattern);

	if (fb.colors.size() != 14 * 22 * 7) {
		throw Test::error("Framebuffer didn't allocate enough color storage locations. Test cannot proceed.");
	}

	if (fb.depths.size() != 14 * 22 * 7) {
		throw Test::error("Framebuffer didn't allocate enough depth storage locations. Test cannot proceed.");
	}

	//fill with unique values:
	for (uint32_t y = 0; y < fb.height; ++y) {
		for (uint32_t x = 0; x < fb.width; ++x) {
			for (uint32_t s = 0; s < pattern.centers_and_weights.size(); ++s) {
				uint32_t i = fb.index(x,y,s);
				if (i >= fb.colors.size()) {
					throw Test::error("fb.index(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(s) + ") is out-of-range.");
				}
				fb.color_at(x,y,s) = Spectrum(float(x),float(y),float(s));
			}
		}
	}

	//read back values:
	for (uint32_t y = 0; y < fb.height; ++y) {
		for (uint32_t x = 0; x < fb.width; ++x) {
			for (uint32_t s = 0; s < pattern.centers_and_weights.size(); ++s) {
				if (Test::differs(fb.color_at(x,y,s), Spectrum(float(x),float(y),float(s)))) {
					throw Test::error("FB value at " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(s) + " got clobbered by " + to_string(fb.color_at(x,y,s)) + ".");
				}
			}
		}
	}

});


//-------------------------------------------------
// framebuffer color resolve:

Test test_a1_task7_resolve("a1.task7.resolve", []() {
	SamplePattern pattern(SamplePattern::CustomBit | 777, "just some other test pattern", std::vector< Vec3 >{
		Vec3(0.1f, 0.1f, 0.3f),
		Vec3(0.2f, 0.2f, 0.7f),
		Vec3(0.3f, 0.3f, 0.0f),
	});

	Framebuffer fb(2, 2, pattern);

	//all sample values get zero:
	fb.colors.assign(fb.colors.size(), Spectrum(0.0f, 0.0f, 0.0f));

	//now set one pixel carefully:
	fb.color_at(1,0,0) = Spectrum(1.0f, 0.0f, 0.0f); //should get weight 0.3
	fb.color_at(1,0,1) = Spectrum(0.0f, 1.0f, 0.0f); //should get weight 0.7
	fb.color_at(1,0,2) = Spectrum(0.0f, 0.0f, 1.0f); //should get weight 0.0

	HDR_Image resolved = fb.resolve_colors();

	if (resolved.w != 2 || resolved.h != 2) {
		throw Test::error("Framebuffer of size " + std::to_string(fb.width) + "x" + std::to_string(fb.height) + " resolved to image of size " + std::to_string(resolved.w) + "x" + std::to_string(resolved.h) + ".");
	}
	if (resolved.at(0,0) != Spectrum(0.0f, 0.0f, 0.0f)
	 || resolved.at(0,1) != Spectrum(0.0f, 0.0f, 0.0f)
	 || resolved.at(1,1) != Spectrum(0.0f, 0.0f, 0.0f) ) {
		throw Test::error("All-black pixel resolved to non-all-black.");
	}
	Spectrum expected(0.3f, 0.7f, 0.0f);
	if (Test::differs(resolved.at(1,0), expected)) {
		throw Test::error("Expected pixel to resolve to " + to_string(expected) + ", got " + to_string(resolved.at(1,0)) + ".");
	}

});

