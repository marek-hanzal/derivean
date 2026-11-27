#include "atmosphere/fn/SphereDamping.hpp"

#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr int PowerSteps = 60;

// max_i Σ_j |K_ij| / A_i: every eigenvalue of A⁻¹K lies in a Gershgorin disc of one of its rows
// (Gershgorin 1931), centred at K_ii / A_i with radius Σ_{j≠i} |K_ij| / A_i, so none exceeds it.
// The row sums are the product of |K| with ones.
double gershgorinBound(const SphereGrid& grid, const StencilMatrix& stiffness) {
    StencilMatrix magnitude = stiffness;
    for (double& v : magnitude.inner) v = std::abs(v);
    for (double& v : magnitude.value) v = std::abs(v);
    const std::vector<double> ones(size_t(grid.cells), 1.0);
    double bound = 0;
    forEachProduct(magnitude, ones.data(), [&](size_t row, double sum) { bound = std::max(bound, sum / grid.area[row]); });
    return bound;
}

// λ_max of A⁻¹K by power iteration from the grid-scale checkerboard, near the top of the spectrum:
// the Rayleigh quotient xᵀKx / xᵀAx approaches it from below. The ripple of 1e-3 · (c mod 7) breaks
// the checkerboard's symmetry, so the start is not orthogonal to the top mode.
double powerEstimate(const SphereGrid& grid, const StencilMatrix& stiffness, std::vector<double>& x, std::vector<double>& kx) {
    const size_t n = size_t(grid.cells);
    const int side = grid.side;
    x.resize(n);
    for (size_t c = 0; c < n; ++c) {
        const int rest = int(c) % (side * side);
        x[c] = ((rest % side + rest / side) % 2 == 0 ? 1.0 : -1.0) + 1e-3 * double(c % 7);
    }
    double estimate = 0;
    for (int step = 0; step < PowerSteps; ++step) {
        applyMatrix(stiffness, x, kx);
        double kxx = 0, axx = 0, top = 0;
        for (size_t c = 0; c < n; ++c) {
            const double energy = kx[c] * x[c];
            const double mass = grid.area[c] * x[c];
            const double weighted = mass * x[c];
            kxx += energy;
            axx += weighted;
        }
        estimate = kxx / axx;
        for (size_t c = 0; c < n; ++c) {
            x[c] = kx[c] / grid.area[c];
            top = std::max(top, std::abs(x[c]));
        }
        for (double& v : x) v /= top;
    }
    return estimate;
}

} // namespace

SphereDamping makeSphereDamping(const SphereGrid& grid, const StencilMatrix& stiffness, double decay) {
    assert(decay > 0);
    SphereDamping damping;
    damping.decay = decay;
    damping.bound = gershgorinBound(grid, stiffness);
    damping.estimate = powerEstimate(grid, stiffness, damping.work, damping.twice);
    const double square = damping.estimate * damping.estimate;
    damping.rate = 1 / (decay * square);
    return damping;
}

void addDamping(SphereDamping& damping, const SphereGrid& grid, const StencilMatrix& stiffness, const std::vector<double>& level,
                std::vector<double>& tendency) {
    const size_t n = size_t(grid.cells);
    assert(level.size() == n);
    if (tendency.empty()) tendency.assign(n, 0.0);
    assert(tendency.size() == n);
    applyMatrix(stiffness, level, damping.work);
    for (size_t c = 0; c < n; ++c) damping.work[c] /= grid.area[c];
    applyMatrix(stiffness, damping.work, damping.twice);
    for (size_t c = 0; c < n; ++c) {
        const double bent = damping.twice[c] / grid.area[c];
        const double damped = damping.rate * bent;
        tendency[c] -= damped;
    }
}

double dampingStiffness(const SphereDamping& damping) {
    const double square = damping.bound * damping.bound;
    return damping.rate * square;
}

} // namespace derivean::sim
