// The flow's hour (SphereHour, S4.1; S4 plan, binding correction 5): a window that crosses the
// Courant limit after a step it took is taken back whole and, with twice the steps, lands on the bits
// of a clean run from the window's start; a window no try can take leaves the flow as it was; the
// steps rise at once and fall only after calm windows, so the leapfrog restarts only then.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereTransport.hpp"

#include <functional>
#include <limits>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Hour = 3600.0;

bool sameFlow(const SphereFlow& a, const SphereFlow& b) {
    return a.vorticity == b.vorticity && a.before == b.before && a.stream == b.stream && a.flux == b.flux && a.started == b.started &&
           a.step == b.step && a.levels == b.levels && a.streamBack == b.streamBack && a.streamBackTwo == b.streamBackTwo &&
           a.productBack == b.productBack && a.productBackTwo == b.productBackTwo;
}

struct Spin {
    SphereGrid grid = makeSphereGrid(4);
    SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> shape;   // 2 sin φ, the vorticity of a unit solid rotation about the axis
};

// A solid rotation about the planet's axis, steady under advection, fast enough that an hour in
// `steps` steps reaches the outflow Courant number `courant`.
Spin spinning(int steps, double courant) {
    Spin s;
    s.shape.resize(size_t(s.grid.cells));
    for (int c = 0; c < s.grid.cells; ++c) s.shape[size_t(c)] = 2 * s.grid.centre[size_t(c)].z;
    std::vector<double> vorticity = s.shape;
    for (double& v : vorticity) v *= 1e-5;
    expect(setVorticity(s.flow, s.grid, s.stencil, s.poisson, vorticity), "the solid rotation did not invert");
    const double scale = courant / (outflowCourant(s.grid, s.flow.flux, Hour / steps));
    for (double& v : vorticity) v *= scale;
    expect(setVorticity(s.flow, s.grid, s.stencil, s.poisson, vorticity), "the solid rotation did not invert");
    return s;
}

// The rotation's vorticity grows by `share` of its start in an hour.
SphereForcing spinUp(const Spin& s, double share) {
    SphereForcing forcing;
    forcing.tendency.resize(size_t(s.grid.cells));
    for (int c = 0; c < s.grid.cells; ++c) forcing.tendency[size_t(c)] = share * s.flow.vorticity[size_t(c)] / Hour;
    return forcing;
}

// The forcing from the flow each step: the vorticity grows at 40 % of itself an hour, the state's
// own (a closure that reads ζ), counting the calls.
struct Growing {
    int calls = 0;
    void operator()(const SphereFlow& flow, SphereForcing& forcing) {
        ++calls;
        forcing.tendency.resize(flow.vorticity.size());
        for (size_t c = 0; c < flow.vorticity.size(); ++c) forcing.tendency[c] = 0.4 * flow.vorticity[c] / Hour;
    }
};

// Two steps of 1800 s start at Courant 0.36; the spin-up of 40 % an hour, read from the flow before
// each step, carries the second past 0.4 after the first was taken. The window is taken back and
// taken in four steps, bit for bit a clean run of four from its start with the same source, which was
// asked before each of the five steps taken; the next window keeps four and its calm count is zero.
void checkTakenBack() {
    Spin s = spinning(2, 0.36);
    const SphereFlow start = s.flow;
    SphereHour hour;
    hour.steps = 2;
    Growing growing;
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, std::ref(growing)), "the window was not taken");
    expect(growing.calls == 5, "the source was not asked before every step", growing.calls);
    expect(hour.refusals.size() == 1 && hour.refusals[0].reason == HourRefusal::Courant && hour.refusals[0].steps == 2 &&
               hour.refusals[0].at == 1,
           "the window was not taken back at its second step", double(hour.refusals.size()));
    SphereFlow clean = start;
    setStep(clean, Hour / 4);
    Growing again;
    SphereForcing forcing;
    for (int k = 0; k < 4; ++k) {
        again(clean, forcing);
        expect(stepSphereFlow(clean, s.grid, s.stencil, s.poisson, forcing), "a clean step was not accepted", k);
    }
    expect(sameFlow(s.flow, clean), "the window taken back differs from a clean run with its steps");
    expect(hour.steps == 4 && hour.calm == 0 && hour.courant <= hour.reserve * hour.limit, "the next window does not keep four steps",
           hour.steps);
}

