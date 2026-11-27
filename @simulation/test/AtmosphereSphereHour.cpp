// The flow's hour (SphereHour, S4.1; Codex DREV-CLAUDE-007-01 point 5): a window that crosses the
// Courant limit after a step it took is taken back whole and, with twice the steps, lands on the bits
// of a clean run from the window's start; a window no try can take leaves the flow as it was; the
// steps rise at once and fall only after calm windows, so the leapfrog restarts only then.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereTransport.hpp"

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

// Two steps of 1800 s start at Courant 0.36; the spin-up of 40 % carries the second past 0.4 after
// the first was taken. The window is taken back and taken in four steps, bit for bit a clean run of
// four from its start, and the next window keeps four.
void checkTakenBack() {
    Spin s = spinning(2, 0.36);
    const SphereForcing forcing = spinUp(s, 0.4);
    const SphereFlow start = s.flow;
    SphereHour hour;
    hour.steps = 2;
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, forcing), "the window was not taken");
    expect(hour.refusals.size() == 1 && hour.refusals[0].reason == HourRefusal::Courant && hour.refusals[0].steps == 2 &&
               hour.refusals[0].at == 1,
           "the window was not taken back at its second step", double(hour.refusals.size()));
    SphereFlow clean = start;
    setStep(clean, Hour / 4);
    for (int k = 0; k < 4; ++k) expect(stepSphereFlow(clean, s.grid, s.stencil, s.poisson, forcing), "a clean step was not accepted", k);
    expect(sameFlow(s.flow, clean), "the window taken back differs from a clean run with its steps");
    expect(hour.steps == 4 && hour.courant <= hour.reserve * hour.limit, "the next window does not keep four steps", hour.steps);
}

// No try converges: four tries, the steps doubled each time, and the flow as before the window,
// history included; with a shortest step of 1000 s the second try stops on it.
void checkExhausted() {
    Spin s = spinning(2, 0.2);
    SphereHour hour;
    hour.steps = 2;
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, {}), "the first window was not taken");
    const SphereFlow start = s.flow;
    s.flow.limit = 0;
    expect(!advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, {}), "a window without convergence was taken");
    expect(hour.refusals.size() == 4, "the tries are not limited", double(hour.refusals.size()));
    for (size_t k = 0; k < hour.refusals.size(); ++k)
        expect(hour.refusals[k].reason == HourRefusal::Inversion && hour.refusals[k].steps == 2 << k, "a try was not doubled", double(k));
    s.flow.limit = start.limit;
    expect(sameFlow(s.flow, start), "a refused window changed the flow");
    hour.shortest = 1000;
    s.flow.limit = 0;
    expect(!advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, {}), "a window without convergence was taken");
    expect(hour.refusals.size() == 2 && hour.refusals[1].reason == HourRefusal::Shortest && hour.refusals[1].steps == 4,
           "the shortest step does not end the tries", double(hour.refusals.size()));
    s.flow.limit = start.limit;
    expect(sameFlow(s.flow, start), "a refused window changed the flow");
}

// Eight steps an hour where three would do (Courant 0.09 a step): the count stays for two windows,
// the leapfrog running on without a restart, and falls to three at the third. A spin-up of 40 %
// then needs four at once.
void checkHysteresis() {
    Spin s = spinning(8, 0.09);
    SphereHour hour;
    hour.steps = 8;
    for (int window = 0; window < 3; ++window) {
        expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, {}) && hour.refusals.empty(), "a calm window was not taken", window);
        if (window < 2) expect(hour.steps == 8 && s.flow.levels == 3 && s.flow.step == Hour / 8, "the steps fell before the calm windows", window);
    }
    expect(hour.steps == 3, "the steps did not fall after three calm windows", hour.steps);
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, {}) && hour.steps == 3, "three steps do not hold", hour.steps);
    expect(advanceSphereHour(hour, s.flow, s.grid, s.stencil, s.poisson, spinUp(s, 0.4)) && hour.refusals.empty(), "the spin-up was refused");
    expect(hour.steps == 4, "the steps did not rise at once", hour.steps);
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkHour() {
    checkTakenBack();
    checkExhausted();
    checkHysteresis();
}
