// The barotropic flow and a passive tracer on the sphere: a wind across face edges and cube vertices,
// the conserved integrals, a Rossby–Haurwitz wave against its analytic drift, and a cosine bell
// carried once round the planet over four cube vertices.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereTransport.hpp"
#include "geometry/fn/Cell.hpp"

#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Pi = std::numbers::pi;
constexpr double Day = 86400.0;

// Solid rotation about an axis at right angles to a cube diagonal: its fastest circle runs through
// four cube vertices, two face centres and two face-edge midpoints. ψ = −ω R² (axis · r̂), ζ = 2ω (axis · r̂).
const Vec3 Axis = normalized({1, -1, 0});
const double Spin = 2 * Pi / (12 * Day);   // Williamson et al. (1992) case 1: one turn in 12 days, 38.6 m/s

std::vector<Vec3> vertexDirections(const SphereGrid& grid) {
    std::vector<Vec3> at(grid.vertex.size());
    for (int c = 0; c < grid.cells; ++c)
        for (int k = 0; k < 4; ++k) at[size_t(grid.corners[size_t(c)][size_t(k)])] = cornerOf(cellAt(grid, c), k);
    return at;
}

SphereFlow solidRotation(const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson, double rotation) {
    SphereFlow flow = makeSphereFlow(grid, rotation);
    std::vector<double> vorticity(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) vorticity[size_t(c)] = 2 * Spin * dot(Axis, grid.centre[size_t(c)]);
    expect(setVorticity(flow, grid, stencil, poisson, vorticity), "solid rotation did not invert", flow.inversion.residual);
    return flow;
}

double largestFlux(const std::vector<double>& flux) {
    double worst = 0;
    for (double value : flux) worst = std::max(worst, std::abs(value));
    return worst;
}

// The step that keeps the outflow Courant number at `courant`.
double stepFor(const SphereGrid& grid, const SphereStencil& stencil, const std::vector<double>& flux, double courant) {
    return courant / outflowCourant(grid, stencil, flux, 1.0);
}

// The Rossby–Haurwitz wave (rossbyHaurwitz in the fixture) with Earth's Ω turns east by 12.2° a day.
// After a day the stepped flow matches it; the integral of the absolute vorticity holds to rounding
// and that of relative vorticity stays zero.
void checkWave() {
    const double rotation = rotationRateAt(PresentAge);
    double errors[3] = {}, enstrophies[3] = {};
    for (int level = 4; level <= 6; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        SphereFlow flow = makeSphereFlow(grid, rotation);
        auto wave = [&](double time) { return rossbyHaurwitz(grid, rotation, time); };
        expect(setVorticity(flow, grid, stencil, poisson, wave(0)), "the wave did not invert", flow.inversion.residual);
        auto integrals = [&](double& absolute, double& relative, double& enstrophy, double& scale) {
            absolute = relative = enstrophy = scale = 0;
            for (int c = 0; c < grid.cells; ++c) {
                const double q = flow.vorticity[size_t(c)] + flow.coriolis[size_t(c)], a = grid.area[size_t(c)];
                absolute += a * q;
                relative += a * flow.vorticity[size_t(c)];
                enstrophy += a * q * q;
                scale += a * std::abs(q);
            }
        };
        double absolute0, relative0, enstrophy0, scale0;
        integrals(absolute0, relative0, enstrophy0, scale0);
        const int steps = int(std::ceil(Day / stepFor(grid, stencil, flow.flux, 0.4)));
        setStep(flow, Day / steps);
        bool accepted = true;
        for (int s = 0; s < steps; ++s) accepted = stepSphereFlow(flow, grid, stencil, poisson) && accepted;
        double absolute, relative, enstrophy, scale;
        integrals(absolute, relative, enstrophy, scale);
        const std::vector<double> exact = wave(Day), start = wave(0);
        double error = 0, moved = 0, amplitude = 0;
        for (int c = 0; c < grid.cells; ++c) {
            error = std::max(error, std::abs(flow.vorticity[size_t(c)] - exact[size_t(c)]));
            moved = std::max(moved, std::abs(start[size_t(c)] - exact[size_t(c)]));
            amplitude = std::max(amplitude, std::abs(exact[size_t(c)]));
        }
        expect(accepted, "a step of the wave was not accepted", flow.inversion.residual);
        expect(moved > 0.7 * amplitude, "the wave did not move", moved / amplitude);
        expect(std::abs(absolute - absolute0) < 1e-14 * scale0, "absolute vorticity is not kept", (absolute - absolute0) / scale0);
        expect(std::abs(relative) < 1e-14 * scale0, "relative vorticity does not integrate to zero", relative / scale0);
        errors[level - 4] = error / amplitude;
        enstrophies[level - 4] = std::abs(enstrophy / enstrophy0 - 1);
    }
    // Measured: 0.125, 0.033, 0.0085 of the amplitude; enstrophy −6.5e-4, −1.8e-4, −4.5e-5 in a day.
    for (int k = 1; k < 3; ++k) {
        expect(errors[k - 1] / errors[k] > 3.5, "Rossby–Haurwitz wave is not second order", errors[k - 1] / errors[k]);
        expect(enstrophies[k - 1] / enstrophies[k] > 3, "enstrophy loss does not fall with resolution", enstrophies[k - 1] / enstrophies[k]);
    }
    expect(errors[2] < 0.012 && enstrophies[2] < 1e-4, "Rossby–Haurwitz wave is off at level 6", errors[2]);
}

} // namespace

