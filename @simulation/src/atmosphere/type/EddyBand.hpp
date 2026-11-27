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
//
// The associated Legendre functions depend on the latitude only, and P̄_lm(−t) = (−1)^(l+m) P̄_lm(t),
// so cells whose |sin φ| agree share one recursion. The cube's symmetries (four equatorial faces a
// quarter turn apart, each mirrored east–west and north–south; two polar faces with eight symmetries
// each, mirrored into each other) put up to 16 cells in a group.
struct EddyBand {
    double centre{};                  // rad/m, wavenumber of the fastest-growing system
    double reach{};                   // rad/m, half-width of the band
    int lowest{}, highest{};          // the degrees with a positive gain
    std::vector<double> gain;         // per degree 0 .. highest
    std::vector<double> a, b;         // Legendre recursion per (l, m), index l · (highest + 1) + m
    // Groups of cells with one |sin φ|: members groupStart[g] .. groupStart[g + 1] − 1, those north
    // of the equator (sin φ ≥ 0) first, those south from groupSouth[g].
    std::vector<int> groupStart, groupSouth;
    std::vector<double> groupSine, groupCosine;   // |sin φ| and cos φ of the group's first member
    // Per member, in group order: its cell, area / 4πR² (the harmonics' quadrature weight), cos λ
    // and sin λ.
    std::vector<int> memberCell;
    std::vector<double> memberWeight, memberCos, memberSin;
    // Work space: coefficients per (l, m), and per member the field or the result and cos mλ, sin mλ.
    std::vector<double> cosineTerms, sineTerms, memberValue, memberCosM, memberSinM;
};

} // namespace derivean::sim
