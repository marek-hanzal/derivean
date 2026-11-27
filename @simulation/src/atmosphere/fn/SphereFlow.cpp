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

// Drops the older time levels, so the next step is a forward step and its first guess the last ψ.
// Only with ζ, ψ and the fluxes consistent: after setVorticity's inversion or when the step changes.
void restart(SphereFlow& flow) {
    flow.before = flow.vorticity;
    flow.started = false;
    flow.levels = 1;
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
    restart(flow);
    return true;
}

void setStep(SphereFlow& flow, double step) {
    assert(step > 0);
    if (step == flow.step) return;
    flow.step = step;
    restart(flow);
}

namespace {

// Whether a forcing can be applied: its vectors empty or one value per cell, every value finite and
// every friction rate at least zero.
bool admissible(const SphereForcing& forcing, size_t cells) {
    if (!forcing.tendency.empty()) {
        if (forcing.tendency.size() != cells) return false;
        for (double t : forcing.tendency)
            if (!std::isfinite(t)) return false;
    }
    if (!forcing.friction.empty()) {
        if (forcing.friction.size() != cells) return false;
        for (double r : forcing.friction)
            if (!(r >= 0 && std::isfinite(r))) return false;
    }
    return true;
}

// Into flow.trialVorticity, the forced level next = keep · (x − c) of stepSphereFlow with a forcing.
// The products are statements of their own, so neither platform fuses them (docs/windows.md).
void forcedLevel(SphereFlow& flow, const SphereGrid& grid, const SphereForcing& forcing, const std::vector<double>& base, double span) {
    const size_t n = size_t(grid.cells);
    const bool pushed = !forcing.tendency.empty(), braked = !forcing.friction.empty();
    flow.keep.resize(n);
    double held = 0, weight = 0;
    for (size_t c = 0; c < n; ++c) {
        const double total = pushed ? flow.tendency[c] + forcing.tendency[c] : flow.tendency[c];
        const double change = span * total;
        const double x = base[c] + change;
        double keep = 1;
        if (braked) {
            const double brake = span * forcing.friction[c];
            keep = 1 / (1 + brake);
        }
        flow.trialVorticity[c] = x;
        flow.keep[c] = keep;
        const double kept = grid.area[c] * keep;
        const double content = kept * x;
        held += content;
        weight += kept;
    }
    const double centre = held / weight;
    for (size_t c = 0; c < n; ++c) {
        const double off = flow.trialVorticity[c] - centre;
        flow.trialVorticity[c] = flow.keep[c] * off;
    }
}

// The candidate's first guess and inversion, then the candidate becomes the state.
bool finishStep(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson) {
    const bool withProduct = guessStream(flow, grid, poisson);
    if (!invertTrial(flow, grid, stencil, poisson, withProduct)) return false;
    acceptTrial(flow);
    std::swap(flow.before, flow.trialBefore);
    flow.started = true;
    return true;
}

} // namespace

bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson) {
    if (!(flow.step > 0)) return false;   // also NaN
    const double step = flow.step;
    const size_t n = size_t(grid.cells);
    tendencyOf(flow, grid, stencil, flow.vorticity, flow.tendency);
    flow.trialVorticity.resize(n);
    flow.trialBefore.resize(n);
    if (!flow.started) {
        for (size_t c = 0; c < n; ++c) {
            flow.trialVorticity[c] = flow.vorticity[c] + step * flow.tendency[c];
            flow.trialBefore[c] = flow.vorticity[c];
        }
    } else {
        for (size_t c = 0; c < n; ++c) {
            const double next = flow.before[c] + 2 * step * flow.tendency[c];
            const double curve = 0.5 * FilterStrength * (flow.before[c] - 2 * flow.vorticity[c] + next);
            flow.trialBefore[c] = flow.vorticity[c] + FilterSplit * curve;
            flow.trialVorticity[c] = next - (1 - FilterSplit) * curve;
        }
    }
    return finishStep(flow, grid, stencil, poisson);
}

bool stepSphereFlow(SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                    const SphereForcing& forcing) {
    if (forcing.tendency.empty() && forcing.friction.empty()) return stepSphereFlow(flow, grid, stencil, poisson);
    if (!(flow.step > 0) || !admissible(forcing, size_t(grid.cells))) return false;
    const size_t n = size_t(grid.cells);
    tendencyOf(flow, grid, stencil, flow.vorticity, flow.tendency);
    flow.trialVorticity.resize(n);
    flow.trialBefore.resize(n);
    if (!flow.started) {
        forcedLevel(flow, grid, forcing, flow.vorticity, flow.step);
        flow.trialBefore = flow.vorticity;
    } else {
        forcedLevel(flow, grid, forcing, flow.before, 2 * flow.step);
        for (size_t c = 0; c < n; ++c) {
            const double next = flow.trialVorticity[c];
            const double curve = 0.5 * FilterStrength * (flow.before[c] - 2 * flow.vorticity[c] + next);
            flow.trialBefore[c] = flow.vorticity[c] + FilterSplit * curve;
            flow.trialVorticity[c] = next - (1 - FilterSplit) * curve;
        }
    }
    return finishStep(flow, grid, stencil, poisson);
}

} // namespace derivean::sim
