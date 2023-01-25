
#include "delta_light.h"
#include "../geometry/util.h"

namespace Delta_Lights {

Incoming Point::incoming(Vec3 p) const {
	Incoming ret;
	ret.direction = -p.unit();
	ret.distance = p.norm();
	ret.radiance = color * intensity;
	return ret;
}

Spectrum Point::display() const {
	return color;
}

Incoming Directional::incoming(Vec3 p) const {
	Incoming ret;
	ret.direction = Vec3(0.0f, -1.0f, 0.0f);
	ret.distance = std::numeric_limits<float>::infinity();
	ret.radiance = color * intensity;
	return ret;
}

Spectrum Directional::display() const {
	return color;
}

Incoming Spot::incoming(Vec3 p) const {
	Incoming ret;
	float angle = std::atan2(Vec2(p.x, p.z).norm(), p.y);
	angle = std::abs(Degrees(angle));
	ret.direction = -p.unit();
	ret.distance = p.norm();
	ret.radiance =
		(1.0f - smoothstep(inner_angle / 2.0f, outer_angle / 2.0f, angle)) * color * intensity;
	return ret;
}

GL::Lines Spot::to_gl() const {
	return Util::spotlight_mesh(color, inner_angle, outer_angle);
}

Spectrum Spot::display() const {
	return color;
}

}; // namespace Delta_Lights

bool operator!=(const Delta_Lights::Point& a, const Delta_Lights::Point& b) {
	return a.color != b.color || a.intensity != b.intensity;
}

bool operator!=(const Delta_Lights::Directional& a, const Delta_Lights::Directional& b) {
	return a.color != b.color || a.intensity != b.intensity;
}

bool operator!=(const Delta_Lights::Spot& a, const Delta_Lights::Spot& b) {
	return a.color != b.color || a.intensity != b.intensity || a.inner_angle != b.inner_angle ||
	       a.outer_angle != b.outer_angle;
}

bool operator!=(const Delta_Light& a, const Delta_Light& b) {
	if (a.light.index() != b.light.index()) return false;
	return std::visit(
		[&](const auto& light) {
			return light != std::get<std::decay_t<decltype(light)>>(b.light);
		},
		a.light);
}
