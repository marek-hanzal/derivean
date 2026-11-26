#pragma once

namespace derivean::sim {

// Real spherical harmonics, 4π-normalised as in geodesy: the mean of Y² over the sphere is 1. Order
// m ≥ 0 multiplies by cos(mλ), m < 0 by sin(|m|λ). The associated Legendre functions come from the
// standard forward column recursions (Holmes & Featherstone 2002), stable far beyond the degrees the
// weather needs. On the sphere of radius R, Δ Y_l = −l(l + 1) / R² Y_l.
double harmonicAt(int degree, int order, double latitudeSine, double longitude);

// The recursion's coefficients for P̄_lm = a · sin φ · P̄_(l−1)m − b · P̄_(l−2)m (l ≥ m + 2), and the
// diagonal and first off-diagonal steps.
double legendreA(int degree, int order);
double legendreB(int degree, int order);
double legendreDiagonal(int order);       // P̄_mm = this · cos φ · P̄_(m−1)(m−1), m ≥ 1
double legendreNext(int order);           // P̄_(m+1)m = this · sin φ · P̄_mm

} // namespace derivean::sim
