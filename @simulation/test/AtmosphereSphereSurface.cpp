// The terrain's squeeze of the column (setSurface, S4.3d): the background f · (1 + min(h, 2500) /
// 10 000) the flow carries with ζ, and what it does to a westerly crossing a mountain.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereSurface.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Radians = std::numbers::pi / 180;

Vec3 pointAt(double latitude, double longitude) {
    return {std::cos(latitude * Radians) * std::cos(longitude * Radians), std::cos(latitude * Radians) * std::sin(longitude * Radians),
            std::sin(latitude * Radians)};
}

double arc(Vec3 a, Vec3 b) { return PlanetRadius * std::acos(std::clamp(a.x * b.x + a.y * b.y + a.z * b.z, -1.0, 1.0)); }

int nearest(const SphereGrid& grid, Vec3 point) {
    int best = 0;
    for (int c = 1; c < grid.cells; ++c)
        if (arc(grid.centre[size_t(c)], point) < arc(grid.centre[size_t(best)], point)) best = c;
    return best;
}

// Two mountains of 2 000 m, radius L (e-folding 1 500 km), at 45° N and 45° S on the meridian 0.
constexpr double Peak = 2000, Spread = 1.5e6;
std::vector<double> mountains(const SphereGrid& grid) {
    std::vector<double> height(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        double h = 0;
        for (double latitude : {45.0, -45.0}) {
            const double d = arc(grid.centre[size_t(c)], pointAt(latitude, 0)) / Spread;
            h += Peak * std::exp(-d * d);
        }
        height[size_t(c)] = h;
    }
    return height;
}

// Sea level, given as nothing or as zeros, is makeSphereFlow's f bit for bit, and so is the terrain
// set back to it; over terrain f · (1 + min(h, 2500) / 10 000) to the last place, the cap reached; a
// height NaN, infinite, below zero or not one per cell, or a flow of another grid, is refused with the
// background as it was.
void checkBackground() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereFlow plain = makeSphereFlow(grid, rotationRateAt(PresentAge));
    SphereFlow flow = plain;
    setSurface(flow, grid, {});
    expect(flow.coriolis == plain.coriolis, "sea level moved the background");
    setSurface(flow, grid, std::vector<double>(size_t(grid.cells), 0.0));
    expect(flow.coriolis == plain.coriolis, "zero terrain moved the background");
    std::vector<double> height = mountains(grid);
    for (double& h : height) h *= 2;   // peaks of 4 000 m, past the cap
    setSurface(flow, grid, height);
    int capped = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double f = 2 * plain.rotation * grid.centre[size_t(c)].z;
        const double expected = f * (1 + std::min(height[size_t(c)], 2500.0) / 10000.0);
        capped += height[size_t(c)] > 2500;
        expect(std::abs(flow.coriolis[size_t(c)] - expected) <= 1e-16 * std::abs(expected), "the squeezed background moved", double(c));
    }
    expect(capped > 0, "no cell reached the terrain's cap");
    const std::vector<double> squeezed = flow.coriolis;
    for (const std::vector<double>& bad : {std::vector<double>(size_t(grid.cells) - 1, 0.0), std::vector<double>(size_t(grid.cells), -1.0),
                                            std::vector<double>(size_t(grid.cells), std::numeric_limits<double>::quiet_NaN()),
                                            std::vector<double>(size_t(grid.cells), std::numeric_limits<double>::infinity())}) {
        bool threw = false;
        try {
            setSurface(flow, grid, bad);
        } catch (const std::invalid_argument&) { threw = true; }
        expect(threw && flow.coriolis == squeezed, "a bad terrain was taken");
    }
    bool threw = false;
    SphereFlow other = makeSphereFlow(makeSphereGrid(3), plain.rotation);
    try {
        setSurface(other, grid, {});
    } catch (const std::invalid_argument&) { threw = true; }
    expect(threw, "a flow of another grid was given a background");
    setSurface(flow, grid, {});
    expect(flow.coriolis == plain.coriolis, "the terrain set back is not sea level");
}

// Zero terrain set on the flow leaves a closed hour bit for bit as it was.
void checkZeroHour() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    expect(setVorticity(flow, grid, stencil, poisson, rossbyHaurwitz(grid, flow.rotation, 0)), "the wave did not invert");
    SphereFlow zero = flow;
    setSurface(zero, grid, std::vector<double>(size_t(grid.cells), 0.0));
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side), same = closures;
    SphereHour hour, other;
    expect(advanceClosedHour(hour, flow, grid, stencil, poisson, closures) && advanceClosedHour(other, zero, grid, stencil, poisson, same),
           "a closed hour was not taken");
    expect(sameState(flow, zero), "zero terrain moved the hour");
}

