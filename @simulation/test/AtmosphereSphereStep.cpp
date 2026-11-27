// The flow's state across steps: an inversion that does not converge publishes nothing, a retry
// computes what a clean run does, and the leapfrog's fixed step is restarted when it changes, as at
// the shorter last step of a window that does not divide by the step.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereFlowGuess.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Day = 86400.0;

bool sameState(const SphereFlow& a, const SphereFlow& b) {
    return a.vorticity == b.vorticity && a.before == b.before && a.stream == b.stream && a.flux == b.flux && a.started == b.started &&
           a.step == b.step && a.levels == b.levels && a.streamBack == b.streamBack && a.streamBackTwo == b.streamBackTwo &&
           a.productBack == b.productBack && a.productBackTwo == b.productBackTwo;
}

// Level 3, a smooth vorticity, CG cut to zero iterations (the case of DREV-CLAUDE-004). A step
// without a step length is refused.
void checkFailure() {
    const SphereGrid grid = makeSphereGrid(3);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    std::vector<double> vorticity(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        const Vec3 p = grid.centre[size_t(c)];
        vorticity[size_t(c)] = 2e-5 * (p.x + p.y * p.z);
    }
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    const SphereFlow fresh = flow;
    expect(!stepSphereFlow(flow, grid, stencil, poisson) && sameState(flow, fresh), "a step without a step length was taken");
    flow.limit = 0;
    expect(!setVorticity(flow, grid, stencil, poisson, vorticity) && !flow.inversion.converged, "an unconverged start was accepted");
    expect(sameState(flow, fresh), "an unconverged start changed the flow");
    flow.limit = 60;
    expect(setVorticity(flow, grid, stencil, poisson, vorticity), "the start did not invert", flow.inversion.residual);
    setStep(flow, 120);
    // Once in the forward step after the start, then in the leapfrog with two and three levels of the
    // first guess's history.
    for (int phase = 0; phase < 3; ++phase) {
        const SphereFlow clean = flow;
        flow.limit = 0;
        expect(!stepSphereFlow(flow, grid, stencil, poisson) && flow.inversion.residual > flow.tolerance, "an unconverged step was accepted",
               phase);
        expect(sameState(flow, clean), "an unconverged step changed the history or the wind", phase);
        flow.limit = 60;
        SphereFlow control = clean;
        expect(stepSphereFlow(flow, grid, stencil, poisson) && stepSphereFlow(control, grid, stencil, poisson), "the retry was not accepted",
               phase);
        expect(sameState(flow, control), "a retry differs from a clean step", phase);
    }
}

// Day of the Rossby–Haurwitz wave at level 5 against the analytic wave, largest error per amplitude.
double waveError(const SphereGrid& grid, const SphereFlow& flow, double rotation) {
    const std::vector<double> exact = rossbyHaurwitz(grid, rotation, Day);
    double error = 0, amplitude = 0;
    for (int c = 0; c < grid.cells; ++c) {
        error = std::max(error, std::abs(flow.vorticity[size_t(c)] - exact[size_t(c)]));
        amplitude = std::max(amplitude, std::abs(exact[size_t(c)]));
    }
    return error / amplitude;
}

// The same step changes nothing, a new one restarts. A day of the wave ending in a half step (with its
// restart) lands where a day of equal steps does: 3.34e-2 of the amplitude off the analytic wave for
// both (the grid's error), 1.7e-5 apart. A leapfrog step of length h₁ from levels h₀ apart lands
// 4.4e-3 away: with tendency F it adds 2h₁F where (h₀ + h₁)F is due, and the filter reads a false
// curvature.
void checkStepChange() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    const double rotation = rotationRateAt(PresentAge);
    SphereFlow start = makeSphereFlow(grid, rotation);
    expect(setVorticity(start, grid, stencil, poisson, rossbyHaurwitz(grid, rotation, 0)), "the wave did not invert");
    // Courant at most 0.4, and the window ends half a step after the last whole one.
    const int whole = int(std::ceil(Day * outflowCourant(grid, start.flux, 1.0) / 0.4));
    const double step = Day / (whole + 0.5), rest = Day - whole * step;

    SphereFlow even = start;
    setStep(even, Day / (whole + 1));
    SphereFlow shortened = start, stale = start;
    setStep(shortened, step);
    setStep(stale, step);
    bool accepted = true;
    for (int s = 0; s <= whole; ++s) accepted = stepSphereFlow(even, grid, stencil, poisson) && accepted;
    for (int s = 0; s < whole; ++s) {
        accepted = stepSphereFlow(shortened, grid, stencil, poisson) && accepted;
        accepted = stepSphereFlow(stale, grid, stencil, poisson) && accepted;
    }
    setStep(shortened, step);
    expect(shortened.started, "the same step restarted the history");
    setStep(shortened, rest);
    expect(!shortened.started && shortened.before == shortened.vorticity && shortened.levels == 1, "a new step did not restart the history");
    stale.step = rest;   // bypasses setStep: a leapfrog step of `rest` from levels `step` apart
    accepted = stepSphereFlow(shortened, grid, stencil, poisson) && stepSphereFlow(stale, grid, stencil, poisson) && accepted;
    expect(accepted, "a step of the window was not accepted");
    const double evenError = waveError(grid, even, rotation), shortError = waveError(grid, shortened, rotation);
    double amplitude = 0, shortOff = 0, staleOff = 0;
    for (int c = 0; c < grid.cells; ++c) {
        amplitude = std::max(amplitude, std::abs(even.vorticity[size_t(c)]));
        shortOff = std::max(shortOff, std::abs(shortened.vorticity[size_t(c)] - even.vorticity[size_t(c)]));
        staleOff = std::max(staleOff, std::abs(stale.vorticity[size_t(c)] - even.vorticity[size_t(c)]));
    }
    expect(shortError < 1.01 * evenError, "a shortened last step lands off the wave", shortError / evenError);
    expect(shortOff < 1e-4 * amplitude, "a shortened last step departs from equal steps", shortOff / amplitude);
    expect(staleOff > 20 * shortOff, "the stale leapfrog step is not caught by the test", staleOff / amplitude);
}

