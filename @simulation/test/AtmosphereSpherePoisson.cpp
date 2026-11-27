// Inverting the Laplacian on the sphere: second-order solutions everywhere (face interior, face edges
// and cube vertices), the null mode taken out by area, and failure that shows.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/fn/SphereMultigrid.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <cmath>
#include <cstdint>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

struct Solved {
    PoissonReport report;
    PlaceErrors errors;
};

// Δψ = −l(l + 1) / R² · Y_l, whose solution with zero mean is Y_l itself.
Solved solveHarmonic(const SphereGrid& grid, SpherePoisson& poisson, int degree, int order) {
    const std::vector<double> u = harmonicField(grid, degree, order);
    std::vector<double> source = u, psi(size_t(grid.cells), 0.0);
    for (double& value : source) value *= laplacianEigenvalue(degree);
    Solved solved;
    solved.report = solveSpherePoisson(poisson, grid, source, psi, 1e-11, 100);
    solved.errors = errorsOf(grid, psi, u);
    return solved;
}

// Levels 5, 6, 7 (6 × 32² … 6 × 128²): the error of the solution falls by about four per level in
// every place, the cube vertices included, although the operator's local error at face edges does not
// fall (AtmosphereSphere.cpp): the band it lives in narrows with the cells. The multigrid-preconditioned
// CG takes the same few iterations on every level.
void checkConvergence() {
    double worst[2][3][3] = {};
    int iterations[3] = {};
    for (int step = 0; step < 3; ++step) {
        const SphereGrid grid = makeSphereGrid(5 + step);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        for (int d = 0; d < 2; ++d) {
            const Solved solved = d == 0 ? solveHarmonic(grid, poisson, 1, 0) : solveHarmonic(grid, poisson, 4, 2);
            expect(solved.report.converged, "Poisson solve did not converge", solved.report.residual);
            expect(std::abs(solved.report.sourceMean) < 1e-20, "harmonic source has a mean", solved.report.sourceMean);
            iterations[step] = std::max(iterations[step], solved.report.iterations);
            for (int band = 0; band < 3; ++band) worst[d][step][band] = solved.errors.worst[band];
        }
    }
    for (int step = 1; step < 3; ++step)
        for (int d = 0; d < 2; ++d)
            for (int band = 0; band < 3; ++band) {
                const double order = std::log2(worst[d][step - 1][band] / worst[d][step][band]);
                expect(order > 1.7, "Poisson solution is not second order", order);
            }
    expect(worst[0][2][0] < 2e-4 && worst[0][2][1] < 2e-4 && worst[0][2][2] < 2e-4, "degree-1 solution is off by 2e-4 at level 7",
           std::max(worst[0][2][1], worst[0][2][2]));
    expect(iterations[2] <= 14 && iterations[2] <= iterations[0] + 3, "multigrid CG iterations grow with resolution", iterations[2]);
}

// A constant added to the source is reported and changes nothing; the result has zero area mean; a
// solve that starts from its answer takes no step; a zero source gives zero.
void checkNullMode() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    std::vector<double> source = harmonicField(grid, 3, 1), shifted = source, plain(size_t(grid.cells), 0.0), moved = plain;
    for (double& value : shifted) value += 3e-5;
    const PoissonReport a = solveSpherePoisson(poisson, grid, source, plain, 1e-12, 100);
    const PoissonReport b = solveSpherePoisson(poisson, grid, shifted, moved, 1e-12, 100);
    expect(a.converged && b.converged, "Poisson solve did not converge");
    expect(std::abs(b.sourceMean / 3e-5 - 1) < 1e-9, "the source's mean is not reported", b.sourceMean);
    double scale = 0, difference = 0;
    for (int c = 0; c < grid.cells; ++c) {
        scale = std::max(scale, std::abs(plain[size_t(c)]));
        difference = std::max(difference, std::abs(plain[size_t(c)] - moved[size_t(c)]));
    }
    expect(difference < 1e-9 * scale, "a constant in the source changes the solution", difference / scale);
    expect(std::abs(areaMean(grid, moved)) < 1e-14 * scale, "solution has an area mean", areaMean(grid, moved));
    const PoissonReport again = solveSpherePoisson(poisson, grid, shifted, moved, 1e-10, 100);
    expect(again.converged && again.iterations == 0, "a solve from its own answer takes steps", again.iterations);
    std::vector<double> zero(size_t(grid.cells), 0.0), result(size_t(grid.cells), 1.0);
    const PoissonReport none = solveSpherePoisson(poisson, grid, zero, result, 1e-10, 100);
    expect(none.converged && result[0] == 0 && result[size_t(grid.cells - 1)] == 0, "zero source does not give zero");
}

