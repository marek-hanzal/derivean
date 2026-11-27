#include "atmosphere/fn/SphereMultigrid.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// The Chebyshev smoother damps the part of the spectrum of D⁻¹K in [ChebyshevLow, ChebyshevHigh] ·
// λmax (Adams, Brezina, Hu & Tuminaro 2003): the low end is left to the coarser levels. The
// polynomial grows only above 2θ = 1.25 times the estimate, so an estimate up to a fifth short is
// safe; the power iteration (PoissonLevel.cpp) falls short by under 1 % (tested to 5 %).
constexpr double ChebyshevLow = 0.2, ChebyshevHigh = 1.05;

// `degree` steps of the Chebyshev iteration for K x = b with the Jacobi preconditioner (Saad 2003,
// Alg. 12.1). Before the coarse correction it starts from x = 0 (the first residual is b and needs no
// product) and leaves the final residual b − K x in level.residual for the restriction; after it, it
// starts from the corrected x and the final residual is not needed. The coefficients are computed in
// double and rounded once to the level's precision.
template <class Real>
void smooth(PoissonLevelOf<Real>& level, const std::vector<Real>& b, std::vector<Real>& x, int degree, bool before) {
    const bool fromZero = before;
    const size_t n = b.size();
    std::vector<Real>& r = level.residual;
    std::vector<Real>& d = level.direction;
    std::vector<Real>& kd = level.spare;
    const double low = ChebyshevLow * level.largest, high = ChebyshevHigh * level.largest;
    const double theta = 0.5 * (high + low), delta = 0.5 * (high - low), sigma = theta / delta;
    double rho = 1 / sigma;
    const Real first = Real(1 / theta);
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
        const double next = 1 / (2 * sigma - rho);
        const Real keep = Real(next * rho), push = Real(2 * next / delta);
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
    std::vector<double>& q = level.spare;
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

// The float32 cycle's coarsest step: its source widened (exactly), solved in double, its correction
// rounded back.
void solveCoarsest(PoissonLevel& level, const std::vector<float>& source, std::vector<float>& solution) {
    for (size_t c = 0; c < source.size(); ++c) level.source[c] = source[c];
    solveCoarsest(level, level.source, level.solution);
    for (size_t c = 0; c < source.size(); ++c) solution[c] = float(level.solution[c]);
}

// Children of coarse cell (I, J) on a face are (2I, 2J), (2I + 1, 2J), (2I, 2J + 1), (2I + 1, 2J + 1).
template <class Real> void restrictResidual(const PoissonLevelOf<Real>& fine, std::vector<Real>& coarse) {
    const int side = fine.side, half = side / 2;
    const Real* r = fine.residual.data();
    for (int face = 0; face < 6; ++face)
        for (int j = 0; j < half; ++j) {
            const size_t low = size_t((face * side + 2 * j) * side), high = low + size_t(side);
            Real* out = &coarse[size_t((face * half + j) * half)];
            for (int i = 0; i < half; ++i) out[i] = r[low + 2 * i] + r[low + 2 * i + 1] + r[high + 2 * i] + r[high + 2 * i + 1];
        }
}

template <class Real> void prolongCorrection(const PoissonLevelOf<Real>& fine, const std::vector<Real>& coarse, std::vector<Real>& solution) {
    const int side = fine.side, half = side / 2;
    for (int face = 0; face < 6; ++face)
        for (int j = 0; j < half; ++j) {
            const size_t low = size_t((face * side + 2 * j) * side), high = low + size_t(side);
            const Real* in = &coarse[size_t((face * half + j) * half)];
            for (int i = 0; i < half; ++i) {
                solution[low + 2 * i] += in[i];
                solution[low + 2 * i + 1] += in[i];
                solution[high + 2 * i] += in[i];
                solution[high + 2 * i + 1] += in[i];
            }
        }
}

// The V-cycle from level `at` down; the last level is solved by solveCoarsest on `coarsest`, in
// double whatever the precision of the levels above it.
template <class Real>
void cycle(std::vector<PoissonLevelOf<Real>>& levels, PoissonLevel& coarsest, size_t at, int degree, const std::vector<Real>& source,
           std::vector<Real>& solution) {
    if (at + 1 == levels.size()) {
        solveCoarsest(coarsest, source, solution);
        return;
    }
    PoissonLevelOf<Real>& level = levels[at];
    PoissonLevelOf<Real>& coarse = levels[at + 1];
    assert(2 * coarse.side == level.side && !level.residual.empty());
    smooth(level, source, solution, degree, true);
    restrictResidual(level, coarse.source);
    cycle(levels, coarsest, at + 1, degree, coarse.source, coarse.solution);
    prolongCorrection(level, coarse.solution, solution);
    smooth(level, source, solution, degree, false);
}

} // namespace

void vCycle(std::vector<PoissonLevel>& levels, int smoothing, const std::vector<double>& r, std::vector<double>& z) {
    assert(smoothing >= 1 && levels.size() >= 2 && r.size() == levels[0].inverseDiagonal.size());
    z.resize(r.size());
    cycle(levels, levels.back(), 0, smoothing, r, z);
}

// The float32 V-cycle as a low-precision preconditioner inside the double CG (Higham & Mary 2022,
// §8.2). r is scaled by the power of two that brings its largest value into [0.5, 1), so that the
// cycle works near 1 whatever the residual's size (a converging solve's falls to 1e-12 of the source
// and below); its result is scaled back. Each factor is applied as two halves, so neither overflows
// even for a subnormal residual, and every step is exact wherever the result is a normal number. The
// cycle is linear, so this changes only its rounding. A zero residual, or one whose largest value is
// infinite or NaN, is not scaled and reaches z as it would in double.
void vCycle(std::vector<PoissonLevelOf<float>>& narrow, PoissonLevel& coarsest, int smoothing, const std::vector<double>& r, std::vector<double>& z) {
    assert(smoothing >= 1 && narrow.size() >= 2 && r.size() == narrow[0].source.size());
    double largest = 0;
    for (double value : r) largest = std::max(largest, std::abs(value));
    int exponent = 0;
    if (std::isfinite(largest) && largest > 0) (void)std::frexp(largest, &exponent);
    const int half = exponent / 2;
    const double down = std::ldexp(1.0, -half), downRest = std::ldexp(1.0, half - exponent);
    const double up = std::ldexp(1.0, half), upRest = std::ldexp(1.0, exponent - half);
    PoissonLevelOf<float>& finest = narrow[0];
    for (size_t c = 0; c < r.size(); ++c) finest.source[c] = float(r[c] * down * downRest);
    cycle(narrow, coarsest, 0, smoothing, finest.source, finest.solution);
    z.resize(r.size());
    for (size_t c = 0; c < r.size(); ++c) z[c] = double(finest.solution[c]) * up * upRest;
}

} // namespace derivean::sim
