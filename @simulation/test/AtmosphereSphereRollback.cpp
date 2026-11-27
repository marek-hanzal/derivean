// The hour's restore when memory has run out (Codex DREV-CLAUDE-007-04, its rollback-oom): an
// allocation that fails inside a window, with every allocation after it failing too, leaves the flow's
// state and the hour as they came in. The window's first accepted step hands the flow's K ψ buffer to
// the history and leaves `product` without capacity, so a restore of the whole flow by copy would
// have to allocate and fail as well. The hour keeps the window's start and its forcing in its own
// capacity (Neon DREV-NEON-059), so neither the source's output nor a refused try's take-back needs
// memory once the window has begun: the source's own allocation at the second step throws and the
// start is swapped back; a refused try with memory exhausted is taken back by copy into the flow's
// own capacity, without allocating; and the source's allocation in a try after a refused one throws
// and the start is swapped back, the hour's refusals moved back.
//
// The test binary's operator new is replaced here: it is malloc unless this thread's gate is shut,
// then it throws std::bad_alloc. The gate is open everywhere except inside this test's sources.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereTransport.hpp"

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

// One window over a flow whose K ψ a refused direct step filled, the forcing from `source`, which
// shuts memory at the second step. With `throws` the window ends in std::bad_alloc, otherwise it is
// refused for its forcing without allocating; either way the flow's state comes out as it went in,
// and the hour too, but for the refusal of the window that was refused.
void rollback(const SphereForcingSource& source, bool throws, const char* where, bool refusedFirst = false) {
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
    // With `refusedFirst` the two-step try exceeds the limit at once and is taken back by copy before
    // the four-step try's source runs.
    if (refusedFirst) hour.limit = 0.75 * outflowCourant(grid, stencil, flow.flux, hour.window / 2);
    const SphereFlow start = flow;
    refused = 0;
    bool caught = false, taken = true;
    try {
        taken = advanceSphereHour(hour, flow, grid, stencil, poisson, source);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    shut = false;
    expect(throws ? caught && refused >= 1 : !caught && !taken && refused == 0, where, refused);
    expect(sameState(flow, start), where);
    const HourTry last = throws ? HourTry{HourRefusal::Courant, 3, 1} : HourTry{HourRefusal::Forcing, 2, 1};
    expect(hour.steps == 2 && hour.calm == 1 && hour.courant == 0.12 && hour.refusals.size() == 1 &&
               hour.refusals[0].reason == last.reason && hour.refusals[0].steps == last.steps && hour.refusals[0].at == last.at,
           where, hour.steps);
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkRollback() {
    // The source's own allocation at the second step.
    rollback(
        [](const SphereFlow& candidate, SphereForcing& forcing) {
            forcing.tendency.assign(candidate.vorticity.size(), 0.0);
            if (!candidate.started) return;
            shut = true;
            const std::vector<double> own(candidate.vorticity.size());
            forcing.tendency[0] = own[0];
        },
        true, "memory running out in the source left the window half done");
    // A forcing that is not admissible at the second step, filled into the capacity of the first:
    // the try is refused and taken back into the flow's own capacity with memory shut.
    rollback(
        [](const SphereFlow& candidate, SphereForcing& forcing) {
            forcing.tendency.assign(candidate.vorticity.size(), candidate.started ? std::numeric_limits<double>::quiet_NaN() : 0.0);
            shut = candidate.started;
        },
        false, "memory running out in taking a try back left the window half done");
    // The source's own allocation in the second try, after the first was refused and taken back: the
    // restore swaps the start in and moves the hour's refusals back.
    rollback(
        [](const SphereFlow& candidate, SphereForcing&) {
            shut = true;
            const std::vector<double> own(candidate.vorticity.size());
        },
        true, "memory running out in the try after a refusal left the window half done", true);
}
