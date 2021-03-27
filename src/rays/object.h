
#pragma once

#include "../lib/mathlib.h"
#include "../scene/object.h"
#include <variant>

#include "bvh.h"
#include "list.h"
#include "shapes.h"
#include "trace.h"
#include "tri_mesh.h"

namespace PT {

class Object {
public:
    Object(Shape&& shape, Scene_ID id, unsigned int m = 0, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), material(m), underlying(std::move(shape)) {
        has_trans = trans != Mat4::I;
    }
    Object(Tri_Mesh&& tri_mesh, Scene_ID id, unsigned int m = 0, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), material(m), underlying(std::move(tri_mesh)) {
        has_trans = trans != Mat4::I;
    }
    Object(List<Object>&& list, Scene_ID id, unsigned int m = 0, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), material(m), underlying(std::move(list)) {
        has_trans = trans != Mat4::I;
    }
    Object(BVH<Object>&& bvh, Scene_ID id, unsigned int m = 0, const Mat4& T = Mat4::I)
        : trans(T), itrans(T.inverse()), _id(id), material(m), underlying(std::move(bvh)) {
        has_trans = trans != Mat4::I;
    }

    Object(const Object& src) = delete;
    Object& operator=(const Object& src) = delete;
    Object& operator=(Object&& src) = default;
    Object(Object&& src) = default;

    BBox bbox() const {
        BBox box = std::visit(overloaded{[](const auto& o) { return o.bbox(); }}, underlying);
        if(has_trans) box.transform(trans);
        return box;
    }

    Trace hit(Ray ray) const {
        if(has_trans) ray.transform(itrans);
        Trace ret =
            std::visit(overloaded{[&ray](const auto& o) { return o.hit(ray); }}, underlying);
        if(ret.hit) {
            ret.material = material;
            if(has_trans) ret.transform(trans, itrans.T());
        }
        return ret;
    }

    size_t visualize(GL::Lines& lines, GL::Lines& active, size_t level, const Mat4& vtrans) const {
        Mat4 next = has_trans ? vtrans * trans : vtrans;
        return std::visit(
            overloaded{
                [&](const BVH<Object>& bvh) { return bvh.visualize(lines, active, level, next); },
                [&](const Tri_Mesh& mesh) { return mesh.visualize(lines, active, level, next); },
                [](const auto&) { return size_t(0); }},
            underlying);
    }

    Scene_ID id() const {
        return _id;
    }
    void set_trans(const Mat4& T) {
        trans = T;
        itrans = T.inverse();
        has_trans = trans != Mat4::I;
    }

private:
    bool has_trans;
    Mat4 trans, itrans;
    unsigned int material;
    Scene_ID _id;
    std::variant<Tri_Mesh, Shape, BVH<Object>, List<Object>> underlying;
};

} // namespace PT