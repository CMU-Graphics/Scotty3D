
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

namespace Instance {

class Geometry_Settings {
public:
	bool visible = true;
	DrawStyle style = DrawStyle::Correct;
};

class Light_Settings {
public:
	bool visible = true;
};

class Simulate_Settings {
public:
	bool visible = true;
	bool wireframe = false;
	bool simulate_here = false;
};

class Mesh {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<Halfedge_Mesh> mesh;
	std::weak_ptr<Material> material;
	Geometry_Settings settings;
};

class Skinned_Mesh {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Skinned_Mesh> mesh;
	std::weak_ptr<Material> material;
	Geometry_Settings settings;
};

class Shape {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Shape> shape;
	std::weak_ptr<Material> material;
	Geometry_Settings settings;
};

class Delta_Light {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Delta_Light> light;
	Light_Settings settings;
};

class Environment_Light {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Environment_Light> light;
	Light_Settings settings;
};

class Particles {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<Halfedge_Mesh> mesh;
	std::weak_ptr<Material> material;
	std::weak_ptr<::Particles> particles;
	Simulate_Settings settings;
};

class Camera {
public:
	std::weak_ptr<Transform> transform;
	std::weak_ptr<::Camera> camera;
};

} // namespace Instance
