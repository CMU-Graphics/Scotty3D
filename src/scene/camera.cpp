
#include "camera.h"
#include "../gui/manager.h"
#include "../pathtracer/samplers.h"
#include "../test.h"

Mat4 Camera::projection() const {
	return Mat4::perspective(vertical_fov, aspect_ratio, near_plane);
}

GL::Lines Camera::to_gl() const {

	GL::Lines cage;

	float ap = near_plane * std::tan(Radians(vertical_fov) / 2.0f);
	float h = 2.0f * std::tan(Radians(vertical_fov) / 2.0f);
	float w = aspect_ratio * h;

	Vec3 tr = Vec3(0.5f * w, 0.5f * h, -1.0f);
	Vec3 tl = Vec3(-0.5f * w, 0.5f * h, -1.0f);
	Vec3 br = Vec3(0.5f * w, -0.5f * h, -1.0f);
	Vec3 bl = Vec3(-0.5f * w, -0.5f * h, -1.0f);

	Vec3 ftr = Vec3(0.5f * ap, 0.5f * ap, -near_plane);
	Vec3 ftl = Vec3(-0.5f * ap, 0.5f * ap, -near_plane);
	Vec3 fbr = Vec3(0.5f * ap, -0.5f * ap, -near_plane);
	Vec3 fbl = Vec3(-0.5f * ap, -0.5f * ap, -near_plane);

	cage.add(ftl, ftr, Gui::Color::black);
	cage.add(ftr, fbr, Gui::Color::black);
	cage.add(fbr, fbl, Gui::Color::black);
	cage.add(fbl, ftl, Gui::Color::black);

	cage.add(ftr, tr, Gui::Color::black);
	cage.add(ftl, tl, Gui::Color::black);
	cage.add(fbr, br, Gui::Color::black);
	cage.add(fbl, bl, Gui::Color::black);

	cage.add(bl, tl, Gui::Color::black);
	cage.add(tl, tr, Gui::Color::black);
	cage.add(tr, br, Gui::Color::black);
	cage.add(br, bl, Gui::Color::black);

	return cage;
}

bool operator!=(const Camera& a, const Camera& b) {
	return a.vertical_fov != b.vertical_fov || a.aspect_ratio != b.aspect_ratio || a.near_plane != b.near_plane
	       || a.film.width != b.film.width || a.film.height != b.film.height
	       || a.film.samples != b.film.samples || a.film.max_ray_depth != b.film.max_ray_depth
	       || a.film.sample_pattern != b.film.sample_pattern
	;
}
