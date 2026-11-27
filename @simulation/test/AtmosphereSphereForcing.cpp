// The closures' forcing in the sphere's step (SphereForcing, S4.1; S4 plan, binding correction 1):
// an empty forcing is the plain step, the forced path is the plain level plus span · F less one
// constant, friction keeps exactly keep = 1 / (1 + span · r) of the level before the area mean is
// taken out where it is kept, both time levels keep a zero mean, and a refused step publishes
// nothing.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "core/fn/Hash.hpp"

#include <limits>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

bool sameFlow(const SphereFlow& a, const SphereFlow& b) {
    return a.vorticity == b.vorticity && a.before == b.before && a.stream == b.stream && a.flux == b.flux && a.started == b.started &&
           a.step == b.step && a.levels == b.levels && a.streamBack == b.streamBack && a.streamBackTwo == b.streamBackTwo &&
           a.productBack == b.productBack && a.productBackTwo == b.productBackTwo;
}

struct Setup {
    SphereGrid grid;
    SphereStencil stencil;
    SpherePoisson poisson;
    SphereFlow flow;
};

// The Rossby–Haurwitz wave at `level`, a step of 600 s.
Setup waveAt(int level) {
    Setup s{makeSphereGrid(level), {}, {}, {}};
    s.stencil = makeSphereStencil(s.grid);
    s.poisson = makeSpherePoisson(s.grid, s.stencil);
    const double rotation = rotationRateAt(PresentAge);
    s.flow = makeSphereFlow(s.grid, rotation);
    expect(setVorticity(s.flow, s.grid, s.stencil, s.poisson, rossbyHaurwitz(s.grid, rotation, 0)), "the wave did not invert");
    setStep(s.flow, 600);
    return s;
}

// A forcing of the closures' size: a tendency of 1e-10 1/s² with a nonzero mean, and friction rates
// from none to one per two days.
SphereForcing forcingOf(const SphereGrid& grid, bool push, bool brake) {
    SphereForcing forcing;
    if (push) {
        forcing.tendency.resize(size_t(grid.cells));
        for (int c = 0; c < grid.cells; ++c) forcing.tendency[size_t(c)] = 1e-10 * (unit(3, c) - 0.3);
    }
    if (brake) {
        forcing.friction.resize(size_t(grid.cells));
        for (int c = 0; c < grid.cells; ++c) forcing.friction[size_t(c)] = unit(5, c) / (2 * 86400.0);
    }
    return forcing;
}

double meanShare(const SphereGrid& grid, const std::vector<double>& field) {
    double net = 0, gross = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double content = grid.area[size_t(c)] * field[size_t(c)];
        net += content;
        gross += std::abs(content);
    }
    return std::abs(net) / gross;
}

// The level the step formed before the filter, from the filtered pair and the level it stepped from:
// RAW moves ζₙ by split · curve and next by −(1 − split) · curve (the filter's 0.53).
std::vector<double> unfiltered(const SphereFlow& after, const std::vector<double>& now) {
    constexpr double Split = 0.53;
    std::vector<double> next(now.size());
    for (size_t c = 0; c < now.size(); ++c) {
        const double curve = (after.before[c] - now[c]) / Split;
        next[c] = after.vorticity[c] + (1 - Split) * curve;
    }
    return next;
}

// An empty forcing steps to the same bits as the plain step, forward and leapfrog.
void checkEmpty() {
    Setup s = waveAt(4);
    SphereFlow plain = s.flow;
    for (int k = 0; k < 4; ++k) {
        expect(stepSphereFlow(plain, s.grid, s.stencil, s.poisson), "a plain step was not accepted", k);
        expect(stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}), "an empty forcing's step was not accepted", k);
        expect(sameFlow(plain, s.flow), "an empty forcing changes the step", k);
    }
}