// An impossible tolerance or too few iterations end unconverged, with the count and residual given,
// and the iterate kept; the diagonal preconditioner on the same problem needs many more steps.
void checkFailure() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SpherePoisson diagonal = makeSpherePoisson(grid, stencil, false);
    const std::vector<double> u = harmonicField(grid, 2, 1);
    std::vector<double> source = u, psi(size_t(grid.cells), 0.0);
    for (double& value : source) value *= laplacianEigenvalue(2);
    const PoissonReport impossible = solveSpherePoisson(poisson, grid, source, psi, 1e-30, 20);
    expect(!impossible.converged && impossible.iterations == 20 && impossible.residual > 1e-30 && impossible.residual < 1e-9,
           "an impossible tolerance does not show as unconverged", impossible.residual);
    // CG's recursive residual falls on below rounding while the true one stops near 1e-13: a solve
    // asked for 1e-16 must not report success.
    std::fill(psi.begin(), psi.end(), 0.0);
    const PoissonReport floor = solveSpherePoisson(poisson, grid, source, psi, 1e-16, 300);
    expect(!floor.converged && floor.residual > 1e-16, "the reported residual is not the true one", floor.residual);
    std::fill(psi.begin(), psi.end(), 0.0);
    const PoissonReport cut = solveSpherePoisson(diagonal, grid, source, psi, 1e-10, 10);
    expect(!cut.converged && cut.iterations == 10 && cut.residual > 1e-10, "a cut-off solve does not show as unconverged", cut.residual);
    std::fill(psi.begin(), psi.end(), 0.0);
    const PoissonReport slow = solveSpherePoisson(diagonal, grid, source, psi, 1e-10, 1000);
    std::fill(psi.begin(), psi.end(), 0.0);
    const PoissonReport fast = solveSpherePoisson(poisson, grid, source, psi, 1e-10, 1000);
    expect(slow.converged && fast.converged && slow.iterations > 4 * fast.iterations, "diagonal preconditioner is not slower than multigrid",
           slow.iterations);
}

// From a tolerance of 1e-10 up the solve reports CG's recursive residual; on levels 5 to 7, cold and
// warm, it stays within a hundredth of the tolerance of the true ‖b − K ψ‖ / ‖b‖.
void checkReportedResidual() {
    for (int level = 5; level <= 7; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        const std::vector<double> u = harmonicField(grid, 4, 2);
        std::vector<double> source = u, psi(size_t(grid.cells), 0.0), product, nudged;
        for (double& value : source) value *= laplacianEigenvalue(4);
        for (double tolerance : {1e-10, 1e-9, 1e-6}) {
            for (int start = 0; start < 2; ++start) {
                // Warm: from the solution of a source moved by a tenth of the tolerance's size.
                std::fill(psi.begin(), psi.end(), 0.0);
                if (start == 1) {
                    nudged = source;
                    for (size_t c = 0; c < nudged.size(); ++c) nudged[c] *= 1 + 1e3 * tolerance * double(c % 3);
                    (void)solveSpherePoisson(poisson, grid, nudged, psi, tolerance, 100);
                }
                const PoissonReport report = solveSpherePoisson(poisson, grid, source, psi, tolerance, 100);
                applyMatrix(poisson.levels[0].stiffness, psi, product);
                double rr = 0, bb = 0;
                for (size_t c = 0; c < psi.size(); ++c) {
                    const double b = -grid.area[c] * (source[c] - report.sourceMean);
                    rr += (b - product[c]) * (b - product[c]);
                    bb += b * b;
                }
                const double truth = std::sqrt(rr / bb);
                expect(report.converged && std::abs(truth - report.residual) < 1e-2 * tolerance, "the recursive residual drifts from the true one",
                       std::abs(truth - report.residual) / tolerance);
            }
        }
    }
}

