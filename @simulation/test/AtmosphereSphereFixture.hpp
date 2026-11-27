#pragma once

#include "atmosphere/fn/SphereStencil.hpp"
#include "atmosphere/type/SphereFlow.hpp"
#include "atmosphere/constant/SphereAir.hpp"
#include "geometry/fn/SphereGrid.hpp"
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

// The Rossby–Haurwitz wave of wavenumber 4 (Haurwitz 1940; Williamson et al. 1992 case 6, ω = K =
// 7.848e-6 1/s), an exact solution of the non-divergent barotropic vorticity equation: its relative
// vorticity at `time`, the pattern turning east at ν = (R(3 + R) ω − 2Ω) / ((1 + R)(2 + R)).
inline std::vector<double> rossbyHaurwitz(const SphereGrid& grid, double rotation, double time) {
    constexpr double Omega = 7.848e-6, Strength = 7.848e-6;
    constexpr int Wave = 4;
    const double drift = (Wave * (3 + Wave) * Omega - 2 * rotation) / ((1 + Wave) * (2 + Wave));
    std::vector<double> vorticity(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        const Vec3 d = grid.centre[size_t(c)];
        const double around = std::hypot(d.x, d.y), longitude = std::atan2(d.y, d.x);
        vorticity[size_t(c)] =
            2 * Omega * d.z - (Wave + 1) * (Wave + 2) * Strength * std::pow(around, Wave) * d.z * std::cos(Wave * (longitude - drift * time));
    }
    return vorticity;
}

// The flow's state (SphereFlowStart's fields, trialProduct while `known`) and what is fixed for it;
// its work space a step writes before it reads is left out.
inline bool sameState(const SphereFlow& a, const SphereFlow& b) {
    return a.rotation == b.rotation && a.coriolis == b.coriolis && a.vorticity == b.vorticity && a.before == b.before &&
           a.stream == b.stream && a.streamBack == b.streamBack && a.streamBackTwo == b.streamBackTwo &&
           a.productBack == b.productBack && a.productBackTwo == b.productBackTwo && a.levels == b.levels && a.known == b.known &&
           (!a.known || a.trialProduct == b.trialProduct) && a.projection == b.projection && a.flux == b.flux &&
           a.tolerance == b.tolerance && a.limit == b.limit && a.step == b.step && a.started == b.started &&
           a.inversion.converged == b.inversion.converged && a.inversion.product == b.inversion.product &&
           a.inversion.iterations == b.inversion.iterations && a.inversion.residual == b.inversion.residual &&
           a.inversion.sourceMean == b.inversion.sourceMean;
}

// The share of the belts' wind sphereSeasonOf keeps at a boundary of sine `sine`, as its header states.
inline double polarShareOf(double sine) {
    const double x = std::min(1.0, std::sqrt(1 - sine * sine) / SpherePolarCapCosine);
    return 0.5 * x * (3 - x * x);
}

// Tests run on several resolutions; levels 4 to 6 keep the suite fast (6 × 16² to 6 × 64² cells).
void checkPoisson();
void checkMultigrid();
void checkFlow();
void checkTracer();
void checkDrift();
void checkStep();
void checkEddyBand();
void checkLatitudeBands();
void checkForcing();
void checkHour();
void checkRollback();
void checkInPlace();
void checkDamping();
void checkClosures();
void checkEddies();
void checkSeason();
void checkSeasonInputs();
void checkSeasonPoles();

} // namespace derivean::sim::atmosphere_sphere_test
