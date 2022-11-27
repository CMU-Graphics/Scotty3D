
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
		knots[time] = value;
	}

	// Removes the knot closest to the given time
	void erase(float time) {
		knots.erase(time);
	}

	// Checks if time t is a control point
	bool has(float t) const {
		return knots.count(t);
	}

	// Checks if there are any control points
	bool any() const {
		return !knots.empty();
	}

	// Removes all control points
	void clear() {
		knots.clear();
	}

	// Removes control points after t
	void crop(float t) {
		auto e = knots.lower_bound(t);
		knots.erase(e, knots.end());
	}

	// Returns set of keys
	std::set<float> keys() const {
		std::set<float> ret;
		for (auto& e : knots) ret.insert(e.first);
		return ret;
	}

	// Given a time between 0 and 1, evaluates a cubic polynomial with
	// the given endpoint and tangent values at the beginning (0) and
	// end (1) of the interval
	static T cubic_unit_spline(float time, const T& position0, const T& position1,
	                           const T& tangent0, const T& tangent1);

	std::map<float, T> knots;
};

template<> class Spline<Quat> {
public:

	//Spline< Quat > uses piecewise linear interpolation:
	Quat at(float time) const {
		if (knots.empty()) return Quat();
		if (knots.size() == 1) return knots.begin()->second;
		if (knots.begin()->first > time) return knots.begin()->second;
		auto k2 = knots.upper_bound(time);
		if (k2 == knots.end()) return std::prev(knots.end())->second;
		auto k1 = std::prev(k2);
		float t = (time - k1->first) / (k2->first - k1->first);
		return slerp(k1->second, k2->second, t);
	}
	Quat operator()(float time) const {
		return at(time);
	}
	void set(float time, Quat value) {
		knots[time] = value;
	}
	void erase(float time) {
		knots.erase(time);
	}
	std::set<float> keys() const {
		std::set<float> ret;
		for (auto& e : knots) ret.insert(e.first);
		return ret;
	}
	bool has(float t) const {
		return knots.count(t);
	}
	bool any() const {
		return !knots.empty();
	}
	void clear() {
		knots.clear();
	}
	void crop(float t) {
		auto e = knots.lower_bound(t);
		knots.erase(e, knots.end());
	}

	std::map<float, Quat> knots;
};

template<> class Spline<bool> {
public:

	//Spline< bool > uses piecewise constant interpolation:
	bool at(float time) const {
		if (knots.empty()) return false;
		if (knots.size() == 1) return knots.begin()->second;
		if (knots.begin()->first > time) return knots.begin()->second;
		auto k2 = knots.upper_bound(time);
		if (k2 == knots.end()) return std::prev(knots.end())->second;
		return std::prev(k2)->second;
	}

	bool operator()(float time) const {
		return at(time);
	}
	void set(float time, bool value) {
		knots[time] = value;
	}
	void erase(float time) {
		knots.erase(time);
	}
	std::set<float> keys() const {
		std::set<float> ret;
		for (auto& e : knots) ret.insert(e.first);
		return ret;
	}
	bool has(float t) const {
		return knots.count(t);
	}
	bool any() const {
		return !knots.empty();
	}
	void clear() {
		knots.clear();
	}
	void crop(float t) {
		auto e = knots.lower_bound(t);
		knots.erase(e, knots.end());
	}

	std::map<float, bool> knots;
};