// Preconditioned CG needs a symmetric positive preconditioner: y · B x = x · B y and x · B x > 0 for
// the V-cycle B on residual-like fields (zero sum).
void checkPreconditioner() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    std::uint64_t state = 7;
    auto randomField = [&] {
        std::vector<double> field(size_t(grid.cells));
        double sum = 0;
        for (double& value : field) {
            state = state * 6364136223846793005ull + 1442695040888963407ull;
            value = double(state >> 11) / double(1ull << 53) - 0.5;
            sum += value;
        }
        for (double& value : field) value -= sum / double(grid.cells);
        return field;
    };
    for (int trial = 0; trial < 3; ++trial) {
        const std::vector<double> x = randomField(), y = randomField();
        std::vector<double> bx, by;
        vCycle(poisson.levels, poisson.smoothing, x, bx);
        vCycle(poisson.levels, poisson.smoothing, y, by);
        const double ybx = dotOf(y, bx), xby = dotOf(x, by);
        expect(std::abs(ybx - xby) < 1e-12 * std::sqrt(dotOf(bx, bx) * dotOf(y, y)), "the V-cycle is not symmetric", ybx - xby);
        expect(dotOf(x, bx) > 0, "the V-cycle is not positive", dotOf(x, bx));
    }
}

// The smoother's interval rests on λmax(D⁻¹K) estimated from 12 power steps: on levels 2 to 5 the
// estimate is below the value 2000 steps from a random start converge to (both are Rayleigh
// quotients, so lower bounds) and within 5 % of it.
void checkLargestEigenvalue() {
    for (int level = 2; level <= 5; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const PoissonLevel at = makePoissonLevel(grid, makeSphereStencil(grid), LevelUse::Coarser);
        std::uint64_t state = uint64_t(level);
        std::vector<double> x(size_t(grid.cells)), kx;
        for (double& value : x) {
            state = state * 6364136223846793005ull + 1442695040888963407ull;
            value = double(state >> 11) / double(1ull << 53) - 0.5;
        }
        double largest = 0;
        for (int step = 0; step < 2000; ++step) {
            applyMatrix(at.stiffness, x, kx);
            double kxx = 0, dxx = 0, top = 0;
            for (size_t c = 0; c < x.size(); ++c) {
                kxx += kx[c] * x[c];
                dxx += x[c] * x[c] / at.inverseDiagonal[c];
            }
            largest = kxx / dxx;
            for (size_t c = 0; c < x.size(); ++c) {
                x[c] = at.inverseDiagonal[c] * kx[c];
                top = std::max(top, std::abs(x[c]));
            }
            for (double& value : x) value /= top;
        }
        expect(at.largest <= largest * (1 + 1e-9) && at.largest > 0.95 * largest, "λmax of D⁻¹K is misestimated", at.largest / largest);
    }
}

} // namespace

namespace derivean::sim::atmosphere_sphere_test {

void checkPoisson() {
    checkConvergence();
    checkNullMode();
    checkFailure();
    checkPreconditioner();
    checkLargestEigenvalue();
    checkReportedResidual();
}

} // namespace derivean::sim::atmosphere_sphere_test
