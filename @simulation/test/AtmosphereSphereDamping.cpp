// The biharmonic damping on its own (SphereDamping; S4 plan, binding correction 3, Codex's
// DREV-CLAUDE-007-01 point 3): the operator, the certified bound, and the discrete step with the
// filter on a flow too weak for advection to matter, for a fast and a slow mode and a local burst
// inside a face, on a face edge and at a cube vertex, at the production step and a quarter of it.
// About 310 lines: one proof of one operator, its cases sharing the course of a damped flow.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/fn/SphereDamping.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"
#include "core/fn/Hash.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/SphereGrid.hpp"

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Hour = 3600.0, Decay = 6 * Hour;

double weighted(const SphereGrid& grid, const std::vector<double>& a, const std::vector<double>& b) {
    double sum = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double area = grid.area[size_t(c)] * a[size_t(c)];
        const double term = area * b[size_t(c)];
        sum += term;
    }
    return sum;
}

void removeMean(const SphereGrid& grid, std::vector<double>& field) {
    double area = 0;
    for (double a : grid.area) area += a;
    const std::vector<double> ones(field.size(), 1.0);
    const double mean = weighted(grid, field, ones) / area;
    for (double& v : field) v -= mean;
}

std::vector<double> dampingOf(SphereDamping& damping, const SphereGrid& grid, const StencilMatrix& k, const std::vector<double>& field) {
    std::vector<double> tendency;
    addDamping(damping, grid, k, field, tendency);
    return tendency;
}

// The operator at level 5: −ν L² is symmetric in the A-weighted product and ⟨ζ, −ν L² ζ⟩_A =
// −ν ‖L ζ‖²_A, both to 1e-13 (measured 2e-15 and 6e-16), and zero on a constant. The certified bound
// lies 1.35 times above the estimate (1.29 to 1.38 on levels 4 to 7, measured once). On Y₄² it is
// not a consistent ∇⁴ (the hand-over's contract): 0.56 of the value off inside the faces, 3.7 on
// face edges, 3.2 at the vertices.
void checkOperator() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    const SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    const StencilMatrix& k = poisson.levels[0].stiffness;
    SphereDamping damping = makeSphereDamping(grid, k, Decay);
    expect(damping.bound > damping.estimate && damping.bound < 1.4 * damping.estimate, "the bound is not just above the estimate",
           damping.bound / damping.estimate);
    std::vector<double> a(size_t(grid.cells)), b(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) a[size_t(c)] = unit(21, c) - 0.5, b[size_t(c)] = unit(22, c) - 0.5;
    const std::vector<double> da = dampingOf(damping, grid, k, a), db = dampingOf(damping, grid, k, b);
    const double ab = weighted(grid, a, db), ba = weighted(grid, b, da);
    std::vector<double> ka;
    applyMatrix(k, a, ka);
    for (int c = 0; c < grid.cells; ++c) ka[size_t(c)] /= grid.area[size_t(c)];
    const double norm = damping.rate * weighted(grid, ka, ka), energy = weighted(grid, a, da);
    expect(std::abs(ab - ba) < 1e-13 * std::abs(ab), "the damping is not symmetric in the area-weighted product", (ab - ba) / ab);
    expect(std::abs(energy + norm) < 1e-13 * norm, "the damping's work is not −ν ‖L ζ‖²", (energy + norm) / norm);
    const std::vector<double> flat = dampingOf(damping, grid, k, std::vector<double>(size_t(grid.cells), 1.0));
    double top = 0, scale = 0;
    for (double v : flat) top = std::max(top, std::abs(v));
    for (double v : da) scale = std::max(scale, std::abs(v));
    expect(top < 1e-13 * scale, "a constant is damped", top / scale);
    // Y_4^2 against −ν (l(l+1)/R²)² inside the faces, on the face edges and at the vertices.
    const std::vector<double> y = harmonicField(grid, 4, 2);
    const std::vector<double> dy = dampingOf(damping, grid, k, y);
    const double lambda = -laplacianEigenvalue(4);
    std::vector<double> exact(y.size());
    for (size_t c = 0; c < y.size(); ++c) exact[c] = -damping.rate * lambda * lambda * y[c];
    const PlaceErrors errors = errorsOf(grid, dy, exact);
    double peak = 0;
    for (double v : exact) peak = std::max(peak, std::abs(v));
    expect(errors.worst[0] < 0.6 * peak && errors.worst[1] < 3.9 * peak && errors.worst[2] < 3.4 * peak, "the damping on Y₄² moved",
           errors.worst[1] / peak);
}

