
#include "../rays/shapes.h"
#include "debug.h"

namespace PT {

const char* Shape_Type_Names[(int)Shape_Type::count] = {"None", "Sphere"};

BBox Sphere::bbox() const {

    BBox box;
    box.enclose(Vec3(-radius));
    box.enclose(Vec3(radius));
    return box;
}

Trace Sphere::hit(const Ray& ray) const {

    // TODO (PathTracer): Task 2
    // Intersect this ray with a sphere of radius Sphere::radius centered at the origin.

    // If the ray intersects the sphere twice, ret should
    // represent the first intersection, but remember to respect
    // ray.dist_bounds! For example, if there are two intersections,
    // but only the _later_ one is within ray.dist_bounds, you should
    // return that one!
    Trace ret;
    float det = pow(dot(ray.dir, ray.point), 2) - ray.point.norm_squared() + (radius*radius); 
    if (det < 0.0f) {
        ret.origin = ray.point;
        ret.hit = false;       // was there an intersection?
        ret.distance = 0.0f;   // at what distance did the intersection occur?
        ret.position = Vec3{}; // where was the intersection?
        ret.normal = Vec3{};   // what was the surface normal at the intersection?
        return ret;
    } else if (det == 0.0f) {
        float t = -dot(ray.dir, ray.point);
        Vec3 inter = ray.point + t*ray.dir;
        if (inter.x < ray.dist_bounds.x || inter.x > ray.dist_bounds.y ||
            inter.y < ray.dist_bounds.x || inter.y > ray.dist_bounds.y ||
            inter.z < ray.dist_bounds.x || inter.z > ray.dist_bounds.y) {
                ret.origin = ray.point;
                ret.hit = false;       // was there an intersection?
                ret.distance = 0.0f;   // at what distance did the intersection occur?
                ret.position = Vec3{}; // where was the intersection?
                ret.normal = Vec3{};   // what was the surface normal at the intersection?
                return ret;
        }
        ret.origin = ray.point;
        ret.hit = true;       // was there an intersection?
        ret.distance = t;   // at what distance did the intersection occur?
        ret.position = inter; // where was the intersection?
        ret.normal = Vec3{};   // what was the surface normal at the intersection?
        return ret;
    }
    float t1 = -dot(ray.dir, ray.point) + sqrt(det);
    float t2 = -dot(ray.dir, ray.point) - sqrt(det);
    float t = std::min(t1, t2);
    Vec3 inter = ray.point + t*ray.dir;

    if (inter.x < ray.dist_bounds.x || inter.x > ray.dist_bounds.y ||
        inter.y < ray.dist_bounds.x || inter.y > ray.dist_bounds.y ||
        inter.z < ray.dist_bounds.x || inter.z > ray.dist_bounds.y ||
        t < 0.0f) {
            if (t != t2) {
                inter = ray.point + t2*ray.dir;
                t = t2;
                if (inter.x < ray.dist_bounds.x || inter.x > ray.dist_bounds.y ||
                    inter.y < ray.dist_bounds.x || inter.y > ray.dist_bounds.y ||
                    inter.z < ray.dist_bounds.x || inter.z > ray.dist_bounds.y ||
                    t < 0.0f) {
                        ret.origin = ray.point;
                        ret.hit = false;       // was there an intersection?
                        ret.distance = 0.0f;   // at what distance did the intersection occur?
                        ret.position = Vec3{}; // where was the intersection?
                        ret.normal = Vec3{};   // what was the surface normal at the intersection?
                        return ret;
                }
            }
    }
            
    

    //Trace ret;
    ret.origin = ray.point;
    ret.hit = true;       // was there an intersection?
    ret.distance = t;   // at what distance did the intersection occur?
    ret.position = inter; // where was the intersection?
    ret.normal = inter.unit();   // what was the surface normal at the intersection?
    return ret;
}

} // namespace PT
