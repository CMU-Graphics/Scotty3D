
#include "spline.h"

template<> class Spline<Quat> {
public:
    Quat at(float time) const {
        if(values.empty()) return Quat();
        if(values.size() == 1) return values.begin()->second;
        if(values.begin()->first > time) return values.begin()->second;
        auto k2 = values.upper_bound(time);
        if(k2 == values.end()) return std::prev(values.end())->second;
        auto k1 = std::prev(k2);
        float t = (time - k1->first) / (k2->first - k1->first);
        return slerp(k1->second, k2->second, t);
    }
    Quat operator()(float time) const {
        return at(time);
    }
    void set(float time, Quat value) {
        values[time] = value;
    }
    void erase(float time) {
        values.erase(time);
    }
    std::set<float> keys() const {
        std::set<float> ret;
        for(auto& e : values) ret.insert(e.first);
        return ret;
    }
    bool has(float t) const {
        return values.count(t);
    }
    bool any() const {
        return !values.empty();
    }
    void clear() {
        values.clear();
    }
    void crop(float t) {
        auto e = values.lower_bound(t);
        values.erase(e, values.end());
    }

private:
    std::map<float, Quat> values;
};

template<> class Spline<bool> {
public:
    bool at(float time) const {
        if(values.empty()) return false;
        if(values.size() == 1) return values.begin()->second;
        if(values.begin()->first > time) return values.begin()->second;
        auto k2 = values.upper_bound(time);
        if(k2 == values.end()) return std::prev(values.end())->second;
        return std::prev(k2)->second;
    }

    bool operator()(float time) const {
        return at(time);
    }
    void set(float time, bool value) {
        values[time] = value;
    }
    void erase(float time) {
        values.erase(time);
    }
    std::set<float> keys() const {
        std::set<float> ret;
        for(auto& e : values) ret.insert(e.first);
        return ret;
    }
    bool has(float t) const {
        return values.count(t);
    }
    bool any() const {
        return !values.empty();
    }
    void clear() {
        values.clear();
    }
    void crop(float t) {
        auto e = values.lower_bound(t);
        values.erase(e, values.end());
    }

private:
    std::map<float, bool> values;
};
