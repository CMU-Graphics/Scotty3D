#include "aperture_shape.h"

std::vector<ApertureShape> const& ApertureShape::all_shapes() {
	static std::vector<ApertureShape> all = [&]() {
		std::vector<ApertureShape> ret;
		ret.emplace_back(1, "Rectangle");
		ret.emplace_back(2, "Circle");
		return ret;
	}();

	return all;
}

ApertureShape const* ApertureShape::from_id(uint32_t id) {
	for (ApertureShape const& as : all_shapes()) {
		if (as.id == id) return &as;
	}
	return nullptr;
}

ApertureShape::ApertureShape(uint32_t id_, std::string const& name_) : id(id_), name(name_) {
}