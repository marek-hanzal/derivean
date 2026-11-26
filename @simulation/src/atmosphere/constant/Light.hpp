#pragma once

#include <array>
#include <cmath>

namespace derivean::sim {

// The wavelengths in nm the air's optics are given at for the picture: red, green and blue, the
// renderer's primaries. Green is the 550 nm every optical depth here is quoted at.
inline constexpr std::array<double, 3> Wavelengths{680.0, 550.0, 440.0};
inline constexpr double ReferenceWavelength = 550.0;

// Clean air alone at 550 nm: 1.16e-5 /m at sea level (Bucholtz 1995), thinning by e over RayleighHeight.
// At other wavelengths as their inverse fourth power (Rayleigh); the dispersion of the air's
// refractive index, which adds about 0.05 to the power, is left out.
inline constexpr double RayleighExtinction = 1.16e-5;   // 1/m
inline constexpr double RayleighHeight = 8000.0;        // m
inline double cleanAirAt(double wavelength) { return RayleighExtinction * std::pow(ReferenceWavelength / wavelength, 4.0); }

} // namespace derivean::sim
