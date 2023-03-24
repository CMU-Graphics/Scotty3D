#include "test.h"
#include "geometry/spline.h"

Spline<Vec3> initialize() {
	Spline<Vec3> test_spline;
	test_spline.set(1.0f, Vec3(1.0f, 0.0f, 0.0f));
	test_spline.set(2.0f, Vec3(2.0f, 0.0f, 0.0f));
	test_spline.set(3.0f, Vec3(3.0f, 0.0f, 0.0f));
	test_spline.set(4.0f, Vec3(4.0f, 0.0f, 0.0f));
	test_spline.set(5.0f, Vec3(5.0f, 0.0f, 0.0f));
	return test_spline;
}

Test test_a4_task1_spline_at_simple("a4.task1.spline.at.simple", []() {
	auto test_spline = initialize();
	Vec3 actual;
	for (float t : test_spline.keys()) {
		actual = test_spline.at(t);
		if (Test::differs(actual, test_spline.knots[t])) {
			throw Test::error("Incorrect return value at the time of some knot!");
		}
	}
	Vec3 expected = Vec3(1.5f, 0.0f, 0.0f);
	actual = test_spline.at(1.5f);
	if (Test::differs(actual, expected)) {
		throw Test::error("Incorrect return value between the time of two knots!");
	}
	expected = Vec3(2.5f, 0.0f, 0.0f);
	actual = test_spline.at(2.5f);
	if (Test::differs(actual, expected)) {
		throw Test::error("Incorrect return value between the time of two knots!");
	}
});

Test test_a4_task1_spline_cubic_unit_simple("a4.task1.spline.cubic_unit.simple", []() {
	{
		float time = 0.1f;
		Vec3 position0 = Vec3(0.0f, 0.0f, 0.0f);
		Vec3 position1 = Vec3(1.0f, 0.0f, 0.0f);
		Vec3 tangent0 = Vec3(1.0f, 0.0f, 0.0f);
		Vec3 tangent1 = Vec3(-1.0f, 0.0f, 0.0f);
		Vec3 expected = Vec3(0.118f, 0.0f, 0.0f);
		Vec3 actual = Spline<Vec3>::cubic_unit_spline(time, position0, position1, tangent0, tangent1);
		if (Test::differs(actual, expected)) {
			throw Test::error("Incorrect result at time " + std::to_string(time) + "!");
		}
	}
	{
		float time = 0.9f;
		Vec3 position0 = Vec3(0.0f, 0.0f, 0.0f);
		Vec3 position1 = Vec3(1.0f, 0.0f, 0.0f);
		Vec3 tangent0 = Vec3(1.0f, 0.0f, 0.0f);
		Vec3 tangent1 = Vec3(-1.0f, 0.0f, 0.0f);
		Vec3 expected = Vec3(1.062f, 0.0f, 0.0f);
		Vec3 actual = Spline<Vec3>::cubic_unit_spline(time, position0, position1, tangent0, tangent1);
		if (Test::differs(actual, expected)) {
			throw Test::error("Incorrect result at time " + std::to_string(time) + "!");
		}
	}
	{
		float time = 0.5f;
		Vec3 position0 = Vec3(0.0f, 0.0f, 0.0f);
		Vec3 position1 = Vec3(1.0f, 0.0f, 0.0f);
		Vec3 tangent0 = Vec3(1.0f, 0.0f, 0.0f);
		Vec3 tangent1 = Vec3(-1.0f, 0.0f, 0.0f);
		Vec3 expected = Vec3(0.75, 0.0f, 0.0f);
		Vec3 actual = Spline<Vec3>::cubic_unit_spline(time, position0, position1, tangent0, tangent1);
		if (Test::differs(actual, expected)) {
			throw Test::error("Incorrect result at time " + std::to_string(time) + "!");
		}
	}
});
