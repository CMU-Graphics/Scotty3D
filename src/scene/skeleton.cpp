
#include <unordered_set>
#include "skeleton.h"

std::weak_ptr<Bone> Bone::add_child(Vec3 e, Vec3 p) {
	auto child = std::make_shared<Bone>();
	child->extent = e;
	child->pose = p;
	child->parent = weak_from_this();
	children.push_back(child);
	return child;
}

Mat4 Skeleton::j_to_bind(std::weak_ptr<Bone const> bone) const {

	// TODO(Animation): Task 2

    return Mat4::I;
}

Mat4 Skeleton::j_to_posed(std::weak_ptr<Bone const> bone) const {

    // TODO(Animation): Task 2

    return Mat4::I;
}

Vec3 Skeleton::end_of(std::weak_ptr<Bone const> bone) const {

    // TODO(Animation): Task 2

    return Vec3{};
}

Vec3 Skeleton::end_of_posed(std::weak_ptr<Bone const> bone) const {
	// TODO(Animation): Task 2

    return Vec3{};
}

std::weak_ptr<Bone> Skeleton::add_root(Vec3 extent, Vec3 pose) {
	auto root = std::make_shared<Bone>();
	root->extent = extent;
	root->pose = pose;
	roots.push_back(root);
	return root;
}

std::weak_ptr<Skeleton::IK_Handle> Skeleton::add_handle(std::weak_ptr<Bone> bone,
                                                        Vec3 target) {
	auto handle = std::make_shared<IK_Handle>();
	handle->bone = bone;
	handle->target = target;
	handles.push_back(handle);
	return handle;
}

void Skeleton::erase(std::weak_ptr<Bone> bone) {
	if (bone.expired()) return;
	auto b = bone.lock();
	
	std::unordered_set<std::shared_ptr<Bone>> removing;
	std::function<void(std::shared_ptr<Bone>)> gather = [&](std::shared_ptr<Bone> b) {
		removing.insert(b);
		for (auto& child : b->children) {
			gather(child);
		}
	};
	gather(b);

	if(b->parent.expired()) {
		roots.erase(std::remove(roots.begin(), roots.end(), b), roots.end());
	} else {
		auto p = b->parent.lock();
		p->children.erase(std::remove(p->children.begin(), p->children.end(), b), p->children.end());
	} 

	handles.erase(std::remove_if(handles.begin(), handles.end(), [&](const std::shared_ptr<IK_Handle>& h) {
		return removing.count(h->bone.lock()) > 0;
	}), handles.end());
}

void Skeleton::erase(std::weak_ptr<IK_Handle> handle) {
	if (handle.expired()) return;
	handles.erase(std::remove(handles.begin(), handles.end(), handle.lock()), handles.end());
}

void Skeleton::clear() {
	roots.clear();
}

void Skeleton::compute_gradients(std::weak_ptr<Bone> bone, Vec3 end_effector,
                                 Vec3 ik_target) {
    // TODO(Animation): Task 2

    // Computes the gradient of IK energy for this joint and, should be called
    // recursively upward in the heirarchy. Each call should storing the result
    // in the angle_gradient for this joint.

    // Target is the target position of the IK handle in skeleton space.
    // Current is the end position of the IK'd joint in skeleton space.
}

bool Skeleton::run_ik(uint32_t steps) {

	// TODO(Animation): Task 2

    // Do several iterations of Jacobian Transpose gradient descent for IK
	return false;
}

std::vector<std::vector<std::weak_ptr<Bone const>>>
Skeleton::find_bones(const Indexed_Mesh& mesh) const {

	// TODO(Animation): Task 3
	const auto& verts = mesh.vertices();

	std::vector<std::vector<std::weak_ptr<Bone const>>> map(verts.size());
	return map;
}

Indexed_Mesh
Skeleton::skin(const Indexed_Mesh& mesh,
               const std::vector<std::vector<std::weak_ptr<Bone const>>>& bone_map) const {
	// TODO(Animation): Task 3
	auto verts = mesh.vertices();
	auto idxs = mesh.indices();

	return Indexed_Mesh(std::move(verts), std::move(idxs));
}

void Skeleton::for_bones(const std::function<void(Bone&)>& f) {
	for (auto& root : roots) {
		for_bones(root, f);
	}
}


void Skeleton::for_bones(const std::function<void(Bone const &)>& f) const {
	for (auto& root : roots) {
		for_bones(root, f);
	}
}


void Skeleton::for_bones(const std::shared_ptr<Bone>& j, const std::function<void(Bone&)>& f) {
	f(*j);
	for (auto& child : j->children) {
		for_bones(child, f);
	}
}

void Skeleton::for_bones(const std::shared_ptr<Bone const>& j, const std::function<void(Bone const &)>& f) const {
	f(*j);
	for (auto& child : j->children) {
		for_bones(child, f);
	}
}


void Skeleton::for_bones(const std::function<void(std::shared_ptr<Bone>)>& f) {
	for (auto& root : roots) {
		for_bones(root, f);
	}
}

void Skeleton::for_bones(const std::shared_ptr<Bone>& j,
                         const std::function<void(std::shared_ptr<Bone>)>& f) {
	f(j);
	for (auto& child : j->children) {
		for_bones(child, f);
	}
}

Skeleton Skeleton::copy() {
	Skeleton copy;
	std::unordered_map<std::shared_ptr<Bone>, std::shared_ptr<Bone>> bone_map;
	std::function<std::shared_ptr<Bone>(const std::shared_ptr<Bone>&)> copy_bone;
	copy_bone = [&](const std::shared_ptr<Bone>& j) {
		auto c = std::make_shared<Bone>(*j);
		c->children.clear();
		bone_map[j] = c;
		for (auto& child : j->children) {
			auto cc = copy_bone(child);
			cc->parent = c;
			c->children.push_back(cc);
		}
		return c;
	};
	for (auto& root : roots) {
		copy.roots.push_back(copy_bone(root));
	}
	for (auto& handle : handles) {
		auto c = std::make_shared<IK_Handle>(*handle);
		c->bone = bone_map[handle->bone.lock()];
		copy.handles.push_back(c);
	}
	copy.base = base;
	return copy;
}

Vec3 Skeleton::closest_on_line_segment(Vec3 start, Vec3 end, Vec3 point) {
	// TODO(Animation): Task 3

    // Return the closest point to 'point' on the line segment from start to end
    return Vec3{};
}

// TODO: these should probably return halfedge meshes:
Indexed_Mesh Skinned_Mesh::bind_mesh() const {
	return Indexed_Mesh::from_halfedge_mesh(mesh, Indexed_Mesh::SplitEdges);
}

Indexed_Mesh Skinned_Mesh::posed_mesh() const {
	auto bind = bind_mesh();
	if (bone_cache.size() != bind.vertices().size()) {
		bone_cache = skeleton.find_bones(bind);
	}
	return skeleton.skin(bind, bone_cache);
}

Skinned_Mesh Skinned_Mesh::copy() {
	return Skinned_Mesh{mesh.copy(), skeleton.copy(), {}};
}
