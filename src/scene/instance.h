
#pragma once

#include <memory>
#include <variant>

#include "../geometry/halfedge.h"
#include "../lib/mathlib.h"

#include "camera.h"
#include "delta_light.h"
#include "env_light.h"
#include "material.h"
#include "particles.h"
#include "shape.h"
#include "skeleton.h"
#include "transform.h"

enum class DrawStyle : uint8_t {
	Wireframe, //lines at edges
	Flat,      //triangles with attributes from first vertex
	Smooth,    //triangles with attributes interpolated on-screen
	Correct,   //triangles with attributes interpolated in 3D
};

enum class BlendStyle : uint8_t {
	Replace,   //replace color
	Add,       //add colors
	Over,      //blend colors
};

enum class DepthStyle : uint8_t {
	Always,    //always take latest fragment
	Never, 	   //never take latest fragment
	Less,	   //take fragments closer to the camera
};

namespace Instance {

class Geometry_Settings {
public:
	bool visible = true;
	bool collides = true;
	DrawStyle draw_style = DrawStyle::Correct;
	BlendStyle blend_style = BlendStyle::Replace;
	DepthStyle depth_style = DepthStyle::Less;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("visible", t.visible);
		f("collides", t.collides);
		introspect_enum< I >(f, "draw style", t.draw_style, std::vector< std::pair< const char *, DrawStyle > >{
			{"Wireframe", DrawStyle::Wireframe},
			{"Flat", DrawStyle::Flat},
			{"Smooth", DrawStyle::Smooth},
			{"Correct", DrawStyle::Correct}
		});
		introspect_enum< I >(f, "blend style", t.blend_style, std::vector< std::pair< const char *, BlendStyle > >{
			{"Blend Replace", BlendStyle::Replace},
			{"Blend Over", BlendStyle::Over},
			{"Blend Add", BlendStyle::Add}
		});
		introspect_enum< I >(f, "depth style", t.depth_style, std::vector< std::pair< const char *, DepthStyle > >{
			{"Depth Always", DepthStyle::Always},
			{"Depth Never", DepthStyle::Never},
			{"Depth Less", DepthStyle::Less}
		});
	}
	static inline const char *TYPE = "Instance::Geometry_Settings";
};

class Light_Settings {
public:
	bool visible = true;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("visible", t.visible);
	}
	static inline const char *TYPE = "Instance::Light_Settings";
};

class Simulate_Settings {
public:
	bool visible = true;
	bool wireframe = false;
	bool simulate_here = false;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		f("visible", t.visible);
		f("wireframe", t.wireframe);
		f("simulate_here", t.simulate_here);
	}
	static inline const char *TYPE = "Instance::Simulate_Settings";
};

class Mesh {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<Halfedge_Mesh> mesh;
	std::weak_ptr<Material> material;
	Geometry_Settings settings;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("mesh", t.mesh);
		if constexpr (I != Intent::Animate) f("material", t.material);
		Geometry_Settings::introspect< I >(std::forward< F >(f), t.settings);
	}
	static inline const char *TYPE = "Instance::Mesh";
};

class Skinned_Mesh {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Skinned_Mesh> mesh;
	std::weak_ptr<Material> material;
	Geometry_Settings settings;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("mesh", t.mesh);
		if constexpr (I != Intent::Animate) f("material", t.material);
		Geometry_Settings::introspect< I >(std::forward< F >(f), t.settings);
	}
	static inline const char *TYPE = "Instance::Skinned_Mesh";
};

class Shape {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Shape> shape;
	std::weak_ptr<Material> material;
	Geometry_Settings settings;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("shape", t.shape);
		if constexpr (I != Intent::Animate) f("material", t.material);
		Geometry_Settings::introspect< I >(std::forward< F >(f), t.settings);
	}
	static inline const char *TYPE = "Instance::Shape";
};

class Delta_Light {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Delta_Light> light;
	Light_Settings settings;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("light", t.light);
		Light_Settings::introspect< I >(std::forward< F >(f), t.settings);
	}
	static inline const char *TYPE = "Instance::Delta_Light";
};

class Environment_Light {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Environment_Light> light;
	Light_Settings settings;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("light", t.light);
		Light_Settings::introspect< I >(std::forward< F >(f), t.settings);
	}
	static inline const char *TYPE = "Instance::Environment_Light";
};

class Particles {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<Halfedge_Mesh> mesh;
	std::weak_ptr<Material> material;
	std::weak_ptr<::Particles> particles;
	Simulate_Settings settings;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("mesh", t.mesh);
		if constexpr (I != Intent::Animate) f("material", t.material);
		if constexpr (I != Intent::Animate) f("particles", t.particles);
		Simulate_Settings::introspect< I >(std::forward< F >(f), t.settings);
	}
	static inline const char *TYPE = "Instance::Particles";
};

class Camera {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Camera> camera;

	//- - - - - - - - - - - -
	template< Intent I, typename F, typename T >
	static void introspect(F&& f, T&& t) {
		if constexpr (I != Intent::Animate) f("transform", t.transform);
		if constexpr (I != Intent::Animate) f("camera", t.camera);
	}
	static inline const char *TYPE = "Instance::Camera";
};

} // namespace Instance