// The steps never go below `shortest`: a rise that would is capped (two steps where the Courant
// number, 0.36 rising to 0.41, asks for three, at 1500 s the shortest), and the next window, whose
// first step starts at 0.41, is refused there and ends the tries, since its two steps are already the
// most allowed.
// A forcing that is not admissible ends them at once, the flow as it was.
void checkBounds() {
    Spin s = spinning(2, 0.36);
    SphereHour hour;
    hour.steps = 2;
    hour.shortest = 1500;
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, spinUp(s, 0.15)) && hour.refusals.empty(), "the window was not taken");
    expect(hour.courant > hour.reserve * hour.limit && hour.steps == 2, "the rise was not capped at the shortest step", hour.steps);
    const SphereFlow start = s.flow;
    expect(!advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}), "a window past the limit was taken");
    expect(hour.refusals.size() == 2 && hour.refusals[0].reason == HourRefusal::Courant && hour.refusals[0].at == 0 &&
               hour.refusals[1].reason == HourRefusal::Shortest && hour.refusals[1].steps == 2 && sameFlow(s.flow, start),
           "doubling past the shortest step was tried", double(hour.refusals.size()));
    SphereForcing wrong = spinUp(s, 0.1);
    wrong.tendency[3] = std::numeric_limits<double>::quiet_NaN();
    hour.shortest = 30, hour.steps = 4;
    expect(!advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, wrong), "a forcing not finite was taken");
    expect(hour.refusals.size() == 1 && hour.refusals[0].reason == HourRefusal::Forcing && sameFlow(s.flow, start),
           "a forcing not finite was retried or changed the flow", double(hour.refusals.size()));
}

// No try converges: four tries, the steps doubled each time, and the flow as before the window,
// history included; with a shortest step of 1000 s (three steps at most) the tries take two and
// three steps and stop there.
void checkExhausted() {
    Spin s = spinning(2, 0.2);
    SphereHour hour;
    hour.steps = 2;
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}), "the first window was not taken");
    const SphereFlow start = s.flow;
    s.flow.limit = 0;
    expect(!advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}), "a window without convergence was taken");
    expect(hour.refusals.size() == 4, "the tries are not limited", double(hour.refusals.size()));
    for (size_t k = 0; k < hour.refusals.size(); ++k)
        expect(hour.refusals[k].reason == HourRefusal::Inversion && hour.refusals[k].steps == 2 << k, "a try was not doubled", double(k));
    s.flow.limit = start.limit;
    expect(sameFlow(s.flow, start), "a refused window changed the flow");
    hour.shortest = 1000;
    s.flow.limit = 0;
    expect(!advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}), "a window without convergence was taken");
    expect(hour.refusals.size() == 3 && hour.refusals[1].reason == HourRefusal::Inversion && hour.refusals[1].steps == 3 &&
               hour.refusals[2].reason == HourRefusal::Shortest,
           "the shortest step does not end the tries", double(hour.refusals.size()));
    s.flow.limit = start.limit;
    expect(sameFlow(s.flow, start), "a refused window changed the flow");
}

// Eight steps an hour where three would do (Courant 0.09 a step): the count stays for two windows
// and falls to three at the third; the three windows are bit for bit one run of 24 steps, so the
// leapfrog ran on without a restart. A spin-up of 40 % then needs four at once.
void checkHysteresis() {
    Spin s = spinning(8, 0.09);
    SphereFlow continuous = s.flow;
    setStep(continuous, Hour / 8);
    for (int k = 0; k < 24; ++k) expect(stepSphereFlow(continuous, s.grid, s.stencil, s.poisson), "a continuous step was not accepted", k);
    SphereHour hour;
    hour.steps = 8;
    for (int window = 0; window < 3; ++window) {
        expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}) && hour.refusals.empty(), "a calm window was not taken",
               window);
        if (window < 2) expect(hour.steps == 8, "the steps fell before the calm windows", window);
    }
    expect(sameFlow(s.flow, continuous), "the calm windows restarted the leapfrog");
    expect(hour.steps == 3, "the steps did not fall after three calm windows", hour.steps);
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}) && hour.steps == 3, "three steps do not hold", hour.steps);
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, spinUp(s, 0.4)) && hour.refusals.empty(), "the spin-up was refused");
    expect(hour.steps == 4, "the steps did not rise at once", hour.steps);
}

// A window taken only at its second try does not count as calm: two steps from Courant 0.41 are
// refused at the first; four would do with three (0.205 a step), which `lower` 0.9 calls calm, and
// one calm window short of `patience` would lower them, but after a retry the four stay.
void checkRetriedNotCalm() {
    Spin s = spinning(2, 0.41);
    SphereHour hour;
    hour.steps = 2;
    hour.lower = 0.9;
    hour.calm = hour.patience - 1;
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, SphereForcing{}) && hour.refusals.size() == 1,
           "the window was not taken at its second try", double(hour.refusals.size()));
    expect(hour.steps == 4 && hour.calm == 0, "a retried window counted as calm", hour.steps);
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkHour() {
    checkTakenBack();
    checkExhausted();
    checkBounds();
    checkRetriedNotCalm();
    checkHysteresis();
}
