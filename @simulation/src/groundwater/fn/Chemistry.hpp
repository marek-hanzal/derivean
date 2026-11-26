#pragma once

#include <algorithm>
#include <cmath>

namespace derivean::sim {

// Closed forms for what hot or charged water holds and gives up. Temperatures in °C, loads in kg/m³.

// Water boils cooler the higher it stands: about a degree per 300 m.
inline float boilingPoint(float aboveSea) { return 100.0f - std::max(0.0f, aboveSea) / 300.0f; }

// Silica held by water that has stood in hot rock long enough to match quartz (Fournier 1977), and
// the far larger load water keeps against the glassy silica that actually comes out of it at the
// surface. Only water that was hotter than about 150 °C at depth arrives with more than it can keep.
inline float quartzSilica(float celsius) { return 1e-3f * std::pow(10.0f, 5.19f - 1309.0f / (std::max(0.0f, celsius) + 273.15f)); }
inline float glassSilica(float celsius) { return 1e-3f * std::pow(10.0f, 4.52f - 731.0f / (std::max(0.0f, celsius) + 273.15f)); }

inline constexpr float AirCarbonDioxide = 4.0e-4f;   // atm

// Carbonate held in balance with a pressure of carbon dioxide: it grows with the cube root of that
// pressure and falls a little with warmth.
inline float calciteLoad(float carbonDioxide, float celsius) {
    return 0.065f * std::cbrt(carbonDioxide / AirCarbonDioxide) * std::clamp(1.0f - 0.012f * (celsius - 10.0f), 0.25f, 1.2f);
}

// Share of water arriving hotter than it can stay liquid that leaves as steam.
inline float flashShare(float arriving, float boiling) { return std::clamp((arriving - boiling) * 4.19f / 2257.0f, 0.0f, 0.9f); }

} // namespace derivean::sim