struct Run {
    double amplitude{}, enstrophy{}, energy{}, spill{};
};

// ½ Σ A ζ², ½ ψᵀ K ψ, and the mode's amplitude and what lies outside it.
Run measure(const SphereGrid& grid, const StencilMatrix& k, const SphereFlow& flow, const std::vector<double>& mode) {
    Run run;
    const double square = weighted(grid, mode, mode);
    run.amplitude = weighted(grid, flow.vorticity, mode) / square;
    run.enstrophy = 0.5 * weighted(grid, flow.vorticity, flow.vorticity);
    std::vector<double> kpsi;
    applyMatrix(k, flow.stream, kpsi);
    double e = 0;
    for (int c = 0; c < grid.cells; ++c) {
        const double term = flow.stream[size_t(c)] * kpsi[size_t(c)];
        e += term;
    }
    run.energy = 0.5 * e;
    std::vector<double> rest = flow.vorticity;
    for (size_t c = 0; c < rest.size(); ++c) rest[c] -= run.amplitude * mode[c];
    run.spill = std::sqrt(weighted(grid, rest, rest) / (2 * run.enstrophy));
    return run;
}

// Steps a flow of `start` (scaled to 1e-12 1/s, too weak for advection to matter: its tendency goes
// as ζ², the damping's as ζ) with the damping alone for `hours`, and reports the worst growth of
// amplitude, enstrophy and energy from one step to the next, and the end against the start.
struct Course {
    double grew{}, flips{}, end{}, enstrophyEnd{}, energyEnd{}, spillStart{}, spillEnd{}, expected{};
};

Course course(int level, const std::vector<double>& shape, const std::vector<double>& mode, double step, double hours) {
    const SphereGrid grid = makeSphereGrid(level);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    const StencilMatrix& k = poisson.levels[0].stiffness;
    SphereDamping damping = makeSphereDamping(grid, k, Decay);
    SphereFlow flow = makeSphereFlow(grid, 0);
    std::vector<double> start = shape;
    removeMean(grid, start);
    double top = 0;
    for (double v : start) top = std::max(top, std::abs(v));
    for (double& v : start) v *= 1e-12 / top;
    expect(setVorticity(flow, grid, stencil, poisson, start), "the start did not invert");
    setStep(flow, step);
    std::vector<double> km;
    applyMatrix(k, mode, km);
    for (size_t c = 0; c < km.size(); ++c) km[c] /= grid.area[c];
    Course result;
    // An eigenmode of L keeps 1 − s ν λ² over each span s: the leapfrog's 2Δt from the older level.
    // A mode that is not one loses ⟨m, ν L² m⟩_A / ⟨m, m⟩_A = ν ‖L m‖²_A / ‖m‖²_A of its amplitude.
    const double bent = weighted(grid, km, km) / weighted(grid, mode, mode);
    const double rateOfMode = damping.rate * bent;
    const double kept = 1 - 2 * step * rateOfMode;
    result.expected = std::pow(kept, hours * Hour / (2 * step));
    const Run first = measure(grid, k, flow, mode);
    Run last = first;
    result.spillStart = first.spill;
    SphereForcing forcing;
    const int steps = int(std::lround(hours * Hour / step));
    for (int s = 0; s < steps; ++s) {
        forcing.tendency.clear();
        addDamping(damping, grid, k, baseLevel(flow), forcing.tendency);
        expect(stepSphereFlow(flow, grid, stencil, poisson, forcing), "a damped step was not accepted", s);
        const Run now = measure(grid, k, flow, mode);
        result.grew = std::max({result.grew, std::abs(now.amplitude) / std::abs(last.amplitude) - 1, now.enstrophy / last.enstrophy - 1,
                                now.energy / last.energy - 1});
        if (now.amplitude * last.amplitude < 0) ++result.flips;
        last = now;
    }
    result.end = last.amplitude / first.amplitude;
    result.enstrophyEnd = last.enstrophy / first.enstrophy;
    result.energyEnd = last.energy / first.energy;
    result.spillEnd = last.spill;
    return result;
}

