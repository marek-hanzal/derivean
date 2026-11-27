#include "atmosphere/fn/SphereFlowGuess.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {

// The first guess of the next step's ψ (Fischer 1998): the combination of the last `count` accepted
// ψ that is closest to the new solution in the energy norm, x K x. With the differences d₀ = ψₙ,
// d₁ = ψₙ − ψₙ₋₁, d₂ = d₁ − (ψₙ₋₁ − ψₙ₋₂) and b the Poisson source of the candidate vorticity, the
// weights solve G w = β with G = dᵢ · K dⱼ and β = dᵢ · b, by Cholesky; a difference whose pivot is
// lost to rounding (the levels nearly collinear) is left out. In exact arithmetic the guess is never
// further from the solution in that norm than the last ψ or its extrapolation in time; CG stops on
// the residual, not that norm, so fewer iterations are measured, not guaranteed. It changes only
// where CG starts, so the result moves within the solver's tolerance. Each sum adds its terms in cell order, one product per statement
// (docs/windows.md#determinism).
void guessStream(SphereFlow& flow, const SphereGrid& grid, const SpherePoisson& poisson) {
    assert(flow.projection >= 0 && flow.projection <= 3);
    const int count = std::min(flow.projection, flow.levels);
    const size_t n = flow.stream.size();
    if (count == 0) {
        flow.trialStream = flow.stream;
        return;
    }
    const std::vector<double>& s0 = flow.stream;
    const std::vector<double>& s1 = flow.streamBack;
    const std::vector<double>& s2 = flow.streamBackTwo;
    applyMatrix(poisson.levels[0].stiffness, s0, flow.product);
    const std::vector<double>& p0 = flow.product;
    const std::vector<double>& p1 = flow.productBack;
    const std::vector<double>& p2 = flow.productBackTwo;
    const std::vector<double>& zeta = flow.trialVorticity;
    const double mean = areaMean(grid, zeta);
    double g[3][3]{}, beta[3]{};
    for (size_t c = 0; c < n; ++c) {
        const double source = -grid.area[c] * (zeta[c] - mean);
        double d[3] = {s0[c]}, kd[3] = {p0[c]};
        if (count > 1) d[1] = s0[c] - s1[c], kd[1] = p0[c] - p1[c];
        if (count > 2) d[2] = d[1] - (s1[c] - s2[c]), kd[2] = kd[1] - (p1[c] - p2[c]);
        for (int i = 0; i < count; ++i) {
            const double load = d[i] * source;
            beta[i] += load;
            for (int j = 0; j <= i; ++j) {
                const double energy = d[i] * kd[j];
                g[i][j] += energy;
            }
        }
    }
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
    std::vector<double>& guess = flow.trialStream;
    guess.resize(n);
    for (size_t c = 0; c < n; ++c) {
        const double d1 = count > 1 ? s0[c] - s1[c] : 0, d2 = count > 2 ? d1 - (s1[c] - s2[c]) : 0;
        guess[c] = w[0] * s0[c] + w[1] * d1 + w[2] * d2;
    }
}

} // namespace derivean::sim
