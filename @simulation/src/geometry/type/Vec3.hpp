#pragma once

#include <algorithm>
#include <cmath>

namespace derivean::sim {

// The one 3-vector of the simulation. In geometry it is the planet-centred Earth-fixed frame (ECEF):
// metres for positions, unit length for directions; Z points to the north pole, X to latitude 0
// longitude 0, Y to latitude 0 longitude 90 E.
struct Vec3 {
    double x{}, y{}, z{};
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(double s, Vec3 a) { return a * s; }
inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline double length(Vec3 a) { return std::sqrt(dot(a, a)); }

// A zero vector stays zero instead of turning into NaN; anything longer than 1e-15 comes out unit.
inline Vec3 normalized(Vec3 a) { return a * (1.0 / std::sqrt(std::max(1e-30, dot(a, a)))); }

} // namespace derivean::sim
