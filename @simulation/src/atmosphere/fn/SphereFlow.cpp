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
    assert(stencil.carry.size() == grid.edge.size());
    tendency.assign(size_t(grid.cells), 0.0);
    for (size_t e = 0; e < grid.edge.size(); ++e) {
        const SphereEdge& edge = grid.edge[e];
        const std::array<double, 2>& carry = stencil.carry[e];
        const size_t l = size_t(edge.left), r = size_t(edge.right);
        const double moved = carry[0] * (absolute[r] - absolute[l]) + carry[1] * (at[size_t(edge.to)] - at[size_t(edge.from)]);
        const double carried = flow.flux[e] * (0.5 * (absolute[l] + absolute[r]) + moved);
        tendency[l] -= carried;
        tendency[r] += carried;
    }
    for (int c = 0; c < grid.cells; ++c) tendency[size_t(c)] /= grid.area[size_t(c)];
}

// The van Leer limiter (van Leer 1974) of the ratio of the upstream to the local difference.
double limiter(double upstream, double local) {
    if (upstream * local <= 0) return 0;
    const double ratio = upstream / local;
    return 2 * ratio / (1 + ratio);
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
    const bool withProduct = guessStream(flow, grid, poisson);
    if (!invertTrial(flow, grid, stencil, poisson, withProduct)) return false;
    acceptTrial(flow);
    std::swap(flow.before, flow.trialBefore);
    flow.started = true;
    return true;
}

double outflowCourant(const SphereGrid& grid, const std::vector<double>& flux, double step) {
    double worst = 0;
    for (int c = 0; c < grid.cells; ++c) {
        double out = 0;
        for (int e : grid.edges[size_t(c)]) {
            const double leaving = grid.edge[size_t(e)].left == c ? flux[size_t(e)] : -flux[size_t(e)];
            out += std::max(0.0, leaving);
        }
        worst = std::max(worst, out * step / grid.area[size_t(c)]);
    }
    return worst;
}

void advectTracer(const SphereGrid& grid, const SphereStencil& stencil, const std::vector<double>& flux, double step, std::vector<double>& tracer,
                  std::vector<double>& work) {
    assert(stencil.behind.size() == grid.edge.size());
    work.assign(size_t(grid.cells), 0.0);
    for (size_t e = 0; e < flux.size(); ++e) {
        const SphereEdge& edge = grid.edge[e];
        const bool outOfLeft = flux[e] > 0;
        const int up = outOfLeft ? edge.left : edge.right, down = outOfLeft ? edge.right : edge.left;
        const int behind = stencil.behind[e][outOfLeft ? 0 : 1];
        const double local = tracer[size_t(down)] - tracer[size_t(up)];
        const double upstream = tracer[size_t(up)] - tracer[size_t(behind)];
        const double courant = std::abs(flux[e]) * step / grid.area[size_t(up)];
        const double face = tracer[size_t(up)] + 0.5 * limiter(upstream, local) * (1 - courant) * local;
        const double moved = flux[e] * step * face;
        work[size_t(edge.left)] -= moved;
        work[size_t(edge.right)] += moved;
    }
    for (int c = 0; c < grid.cells; ++c) tracer[size_t(c)] += work[size_t(c)] / grid.area[size_t(c)];
}

} // namespace derivean::sim
