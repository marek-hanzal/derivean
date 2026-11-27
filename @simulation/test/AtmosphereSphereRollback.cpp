// The hour's restore when memory has run out (Codex DREV-CLAUDE-007-04, its rollback-oom): an
// allocation that fails inside a window, with every allocation after it failing too, leaves the flow
// and the hour as they came in. The window's first accepted step hands the flow's K ψ buffer to the
// history and leaves `product` without capacity, so a restore by copy would have to allocate and
// fail as well. Two places: the source's own allocation at the second step, and the copy that takes
// a refused try back.
//
// The test binary's operator new is replaced here: it is malloc unless this thread's gate is shut,
// then it throws std::bad_alloc. The gate is open everywhere except inside this test's sources.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"

#include <cstdlib>
#include <limits>
#include <new>

namespace {
thread_local bool shut = false;
thread_local int refused = 0;
} // namespace

void* operator new(std::size_t size) {
    if (shut) {
        ++refused;
        throw std::bad_alloc{};
    }
    if (void* pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc{};
}

void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

// The whole flow, its work space included (trialProduct carries K ψ while `known`).
bool sameWhole(const SphereFlow& a, const SphereFlow& b) {
    return a.rotation == b.rotation && a.coriolis == b.coriolis && a.vorticity == b.vorticity && a.before == b.before &&
           a.stream == b.stream && a.streamBack == b.streamBack && a.streamBackTwo == b.streamBackTwo &&
           a.productBack == b.productBack && a.productBackTwo == b.productBackTwo && a.product == b.product && a.levels == b.levels &&
           a.known == b.known && a.projection == b.projection && a.flux == b.flux && a.tolerance == b.tolerance && a.limit == b.limit &&
           a.step == b.step && a.started == b.started && a.trialVorticity == b.trialVorticity && a.trialBefore == b.trialBefore &&
           a.trialStream == b.trialStream && a.trialFlux == b.trialFlux && a.trialProduct == b.trialProduct && a.tendency == b.tendency &&
           a.absolute == b.absolute && a.atVertex == b.atVertex && a.inversion.converged == b.inversion.converged &&
           a.inversion.product == b.inversion.product;
}

// One window over a flow whose K ψ a refused direct step filled, the forcing from `source`; the
// allocation the source sets up fails, and the flow and the hour must come out as they went in.
void rollback(const SphereForcingSource& source, const char* where) {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    flow.tolerance = 1e-11;
    std::vector<double> vorticity(size_t(grid.cells));
    for (size_t c = 0; c < vorticity.size(); ++c) {
        const Vec3 p = grid.centre[c];
        vorticity[c] = 1e-5 * (p.z + p.x * p.y);
    }
    expect(setVorticity(flow, grid, stencil, poisson, vorticity), "the start did not invert");
    setStep(flow, 1800);
    flow.limit = 0;
    expect(!stepSphereFlow(flow, grid, stencil, poisson), "a step with no iterations was accepted");
    flow.limit = 60;
    expect(!flow.product.empty(), "the refused step left no K ψ");
    SphereHour hour;
    hour.steps = 2, hour.calm = 1, hour.courant = 0.12;
    hour.refusals.push_back({HourRefusal::Courant, 3, 1});
    const SphereFlow start = flow;
    refused = 0;
    bool caught = false;
    try {
        (void)advanceSphereHour(hour, flow, grid, stencil, poisson, source);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    shut = false;
    expect(caught && refused >= 1, where, refused);
    expect(sameWhole(flow, start), where);
    expect(hour.steps == 2 && hour.calm == 1 && hour.courant == 0.12 && hour.refusals.size() == 1 &&
               hour.refusals[0].reason == HourRefusal::Courant && hour.refusals[0].steps == 3 && hour.refusals[0].at == 1,
           where, hour.steps);
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkRollback() {
    // The source's own allocation at the second step.
    rollback(
        [](const SphereFlow& candidate, SphereForcing& forcing) {
            if (!candidate.started) return;
            shut = true;
            forcing.tendency.resize(candidate.vorticity.size());
        },
        "memory running out in the source left the window half done");
    // A forcing that is not admissible at the second step, filled into the capacity of the first:
    // the try is refused and its copy of the start, which must allocate, fails.
    rollback(
        [](const SphereFlow& candidate, SphereForcing& forcing) {
            forcing.tendency.assign(candidate.vorticity.size(), candidate.started ? std::numeric_limits<double>::quiet_NaN() : 0.0);
            shut = candidate.started;
        },
        "memory running out in taking a try back left the window half done");
}
