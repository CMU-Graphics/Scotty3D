
#pragma once

#include "../lib/mathlib.h"
#include <map>
#include <set>

template <typename T> class Spline {
public:
    // Returns the interpolated value.
    T at(float time) const;

    // Purely for convenience, returns the exact same
    // value as at()---simply lets one evaluate a spline
    // f as though it were a function f(t) (which it is!)
    T operator()(float time) const { return at(time); }

    // Sets the value of the spline at a given time (i.e., knot),
    // creating a new knot at this time if necessary.
    void set(float time, T value) { control_points[time] = value; }

    // Removes the knot closest to the given time
    void erase(float time) { control_points.erase(time); }

    // Checks if time t is a control point
    bool has(float t) const { return control_points.count(t); }

    // Checks if there are any control points
    bool any() const { return !control_points.empty(); }

    // Removes all control points
    void clear() { control_points.clear(); }

    // Removes control points after t
    void crop(float t) {
        auto e = control_points.lower_bound(t);
        control_points.erase(e, control_points.end());
    }

    // Returns set of keys
    std::set<float> keys() const {
        std::set<float> ret;
        for (auto &e : control_points)
            ret.insert(e.first);
        return ret;
    }

private:
    std::map<float, T> control_points;

    // Given a time between 0 and 1, evaluates a cubic polynomial with
    // the given endpoint and tangent values at the beginning (0) and
    // end (1) of the interval
    static T cubic_unit_spline(float time, const T &position0, const T &position1,
                               const T &tangent0, const T &tangent1);
};

template <> class Spline<Quat> {

public:
    Quat at(float time) const {
        if (values.empty())
            return Quat::euler(Vec3(0.0f));
        if (values.size() == 1)
            return values.begin()->second;
        if (values.begin()->first > time)
            return values.begin()->second;
        auto k2 = values.upper_bound(time);
        if (k2 == values.end())
            return std::prev(values.end())->second;

        Quat p0, p1, p2, p3;
        float t1, t2;
        p2 = k2->second;
        t2 = k2->first;

        auto k1 = std::prev(k2);
        p1 = k1->second;
        t1 = k1->first;

        float dt = t2 - t1;
        if (k1 == values.begin()) {
            p0 = p1;
        } else {
            auto k0 = std::prev(k1);
            p0 = k0->second;
        }

        if (next(k2) == values.end()) {
            p3 = p2;
        } else {
            auto k3 = std::next(k2);
            p3 = k3->second;
        }

        float norm_t = (time - t1) / dt;
        Quat qa = tangent(p0, p1, p2);
        Quat pb = tangent(p1, p2, p3);

        float slerpT = 2.0f * norm_t * (1.0f - norm_t);
        Quat slerp1 = p1.slerp(p2, norm_t);
        Quat slerp2 = qa.slerp(pb, norm_t);
        return slerp1.slerp(slerp2, slerpT);
    }

    Quat operator()(float time) const { return at(time); }
    void set(float time, Quat value) { values[time] = value; }
    void erase(float time) { values.erase(time); }
    std::set<float> keys() const {
        std::set<float> ret;
        for (auto &e : values)
            ret.insert(e.first);
        return ret;
    }
    bool has(float t) const { return values.count(t); }
    bool any() const { return !values.empty(); }
    void clear() { values.clear(); }
    void crop(float t) {
        auto e = values.lower_bound(t);
        values.erase(e, values.end());
    }

private:
    std::map<float, Quat> values;
    static Quat tangent(const Quat &q0, const Quat &q1, const Quat &q2) {
        Quat q1inv = q1.inverse();
        Quat c1 = q1inv * q2;
        Quat c2 = q1inv * q0;
        c1.make_log();
        c2.make_log();
        Quat c3 = c2 + c1;
        c3.scale(-0.25f);
        c3.make_exp();
        Quat r = q1 * c3;
        return r.unit();
    }
};

template <typename T, typename... Ts> class Splines {
public:
    void set(float t, T arg, Ts... args) {
        head.set(t, arg);
        tail.set(t, args...);
    }
    void erase(float t) {
        head.erase(t);
        tail.erase(t);
    }
    bool any() const { return head.any() || tail.any(); }
    bool has(float t) const { return head.has(t) || tail.has(t); }
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

template <typename T> class Splines<T> {
public:
    void set(float t, T arg) { head.set(t, arg); }
    void erase(float t) { head.erase(t); }
    bool any() const { return head.any(); }
    bool has(float t) const { return head.has(t); }
    void crop(float t) { head.crop(t); }
    void clear() { head.clear(); }
    std::set<float> keys() const { return head.keys(); }
    std::tuple<T> at(float t) const { return std::make_tuple(head.at(t)); }

private:
    Spline<T> head;
};

#ifdef SCOTTY3D_BUILD_REF
#include "../reference/spline.inl"
#else
#include "../student/spline.inl"
#endif
