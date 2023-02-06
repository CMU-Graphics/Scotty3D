#include "test.h"
#include "scene/transform.h"

#include <limits>
#include <iomanip>
#include <algorithm>

//a basic transform hierarchy used in the following tests:
struct TestHierarchy {
	//A is the parent of B is the parent of C.
	std::shared_ptr< Transform > A, B, C;
	TestHierarchy() {
		A = std::make_shared< Transform >(
			Vec3{1.0f, 0.0f, 0.0f}, //translate by (1,0,0)
			Vec3{0.0f, 0.0f, 0.0f}, //no rotation
			Vec3{2.0f, 2.0f, 2.0f}  //scale by 2x
		);
		B = std::make_shared< Transform >(
			Vec3{0.0f, 1.0f, 0.0f}, //translate by (0,1,0)
			Vec3{0.0f, 0.0f, 0.0f}, //no rotation
			Vec3{2.0f, 2.0f, 2.0f}  //scale by 2x
		);
		C = std::make_shared< Transform >(
			Vec3{0.0f, 0.0f, 1.0f}, //translate by (0,0,1)
			Vec3{0.0f, 0.0f, 0.0f}, //no rotation
			Vec3{2.0f, 2.0f, 2.0f}  //scale by 2x
		);
		B->parent = A;
		C->parent = B;
	}
};

Test test_a1_task1_local_to_world_no_parent("a1.task1.local_to_world.no_parent", []() {
	TestHierarchy hierarchy;

	Mat4 expected = hierarchy.A->local_to_parent();
	Mat4 mat = hierarchy.A->local_to_world();

	if (Test::differs(mat, expected)) {
		info("Transform A's local_to_world:");
		Test::print_matrix(mat);
		info("Transform A's local_to_parent:");
		Test::print_matrix(expected);
		throw Test::error("Transform without parent's local_to_world doesn't match local_to_parent.");
	}
});

Test test_a1_task1_world_to_local_no_parent("a1.task1.world_to_local.no_parent", []() {
	TestHierarchy hierarchy;

	Mat4 expected = hierarchy.A->parent_to_local();
	Mat4 mat = hierarchy.A->world_to_local();

	if (Test::differs(mat, expected)) {
		info("Transform A's world_to_local:");
		Test::print_matrix(mat);
		info("Transform A's parent_to_local:");
		Test::print_matrix(expected);
		throw Test::error("Transform without parent's world_to_local doesn't match parent_to_local.");
	}
});


Test test_a1_task1_local_to_world("a1.task1.local_to_world", []() {
	TestHierarchy hierarchy;

	Mat4 got = hierarchy.C->local_to_world();
	Mat4 expected = Mat4{
		8.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 8.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 8.0f, 0.0f,
		1.0f, 2.0f, 4.0f, 1.0f
	};

	if (Test::differs(got, expected)) {
		info("Transform C's local_to_world:");
		Test::print_matrix(got);
		info("Expected:");
		Test::print_matrix(expected);
		throw Test::error("Transform's local_to_world doesn't match expected.");
	}
});

Test test_a1_task1_world_to_local("a1.task1.world_to_local", []() {
	TestHierarchy hierarchy;

	Mat4 got = hierarchy.C->world_to_local();
	Mat4 expected = Mat4{
		0.125f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.125f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.125f, 0.0f,
		-0.125f,-0.25f,-0.5f, 1.0f
	};

	if (Test::differs(got, expected)) {
		info("Transform C's world_to_local:");
		Test::print_matrix(got);
		info("Expected:");
		Test::print_matrix(expected);
		throw Test::error("Transform's world_to_local doesn't match expected.");
	}
});


