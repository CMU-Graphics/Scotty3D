
#include "../rays/tri_mesh.h"
#include "debug.h"

namespace PT {

BBox Triangle::bbox() const {

    // TODO (PathTracer): Task 2
    // compute the bounding box of the triangle

    // Beware of flat/zero-volume boxes! You may need to
    // account for that here, or later on in BBox::intersect
    Vec3 v_0 = vertex_list[v0].position;
    Vec3 v_1 = vertex_list[v1].position;
    Vec3 v_2 = vertex_list[v2].position;
    float minx = std::min(std::min(v_0.x, v_1.x), v_2.x);
    float maxx = std::max(std::max(v_0.x, v_1.x), v_2.x);
    float miny = std::min(std::min(v_0.y, v_1.y), v_2.y);
    float maxy = std::max(std::max(v_0.y, v_1.y), v_2.y);
    float minz = std::min(std::min(v_0.z, v_1.z), v_2.z);
    float maxz = std::max(std::max(v_0.z, v_1.z), v_2.z);
    if(minx == maxx || miny == maxy || minz == maxz) {
        return BBox();
    }
    Vec3 minV (minx, miny, minz);
    Vec3 maxV (maxx, maxy, maxz);
    return BBox(minV, maxV);
}

Trace Triangle::hit(const Ray& ray) const {

    // Vertices of triangle - has postion and surface normal
    Tri_Mesh_Vert v_0 = vertex_list[v0];
    Tri_Mesh_Vert v_1 = vertex_list[v1];
    Tri_Mesh_Vert v_2 = vertex_list[v2];
    //(void)v_0;
    //(void)v_1;
    //(void)v_2;
    Trace ret;
    Vec3 e1 = v_1.position - v_0.position;
    Vec3 e2 = v_2.position - v_0.position;
    Vec3 s = ray.point - v_0.position;
    float a = dot(cross(e1, ray.dir), e2);
    if (a == 0.0f) {
        ret.origin = ray.point;
        ret.hit = false;       // was there an intersection?
        ret.distance = 0.0f;   // at what distance did the intersection occur?
        ret.position = Vec3{}; // where was the intersection?
        ret.normal = Vec3{};   // what was the surface normal at the intersection?
                            // (this should be interpolated between the three vertex normals)
        return ret;
    }
    
    Vec3 scross = cross(s, e2);
    float u = (dot(-scross, ray.dir)) / a;
    float v = (dot(cross(e1, ray.dir), s)) / a;
    float t = (dot(-scross, e1)) / a;
    Vec3 intersect = ray.point + t * ray.dir;
    // TODO (PathTracer): Task 2
    // Intersect this ray with a triangle defined by the three above points.
    if(u < 0.0f || v < 0.0f || u+v > 1.0f || t < 0.0f || 
        intersect.x < ray.dist_bounds.x || intersect.x > ray.dist_bounds.y ||
        intersect.y < ray.dist_bounds.x || intersect.y > ray.dist_bounds.y ||
        intersect.z < ray.dist_bounds.x || intersect.z > ray.dist_bounds.y) {
        ret.origin = ray.point;
        ret.hit = false;       // was there an intersection?
        ret.distance = 0.0f;   // at what distance did the intersection occur?
        ret.position = Vec3{}; // where was the intersection?
        ret.normal = Vec3{};   // what was the surface normal at the intersection?
                            // (this should be interpolated between the three vertex normals)
        return ret;
    }
    BBox box = bbox();
    ray.dist_bounds.x = std::min(box.min.x, std::min(box.min.y, box.min.z));
    ray.dist_bounds.y = std::max(box.max.x, std::min(box.max.y, box.max.z));
    //Trace ret;
    ret.origin = ray.point;
    ret.hit = true;       // was there an intersection?
    ret.distance = t;   // at what distance did the intersection occur?
    ret.position = ray.point + t * ray.dir; // where was the intersection?
    ret.normal = (1.0f-u-v)*v_0.position + u * v_1.position + v * v_2.position;   // what was the surface normal at the intersection?
                           // (this should be interpolated between the three vertex normals)
    return ret;
}

Triangle::Triangle(Tri_Mesh_Vert* verts, unsigned int v0, unsigned int v1, unsigned int v2)
    : vertex_list(verts), v0(v0), v1(v1), v2(v2) {
}

void Tri_Mesh::build(const GL::Mesh& mesh) {

    verts.clear();
    triangles.clear();

    for(const auto& v : mesh.verts()) {
        verts.push_back({v.pos, v.norm});
    }

    const auto& idxs = mesh.indices();

    std::vector<Triangle> tris;
    for(size_t i = 0; i < idxs.size(); i += 3) {
        tris.push_back(Triangle(verts.data(), idxs[i], idxs[i + 1], idxs[i + 2]));
    }

    triangles.build(std::move(tris), 4);
}

Tri_Mesh::Tri_Mesh(const GL::Mesh& mesh) {
    build(mesh);
}

Tri_Mesh Tri_Mesh::copy() const {
    Tri_Mesh ret;
    ret.verts = verts;
    ret.triangles = triangles.copy();
    return ret;
}

BBox Tri_Mesh::bbox() const {
    return triangles.bbox();
}

Trace Tri_Mesh::hit(const Ray& ray) const {
    Trace t = triangles.hit(ray);
    return t;
}

size_t Tri_Mesh::visualize(GL::Lines& lines, GL::Lines& active, size_t level,
                           const Mat4& trans) const {
    return triangles.visualize(lines, active, level, trans);
}

} // namespace PT
