// The circulation's closures on the sphere (SphereClosures, S4.3a): the hour's forcing and its step
// check read one damping, made from the flow's own K (Codex DREV-CLAUDE-007-06, point 1); the zonal
// mean relaxes per band to the belts' jet without moving ζ's area mean; the friction rate follows
// the terrain.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereDamping.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "core/fn/Hash.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <cmath>
#include <stdexcept>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Hour = 3600.0;

bool sameFlow(const SphereFlow& a, const SphereFlow& b) {
    return a.vorticity == b.vorticity && a.before == b.before && a.stream == b.stream && a.flux == b.flux && a.started == b.started &&
           a.step == b.step && a.levels == b.levels;
}

// The belts' jet of a solid eastward wind U cos φ, by Stokes on the band boundaries.
std::vector<double> solidJet(const LatitudeBands& bands, double speed) {
    std::vector<double> eastward(size_t(bands.count) + 1);
    for (size_t k = 0; k < eastward.size(); ++k) eastward[k] = speed * std::sqrt(std::max(0.0, 1 - bands.boundary[k] * bands.boundary[k]));
    std::vector<double> jet;
    zonalWindVorticity(bands, eastward, jet);
    return jet;
}

// A damping five times stiffer than production's (decay 6 h / 5) needs more steps an hour than the
// Courant number does (three, at 2Δt · stiffest 0.93; production's takes one); the closed hour takes
// them, and its window is the clean run of the same steps with a damping made separately from the
// same K, the relaxation and the friction, bit for bit. Closures made from another Poisson are
// refused before anything changes.
void checkWiring() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
    closures.damping = makeSphereDamping(grid, poisson.levels[0].stiffness, SphereGridDamping / 5);
    closures.jet = solidJet(closures.bands, 10);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> start = harmonicField(grid, 4, 2);
    for (double& v : start) v *= 1e-6;
    expect(setVorticity(flow, grid, stencil, poisson, start), "the start did not invert");
    const SphereFlow begin = flow;
    {
        SphereClosures production = makeSphereClosures(grid, poisson, {}, grid.side);
        production.jet = closures.jet;
        SphereFlow other = begin;
        SphereHour hour;
        expect(advanceClosedHour(hour, other, grid, stencil, poisson, production) && other.step == Hour,
               "production's damping took more than one step", Hour / other.step);
        SpherePoisson elsewhere = makeSpherePoisson(grid, stencil);
        bool refused = false;
        try {
            (void)advanceClosedHour(hour, other, grid, stencil, elsewhere, production);
        } catch (const std::invalid_argument&) {
            refused = true;
        }
        expect(refused && other.step == Hour, "closures of another Poisson were not refused");
    }
    SphereHour hour;
    expect(advanceClosedHour(hour, flow, grid, stencil, poisson, closures), "the closed window was not taken");
    const double stiffest = dampingStiffness(closures.damping);
    const int steps = int(std::lround(Hour / flow.step));
    expect(hour.stiffest == stiffest && 2 * flow.step * stiffest <= 1 && steps == 3, "the hour does not hold the closures' damping", steps);
    SphereDamping separate = makeSphereDamping(grid, poisson.levels[0].stiffness, SphereGridDamping / 5);
    SphereFlow clean = begin;
    setStep(clean, Hour / steps);
    std::vector<double> mean, pull;
    for (int k = 0; k < steps; ++k) {
        SphereForcing forcing;
        bandMeans(closures.bands, grid, baseLevel(clean), mean);
        for (size_t b = 0; b < mean.size(); ++b) mean[b] = (closures.jet[b] - mean[b]) / SphereMeanRelaxation;
        spreadBands(closures.bands, mean, pull);
        forcing.tendency = pull;
        SphereForcing filled;
        fillClosures(closures, grid, poisson.levels[0].stiffness, clean, filled);
        addDamping(separate, grid, poisson.levels[0].stiffness, baseLevel(clean), forcing.tendency);
        forcing.friction.assign(size_t(grid.cells), 1 / SphereSurfaceDrag);
        expect(filled.tendency == forcing.tendency && filled.friction == forcing.friction, "fillClosures is not the closures' forcing", k);
        expect(stepSphereFlow(clean, grid, stencil, poisson, forcing), "a clean closed step was not accepted", k);
    }
    expect(sameFlow(flow, clean), "the closed window is not the clean run with the same damping");
}

// From rest, an hour of relaxation to the jet of a 10 m/s solid wind puts t / 4 d of the jet into
// every band's mean, less the friction's t / 12 d and what the weights' spread and gather lose
// (0.981 to 0.984 of it in the bands above a fifth of the jet's peak). The relaxation's own area
// integral is zero to rounding: the step would take any mean out anyway, so it is read from the
// tendency, before the step.
void checkRelaxation() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
    closures.jet = solidJet(closures.bands, 10);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    expect(setVorticity(flow, grid, stencil, poisson, std::vector<double>(size_t(grid.cells), 0.0)), "rest did not invert");
    SphereForcing forcing;
    fillClosures(closures, grid, poisson.levels[0].stiffness, flow, forcing);
    double total = 0, scale = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double content = grid.area[size_t(c)] * forcing.tendency[size_t(c)];
        total += content;
        scale += std::abs(content);
    }
    expect(std::abs(total) < 1e-14 * scale, "the relaxation has an area integral", total / scale);
    SphereHour hour;
    expect(advanceClosedHour(hour, flow, grid, stencil, poisson, closures), "the relaxed window was not taken");
    std::vector<double> mean;
    bandMeans(closures.bands, grid, flow.vorticity, mean);
    double top = 0, low = 1e9, high = 0;
    for (double j : closures.jet) top = std::max(top, std::abs(j));
    for (size_t b = 0; b < mean.size(); ++b) {
        if (std::abs(closures.jet[b]) < 0.2 * top) continue;
        const double ratio = mean[b] / closures.jet[b] / (Hour / SphereMeanRelaxation);
        low = std::min(low, ratio), high = std::max(high, ratio);
    }
    expect(low > 0.97 && high < 0.99, "the bands did not take an hour's share of the jet", low);
}

// The friction rate is (1 + h / 1500 m) / 12 d per cell, sea level everywhere without a height.
void checkFriction() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    const SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    std::vector<double> height(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) height[size_t(c)] = 3000 * unit(31, c);
    const SphereClosures closures = makeSphereClosures(grid, poisson, height, grid.side);
    const SphereClosures flat = makeSphereClosures(grid, poisson, {}, grid.side);
    for (int c = 0; c < grid.cells; ++c) {
        const double rate = (1 + height[size_t(c)] / SphereRoughHeight) / SphereSurfaceDrag;
        expect(std::abs(closures.friction[size_t(c)] - rate) <= 1e-15 * rate && flat.friction[size_t(c)] == 1 / SphereSurfaceDrag,
               "the friction does not follow the terrain", closures.friction[size_t(c)]);
    }
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkClosures() {
    checkWiring();
    checkRelaxation();
    checkFriction();
}
