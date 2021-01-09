
#pragma once

#include "../lib/mathlib.h"
#include "trace.h"

namespace PT {

template<typename Primitive> class List {
public:
    List() {
    }
    List(std::vector<Primitive>&& primitives) : prims(primitives) {
    }

    BBox bbox() const {
        BBox ret;
        for(const auto& p : prims) {
            ret.enclose(p.bbox());
        }
        return ret;
    }

    Trace hit(const Ray& ray) const {
        Trace ret;
        for(const auto& p : prims) {
            Trace test = p.hit(ray);
            ret = Trace::min(ret, test);
        }
        return ret;
    }

    void append(Primitive&& prim) {
        prims.push_back(std::move(prim));
    }

private:
    std::vector<Primitive> prims;
};

} // namespace PT