// A westerly of U cos φ (U = 10 m/s) crosses the mountains for an hour: against the flow without
// them, ζ falls on the windward slope and rises on the lee in the north, the other way round in the
// south, where f < 0, by the column's squeeze, −t u ∂(f h / D)/∂x, to 5 % at the slopes' steepest
// (L / √2 either side of the peaks, level 5; measured 0.974 windward, 0.993 lee).
void checkMountainSpin() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    constexpr double U = 10, T = 3600;
    SphereFlow flat = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> solid(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) solid[size_t(c)] = 2 * U / PlanetRadius * grid.centre[size_t(c)].z;
    expect(setVorticity(flat, grid, stencil, poisson, solid), "the westerly did not invert");
    SphereFlow hilly = flat;
    setSurface(hilly, grid, mountains(grid));
    setStep(flat, T / 8);
    setStep(hilly, T / 8);
    for (int k = 0; k < 8; ++k)
        expect(stepSphereFlow(flat, grid, stencil, poisson) && stepSphereFlow(hilly, grid, stencil, poisson), "a step did not converge", k);
    const double offset = Spread / std::sqrt(2.0);
    for (double latitude : {45.0, -45.0}) {
        const double f = 2 * flat.rotation * std::sin(latitude * Radians), u = U * std::cos(latitude * Radians);
        const double slope = Peak * std::sqrt(2.0) / Spread * std::exp(-0.5);   // |∂h/∂x| at L / √2
        const double expected = T * u * f / SphereTerrainDepth * slope;          // the lee's rise
        const double degrees = offset / (PlanetRadius * std::cos(latitude * Radians)) / Radians;
        for (int side : {-1, 1}) {
            const int c = nearest(grid, pointAt(latitude, side * degrees));
            const double change = hilly.vorticity[size_t(c)] - flat.vorticity[size_t(c)];
            expect(std::abs(change / (side * expected) - 1) < 0.05, "the mountain did not squeeze the column", change / (side * expected));
        }
    }
}

// Terrain set after a taken window of four 900 s steps at level 5, on the westerly above: the next
// step's response at the windward and lee slopes is the response of twelve steps of 75 s to 3 %
// (measured 1.002 and 0.998; 1.958 with the old background's history kept, as Codex DREV-CLAUDE-010-04
// found 1.953 on his own wind). Setting the same terrain again, and sea level on a flow at sea level,
// restarts nothing: the next window is bit for bit the one without the call. The restart is setStep's,
// bit for bit, with no inversion.
void checkSurfaceSwitch() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    constexpr double U = 10;
    SphereFlow sea = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> solid(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) solid[size_t(c)] = 2 * U / PlanetRadius * grid.centre[size_t(c)].z;
    expect(setVorticity(sea, grid, stencil, poisson, solid), "the westerly did not invert");
    SphereHour hour;
    hour.steps = 4;
    expect(advanceSphereHour(hour, sea, grid, stencil, poisson, SphereForcing{}) && sea.step == 900 && sea.started, "the window was not four steps");
    const std::vector<double> height = mountains(grid);

    SphereFlow hill = sea, fineSea = sea, fineHill = sea;
    setSurface(hill, grid, height);
    setSurface(fineHill, grid, height);
    // The restart is setStep's, with ζ, ψ and the fluxes as they were (no inversion).
    SphereFlow stepped = sea;
    stepped.coriolis = hill.coriolis;
    setStep(stepped, 450);
    setStep(stepped, 900);
    expect(sameState(hill, stepped) && !hill.started && hill.inversion.iterations == sea.inversion.iterations, "the new background was not setStep's restart");
    setStep(fineSea, 75);
    setStep(fineHill, 75);
    const SphereFlow seaBefore = sea;
    expect(stepSphereFlow(sea, grid, stencil, poisson) && stepSphereFlow(hill, grid, stencil, poisson), "a 900 s step did not converge");
    for (int k = 0; k < 12; ++k)
        expect(stepSphereFlow(fineSea, grid, stencil, poisson) && stepSphereFlow(fineHill, grid, stencil, poisson), "a 75 s step did not converge", k);
    const double degrees = Spread / std::sqrt(2.0) / (PlanetRadius * std::cos(45 * Radians)) / Radians;
    for (double latitude : {45.0, -45.0})
        for (int side : {-1, 1}) {
            const size_t c = size_t(nearest(grid, pointAt(latitude, side * degrees)));
            const double ratio = (hill.vorticity[c] - sea.vorticity[c]) / (fineHill.vorticity[c] - fineSea.vorticity[c]);
            expect(std::abs(ratio - 1) < 0.03, "a terrain set after a window took the old history", ratio);
        }

    // The same terrain again and sea level again: the next window as without the call, bit for bit.
    SphereFlow again = hill, plain = hill, seaAgain = seaBefore, seaPlain = seaBefore;
    setSurface(again, grid, height);
    setSurface(seaAgain, grid, {});
    expect(again.started && seaAgain.started, "the same background restarted the history");
    SphereHour a = hour, b = hour, c = hour, d = hour;
    expect(advanceSphereHour(a, again, grid, stencil, poisson, SphereForcing{}) && advanceSphereHour(b, plain, grid, stencil, poisson, SphereForcing{}) &&
               advanceSphereHour(c, seaAgain, grid, stencil, poisson, SphereForcing{}) &&
               advanceSphereHour(d, seaPlain, grid, stencil, poisson, SphereForcing{}),
           "a window after the same background was not taken");
    expect(sameState(again, plain) && sameState(seaAgain, seaPlain), "the same background changed the run");
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkSurface() {
    checkBackground();
    checkZeroHour();
    checkMountainSpin();
    checkSurfaceSwitch();
}
