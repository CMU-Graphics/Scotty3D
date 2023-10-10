
#pragma once

#include "../platform/gl.h"
#include "indexed.h"

#include <string>

namespace Util {

enum class Shape : uint8_t {
	cube,
	square,
	cylinder,
	torus,
	closed_sphere,
	texture_sphere,
	hemisphere,
	cone,
	capsule,
	arrow,
	scale,
	pentagon
};

Indexed_Mesh cube_mesh(float radius);
Indexed_Mesh square_mesh(float radius);
Indexed_Mesh quad_mesh(float x, float y);
Indexed_Mesh pentagon_mesh(float radius);
Indexed_Mesh cyl_mesh(float radius, float height, uint32_t sides = 12, bool cap = true);
Indexed_Mesh cyl_mesh_disjoint(float radius, float height, uint32_t sides = 12);
Indexed_Mesh torus_mesh(float iradius, float oradius, uint32_t segments = 48, uint32_t sides = 24);
Indexed_Mesh texture_sphere_mesh(float r, uint32_t subdivsions);
Indexed_Mesh closed_sphere_mesh(float r, uint32_t subdivsions);
Indexed_Mesh hemi_mesh(float r);
Indexed_Mesh cone_mesh(float bradius, float tradius, float height, uint32_t sides = 12,
                       bool cap = true);
Indexed_Mesh capsule_mesh(float h, float r);

Indexed_Mesh arrow_mesh(float base, float tip, float height);
Indexed_Mesh scale_mesh();

GL::Lines spotlight_mesh(Spectrum color, float inner, float outer);

namespace Gen {

struct Data {
	std::vector<Indexed_Mesh::Vert> verts;
	std::vector<Indexed_Mesh::Index> elems;
};
struct LData {
	std::vector<GL::Lines::Vert> verts;
};

Indexed_Mesh merge(Data&& l, Data&& r);
LData merge(LData&& l, LData&& r);
LData circle(Spectrum color, float r, uint32_t sides);
Indexed_Mesh dedup(Data&& d);

// https://wiki.unity3d.com/index.php/ProceduralPrimitives
Data cube(float r);
Data quad(float x, float y);
Data pentagon(float r);
Data texture_ico_sphere(float radius, uint32_t level);
Data closed_ico_sphere(float radius, uint32_t level);
Data uv_hemisphere(float radius);
Data cone(float bradius, float tradius, float height, uint32_t sides, bool caps);
Data torus(float iradius, float oradius, uint32_t segments, uint32_t sides);

} // namespace Gen
} // namespace Util
