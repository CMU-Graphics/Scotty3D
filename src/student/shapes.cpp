
#include "../rays/shapes.h"
#include "debug.h"

namespace PT {

const char *Shape_Type_Names[(int)Shape_Type::count] = {"None", "Sphere"};

BBox Sphere::bbox() const {

    BBox box;
    box.enclose(Vec3(-radius));
    box.enclose(Vec3(radius));
    return box;
}

Trace Sphere::hit(const Ray &ray) const {

    // TODO (PathTracer): Task 2
    // Intersect this ray with a sphere of radius Sphere::radius centered at the origin.

    // If the ray intersects the sphere twice, ret should
    // represent the first intersection, but remember to respect
    // ray.time_bounds! For example, if there are two intersections,
    // but only the _later_ one is within ray.time_bounds, you should
    // return that one!

    // Note: ray.dir is not necessarily a unit vector if this object has a scale transform.
    // If you want it to be so, you should use ray.dir.unit(), and also note that you will
    // have to re-scale ret.time to be the distance along the original non-unit ray.dir.

    Trace ret;
    ret.hit = false;       // was there an intersection?
    ret.time = 0.0f;       // at what time did the intersection occur?
    ret.position = Vec3{}; // where was the intersection?
    ret.normal = Vec3{};   // what was the surface normal at the intersection?
    return ret;
}

} // namespace PT
