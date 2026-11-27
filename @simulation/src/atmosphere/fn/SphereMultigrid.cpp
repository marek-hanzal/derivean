#include "atmosphere/fn/SphereMultigrid.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/SphereStencil.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// A multigrid V-cycle (Brandt 1977) on the quadtree's levels; the coarsest is solved by plain CG.
constexpr int CoarsestLevel = 1;

// The Chebyshev smoother damps the part of the spectrum of D⁻¹K in [ChebyshevLow, ChebyshevHigh] ·
// λmax (Adams, Brezina, Hu & Tuminaro 2003): the low end is left to the coarser levels. The
// polynomial grows only above 2θ = 1.25 times the estimate, so an estimate up to a fifth short is
// safe; the power iteration falls short by under 1 % (tested to 5 %).
constexpr double ChebyshevLow = 0.2, ChebyshevHigh = 1.05;
constexpr int PowerSteps = 12;

// λmax of D⁻¹K by power iteration from the grid-scale checkerboard, which lies close to the top of
// the spectrum; the Rayleigh quotient xᵀKx / xᵀDx approaches λmax from below.
double largestEigenvalue(PoissonLevel& level) {
    const size_t n = level.inverseDiagonal.size();
    std::vector<double>& x = level.direction;
    std::vector<double>& kx = level.product;
    for (size_t c = 0; c < n; ++c) {
        const int rest = int(c) % (level.side * level.side);
        x[c] = ((rest % level.side + rest / level.side) % 2 == 0 ? 1.0 : -1.0) + 1e-3 * double(c % 7);
    }
    double estimate = 0;
    for (int step = 0; step < PowerSteps; ++step) {
        applyMatrix(level.stiffness, x, kx);
        double kxx = 0, dxx = 0, largest = 0;
        for (size_t c = 0; c < n; ++c) {
            kxx += kx[c] * x[c];
            dxx += x[c] * x[c] / level.inverseDiagonal[c];
        }
        estimate = kxx / dxx;
        for (size_t c = 0; c < n; ++c) {
            x[c] = level.inverseDiagonal[c] * kx[c];
            largest = std::max(largest, std::abs(x[c]));
        }
        for (double& value : x) value /= largest;
    }
    return estimate;
}

// `degree` steps of the Chebyshev iteration for K x = b with the Jacobi preconditioner (Saad 2003,
// Alg. 12.1). Before the coarse correction it starts from x = 0 (the first residual is b and needs no
// product) and leaves the final residual b − K x in level.residual for the restriction; after it, it
// starts from the corrected x and the final residual is not needed.
void smooth(PoissonLevel& level, const std::vector<double>& b, std::vector<double>& x, int degree, bool before) {
    const bool fromZero = before;
    const size_t n = b.size();
    std::vector<double>& r = level.residual;
    std::vector<double>& d = level.direction;
    std::vector<double>& kd = level.product;
    const double low = ChebyshevLow * level.largest, high = ChebyshevHigh * level.largest;
    const double theta = 0.5 * (high + low), delta = 0.5 * (high - low), sigma = theta / delta;
    double rho = 1 / sigma;
    const double first = 1 / theta;
    if (fromZero) {
        for (size_t c = 0; c < n; ++c) {
            r[c] = b[c];
            d[c] = first * level.inverseDiagonal[c] * r[c];
            x[c] = d[c];
        }
    } else {
        applyMatrix(level.stiffness, x, kd);
        for (size_t c = 0; c < n; ++c) {
            r[c] = b[c] - kd[c];
            d[c] = first * level.inverseDiagonal[c] * r[c];
            x[c] += d[c];
        }
    }
    for (int k = 1; k < degree; ++k) {
        applyMatrix(level.stiffness, d, kd);
        const double next = 1 / (2 * sigma - rho), keep = next * rho, push = 2 * next / delta;
        rho = next;
        for (size_t c = 0; c < n; ++c) {
            r[c] -= kd[c];
            d[c] = keep * d[c] + push * level.inverseDiagonal[c] * r[c];
            x[c] += d[c];
        }
    }
    if (!before) return;
    applyMatrix(level.stiffness, d, kd);
    for (size_t c = 0; c < n; ++c) r[c] -= kd[c];
}

