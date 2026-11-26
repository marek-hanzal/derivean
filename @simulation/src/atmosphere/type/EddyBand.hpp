#pragma once

#include <vector>

namespace derivean::sim {

// The band of weather-system scales whose vorticity the single-layer circulation lets grow (the
// closure that stands in for baroclinic instability). On the flat world it is a ring of Fourier
// waves picked by PoissonSolver::pass; on the sphere it is a gain per spherical-harmonic degree l,
// applied to every order m ≠ 0 (m = 0 are the zonal bands, left to the belts as the flat world leaves
// its waves that do not vary along X). Degree l has the wavenumber √(l(l + 1)) / R, the eigenvalue
// of the Laplacian, so the gain is the flat world's ring at the same physical scale:
// gain = max(0, 1 − ((k − centre) / reach)²).
struct EddyBand {
    double centre{};                  // rad/m, wavenumber of the fastest-growing system
    double reach{};                   // rad/m, half-width of the band
    int lowest{}, highest{};          // the degrees with a positive gain
    std::vector<double> gain;         // per degree 0 .. highest
    std::vector<double> a, b;         // Legendre recursion per (l, m), index l · (highest + 1) + m
    // Per cell: sin φ, cos φ, cos λ, sin λ and area / 4πR² (the harmonics' quadrature weight).
    std::vector<double> sine, cosine, cosLongitude, sinLongitude, weight;
    std::vector<double> cosineTerms, sineTerms;   // coefficients per (l, m), work space
};

} // namespace derivean::sim