std::vector<double> fastMode(int level) {
    const SphereGrid grid = makeSphereGrid(level);
    const SphereStencil stencil = makeSphereStencil(grid);
    const SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    const StencilMatrix& k = poisson.levels[0].stiffness;
    std::vector<double> x(size_t(grid.cells)), kx;
    for (int c = 0; c < grid.cells; ++c) {
        const Cell at = cellAt(grid, c);
        x[size_t(c)] = ((at.i + at.j) % 2 == 0 ? 1.0 : -1.0) + 1e-3 * double(c % 7);
    }
    for (int s = 0; s < 400; ++s) {
        applyMatrix(k, x, kx);
        double top = 0;
        for (int c = 0; c < grid.cells; ++c) x[size_t(c)] = kx[size_t(c)] / grid.area[size_t(c)], top = std::max(top, std::abs(x[size_t(c)]));
        for (double& v : x) v /= top;
    }
    return x;
}

// A burst of one cell (course takes its mean out).
std::vector<double> burst(const SphereGrid& grid, int cell) {
    std::vector<double> x(size_t(grid.cells), 0.0);
    x[size_t(cell)] = 1;
    return x;
}

// The fast mode (the top of A⁻¹K, by power iteration) and a slow one (Y₄²) for 6 h at the production
// step (900 s) and a quarter of it: amplitude, enstrophy and energy never grow from one step to the
// next, the mode keeps its sign, and it keeps (1 − 2Δt ν λ²) per two steps to 1e-3 (fast at 900 s:
// 0.352 where exp(−t ν λ²) gives 0.368, the leapfrog's lag). The slow mode keeps 0.99993: the damping
// is for the grid scale; what it loses, 7e-5, is ν ‖L m‖² over the time to 1e-3 (measured 3e-5).
// What leaves the mode stays small: 6e-4 of the fast mode, 6e-5 of Y₄².
void checkModes() {
    const int level = 5;
    const SphereGrid grid = makeSphereGrid(level);
    const std::vector<double> fast = fastMode(level), slow = harmonicField(grid, 4, 2);
    for (double step : {900.0, 225.0}) {
        for (const std::vector<double>* mode : {&fast, &slow}) {
            const Course c = course(level, *mode, *mode, step, 6);
            expect(c.grew < 1e-12 && c.flips == 0, "the damping let a mode grow or flip", c.grew);
            expect(std::abs(c.end / c.expected - 1) < 1e-3, "a mode does not keep 1 − 2Δt ν λ² per two steps", c.end / c.expected - 1);
            expect(c.spillEnd < 1e-3, "the damping moved a mode into others", c.spillEnd);
            const double lost = (1 - c.end) / (1 - c.expected) - 1;
            expect(std::abs(lost) < 1e-3, "a mode does not lose what ν ‖L m‖² takes", lost);
        }
    }
}

// A burst of one cell inside a face, at a face edge's midpoint and at a cube vertex, for an hour at
// 900 s and 225 s: nothing grows from one step to the next. The seams damp more than the inside:
// the enstrophy keeps 0.94 inside, 0.87 at the face edge and 0.89 at the vertex (900 s), since K's
// error there is amplified by its second application.
void checkBursts() {
    const int level = 5;
    const SphereGrid grid = makeSphereGrid(level);
    const int side = grid.side;
    const int inside = indexOf(grid, {0, level, side / 2, side / 2}), seam = indexOf(grid, {0, level, side / 2, 0});
    const int vertex = indexOf(grid, {0, level, 0, 0});
    for (double step : {900.0, 225.0}) {
        double kept[3]{};
        int k = 0;
        for (int cell : {inside, seam, vertex}) {
            const std::vector<double> b = burst(grid, cell);
            const Course c = course(level, b, b, step, 1);
            expect(c.grew < 1e-12 && c.flips == 0, "a burst grew", c.grew);
            kept[k++] = c.enstrophyEnd;
        }
        expect(kept[0] > 0.93 && kept[0] < 0.95 && kept[1] > 0.86 && kept[1] < 0.89 && kept[2] > 0.88 && kept[2] < 0.90,
               "a burst's enstrophy moved from the measured", kept[1]);
    }
}