// The coarsest level is solved by plain CG to rounding; its 24 cells need at most a few dozen steps.
void solveCoarsest(PoissonLevel& level, const std::vector<double>& source, std::vector<double>& solution) {
    const size_t n = source.size();
    double mean = 0;
    for (double value : source) mean += value;
    mean /= double(n);
    std::vector<double>& r = level.residual;
    std::vector<double>& p = level.direction;
    std::vector<double>& q = level.product;
    for (size_t c = 0; c < n; ++c) {
        solution[c] = 0;
        r[c] = source[c] - mean;
        p[c] = r[c];
    }
    double rr = dotOf(r, r);
    const double floor = rr * 1e-28;
    for (int step = 0; step < int(4 * n) && rr > floor; ++step) {
        applyMatrix(level.stiffness, p, q);
        const double alpha = rr / dotOf(p, q);
        for (size_t c = 0; c < n; ++c) {
            solution[c] += alpha * p[c];
            r[c] -= alpha * q[c];
        }
        const double next = dotOf(r, r);
        for (size_t c = 0; c < n; ++c) p[c] = r[c] + next / rr * p[c];
        rr = next;
    }
}

// Children of coarse cell (I, J) on a face are (2I, 2J), (2I + 1, 2J), (2I, 2J + 1), (2I + 1, 2J + 1).
void restrictResidual(const PoissonLevel& fine, std::vector<double>& coarse) {
    const int side = fine.side, half = side / 2;
    const double* r = fine.residual.data();
    for (int face = 0; face < 6; ++face)
        for (int j = 0; j < half; ++j) {
            const size_t low = size_t((face * side + 2 * j) * side), high = low + size_t(side);
            double* out = &coarse[size_t((face * half + j) * half)];
            for (int i = 0; i < half; ++i) out[i] = r[low + 2 * i] + r[low + 2 * i + 1] + r[high + 2 * i] + r[high + 2 * i + 1];
        }
}

void prolongCorrection(const PoissonLevel& fine, const std::vector<double>& coarse, std::vector<double>& solution) {
    const int side = fine.side, half = side / 2;
    for (int face = 0; face < 6; ++face)
        for (int j = 0; j < half; ++j) {
            const size_t low = size_t((face * side + 2 * j) * side), high = low + size_t(side);
            const double* in = &coarse[size_t((face * half + j) * half)];
            for (int i = 0; i < half; ++i) {
                solution[low + 2 * i] += in[i];
                solution[low + 2 * i + 1] += in[i];
                solution[high + 2 * i] += in[i];
                solution[high + 2 * i + 1] += in[i];
            }
        }
}

void cycle(std::vector<PoissonLevel>& levels, size_t at, int degree, const std::vector<double>& source, std::vector<double>& solution) {
    PoissonLevel& level = levels[at];
    if (at + 1 == levels.size()) {
        solveCoarsest(level, source, solution);
        return;
    }
    PoissonLevel& coarse = levels[at + 1];
    smooth(level, source, solution, degree, true);
    restrictResidual(level, coarse.source);
    cycle(levels, at + 1, degree, coarse.source, coarse.solution);
    prolongCorrection(level, coarse.solution, solution);
    smooth(level, source, solution, degree, false);
}

} // namespace

PoissonLevel makePoissonLevel(const SphereGrid& grid, const SphereStencil& stencil) {
    PoissonLevel level;
    level.level = grid.level;
    level.side = grid.side;
    level.stiffness = makeStencilMatrix(makeLaplacian(grid, stencil), grid.side);
    const size_t n = size_t(grid.cells);
    level.solution.assign(n, 0.0);
    level.source = level.residual = level.direction = level.product = level.solution;
    level.inverseDiagonal.resize(n);
    for (size_t c = 0; c < n; ++c) level.inverseDiagonal[c] = 1.0 / level.stiffness.diagonal[c];
    level.largest = largestEigenvalue(level);
    return level;
}

std::vector<PoissonLevel> coarserLevels(int finest) {
    std::vector<PoissonLevel> levels;
    for (int l = finest - 1; l >= CoarsestLevel; --l) {
        const SphereGrid grid = makeSphereGrid(l);
        levels.push_back(makePoissonLevel(grid, makeSphereStencil(grid)));
    }
    return levels;
}

void vCycle(std::vector<PoissonLevel>& levels, int smoothing, const std::vector<double>& r, std::vector<double>& z) {
    assert(smoothing >= 1 && r.size() == levels[0].source.size());
    z.resize(r.size());
    cycle(levels, 0, smoothing, r, z);
}

} // namespace derivean::sim
