
#pragma once

#include "../geometry/halfedge.h"
#include "../geometry/indexed.h"
#include "../lib/mathlib.h"
#include "introspect.h"

#include <functional>
#include <memory>

class Skeleton {
public:
	using BoneIndex = uint32_t;
	using HandleIndex = uint32_t;

	//bones with no parent (parent == -1U) begin at base:
	// (this is to avoid awkward bones from (0,0,0) to the natural root of the associated mesh)
	Vec3 base = Vec3(0,0,0); //position of base in the bind pose
	Vec3 base_offset = Vec3(0,0,0); //base in the current pose is base + base_offset

	//bones define the transformation hierarchy of the skeleton:
	struct Bone {
		//bone's shape and local rotation axes are given by 'extent' and 'roll':
		Vec3 extent = Vec3(0,0,0); //where child bones start, also direction of local 'y' axis
		float roll = 0.0f; //direction of local 'x' axis relative to global x axis (angle in degrees)
		
		float radius = 0.5f; //radius of capsule drawn to represent bone (also influences automatic skinning weights)

		//index of parent. (-1U if no parent)
		// bone is attached to the tip of the parent bone
		BoneIndex parent = -1U;

		//bone's current pose is given by an x,y,z euler rotation of its bind pose:
		// (angles in degrees)
		Vec3 pose = Vec3(0,0,0);

		//the rotation axes for 'pose' are:
		//x: the direction pointing toward the skeleton's x axis, perpendicular to extent
		//y: the direction of 'extent'
		//z: the direction perpendicular to x and y
		// (x,z are also rotated ccw around y by 'roll')
		void compute_rotation_axes(Vec3 *x, Vec3 *y, Vec3 *z) const;
		//NOTE: these are *just* the axes for pose's rotation -- they otherwise do not influence child bones (or skinned vertices)

		//these are unique for every bone so that deleting a bone doesn't screw up animation channels:
		uint32_t channel_id = 0;

		//- - - - - - - - - - - -
		template< Intent I, typename F, typename T >
		static void introspect(F&& f, T&& t) {
			if constexpr (I != Intent::Animate) f("extent", t.extent);
			if constexpr (I != Intent::Animate) f("roll", t.roll);
			if constexpr (I != Intent::Animate) f("radius", t.radius);
			if constexpr (I != Intent::Animate) f("parent", t.parent);
			f("pose", t.pose);
			if constexpr (I != Intent::Animate) f("channel_id", t.channel_id);
		}
		static inline const char *TYPE = "Bone";
	};
	std::vector< Bone > bones;
	//NOTE: care is taken to keep bones topologically sorted (children appear after parents)

	// --- forward kinematics ---

	//get the bind pose (the pose without bone rotations) for the skeleton:
	std::vector< Mat4 > bind_pose() const;

	//get the current pose (the pose *with* bone rotations) for the skeleton:
	std::vector< Mat4 > current_pose() const;

	// --- inverse kinematics ---

	//ik handles specify targets which can be used to drive the skeleton's pose:
	struct Handle {
		BoneIndex bone = -1U; //handle applies to the tip of this bone
		Vec3 target = Vec3(0.0f, 0.0f, 0.0f); //handle tries to move tip of the bone to this position
		bool enabled = false;

		//these are unique for every bone so that deleting a bone doesn't screw up animation channels:
		uint32_t channel_id = 0;

		//- - - - - - - - - - - -
		template< Intent I, typename F, typename T >
		static void introspect(F&& f, T&& t) {
			if constexpr (I != Intent::Animate) f("bone", t.bone);
			f("target", t.target);
			f("enabled", t.enabled);
			if constexpr (I != Intent::Animate) f("channel_id", t.channel_id);
		}
		static inline const char *TYPE = "Handle";
	};
	std::vector< Handle > handles;

	//first derivative (with respect to Bone::pose) of the function
	// which measures squared distance from each enabled ik handle's target to the tip of its bone.
	std::vector< Vec3 > gradient_in_current_pose() const;

	//move skeleton toward ik handles by gradient descent:
	// terminate after either `steps` steps (returns false)
	// or after converging to a solution (returns true)
	bool solve_ik(uint32_t steps = 10);

	//assign Vertex::bone weights on halfedge mesh:
	// vertices are assigned weights for every bone for which they are closer than bone.radius (in the bind pose)
	// weights are proportional to (radius - distance-to-bone) / radius
	// weights are normalized to sum to 1
	void assign_bone_weights(Halfedge_Mesh *mesh) const;

	//return the closest point on line segment a-b to point p:
	// (a helper used by assign_bone_weights)
	static Vec3 closest_point_on_line_segment(Vec3 const &a, Vec3 const &b, Vec3 const &p);

	//Compute linear-blend-skinning vertex positions for a Halfedge_Mesh using the bone weights recorded on each vertex.
	// applies the transform between the 'bind' and 'current' parameters
	// vertices with empty bone weights are not moved
	// outputs an Indexed_Mesh with split normals
	static Indexed_Mesh skin(Halfedge_Mesh const &mesh, std::vector< Mat4 > const &bind, std::vector< Mat4 > const &current);

	//helpers for the UI:

	//erase bone/handle (re-index/re-sort as needed):
	void erase_bone(BoneIndex bone);
	void erase_handle(HandleIndex handle);

	//add bone/handle, return index:
	BoneIndex add_bone(BoneIndex parent, Vec3 extent); //for bone with no parent, pass -1U for first param
	HandleIndex add_handle(BoneIndex bone, Vec3 target);

	uint32_t next_bone_channel_id = 0;
	uint32_t next_handle_channel_id = 0;

	Skeleton copy();

	void for_bones(const std::function<void(Bone&)>& f);

	//project this to a valid thing, possibly producing some warnings.
	// (throw if it cannot be made valid)
	void make_valid();

	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("bones", t.bones);
		f("handles", t.handles);
		if constexpr (I != Intent::Animate) f("base", t.base);
		f("base_offset", t.base_offset);

		if constexpr (I == Intent::Write) t.make_valid();
	}
	static inline const char *TYPE = "Skeleton";
};

class Skinned_Mesh {
public:
	Halfedge_Mesh mesh;
	Skeleton skeleton;

	Skinned_Mesh copy();

	Indexed_Mesh bind_mesh() const;
	Indexed_Mesh posed_mesh() const;

	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("mesh", t.mesh);
		f("skeleton", t.skeleton);
	}
	static inline const char *TYPE = "Skinned_Mesh";
};
