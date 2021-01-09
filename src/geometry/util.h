
#pragma once

#include "../platform/gl.h"

#include <string>

namespace Util {

GL::Mesh cube_mesh(float radius);
GL::Mesh square_mesh(float radius);
GL::Mesh quad_mesh(float x, float y);
GL::Mesh cyl_mesh(float radius, float height, int sides = 12, bool cap = true);
GL::Mesh torus_mesh(float iradius, float oradius, int segments = 48, int sides = 24);
GL::Mesh sphere_mesh(float r, int subdivsions);
GL::Mesh hemi_mesh(float r);
GL::Mesh cone_mesh(float bradius, float tradius, float height, int sides = 12, bool cap = true);
GL::Mesh capsule_mesh(float h, float r);

GL::Mesh arrow_mesh(float base, float tip, float height);
GL::Mesh scale_mesh();

GL::Lines spotlight_mesh(Vec3 color, float inner, float outer);

namespace Gen {

struct Data {
    std::vector<GL::Mesh::Vert> verts;
    std::vector<GL::Mesh::Index> elems;
};
struct LData {
    std::vector<GL::Lines::Vert> verts;
};

GL::Mesh merge(Data&& l, Data&& r);
LData merge(LData&& l, LData&& r);
LData circle(Vec3 color, float r, int sides);
GL::Mesh dedup(Data&& d);

// https://wiki.unity3d.com/index.php/ProceduralPrimitives
Data cube(float r);
Data quad(float x, float y);
Data ico_sphere(float radius, int level);
Data uv_hemisphere(float radius);
Data cone(float bradius, float tradius, float height, int sides, bool caps);
Data torus(float iradius, float oradius, int segments, int sides);

} // namespace Gen
} // namespace Util
