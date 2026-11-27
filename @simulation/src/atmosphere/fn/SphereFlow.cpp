#include "atmosphere/fn/SphereFlow.hpp"

#include "astronomy/fn/DeepTime.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SphereFlowGuess.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <utility>

namespace derivean::sim {
namespace {

// The Robert–Asselin–Williams filter of the leapfrog step (Robert 1966, Asselin 1972, Williams 2009):
// a copy of FilterStrength and FilterSplit in atmosphere/fn/Circulation.cpp, which owns them until
// the switch (S4) retires the flat-world step.
constexpr double FilterStrength = 0.1;
constexpr double FilterSplit = 0.53;

// Inverts the candidate vorticity into the candidate stream function and fluxes, starting from the
// guess in flow.trialStream (whose K is flow.trialProduct when `withProduct`); the accepted state is
// not touched.
bool invertTrial(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                 bool withProduct = false) {
    flow.inversion = solveSpherePoisson(poisson, grid, flow.trialVorticity, flow.trialStream, flow.tolerance, flow.limit,
                                        withProduct ? &flow.trialProduct : nullptr);
    if (!flow.inversion.converged) return false;
    // Below a tolerance of 1e-10 the solver formed K of the candidate ψ: kept for the next guess.
    if (flow.inversion.product) std::swap(flow.trialProduct, poisson.product);
    streamFluxOf(grid, stencil, flow.trialStream, flow.trialFlux, flow.atVertex);
    return true;
}

// The candidate becomes the state; the accepted ψ move one level back.
void acceptTrial(SphereFlow& flow) {
    std::swap(flow.vorticity, flow.trialVorticity);
    std::swap(flow.streamBackTwo, flow.streamBack);
    std::swap(flow.streamBack, flow.stream);
    std::swap(flow.stream, flow.trialStream);
    std::swap(flow.productBackTwo, flow.productBack);
    std::swap(flow.productBack, flow.product);
    std::swap(flow.flux, flow.trialFlux);
    flow.known = flow.inversion.product;
    // Without the projection no K ψ was formed, so only the new ψ itself counts.
    flow.levels = flow.projection > 0 ? std::min(flow.levels + 1, 3) : 1;
}

// −∇·(v q) per cell with the edge value of q the mean of its two cells moved to the edge's midpoint
// by the edge gradient (EdgeStencil::offset, read as SphereStencil::carry).
void tendencyOf(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, const std::vector<double>& vorticity,
                std::vector<double>& tendency) {
    std::vector<double>& absolute = flow.absolute;
    absolute.resize(size_t(grid.cells));
    for (size_t c = 0; c < absolute.size(); ++c) absolute[c] = vorticity[c] + flow.coriolis[c];
    std::vector<double>& at = flow.atVertex;
    vertexValues(grid, stencil, absolute, at);
    assert(stencil.carry.size() == grid.edge.size() && stencil.topology.size() == grid.edge.size());
    tendency.assign(size_t(grid.cells), 0.0);
    for (size_t e = 0; e < stencil.topology.size(); ++e) {
        const EdgeTopology& edge = stencil.topology[e];
        const std::array<double, 2>& carry = stencil.carry[e];
        const size_t l = size_t(edge.left), r = size_t(edge.right);
        const double moved = carry[0] * (absolute[r] - absolute[l]) + carry[1] * (at[size_t(edge.to)] - at[size_t(edge.from)]);
        const double carried = flow.flux[e] * (0.5 * (absolute[l] + absolute[r]) + moved);
        tendency[l] -= carried;
        tendency[r] += carried;
    }
    for (int c = 0; c < grid.cells; ++c) tendency[size_t(c)] /= grid.area[size_t(c)];
}

} // namespace

void restartHistory(SphereFlow& flow) {
    flow.before = flow.vorticity;
    flow.started = false;
    flow.levels = 1;
}

double rotationRateAt(double age) { return 2 * std::numbers::pi / deepTimeAt(age).siderealDay; }

SphereFlow makeSphereFlow(const SphereGrid& grid, double rotation) {
    SphereFlow flow;
    flow.rotation = rotation;
    flow.coriolis.resize(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) flow.coriolis[size_t(c)] = 2 * rotation * grid.centre[size_t(c)].z;   // sin φ
    flow.vorticity.assign(size_t(grid.cells), 0.0);
    flow.before = flow.vorticity;
    flow.stream = flow.vorticity;
    flow.flux.assign(grid.edge.size(), 0.0);
    return flow;
}

bool setVorticity(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                  const std::vector<double>& vorticity) {
    assert(vorticity.size() == size_t(grid.cells));
    flow.trialVorticity = vorticity;
    flow.trialStream = flow.stream;
    if (!invertTrial(flow, grid, stencil, poisson)) return false;
    acceptTrial(flow);
    restartHistory(flow);
    return true;
}

void setStep(SphereFlow& flow, double step) {
    assert(step > 0);
    if (step == flow.step) return;
    flow.step = step;
    restartHistory(flow);
}

const std::vector<double>& baseLevel(const SphereFlow& flow) { return flow.started ? flow.before : flow.vorticity; }

bool admissibleTendency(const std::vector<double>& tendency, size_t cells) {
    if (tendency.empty()) return true;
    if (tendency.size() != cells) return false;
    for (double t : tendency)
        if (!std::isfinite(t)) return false;
    return true;
}

bool admissibleFriction(const std::vector<double>& friction, size_t cells) {
    if (friction.empty()) return true;
    if (friction.size() != cells) return false;
    for (double r : friction)
        if (!(r >= 0 && std::isfinite(r))) return false;
    return true;
}

bool admissibleForcing(const SphereForcing& forcing, size_t cells) {
    return admissibleTendency(forcing.tendency, cells) && admissibleFriction(forcing.friction, cells);
}

namespace {

// Into flow.trialVorticity, the new level without forcing: ζₙ + Δt F in the forward step, ζₙ₋₁ + 2Δt F
// in the leapfrog, F the advection's tendency.
void plainLevel(SphereFlow& flow, size_t n) {
    const double step = flow.step;
    if (!flow.started) {
        for (size_t c = 0; c < n; ++c) flow.trialVorticity[c] = flow.vorticity[c] + step * flow.tendency[c];
    } else {
        for (size_t c = 0; c < n; ++c) flow.trialVorticity[c] = flow.before[c] + 2 * step * flow.tendency[c];
    }
}

// Into flow.trialVorticity, the forced level next = keep · (x − c) of stepSphereFlow with a forcing;
// flow.trialBefore holds keep per cell until the filter or the forward step writes it. The products
// in the sums are statements of their own, so neither platform fuses them (docs/windows.md).
void forcedLevel(SphereFlow& flow, const SphereGrid& grid, const std::vector<double>& tendency, const std::vector<double>& friction) {
    const size_t n = size_t(grid.cells);
    const bool pushed = !tendency.empty(), braked = !friction.empty();
    const std::vector<double>& base = baseLevel(flow);
    const double span = flow.started ? 2 * flow.step : flow.step;
    std::vector<double>& keeps = flow.trialBefore;
    double held = 0, weight = 0;
    for (size_t c = 0; c < n; ++c) {
        const double total = pushed ? flow.tendency[c] + tendency[c] : flow.tendency[c];
        const double change = span * total;
        const double x = base[c] + change;
        double keep = 1;
        if (braked) {
            const double brake = span * friction[c];
            keep = 1 / (1 + brake);
        }
        flow.trialVorticity[c] = x;
        keeps[c] = keep;
        const double kept = grid.area[c] * keep;
        const double content = kept * x;
        held += content;
        weight += kept;
    }
    const double centre = held / weight;
    for (size_t c = 0; c < n; ++c) {
        const double off = flow.trialVorticity[c] - centre;
        flow.trialVorticity[c] = keeps[c] * off;
    }
}

} // namespace

bool stepAdmittedFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                      const std::vector<double>& tendency, const std::vector<double>& friction) {
    if (!(flow.step > 0)) return false;   // also NaN
    const size_t n = size_t(grid.cells);
    if ((!tendency.empty() && tendency.size() != n) || (!friction.empty() && friction.size() != n)) return false;
    assert(admissibleTendency(tendency, n) && admissibleFriction(friction, n));
    const bool forced = !tendency.empty() || !friction.empty();
    tendencyOf(flow, grid, stencil, flow.vorticity, flow.tendency);
    flow.trialVorticity.resize(n);
    flow.trialBefore.resize(n);
    if (forced) forcedLevel(flow, grid, tendency, friction);
    else plainLevel(flow, n);
    if (!flow.started) {
        for (size_t c = 0; c < n; ++c) flow.trialBefore[c] = flow.vorticity[c];
    } else {
        for (size_t c = 0; c < n; ++c) {
            const double next = flow.trialVorticity[c];
            const double curve = 0.5 * FilterStrength * (flow.before[c] - 2 * flow.vorticity[c] + next);
            flow.trialBefore[c] = flow.vorticity[c] + FilterSplit * curve;
            flow.trialVorticity[c] = next - (1 - FilterSplit) * curve;
        }
    }
    const bool withProduct = guessStream(flow, grid, poisson);
    if (!invertTrial(flow, grid, stencil, poisson, withProduct)) return false;
    acceptTrial(flow);
    std::swap(flow.before, flow.trialBefore);
    flow.started = true;
    return true;
}

bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                    const SphereForcing& forcing) {
    if (!admissibleForcing(forcing, size_t(grid.cells))) return false;
    return stepAdmittedFlow(flow, grid, stencil, poisson, forcing.tendency, forcing.friction);
}

bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson) {
    static const std::vector<double> None;
    return stepAdmittedFlow(flow, grid, stencil, poisson, None, None);
}

} // namespace derivean::sim