namespace derivean::sim::atmosphere_sphere_test {

// Solid rotation over cube vertices without planetary rotation: the stream function's edge fluxes
// against the exact ∫ v · n dl = ψ(from) − ψ(to), and the vorticity, an exact steady state, after a
// day of steps. The fluxes are first order where the stream function's error bends at face edges
// (0.4e-3, 1.4e-3 and 2.6e-3 of the largest flux at level 6 inside, on face edges and at cube
// vertices); the steady state holds to second order, 3.9e-4 of the vorticity after a day.
void checkFlow() {
    double fluxes[3][3] = {}, changes[3] = {};
    for (int level = 4; level <= 6; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        SphereFlow flow = solidRotation(grid, stencil, poisson, 0);
        const std::vector<Vec3> at = vertexDirections(grid);
        double worst[3] = {};
        const double scale = largestFlux(flow.flux);
        for (size_t e = 0; e < grid.edge.size(); ++e) {
            const SphereEdge& edge = grid.edge[e];
            const double exact = -Spin * PlanetRadius * PlanetRadius * (dot(Axis, at[size_t(edge.from)]) - dot(Axis, at[size_t(edge.to)]));
            const int band = std::max(bandOf(grid, edge.left), bandOf(grid, edge.right));
            worst[band] = std::max(worst[band], std::abs(flow.flux[e] - exact) / scale);
        }
        const std::vector<double> start = flow.vorticity;
        const double step = stepFor(grid, stencil, flow.flux, 0.4);
        const int steps = int(std::ceil(Day / step));
        setStep(flow, Day / steps);
        for (int s = 0; s < steps; ++s) expect(stepSphereFlow(flow, grid, stencil, poisson), "a step of the rotation was not accepted", s);
        double change = 0;
        for (int c = 0; c < grid.cells; ++c) change = std::max(change, std::abs(flow.vorticity[size_t(c)] - start[size_t(c)]) / (2 * Spin));
        for (int band = 0; band < 3; ++band) fluxes[level - 4][band] = worst[band];
        changes[level - 4] = change;
    }
    for (int k = 1; k < 3; ++k) {
        for (int band = 0; band < 3; ++band)
            expect(fluxes[k - 1][band] / fluxes[k][band] > 1.8, "edge fluxes are not first order", fluxes[k - 1][band] / fluxes[k][band]);
        expect(changes[k - 1] / changes[k] > 3.3, "the steady state does not hold to second order", changes[k - 1] / changes[k]);
    }
    expect(fluxes[2][2] < 4e-3 && changes[2] < 6e-4, "solid rotation is off at level 6", std::max(fluxes[2][2], changes[2]));
    expect(std::abs(rotationRateAt(PresentAge) / 7.2921e-5 - 1) < 1e-4, "today's rotation rate is not Earth's", rotationRateAt(PresentAge));
    checkWave();
}

// A cosine bell (Williamson et al. 1992 case 1) carried once round by the solid rotation, over four
// cube vertices, two face centres and two face-edge midpoints: the mass is kept to rounding, no value
// goes below zero or above the start, a uniform tracer stays uniform, and the shape's l1 error falls
// with resolution (1.07, 0.57 and 0.35 at levels 4 to 6: the limited scheme is upwind across the
// grid lines, so the bell spreads where the wind crosses them at an angle).
void checkTracer() {
    double errors[3] = {};
    for (int level = 4; level <= 6; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        const SphereFlow flow = solidRotation(grid, stencil, poisson, 0);
        const Vec3 start = normalized({1, 1, 1});
        std::vector<double> bell(size_t(grid.cells)), work, uniform(size_t(grid.cells), 1.0);
        for (int c = 0; c < grid.cells; ++c) {
            const double r = std::acos(std::clamp(dot(start, grid.centre[size_t(c)]), -1.0, 1.0));
            bell[size_t(c)] = r < 1.0 / 3.0 ? 500 * (1 + std::cos(3 * Pi * r)) : 0.0;
        }
        const std::vector<double> exact = bell;
        const double mass = areaMean(grid, bell);
        const double turn = 2 * Pi / Spin;
        const int steps = int(std::ceil(turn / stepFor(grid, stencil, flow.flux, 0.45)));
        const double peak = *std::max_element(bell.begin(), bell.end());
        double lowest = 0, highest = 0;
        for (int s = 0; s < steps; ++s) {
            advectTracer(grid, stencil, flow.flux, turn / steps, bell, work);
            if (s < 10) advectTracer(grid, stencil, flow.flux, turn / steps, uniform, work);
            for (double value : bell) {
                lowest = std::min(lowest, value);
                highest = std::max(highest, value);
            }
        }
        double l1 = 0, norm = 0, off = 0;
        for (int c = 0; c < grid.cells; ++c) {
            l1 += grid.area[size_t(c)] * std::abs(bell[size_t(c)] - exact[size_t(c)]);
            norm += grid.area[size_t(c)] * std::abs(exact[size_t(c)]);
            off = std::max(off, std::abs(uniform[size_t(c)] - 1));
        }
        expect(std::abs(areaMean(grid, bell) / mass - 1) < 1e-14, "tracer mass is not kept", areaMean(grid, bell) / mass - 1);
        expect(lowest >= 0 && highest <= peak, "tracer left its bounds", lowest < 0 ? lowest : highest - peak);
        expect(off < 1e-14, "a uniform tracer does not stay uniform", off);
        errors[level - 4] = l1 / norm;
    }
    expect(errors[0] / errors[1] > 1.5 && errors[1] / errors[2] > 1.5 && errors[2] < 0.4, "the bell's shape does not converge", errors[2]);
}

} // namespace derivean::sim::atmosphere_sphere_test
