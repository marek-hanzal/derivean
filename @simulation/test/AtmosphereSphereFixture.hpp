#pragma once

#include "atmosphere/fn/SphereGrid.hpp"
#include "atmosphere/fn/SphereHarmonics.hpp"
#include "geometry/constant/Sphere.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace derivean::sim::atmosphere_sphere_test {

inline void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "AtmosphereSphere: " << message << " (" << value << ")\n";
    std::exit(1);
}

// A real spherical harmonic sampled at the cell centres.
inline std::vector<double> harmonicField(const SphereGrid& grid, int degree, int order) {
    std::vector<double> field(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        const Vec3 d = grid.centre[size_t(c)];
        field[size_t(c)] = harmonicAt(degree, order, d.z, std::atan2(d.y, d.x));
    }
    return field;
}

inline double laplacianEigenvalue(int degree) { return -degree * (degree + 1) / (PlanetRadius * PlanetRadius); }

// The largest error per place: [0] inside a face (two or more cells from a face edge), [1] the band
// of one cell either side of a face edge, [2] the cells touching a cube vertex. The band is two
// cells wide because the operator's stencil reaches one cell beyond its edges.
struct PlaceErrors {
    double worst[3]{};
};

inline int bandOf(const SphereGrid& grid, int cell) {
    if (placeOf(grid, cell) == SpherePlace::CubeCorner) return 2;
    const Cell at = cellAt(grid, cell);
    const int distance = std::min(std::min(at.i, at.j), std::min(grid.side - 1 - at.i, grid.side - 1 - at.j));
    return distance <= 1 ? 1 : 0;
}

inline PlaceErrors errorsOf(const SphereGrid& grid, const std::vector<double>& value, const std::vector<double>& exact) {
    PlaceErrors errors;
    for (int c = 0; c < grid.cells; ++c) {
        const int band = bandOf(grid, c);
        errors.worst[band] = std::max(errors.worst[band], std::abs(value[size_t(c)] - exact[size_t(c)]));
    }
    return errors;
}

// Tests run on several resolutions; levels 4 to 6 keep the suite fast (6 × 16² to 6 × 64² cells).
void checkPoisson();
void checkFlow();
void checkTracer();
void checkDrift();
void checkEddyBand();

} // namespace derivean::sim::atmosphere_sphere_test
