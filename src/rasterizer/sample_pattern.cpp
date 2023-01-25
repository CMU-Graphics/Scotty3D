#include "sample_pattern.h"

std::vector< SamplePattern > const &SamplePattern::all_patterns() {
	//helper to make grid-like sampling patterns:
	auto Grid = [](uint32_t w, uint32_t h = -1U, uint32_t id = -1U) {
		if (h == -1U) h = w;
		if (id == -1U) id = w * h;
		std::vector< Vec3 > centers_and_weights;
		centers_and_weights.reserve(w*h);
		float weight = 1.0f / (w*h);
		for (uint32_t y = 0; y < h; ++y) {
			for (uint32_t x = 0; x < w; ++x) {
				centers_and_weights.emplace_back((x + 0.5f) / w, (y + 0.5f) / h, weight);
			}
		}
		return SamplePattern(id, "Grid (" + std::to_string(w) + "x" + std::to_string(h) + ")", centers_and_weights);
	};
	static std::vector< SamplePattern > all = [&]() {
		std::vector< SamplePattern > ret;
		ret.emplace_back(1, "Center", std::vector< Vec3 >{Vec3{0.5f, 0.5f, 1.0f}});
		ret.emplace_back(Grid(2));
		ret.emplace_back(Grid(4));
		ret.emplace_back(Grid(8));

		//TODO: add some custom patterns here if you want!
		return ret;
	}();

	return all;
}

SamplePattern const *SamplePattern::from_id(uint32_t id) {
	for (SamplePattern const &sp : all_patterns()) {
		if (sp.id == id) return &sp;
	}
	return nullptr;
}

SamplePattern::SamplePattern(uint32_t id_, std::string const &name_, std::vector< Vec3 > const &centers_and_weights_)
	: id(id_), name(name_), centers_and_weights(centers_and_weights_) {
}
