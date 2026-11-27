#include "atmosphere/fn/SpherePoisson.hpp"

#include "atmosphere/fn/PoissonLevel.hpp"
#include "atmosphere/fn/SphereMultigrid.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr double TrustedResidual = 1e-10;
// A given K · result starts the residual from a tolerance of 1e-9 up. Its rounding δ leaves the
// recursive residual ‖δ‖/‖b‖ from the true one, 1.6e-12 at level 8 and fourfold a level (the true
// residual then at most 1.6e-3 of the tolerance from the reported one at level 8, 1.5e-5 with the
// product formed here); below 1e-9 the product is formed here.
constexpr double GivenStart = 1e-9;

void precondition(SpherePoisson& poisson, const std::vector<double>& r, std::vector<double>& z) {
    if (poisson.multigrid) {
        assert(poisson.smoothing >= 1);   // without smoothing the V-cycle is singular and CG breaks down
        if (poisson.narrow.empty())
            vCycle(poisson.levels, poisson.smoothing, r, z);
        else
            vCycle(poisson.narrow, poisson.levels.back(), poisson.smoothing, r, z);
        return;
    }
    const std::vector<double>& inverse = poisson.levels[0].inverseDiagonal;
    for (size_t c = 0; c < r.size(); ++c) z[c] = inverse[c] * r[c];
}

} // namespace

SpherePoisson makeSpherePoisson(const SphereGrid& grid, const SphereStencil& stencil, bool multigrid, bool narrow) {
    SpherePoisson poisson;
    poisson.multigrid = multigrid && grid.level >= 2;
    poisson.levels.push_back(makePoissonLevel(grid, stencil, poisson.multigrid ? LevelUse::Finest : LevelUse::Diagonal));
    if (poisson.multigrid)
        for (PoissonLevel& level : coarserLevels(grid.level)) poisson.levels.push_back(std::move(level));
    if (poisson.multigrid && narrow) {
        poisson.narrow = narrowLevels(poisson.levels);
        // From here on only the finest's matrix (CG's product) and the coarsest (solved in double) are
        // read: the double levels between them and the finest's smoother space and diagonal are
        // released, so `narrow` must not be emptied afterwards.
        poisson.levels.erase(poisson.levels.begin() + 1, poisson.levels.end() - 1);
        PoissonLevel& finest = poisson.levels[0];
        for (std::vector<double>* unused : {&finest.inverseDiagonal, &finest.residual, &finest.direction, &finest.spare})
            std::vector<double>().swap(*unused);
    }
    return poisson;
}

// Preconditioned conjugate gradients (Hestenes & Stiefel 1952) on K ψ = b with b = −area · (source −
// its mean). K is symmetric and positive on the subspace orthogonal to the constant, b lies in it,
// and so do every residual and K-product, so the iteration never sees the null mode; the constant
// the iterate may pick up is removed by area at the end.
PoissonReport solveSpherePoisson(SpherePoisson& poisson, const SphereGrid& grid, const std::vector<double>& source,
                                 std::vector<double>& result, double tolerance, int limit, const std::vector<double>* product) {
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
    if (tolerance < GivenStart) product = nullptr;
    if (!product) applyMatrix(poisson.levels[0].stiffness, result, q);
    const std::vector<double>& start = product ? *product : q;
    assert(start.size() == n);
    std::vector<double>& b = poisson.target;
    b.resize(n);
    for (size_t c = 0; c < n; ++c) {
        b[c] = -grid.area[c] * (source[c] - report.sourceMean);
        r[c] = b[c] - start[c];
    }
    const double bNorm = std::sqrt(dotOf(b, b));
    if (bNorm == 0) {
        for (double& value : result) value = 0;
        report.converged = true;
        return report;
    }
    // The tolerance is checked before each preconditioning, so the last iteration does not run a
    // V-cycle whose direction would be thrown away.
    report.residual = std::sqrt(dotOf(r, r)) / bNorm;
    double rz = 0;
    if (report.residual > tolerance && limit > 0) {
        precondition(poisson, r, z);
        p = z;
        rz = dotOf(r, z);
    }
    // K · p goes through applyMatrix, whose NEON kernel outruns the row-by-row forEachProduct on arm64,
    // and p · q is a separate pass. r · r is summed in the loop that forms r, in ascending order as
    // dotOf sums it; the product is its own statement so it is not contracted into a fused
    // multiply-add, which dotOf does not use either.
    while (report.residual > tolerance && report.iterations < limit) {
        applyMatrix(poisson.levels[0].stiffness, p, q);
        const double alpha = rz / dotOf(p, q);
        double rr = 0;
        for (size_t c = 0; c < n; ++c) {
            result[c] += alpha * p[c];
            r[c] -= alpha * q[c];
            const double term = r[c] * r[c];
            rr += term;
        }
        ++report.iterations;
        report.residual = std::sqrt(rr) / bNorm;
        if (report.residual <= tolerance || report.iterations == limit) break;
        precondition(poisson, r, z);
        const double next = dotOf(r, z), beta = next / rz;
        rz = next;
        for (size_t c = 0; c < n; ++c) p[c] = z[c] + beta * p[c];
    }
    // The recursive residual drifts from the true one below about 1e-13 of ‖b‖ (it fell to 1e-22
    // while the true one stayed at 2e-13). From TrustedResidual up the drift is at most 1.3e-5 of the
    // tolerance (levels 5 to 7, cold and warm; tested to a hundredth), so the recursive residual is
    // reported; below it the report and `converged` come from one more product.
    if (tolerance < TrustedResidual) {
        applyMatrix(poisson.levels[0].stiffness, result, q);
        for (size_t c = 0; c < n; ++c) r[c] = b[c] - q[c];
        report.residual = std::sqrt(dotOf(r, r)) / bNorm;
    }
    report.converged = report.residual <= tolerance;
    const double mean = areaMean(grid, result);
    for (double& value : result) value -= mean;
    return report;
}

} // namespace derivean::sim
