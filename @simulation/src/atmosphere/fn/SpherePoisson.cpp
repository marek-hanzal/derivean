#include "atmosphere/fn/SpherePoisson.hpp"

#include "atmosphere/fn/SphereMultigrid.hpp"
#include "atmosphere/fn/SphereOperators.hpp"

#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

void precondition(SpherePoisson& poisson, const std::vector<double>& r, std::vector<double>& z) {
    if (poisson.multigrid) {
        assert(poisson.smoothing >= 1);   // without smoothing the V-cycle is singular and CG breaks down
        vCycle(poisson.levels, poisson.smoothing, r, z);
        return;
    }
    for (size_t c = 0; c < r.size(); ++c) z[c] = poisson.inverseDiagonal[c] * r[c];
}

} // namespace

SpherePoisson makeSpherePoisson(const SphereGrid& grid, bool multigrid) {
    SpherePoisson poisson;
    PoissonLevel own;
    own.level = grid.level;
    own.side = grid.side;
    own.stiffness = makeLaplacian(grid);
    own.solution.assign(size_t(grid.cells), 0.0);
    own.source = own.solution;
    own.residual = own.solution;
    poisson.levels.push_back(std::move(own));
    poisson.multigrid = multigrid && grid.level >= 2;
    if (poisson.multigrid)
        for (PoissonLevel& level : coarserLevels(grid.level)) poisson.levels.push_back(std::move(level));
    poisson.inverseDiagonal.resize(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) poisson.inverseDiagonal[size_t(c)] = 1.0 / poisson.levels[0].stiffness.diagonal[size_t(c)];
    return poisson;
}

// Preconditioned conjugate gradients (Hestenes & Stiefel 1952) on K ψ = b with b = −area · (source −
// its mean). K is symmetric and positive on the subspace orthogonal to the constant, b lies in it,
// and so do every residual and K-product, so the iteration never sees the null mode; the constant
// the iterate may pick up is removed by area at the end.
PoissonReport solveSpherePoisson(SpherePoisson& poisson, const SphereGrid& grid, const std::vector<double>& source,
                                 std::vector<double>& result, double tolerance, int limit) {
    PoissonReport report;
    const size_t n = size_t(grid.cells);
    assert(source.size() == n && result.size() == n);
    report.sourceMean = areaMean(grid, source);
    std::vector<double>& r = poisson.residual;
    std::vector<double>& p = poisson.direction;
    std::vector<double>& q = poisson.product;
    std::vector<double>& z = poisson.preconditioned;
    r.resize(n);
    z.resize(n);
    applyMatrix(poisson.levels[0].stiffness, result, q);
    std::vector<double>& b = poisson.target;
    b.resize(n);
    for (size_t c = 0; c < n; ++c) {
        b[c] = -grid.area[c] * (source[c] - report.sourceMean);
        r[c] = b[c] - q[c];
    }
    const double bNorm = std::sqrt(dotOf(b, b));
    if (bNorm == 0) {
        for (double& value : result) value = 0;
        report.converged = true;
        return report;
    }
    precondition(poisson, r, z);
    p = z;
    double rz = dotOf(r, z);
    report.residual = std::sqrt(dotOf(r, r)) / bNorm;
    while (report.residual > tolerance && report.iterations < limit) {
        applyMatrix(poisson.levels[0].stiffness, p, q);
        const double alpha = rz / dotOf(p, q);
        for (size_t c = 0; c < n; ++c) {
            result[c] += alpha * p[c];
            r[c] -= alpha * q[c];
        }
        precondition(poisson, r, z);
        const double next = dotOf(r, z), beta = next / rz;
        rz = next;
        for (size_t c = 0; c < n; ++c) p[c] = z[c] + beta * p[c];
        ++report.iterations;
        report.residual = std::sqrt(dotOf(r, r)) / bNorm;
    }
    // The recursive residual drifts from the true one below about 1e-13 (it fell to 1e-22 while the
    // true one stayed at 2e-13), so the report and `converged` come from one more product.
    applyMatrix(poisson.levels[0].stiffness, result, q);
    for (size_t c = 0; c < n; ++c) r[c] = b[c] - q[c];
    report.residual = std::sqrt(dotOf(r, r)) / bNorm;
    report.converged = report.residual <= tolerance;
    const double mean = areaMean(grid, result);
    for (double& value : result) value -= mean;
    return report;
}

} // namespace derivean::sim
