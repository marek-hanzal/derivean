#include "atmosphere/fn/SphereFlowGuess.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

namespace derivean::sim {

namespace {

struct Gram {
    double g[3][3]{};   // dᵢ · K dⱼ, j ≤ i
    double beta[3]{};   // dᵢ · b
};

// G and β over the first Count differences in one pass over the cells. Count is fixed at compile
// time so the nine sums stay in registers; known only at run time, the loop takes 4.6 times the
// instructions at level 8. Each sum adds its terms in cell order, one product per statement
// (docs/windows.md#determinism), so the sums do not depend on Count beyond which of them are formed.
template <int Count> Gram gramOf(const SphereFlow& flow, const SphereGrid& grid) {
    const double *s0 = flow.stream.data(), *s1 = flow.streamBack.data(), *s2 = flow.streamBackTwo.data();
    const double *p0 = flow.product.data(), *p1 = flow.productBack.data(), *p2 = flow.productBackTwo.data();
    const double *zeta = flow.trialVorticity.data(), *area = grid.area.data();
    const double mean = areaMean(grid, flow.trialVorticity);
    double g00 = 0, g10 = 0, g11 = 0, g20 = 0, g21 = 0, g22 = 0, b0 = 0, b1 = 0, b2 = 0;
    const size_t n = flow.stream.size();
    for (size_t c = 0; c < n; ++c) {
        const double source = -area[c] * (zeta[c] - mean);
        const double d0 = s0[c], k0 = p0[c];
        const double load0 = d0 * source;
        b0 += load0;
        const double energy00 = d0 * k0;
        g00 += energy00;
        if constexpr (Count > 1) {
            const double d1 = s0[c] - s1[c], k1 = p0[c] - p1[c];
            const double load1 = d1 * source;
            b1 += load1;
            const double energy10 = d1 * k0, energy11 = d1 * k1;
            g10 += energy10;
            g11 += energy11;
            if constexpr (Count > 2) {
                const double d2 = d1 - (s1[c] - s2[c]), k2 = k1 - (p1[c] - p2[c]);
                const double load2 = d2 * source;
                b2 += load2;
                const double energy20 = d2 * k0, energy21 = d2 * k1, energy22 = d2 * k2;
                g20 += energy20;
                g21 += energy21;
                g22 += energy22;
            }
        }
    }
    Gram sums;
    sums.g[0][0] = g00, sums.g[1][0] = g10, sums.g[1][1] = g11, sums.g[2][0] = g20, sums.g[2][1] = g21, sums.g[2][2] = g22;
    sums.beta[0] = b0, sums.beta[1] = b1, sums.beta[2] = b2;
    return sums;
}

// The guess Σ wᵢ dᵢ and its K, the same combination of the K dᵢ, so CG's first residual needs no
// product. It differs from K applied to the guess by that product's own rounding: 3.5e-12 of the
// largest K ψ at level 8, as much as moving every value of the guess by one ulp, growing fourfold a
// level. The weights of the differences left out are zero.
template <int Count> void combine(SphereFlow& flow, const double w[3]) {
    const double *s0 = flow.stream.data(), *s1 = flow.streamBack.data(), *s2 = flow.streamBackTwo.data();
    const double *p0 = flow.product.data(), *p1 = flow.productBack.data(), *p2 = flow.productBackTwo.data();
    double *guess = flow.trialStream.data(), *product = flow.trialProduct.data();
    const double w0 = w[0], w1 = w[1], w2 = w[2];
    const size_t n = flow.stream.size();
    for (size_t c = 0; c < n; ++c) {
        double d1 = 0, d2 = 0, k1 = 0, k2 = 0;
        if constexpr (Count > 1) d1 = s0[c] - s1[c], k1 = p0[c] - p1[c];
        if constexpr (Count > 2) d2 = d1 - (s1[c] - s2[c]), k2 = k1 - (p1[c] - p2[c]);
        guess[c] = w0 * s0[c] + w1 * d1 + w2 * d2;
        product[c] = w0 * p0[c] + w1 * k1 + w2 * k2;
    }
}

} // namespace

// The first guess of the next step's ψ (Fischer 1998): the combination of the last `count` accepted
// ψ that is closest to the new solution in the energy norm, x K x. With the differences d₀ = ψₙ,
// d₁ = ψₙ − ψₙ₋₁, d₂ = d₁ − (ψₙ₋₁ − ψₙ₋₂) and b the Poisson source of the candidate vorticity, the
// weights solve G w = β with G = dᵢ · K dⱼ and β = dᵢ · b, by Cholesky; a difference whose pivot is
// lost to rounding (the levels nearly collinear) is left out. In exact arithmetic the guess is never
// further from the solution in that norm than the last ψ or its extrapolation in time; CG stops on
// the residual, not that norm, so fewer iterations are measured, not guaranteed. It changes only
// where CG starts, so the result moves within the solver's tolerance.
bool guessStream(SphereFlow& flow, const SphereGrid& grid, const SpherePoisson& poisson) {
    assert(flow.projection >= 0 && flow.projection <= 3);
    const int count = std::min(flow.projection, flow.levels);
    const size_t n = flow.stream.size();
    if (count == 0) {
        flow.trialStream = flow.stream;
        return false;
    }
    // K ψₙ, unless the inversion that gave ψₙ left it (the same product, bit for bit).
    if (flow.known) std::swap(flow.product, flow.trialProduct);
    else applyMatrix(poisson.levels[0].stiffness, flow.stream, flow.product);
    flow.known = false;
    const Gram sums = count == 1   ? gramOf<1>(flow, grid)
                      : count == 2 ? gramOf<2>(flow, grid)
                                   : gramOf<3>(flow, grid);
    const auto& g = sums.g;
    const auto& beta = sums.beta;
    double l[3][3]{}, w[3]{};
    bool kept[3]{};
    for (int i = 0; i < count; ++i) {
        double pivot = g[i][i];
        for (int k = 0; k < i; ++k)
            if (kept[k]) pivot -= l[i][k] * l[i][k];
        kept[i] = pivot > 1e-12 * g[i][i];
        if (!kept[i]) continue;
        l[i][i] = std::sqrt(pivot);
        for (int j = i + 1; j < count; ++j) {
            double sum = g[j][i];
            for (int k = 0; k < i; ++k)
                if (kept[k]) sum -= l[j][k] * l[i][k];
            l[j][i] = sum / l[i][i];
        }
    }
    double y[3]{};
    for (int i = 0; i < count; ++i) {
        if (!kept[i]) continue;
        double sum = beta[i];
        for (int k = 0; k < i; ++k)
            if (kept[k]) sum -= l[i][k] * y[k];
        y[i] = sum / l[i][i];
    }
    for (int i = count - 1; i >= 0; --i) {
        if (!kept[i]) continue;
        double sum = y[i];
        for (int k = i + 1; k < count; ++k)
            if (kept[k]) sum -= l[k][i] * w[k];
        w[i] = sum / l[i][i];
    }
    flow.trialStream.resize(n);
    flow.trialProduct.resize(n);
    if (count == 1) combine<1>(flow, w);
    else if (count == 2) combine<2>(flow, w);
    else combine<3>(flow, w);
    return true;
}

} // namespace derivean::sim
