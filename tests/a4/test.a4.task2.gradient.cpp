#include "test.h"
#include "scene/skeleton.h"

inline std::vector<Skeleton::BoneIndex> setup_skeleton_gradient(Skeleton& skeleton, bool pose) {
	skeleton.bones.clear();

	auto root   = 	skeleton.add_bone(-1U, 		Vec3(1.0f, 0.0f, 0.0f));
	auto child1 = 	skeleton.add_bone(root, 	Vec3(0.0f, 1.0f, 0.0f));

	if (pose) {
        skeleton.bones[root].pose = Vec3(90.0f, 0.0f, 0.0f);
        skeleton.bones[child1].pose = Vec3(0.0f, 90.0f, 0.0f);
	}

	skeleton.base = Vec3(0.0f, 0.0f, 1.0f);

	return {root, child1};
}

Test a4_task2_gradient_single_handle_pose("a4.task2.gradient.single_handle.pose", []() {
	Skeleton skeleton;
	auto j = setup_skeleton_gradient(skeleton, true);
	auto handle1 = skeleton.add_handle(j[1], Vec3(0.0f, 0.0f, 1.0f));
    skeleton.handles[handle1].enabled = true;
	auto grads = skeleton.gradient_in_current_pose();
	if (Test::differs(grads[j[0]], Vec3(0.0f, 0.0f, 0.0f))) {
		throw Test::error("Wrong gradient computed at the root bone!");
	}
	if (Test::differs(grads[j[1]].unit(), Vec3(1.0f, 0.0f, -1.0f).unit())) {
		throw Test::error("Wrong gradient computed at the first child bone!");
	}
});

Test a4_task2_gradient_single_handle_bind("a4.task2.gradient.single_handle.bind", []() {
	Skeleton skeleton;
	auto j = setup_skeleton_gradient(skeleton, false);
	auto handle1 = skeleton.add_handle(j[1], Vec3(0.0f, 0.0f, 1.0f));
    skeleton.handles[handle1].enabled = true;
	auto grads = skeleton.gradient_in_current_pose();
	if (Test::differs(grads[j[0]], Vec3(0.0f, 0.0f, 0.0f))) {
		throw Test::error("Wrong gradient computed at the root bone!");
	}
	if (Test::differs(grads[j[1]].unit(), Vec3(0.0f, 0.0f, -1.0f).unit())) {
		throw Test::error("Wrong gradient computed at the first child bone!");
	}
});

Test a4_task2_gradient_no_handle("a4.task2.gradient.no_handle", []() {
	Skeleton skeleton;
	auto j = setup_skeleton_gradient(skeleton, false);
	auto handle1 = skeleton.add_handle(j[1], Vec3(0.0f, 0.0f, 1.0f));
    skeleton.handles[handle1].enabled = false;
	auto grads = skeleton.gradient_in_current_pose();
	if (Test::differs(grads[j[0]], Vec3(0.0f, 0.0f, 0.0f))) {
		throw Test::error("Wrong gradient computed at the root bone!");
	}
	if (Test::differs(grads[j[1]], Vec3(0.0f, 0.0f, 0.0f))) {
		throw Test::error("Wrong gradient computed at the first child bone!");
	}
});
