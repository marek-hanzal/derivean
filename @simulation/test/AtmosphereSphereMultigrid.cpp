// The multigrid V-cycle that preconditions the Poisson solve, in double and in float32: symmetric and
// positive, as CG needs, and the smoother's interval resting on a sound λmax.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/fn/PoissonLevel.hpp"
#include "atmosphere/fn/SphereMultigrid.hpp"
#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

// Preconditioned CG needs a symmetric positive preconditioner: y · B x = x · B y and x · B x > 0 for
// the V-cycle B on residual-like fields (zero sum). In double the asymmetry is rounding (1e-16); the
// float32 cycle is B up to float32 rounding, its asymmetry 4.2e-9 and its z within 3.2e-7 of the
// double cycle's on this level (under 4.3e-9 and 6e-7 on levels 5 to 8, measured).
void checkPreconditioner() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil, true, false);
    SpherePoisson narrow = makeSpherePoisson(grid, stencil, true, true);
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
        std::vector<double> bx, by, nx, ny;
        vCycle(poisson.levels, poisson.smoothing, x, bx);
        vCycle(poisson.levels, poisson.smoothing, y, by);
        vCycle(narrow.narrow, narrow.levels.back(), narrow.smoothing, x, nx);
        vCycle(narrow.narrow, narrow.levels.back(), narrow.smoothing, y, ny);
        const double ybx = dotOf(y, bx), xby = dotOf(x, by), ynx = dotOf(y, nx), xny = dotOf(x, ny);
        expect(std::abs(ybx - xby) < 1e-12 * std::sqrt(dotOf(bx, bx) * dotOf(y, y)), "the V-cycle is not symmetric", ybx - xby);
        expect(std::abs(ynx - xny) < 3e-8 * std::sqrt(dotOf(nx, nx) * dotOf(y, y)), "the float32 V-cycle is not symmetric to its rounding",
               (ynx - xny) / std::sqrt(dotOf(nx, nx) * dotOf(y, y)));
        expect(dotOf(x, bx) > 0 && dotOf(x, nx) > 0, "the V-cycle is not positive", dotOf(x, nx));
        double moved = 0, largest = 0;
        for (size_t c = 0; c < bx.size(); ++c) {
            moved = std::max(moved, std::abs(nx[c] - bx[c]));
            largest = std::max(largest, std::abs(bx[c]));
        }
        expect(moved < 3e-6 * largest, "the float32 V-cycle is not the double one to its rounding", moved / largest);
        // The residual is scaled by a power of two, exactly, so a residual 2⁻⁶⁰ or 2⁷⁰ times the size
        // gives the same correction to the same power, bit for bit; a zero one gives zero.
        for (int power : {-60, 70}) {
            std::vector<double> scaled = x, z;
            for (double& value : scaled) value = std::ldexp(value, power);
            vCycle(narrow.narrow, narrow.levels.back(), narrow.smoothing, scaled, z);
            for (double& value : z) value = std::ldexp(value, -power);
            expect(z == nx, "the float32 V-cycle depends on the residual's size", power);
        }
        // A subnormal residual (bits lost already) still gives a finite correction.
        std::vector<double> tiny = x, z;
        for (double& value : tiny) value = std::ldexp(value, -1060);
        vCycle(narrow.narrow, narrow.levels.back(), narrow.smoothing, tiny, z);
        expect(std::all_of(z.begin(), z.end(), [](double value) { return std::isfinite(value); }), "a subnormal residual has no finite correction");
    }
    std::vector<double> zero(size_t(grid.cells), 0.0), z;
    vCycle(narrow.narrow, narrow.levels.back(), narrow.smoothing, zero, z);
    expect(std::all_of(z.begin(), z.end(), [](double value) { return value == 0; }), "a zero residual has a correction");
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

// The float32 cycle is close enough to the double one that CG takes the same iterations with either,
// so plain CG serves and the flexible variant is not needed (on levels 5 to 8 from 1e-6 to 1e-12,
// measured; checked here on levels 5 and 6).
void checkNarrowIterations() {
    for (int level = 5; level <= 6; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson wide = makeSpherePoisson(grid, stencil, true, false), narrow = makeSpherePoisson(grid, stencil, true, true);
        std::vector<double> source = harmonicField(grid, 4, 2);
        const std::vector<double> fine = harmonicField(grid, 9, 5);
        for (size_t c = 0; c < source.size(); ++c) source[c] += 0.1 * fine[c];
        for (double tolerance : {1e-6, 1e-9, 1e-12}) {
            std::vector<double> a(size_t(grid.cells), 0.0), b = a;
            const PoissonReport x = solveSpherePoisson(wide, grid, source, a, tolerance, 100);
            const PoissonReport y = solveSpherePoisson(narrow, grid, source, b, tolerance, 100);
            expect(x.converged && y.converged && x.iterations == y.iterations, "the float32 V-cycle changes CG's iterations",
                   y.iterations - x.iterations);
        }
    }
}

// The float32 shared product, through the arm64 kernel where there is one, equals forEachProduct's
// row sums bit for bit, as the double one does (checkSharedFaces); levels 5 and 6 have blocks of 16
// rows and shorter tails.
void checkNarrowProduct() {
    for (int level = 5; level <= 6; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const PoissonLevel wide = makePoissonLevel(grid, makeSphereStencil(grid), LevelUse::Diagonal);
        const StencilMatrixOf<float> matrix = narrowed(wide.stiffness);
        std::uint64_t state = 3;
        std::vector<float> x(size_t(grid.cells)), product, rows(x.size());
        for (float& value : x) {
            state = state * 6364136223846793005ull + 1442695040888963407ull;
            value = float(double(state >> 11) / double(1ull << 53) - 0.5);
        }
        applyMatrix(matrix, x, product);
        forEachProduct(matrix, x.data(), [&](size_t c, float sum) { rows[c] = sum; });
        expect(matrix.shared && product == rows, "the float32 shared product differs from the row sums", level);
    }
}

} // namespace

namespace derivean::sim::atmosphere_sphere_test {

void checkMultigrid() {
    checkPreconditioner();
    checkNarrowIterations();
    checkNarrowProduct();
    checkLargestEigenvalue();
}

} // namespace derivean::sim::atmosphere_sphere_test
