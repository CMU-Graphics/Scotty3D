
#pragma once

#include "../geometry/halfedge.h"
#include "../geometry/indexed.h"
#include "../lib/mathlib.h"

#include <functional>
#include <memory>

class Bone : public std::enable_shared_from_this<Bone> {
	public:
		std::weak_ptr<Bone> add_child(Vec3 extent, Vec3 pose = {});

		Vec3 extent, pose;
		Vec3 pose_gradient = Vec3(0.0f, 0.0f, 0.0f);
		float radius = 0.5f;

		std::weak_ptr<Bone> parent;
		std::vector<std::shared_ptr<Bone>> children;
	};


class Skeleton {
public:
	class IK_Handle {
	public:
		std::weak_ptr<Bone> bone;
		Vec3 target = Vec3(0.0f, 0.0f, 0.0f);
		bool enabled = false;
	};

	Mat4 j_to_bind(std::weak_ptr<Bone const> bone) const;
	Mat4 j_to_posed(std::weak_ptr<Bone const> bone) const;

	Vec3 end_of(std::weak_ptr<Bone const> bone) const;
	Vec3 end_of_posed(std::weak_ptr<Bone const> bone) const;

	bool run_ik(uint32_t steps);
	void compute_gradients(std::weak_ptr<Bone> bone, Vec3 end_effector, Vec3 ik_target);

	std::vector<std::vector<std::weak_ptr<Bone const>>> find_bones(const Indexed_Mesh& mesh) const;
	Indexed_Mesh skin(const Indexed_Mesh& mesh,
	                  const std::vector<std::vector<std::weak_ptr<Bone const>>>& bone_map) const;

	std::weak_ptr<Bone> add_root(Vec3 extent, Vec3 pose = {});
	std::weak_ptr<IK_Handle> add_handle(std::weak_ptr<Bone> bone, Vec3 target);

	void clear();
	void erase(std::weak_ptr<Bone> bone);
	void erase(std::weak_ptr<IK_Handle> handle);
	Skeleton copy();

	static Vec3 closest_on_line_segment(Vec3 start, Vec3 end, Vec3 point);

	Vec3 base = Vec3(0.0f, 0.0f, 0.0f);
	std::vector<std::shared_ptr<Bone>> roots;
	std::vector<std::shared_ptr<IK_Handle>> handles;

	void for_bones(const std::function<void(std::shared_ptr<Bone>)>& f);
private:
	void for_bones(const std::function<void(Bone&)>& f);
	void for_bones(const std::function<void(Bone const &)>& f) const;
	void for_bones(const std::shared_ptr<Bone>& j, const std::function<void(Bone&)>& f);
	void for_bones(const std::shared_ptr<Bone const>& j, const std::function<void(Bone const &)>& f) const;
	void for_bones(const std::shared_ptr<Bone>& j, const std::function<void(std::shared_ptr<Bone>)>& f);
};

class Skinned_Mesh {
public:
	// TODO: mesh should store bone weights
	Halfedge_Mesh mesh;
	Skeleton skeleton;

	mutable std::vector<std::vector<std::weak_ptr<Bone const>>> bone_cache;

	Skinned_Mesh copy();

	Indexed_Mesh bind_mesh() const;
	Indexed_Mesh posed_mesh() const;
};
