#include "test.h"
#include "scene/skeleton.h"

Test test_a4_task3_closest_point_on_line_segment_same_direction_in_between(
	"a4.task3.closest_point_on_line_segment.same_direction.in_between", []() {
		Vec3 start = Vec3(1, 1, 1);
		Vec3 end = Vec3(2, 2, 2);
		Vec3 point = Vec3(1.5f, 1.5f, 1.5f);

		Vec3 expected = Vec3(1.5f, 1.5f, 1.5f);
		Vec3 actual = Skeleton::closest_point_on_line_segment(start, end, point);
		if (Test::differs(expected, actual)) {
			throw Test::error("The closest point should be the point itself!");
		}
	});

Test test_a4_task3_closest_point_on_line_segment_same_direction_before_start(
	"a4.task3.closest_point_on_line_segment.same_direction.before_start", []() {
		Vec3 start = Vec3(1, 1, 1);
		Vec3 end = Vec3(2, 2, 2);
		Vec3 point = Vec3(0.5f, 0.5f, 0.5f);

		Vec3 expected = Vec3(1, 1, 1);
		Vec3 actual = Skeleton::closest_point_on_line_segment(start, end, point);
		if (Test::differs(expected, actual)) {
			throw Test::error("The closest point should be the start point!");
		}
	});

Test test_a4_task3_closest_point_on_line_segment_same_direction_after_end(
	"a4.task3.closest_point_on_line_segment.same_direction.after_end", []() {
		Vec3 start = Vec3(1, 1, 1);
		Vec3 end = Vec3(2, 2, 2);
		Vec3 point = Vec3(3.5f, 3.5f, 3.5f);

		Vec3 expected = Vec3(2, 2, 2);
		Vec3 actual = Skeleton::closest_point_on_line_segment(start, end, point);
		if (Test::differs(expected, actual)) {
			throw Test::error("The closest point should be the end point!");
		}
	});
