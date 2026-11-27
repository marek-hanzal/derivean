// The hour's and the damping's work in place (Neon DREV-NEON-059, 065), each the same bit for bit as
// what it replaces: a friction fixed for the window and read in place steps as a source that fills the
// same friction each step does; a source that fills a friction besides, and a fixed friction that is
// not admissible, are refused (`Forcing`) with the flow's state as it was, and the admitted step refuses
// a term of the wrong size. Gershgorin's bound read
// from K in place is that of |K|'s product with ones, row by row (levels 2, 4 and 6: without interior
// rows, with the faces' interiors shared, and stored apart).
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereDamping.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <limits>
#include <stdexcept>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

void checkFixedFriction() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> vorticity(size_t(grid.cells)), push(size_t(grid.cells)), friction(size_t(grid.cells));
    for (size_t c = 0; c < vorticity.size(); ++c) {
        const Vec3 p = grid.centre[c];
        vorticity[c] = 2e-5 * p.z + 1e-5 * p.x * p.y;
        push[c] = 1e-11 * p.x;
        friction[c] = (1 + 0.5 * p.y * p.y) / (12 * 86400.0);
    }
    expect(setVorticity(flow, grid, stencil, poisson, vorticity), "the start did not invert");
    const SphereForcingSource pushed = [&](const SphereFlow&, SphereForcing& forcing) { forcing.tendency = push; };
    const SphereForcingSource both = [&](const SphereFlow&, SphereForcing& forcing) {
        forcing.tendency = push;
        forcing.friction = friction;
    };
    SphereFlow fixed = flow, filled = flow;
    SphereHour fixedHour, filledHour;
    for (int k = 0; k < 3; ++k) {
        expect(advanceSphereHour(fixedHour, fixed, grid, stencil, poisson, pushed, friction) &&
                   advanceSphereHour(filledHour, filled, grid, stencil, poisson, both),
               "a window was not taken", k);
        expect(sameState(fixed, filled) && fixedHour.steps == filledHour.steps && fixedHour.courant == filledHour.courant,
               "a friction read in place stepped otherwise than one filled each step", k);
    }
    const SphereFlow start = fixed;
    expect(!advanceSphereHour(fixedHour, fixed, grid, stencil, poisson, both, friction) && fixedHour.refusals.size() == 1 &&
               fixedHour.refusals[0].reason == HourRefusal::Forcing && sameState(fixed, start),
           "a source filling a friction beside the fixed one was taken");
    std::vector<double> wrong = friction;
    wrong[7] = std::numeric_limits<double>::quiet_NaN();
    expect(!advanceSphereHour(fixedHour, fixed, grid, stencil, poisson, pushed, wrong) && fixedHour.refusals.size() == 1 &&
               fixedHour.refusals[0].reason == HourRefusal::Forcing && fixedHour.refusals[0].at == 0 && sameState(fixed, start),
           "a fixed friction not finite was taken");
    // The admitted step still checks the terms' sizes, so a short one is refused, not read past.
    const std::vector<double> shortPush(push.begin(), push.end() - 1), none;
    expect(!stepAdmittedFlow(fixed, grid, stencil, poisson, shortPush, none) && !stepAdmittedFlow(fixed, grid, stencil, poisson, none, shortPush) &&
               sameState(fixed, start),
           "an admitted step took a term of the wrong size");
}

// With a tolerance below 1e-10 the flow keeps K ψ (`known`) in trialProduct, which a step reads: a
// try refused at its second step takes it back with the rest of the state, and so does one that
// throws there.
void checkKnownTakenBack() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    flow.tolerance = 1e-11;
    std::vector<double> vorticity(size_t(grid.cells));
    for (size_t c = 0; c < vorticity.size(); ++c) vorticity[c] = 1e-5 * (grid.centre[c].z + grid.centre[c].x * grid.centre[c].y);
    expect(setVorticity(flow, grid, stencil, poisson, vorticity) && flow.known, "the start did not keep K ψ");
    SphereHour hour;
    hour.steps = 2;
    const SphereFlow start = flow;
    const SphereForcingSource refused = [](const SphereFlow& candidate, SphereForcing& forcing) {
        forcing.tendency.assign(candidate.vorticity.size(), candidate.started ? std::numeric_limits<double>::quiet_NaN() : 0.0);
    };
    expect(!advanceSphereHour(hour, flow, grid, stencil, poisson, refused) && hour.refusals.size() == 1 &&
               hour.refusals[0].reason == HourRefusal::Forcing && hour.refusals[0].at == 1 && sameState(flow, start),
           "a refused try left its K ψ");
    const SphereForcingSource throwing = [](const SphereFlow& candidate, SphereForcing&) {
        if (candidate.started) throw std::runtime_error("the source failed");
    };
    bool caught = false;
    try {
        (void)advanceSphereHour(hour, flow, grid, stencil, poisson, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    expect(caught && sameState(flow, start), "a try that threw left its K ψ");
}

void checkBoundInPlace() {
    for (int level : {2, 4, 6}) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        const SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        const StencilMatrix& stiffness = poisson.levels[0].stiffness;
        StencilMatrix magnitude = stiffness;
        for (double& v : magnitude.inner) v = std::abs(v);
        for (double& v : magnitude.value) v = std::abs(v);
        const std::vector<double> ones(size_t(grid.cells), 1.0);
        std::vector<double> product(size_t(grid.cells), -1.0), sums(size_t(grid.cells), -2.0);
        double bound = 0;
        forEachProduct(magnitude, ones.data(), [&](size_t row, double sum) {
            product[row] = sum;
            bound = std::max(bound, sum / grid.area[row]);
        });
        forEachMagnitudeSum(stiffness, [&](size_t row, double sum) { sums[row] = sum; });
        const SphereDamping damping = makeSphereDamping(grid, stiffness, 6 * 3600.0);
        expect(sums == product && damping.bound == bound, "the row sums read in place are not the product of |K| with ones", level);
        if (!stiffness.shared) continue;
        // Each face's interior stored apart, as without shareFaceInteriors, scaled by face so that a
        // face read in another's place shows.
        StencilMatrix apart = stiffness, apartMagnitude = magnitude;
        apart.shared = apartMagnitude.shared = false;
        apart.inner.clear(), apartMagnitude.inner.clear();
        for (int face = 0; face < 6; ++face)
            for (size_t k = 0; k < stiffness.inner.size(); ++k) {
                apart.inner.push_back(-(1 + face) * stiffness.inner[k]);
                apartMagnitude.inner.push_back((1 + face) * magnitude.inner[k]);
            }
        std::vector<double> apartProduct(size_t(grid.cells), -1.0), apartSums(size_t(grid.cells), -2.0);
        forEachProduct(apartMagnitude, ones.data(), [&](size_t row, double sum) { apartProduct[row] = sum; });
        forEachMagnitudeSum(apart, [&](size_t row, double sum) { apartSums[row] = sum; });
        expect(apartSums == apartProduct, "the row sums of faces stored apart are not the product of |K| with ones", level);
    }
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkInPlace() {
    checkFixedFriction();
    checkKnownTakenBack();
    checkBoundInPlace();
}