// The first guess projected on the last three ψ (DREV-NEON-019): 20 steps of the wave at level 5 to
// 1e-9 take 0.58 of the iterations of starting from the last ψ (3.5 against 6 a step, the first
// steps with a shorter history included) and land 4.4e-11 of the largest vorticity from them, within
// the solver's tolerance; the history restarts with setVorticity and setStep, and a run started with
// the projection off forms no products, so switching it on later starts a fresh history. The guess's
// K, the combination of the K dᵢ, lies within 2e-13 of the largest K ψ of K applied to the guess.
void checkGuess() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    const double rotation = rotationRateAt(PresentAge);
    SphereFlow projected = makeSphereFlow(grid, rotation);
    expect(setVorticity(projected, grid, stencil, poisson, rossbyHaurwitz(grid, rotation, 0)) && projected.levels == 1,
           "the start did not restart the guess's history");
    SphereFlow last = projected;
    last.projection = 0;
    const double step = 0.4 / outflowCourant(grid, projected.flux, 1.0);
    setStep(projected, step);
    setStep(last, step);
    int fewer = 0, more = 0;
    for (int s = 0; s < 20; ++s) {
        expect(stepSphereFlow(projected, grid, stencil, poisson) && stepSphereFlow(last, grid, stencil, poisson), "a step was not accepted", s);
        fewer += projected.inversion.iterations;
        more += last.inversion.iterations;
    }
    expect(projected.levels == 3 && fewer * 3 < more * 2, "the projected guess does not save iterations", double(fewer) / more);
    // The guess's K for one, two and three levels; below 1e-9 the solver forms its own (a wrong one
    // given changes nothing); the reported residual of the projected run against the true one.
    for (int projection = 1; projection <= 3; ++projection) {
        SphereFlow next = projected;
        next.projection = projection;
        next.trialVorticity = next.vorticity;
        expect(guessStream(next, grid, poisson), "the projected guess did not form its product", projection);
        std::vector<double> product;
        applyMatrix(poisson.levels[0].stiffness, next.trialStream, product);
        double apart = 0, scale = 0;
        for (int c = 0; c < grid.cells; ++c) {
            apart = std::max(apart, std::abs(next.trialProduct[size_t(c)] - product[size_t(c)]));
            scale = std::max(scale, std::abs(product[size_t(c)]));
        }
        expect(apart < 2e-13 * scale, "K of the guess is not the combination of the K dᵢ", apart / scale);
    }
    {
        std::vector<double> given = projected.stream, own = projected.stream, wrong(given.size(), 1.0);
        const PoissonReport a = solveSpherePoisson(poisson, grid, projected.vorticity, given, 1e-10, 60, &wrong);
        const PoissonReport b = solveSpherePoisson(poisson, grid, projected.vorticity, own, 1e-10, 60);
        expect(given == own && a.iterations == b.iterations, "a given product was used below 1e-9");
        std::vector<double> kpsi;
        applyMatrix(poisson.levels[0].stiffness, projected.stream, kpsi);
        const double mean = areaMean(grid, projected.vorticity);
        double rr = 0, bb = 0;
        for (int c = 0; c < grid.cells; ++c) {
            const double source = -grid.area[size_t(c)] * (projected.vorticity[size_t(c)] - mean), r = source - kpsi[size_t(c)];
            rr += r * r, bb += source * source;
        }
        const double gap = std::abs(std::sqrt(rr / bb) - projected.inversion.residual) / projected.tolerance;
        expect(gap < 1e-2, "the residual reported from the guess's K is not the true one", gap);
    }
    double off = 0, top = 0;
    for (int c = 0; c < grid.cells; ++c) {
        off = std::max(off, std::abs(projected.vorticity[size_t(c)] - last.vorticity[size_t(c)]));
        top = std::max(top, std::abs(last.vorticity[size_t(c)]));
    }
    expect(off < 1e-9 * top, "the projected guess moves the flow beyond the solver's tolerance", off / top);
    setStep(projected, step / 2);
    expect(projected.levels == 1, "a new step did not restart the guess's history");
    expect(last.levels == 1, "steps without the projection keep a history without products", last.levels);
}

} // namespace

namespace derivean::sim::atmosphere_sphere_test {

void checkStep() {
    checkFailure();
    checkStepChange();
    checkGuess();
}

} // namespace derivean::sim::atmosphere_sphere_test
