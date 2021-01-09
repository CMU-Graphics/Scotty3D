
#pragma once

#include <algorithm>
#include <cmath>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#include "line.h"
#include "plane.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"

#define EPS_F 0.00001f
#define PI_F 3.14159265358979323846264338327950288f
#define Radians(v) ((v) * (PI_F / 180.0f))
#define Degrees(v) ((v) * (180.0f / PI_F))

template<typename T> inline T clamp(T x, T min, T max) {
    return std::min(std::max(x, min), max);
}
template<> inline Vec2 clamp(Vec2 v, Vec2 min, Vec2 max) {
    return Vec2(clamp(v.x, min.x, max.x), clamp(v.y, min.y, max.y));
}
template<> inline Vec3 clamp(Vec3 v, Vec3 min, Vec3 max) {
    return Vec3(clamp(v.x, min.x, max.x), clamp(v.y, min.y, max.y), clamp(v.z, min.z, max.z));
}
template<> inline Vec4 clamp(Vec4 v, Vec4 min, Vec4 max) {
    return Vec4(clamp(v.x, min.x, max.x), clamp(v.y, min.y, max.y), clamp(v.z, min.z, max.z),
                clamp(v.w, min.w, max.w));
}

template<typename T> T lerp(T start, T end, float t) {
    return start + (end - start) * t;
}

inline float sign(float x) {
    return x > 0.0f ? 1.0f : x < 0.0f ? -1.0f : 0.0f;
}

inline float frac(float x) {
    return x - (long long)x;
}

inline float smoothstep(float e0, float e1, float x) {
    float t = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

#include "bbox.h"
#include "mat4.h"
#include "quat.h"
#include "ray.h"
