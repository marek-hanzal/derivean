#include "atmosphere/fn/SphereTransport.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// The van Leer limiter (van Leer 1974) of the ratio r of the upstream to the local difference, times
// the local difference: φ(r) · local = 2 · upstream · local / (upstream + local), zero where they
// differ in sign, with one division instead of the ratio's two. It is formed as 2 · upstream · (local /
// (upstream + local)), a share in (0, 1): the product itself falls below the normal range in a
// tracer's tail (differences of 1e-165) and, rounded there, let the tracer dip below zero.
double limitedDifference(double upstream, double local) {
    if (upstream * local <= 0) return 0;
    const double share = local / (upstream + local);
    return 2 * upstream * share;
}

} // namespace

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
    assert(stencil.behind.size() == flux.size() && stencil.topology.size() == flux.size() && flux.size() == grid.edge.size());
    work.assign(size_t(grid.cells), 0.0);
    for (size_t e = 0; e < flux.size(); ++e) {
        const EdgeTopology& edge = stencil.topology[e];
        const bool outOfLeft = flux[e] > 0;
        const int up = outOfLeft ? edge.left : edge.right, down = outOfLeft ? edge.right : edge.left;
        const int behind = stencil.behind[e][outOfLeft ? 0 : 1];
        const double local = tracer[size_t(down)] - tracer[size_t(up)];
        const double upstream = tracer[size_t(up)] - tracer[size_t(behind)];
        const double courant = std::abs(flux[e]) * step / grid.area[size_t(up)];
        const double face = tracer[size_t(up)] + 0.5 * limitedDifference(upstream, local) * (1 - courant);
        const double moved = flux[e] * step * face;
        work[size_t(edge.left)] -= moved;
        work[size_t(edge.right)] += moved;
    }
    for (int c = 0; c < grid.cells; ++c) tracer[size_t(c)] += work[size_t(c)] / grid.area[size_t(c)];
}

} // namespace derivean::sim