// Friction alone, with r constant and then varying from cell to cell, against the same step with
// r = 0, which takes the mean out of x: at constant r the level is keep times that one; at varying r
// it is keep_i · (x_i − c) with one c for every cell. In the forward step the level is the new ζ
// itself; in the leapfrog it is read back through the filter.
void checkFriction() {
    for (bool leapfrog : {false, true}) {
        Setup s = waveAt(4);
        if (leapfrog) expect(stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson), "the first step was not accepted");
        const double span = leapfrog ? 2 * s.flow.step : s.flow.step;
        const std::vector<double> now = s.flow.vorticity;
        SphereForcing none;
        none.friction.assign(size_t(s.grid.cells), 0.0);
        SphereFlow free = s.flow;
        expect(stepSphereFlow(free, s.grid, s.stencil, s.poisson, none), "the step with r = 0 was not accepted");
        const std::vector<double> base = leapfrog ? unfiltered(free, now) : free.vorticity;
        double top = 0;
        for (double v : base) top = std::max(top, std::abs(v));
        for (bool varying : {false, true}) {
            SphereForcing forcing = forcingOf(s.grid, false, varying);
            if (!varying) forcing.friction.assign(size_t(s.grid.cells), 1 / 86400.0);
            SphereFlow braked = s.flow;
            expect(stepSphereFlow(braked, s.grid, s.stencil, s.poisson, forcing), "the braked step was not accepted", varying);
            const std::vector<double> level = leapfrog ? unfiltered(braked, now) : braked.vorticity;
            double off = 0, low = 1e300, high = -1e300;
            for (int c = 0; c < s.grid.cells; ++c) {
                const double keep = 1 / (1 + span * forcing.friction[size_t(c)]);
                off = std::max(off, std::abs(level[size_t(c)] - keep * base[size_t(c)]));
                const double shift = base[size_t(c)] - level[size_t(c)] / keep;   // c_r − c_0, one for all
                low = std::min(low, shift), high = std::max(high, shift);
            }
            if (!varying) expect(off < 1e-14 * top, "constant friction does not keep exactly keep", off / top);
            expect(high - low < 1e-14 * top, "the mean is not taken out as one constant", (high - low) / top);
            if (varying) expect(std::abs(high) > 1e-6 * top, "varying friction does not shift the mean", high / top);
            expect(meanShare(s.grid, braked.vorticity) < 1e-14 && meanShare(s.grid, braked.before) < 1e-14,
                   "friction leaves a mean in a time level", meanShare(s.grid, braked.vorticity));
        }
    }
}

// 200 steps of tendency and friction: both time levels keep a zero area mean to rounding, though
// the tendency has a mean of its own.
void checkMean() {
    Setup s = waveAt(4);
    const SphereForcing forcing = forcingOf(s.grid, true, true);
    double worst = 0;
    for (int k = 0; k < 200; ++k) {
        expect(stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, forcing), "a forced step was not accepted", k);
        worst = std::max(worst, std::max(meanShare(s.grid, s.flow.vorticity), meanShare(s.grid, s.flow.before)));
    }
    expect(worst < 1e-14, "a forced time level has an area mean", worst);
}

