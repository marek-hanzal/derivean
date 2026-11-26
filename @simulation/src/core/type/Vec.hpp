#pragma once

#include <algorithm>
#include <cmath>

namespace derivean::sim {

// Horizontal world vector in metres. X grows east, Z grows south; Y is always elevation.
struct Vec2 {
    double x{}, z{};
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.z + b.z}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.z - b.z}; }
inline Vec2 operator*(Vec2 a, double s) { return {a.x * s, a.z * s}; }
inline double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.z * b.z; }
inline double cross(Vec2 a, Vec2 b) { return a.x * b.z - a.z * b.x; }
inline double length(Vec2 a) { return std::sqrt(dot(a, a)); }
inline Vec2 perpendicular(Vec2 a) { return {-a.z, a.x}; }

inline Vec2 normalized(Vec2 a) {
    const double size = length(a);
    return size > 1e-12 ? a * (1.0 / size) : Vec2{1, 0};
}

inline double saturate(double value) { return std::clamp(value, 0.0, 1.0); }
inline double mix(double a, double b, double t) { return a + (b - a) * t; }

inline double smoothstep(double low, double high, double value) {
    const double t = saturate((value - low) / (high - low));
    return t * t * (3 - 2 * t);
}

// Signed shortest offset on a periodic axis.
inline double wrapDelta(double delta, double period) {
    return delta - period * std::floor(delta / period + 0.5);
}

inline int wrapIndex(int index, int count) {
    const int rest = index % count;
    return rest < 0 ? rest + count : rest;
}

inline int floorToInt(double value) { return int(std::floor(value)); }

} // namespace derivean::sim
