
#pragma once

#include "../lib/mathlib.h"
#include <map>
#include <set>

template<typename T> class Spline {
public:
    // Returns the interpolated value.
    T at(float time) const;

    // Purely for convenience, returns the exact same
    // value as at()---simply lets one evaluate a spline
    // f as though it were a function f(t) (which it is!)
    T operator()(float time) const {
        return at(time);
    }

    // Sets the value of the spline at a given time (i.e., knot),
    // creating a new knot at this time if necessary.
    void set(float time, T value) {
        control_points[time] = value;
    }

    // Removes the knot closest to the given time
    void erase(float time) {
        control_points.erase(time);
    }

    // Checks if time t is a control point
    bool has(float t) const {
        return control_points.count(t);
    }

    // Checks if there are any control points
    bool any() const {
        return !control_points.empty();
    }

    // Removes all control points
    void clear() {
        control_points.clear();
    }

    // Removes control points after t
    void crop(float t) {
        auto e = control_points.lower_bound(t);
        control_points.erase(e, control_points.end());
    }

    // Returns set of keys
    std::set<float> keys() const {
        std::set<float> ret;
        for(auto& e : control_points) ret.insert(e.first);
        return ret;
    }

private:
    std::map<float, T> control_points;

    // Given a time between 0 and 1, evaluates a cubic polynomial with
    // the given endpoint and tangent values at the beginning (0) and
    // end (1) of the interval
    static T cubic_unit_spline(float time, const T& position0, const T& position1,
                               const T& tangent0, const T& tangent1);
};

template<typename T, typename... Ts> class Splines {
public:
    void set(float t, T arg, Ts... args) {
        head.set(t, arg);
        tail.set(t, args...);
    }
    void erase(float t) {
        head.erase(t);
        tail.erase(t);
    }
    bool any() const {
        return head.any() || tail.any();
    }
    bool has(float t) const {
        return head.has(t) || tail.has(t);
    }
    void clear() {
        head.clear();
        tail.clear();
    }
    void crop(float t) {
        head.crop(t);
        tail.crop(t);
    }
    std::set<float> keys() const {
        auto first = head.keys();
        auto rest = tail.keys();
        rest.insert(first.begin(), first.end());
        return rest;
    }
    std::tuple<T, Ts...> at(float t) const {
        return std::tuple_cat(std::make_tuple(head.at(t)), tail.at(t));
    }

private:
    Spline<T> head;
    Splines<Ts...> tail;
};

template<typename T> class Splines<T> {
public:
    void set(float t, T arg) {
        head.set(t, arg);
    }
    void erase(float t) {
        head.erase(t);
    }
    bool any() const {
        return head.any();
    }
    bool has(float t) const {
        return head.has(t);
    }
    void crop(float t) {
        head.crop(t);
    }
    void clear() {
        head.clear();
    }
    std::set<float> keys() const {
        return head.keys();
    }
    std::tuple<T> at(float t) const {
        return std::make_tuple(head.at(t));
    }

private:
    Spline<T> head;
};

#ifdef SCOTTY3D_BUILD_REF
#include "../reference/spline.inl"
#else
#include "../student/spline.inl"
#endif
#include "spline.inl"