// The forced path against the plain step, forward and in the leapfrog: with a tendency of zeros it is
// the plain level less its mean (rounding apart, 1e-15 of the largest value), and with a tendency F
// the plain level plus span · F less one constant for every cell.
void checkPushed() {
    for (bool leapfrog : {false, true}) {
        Setup s = waveAt(4);
        if (leapfrog) expect(stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson), "the first step was not accepted");
        const double span = leapfrog ? 2 * s.flow.step : s.flow.step;
        const std::vector<double> now = s.flow.vorticity;
        SphereFlow plain = s.flow;
        expect(stepSphereFlow(plain, s.grid, s.stencil, s.poisson), "the plain step was not accepted");
        const std::vector<double> base = leapfrog ? unfiltered(plain, now) : plain.vorticity;
        double top = 0;
        for (double v : base) top = std::max(top, std::abs(v));
        for (bool zeros : {true, false}) {
            SphereForcing forcing = forcingOf(s.grid, true, false);
            if (zeros) forcing.tendency.assign(size_t(s.grid.cells), 0.0);
            SphereFlow pushed = s.flow;
            expect(stepSphereFlow(pushed, s.grid, s.stencil, s.poisson, forcing), "the pushed step was not accepted", zeros);
            const std::vector<double> level = leapfrog ? unfiltered(pushed, now) : pushed.vorticity;
            double low = 1e300, high = -1e300;
            for (int c = 0; c < s.grid.cells; ++c) {
                const double shift = base[size_t(c)] + span * forcing.tendency[size_t(c)] - level[size_t(c)];
                low = std::min(low, shift), high = std::max(high, shift);
            }
            expect(high - low < 1e-14 * top, "the forced level is not the plain one plus span · F less a constant", (high - low) / top);
            if (zeros) expect(std::abs(high) < 1e-15 * top, "a tendency of zeros moves the plain level", high / top);
        }
    }
}

// From a start with an area mean (setVorticity keeps the given one in ζ), the forced step takes it
// out of each new level and the filter carries what the older level holds out within steps: after
// 30 steps both levels are at rounding (4e-16 of Σ A |ζ|, from a mean of 1e-2 of the largest ζ).
void checkMeanStart() {
    Setup s = waveAt(4);
    std::vector<double> start = s.flow.vorticity;
    double top = 0;
    for (double v : start) top = std::max(top, std::abs(v));
    for (double& v : start) v += 1e-2 * top;
    expect(setVorticity(s.flow, s.grid, s.stencil, s.poisson, start), "the start did not invert");
    const SphereForcing forcing = forcingOf(s.grid, true, true);
    for (int k = 0; k < 30; ++k) expect(stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, forcing), "a forced step was not accepted", k);
    const double worst = std::max(meanShare(s.grid, s.flow.vorticity), meanShare(s.grid, s.flow.before));
    expect(worst < 1e-14, "the start's mean stays in a forced time level", worst);
}

// A forced step whose inversion does not converge publishes nothing, and its retry computes what a
// clean forced step does; a forcing with a value not finite, a negative rate or the wrong size is
// refused before anything changes.
void checkRefused() {
    Setup s = waveAt(3);
    const SphereForcing forcing = forcingOf(s.grid, true, true);
    for (int phase = 0; phase < 3; ++phase) {
        const SphereFlow clean = s.flow;
        s.flow.limit = 0;
        expect(!stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, forcing), "an unconverged forced step was accepted", phase);
        expect(sameFlow(s.flow, clean), "an unconverged forced step changed the flow", phase);
        s.flow.limit = 60;
        SphereFlow control = clean;
        expect(stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, forcing) && stepSphereFlow(control, s.grid, s.stencil, s.poisson, forcing),
               "the forced retry was not accepted", phase);
        expect(sameFlow(s.flow, control), "a forced retry differs from a clean step", phase);
    }
    const SphereFlow clean = s.flow;
    SphereForcing wrong = forcing;
    wrong.friction[7] = -1e-9;
    expect(!stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, wrong) && sameFlow(s.flow, clean), "a negative friction rate was taken");
    wrong = forcing;
    wrong.friction[7] = std::numeric_limits<double>::infinity();
    expect(!stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, wrong) && sameFlow(s.flow, clean), "an infinite friction rate was taken");
    wrong = forcing;
    wrong.tendency[7] = std::numeric_limits<double>::quiet_NaN();
    expect(!stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, wrong) && sameFlow(s.flow, clean), "a tendency not finite was taken");
    wrong = forcing;
    wrong.tendency.pop_back();
    expect(!stepSphereFlow(s.flow, s.grid, s.stencil, s.poisson, wrong) && sameFlow(s.flow, clean), "a tendency of the wrong size was taken");
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkForcing() {
    checkEmpty();
    checkFriction();
    checkMean();
    checkPushed();
    checkMeanStart();
    checkRefused();
}