// The bound. With the filter (strength 0.1, split 0.53) a mode's two roots per step stay real up to
// s ν λ² = 0.976, the physical one positive and dominant, the leapfrog's computational one negative
// below 0.9746, and the step is stable up to 2.06; between 0.976 and 2.06 the mode decays while its
// sign swings. So, started by the forward step, the fast mode at s ν λ² = 0.95 keeps its sign, at
// 1.05 it swings and still decays, and at 2.5 it swings about every second step and grows. The hour's check on the
// bound holds s ν λ_max² at (estimate / bound)², 0.55 at level 5.
void checkBound() {
    const int level = 5;
    const SphereGrid grid = makeSphereGrid(level);
    const std::vector<double> fast = fastMode(level);
    const SphereStencil stencil = makeSphereStencil(grid);
    const SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    const SphereDamping probe = makeSphereDamping(grid, poisson.levels[0].stiffness, Decay);
    const double fastest = probe.rate * probe.estimate * probe.estimate;
    const double below = 0.95 / (2 * fastest), above = 1.05 / (2 * fastest);
    const Course held = course(level, fast, fast, below, 12 * below / Hour);
    expect(held.grew < 1e-12 && held.flips == 0 && std::abs(held.end) < 1e-6, "the fast mode below 0.976 swung or grew", held.grew);
    const Course swung = course(level, fast, fast, above, 12 * above / Hour);
    expect(swung.flips > 0 && std::abs(swung.end) < 1e-6, "the fast mode above 0.976 did not swing and decay", swung.flips);
    const double past = 2.5 / (2 * fastest);
    const Course broken = course(level, fast, fast, past, 60 * past / Hour);
    expect(broken.flips > 20 && broken.end > 1, "past the bound the fast mode does not break away", broken.end);
}

// The hour keeps the damping's bound: with stiffest 1/1800 s a window needs 2Δt ≤ 1800 s, four steps;
// from one it starts at four with nothing refused, and calm windows on a still flow do not lower the
// steps below four. At a stiffest of 41/7200 s and 57/7200 s, where ceil(2 W S) and the try's own
// test round apart, up and down, the window starts at the fewest count the try's test passes. A shortest step of 1800 s, two steps at most,
// cannot meet it: the window is refused for the damping, the flow and the steps as they were.
void checkHourBound() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, 0);
    std::vector<double> start = harmonicField(grid, 4, 2);
    for (double& v : start) v *= 1e-12;
    expect(setVorticity(flow, grid, stencil, poisson, start), "the start did not invert");
    const SphereFlow still = flow;
    SphereHour hour;
    hour.steps = 1;
    hour.stiffest = 1 / 1800.0;
    expect(advanceSphereHour(hour, flow, grid, stencil, poisson, SphereForcing{}), "the window was not taken");
    expect(hour.refusals.empty() && flow.step == Hour / 4, "the damping's bound did not set the steps", flow.step);
    for (int window = 0; window < 4; ++window)
        expect(advanceSphereHour(hour, flow, grid, stencil, poisson, SphereForcing{}) && hour.steps == 4, "calm windows went below the bound",
               hour.steps);
    flow = still;
    hour = SphereHour{};
    hour.stiffest = 41 / 7200.0;
    expect(advanceSphereHour(hour, flow, grid, stencil, poisson, SphereForcing{}) && hour.refusals.empty(), "the rounded count was refused",
           double(hour.refusals.size()));
    expect(2 * flow.step * hour.stiffest <= 1, "the hour stepped past the damping's bound", 2 * flow.step * hour.stiffest);
    flow = still;
    hour = SphereHour{};
    hour.stiffest = 57 / 7200.0;
    expect(advanceSphereHour(hour, flow, grid, stencil, poisson, SphereForcing{}) && flow.step == Hour / 57, "the rounded count was not the fewest",
           Hour / flow.step);
    flow = still;
    hour = SphereHour{};
    hour.stiffest = 1 / 1800.0;
    hour.shortest = 1800;
    const bool taken = advanceSphereHour(hour, flow, grid, stencil, poisson, SphereForcing{});
    expect(!taken && hour.refusals.size() == 1 && hour.refusals[0].reason == HourRefusal::Damping && hour.steps == 1 && !flow.started &&
               flow.vorticity == still.vorticity,
           "a window that cannot meet the damping was not refused as it came", double(hour.refusals.size()));
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkDamping() {
    checkOperator();
    checkModes();
    checkBursts();
    checkBound();
    checkHourBound();
}
