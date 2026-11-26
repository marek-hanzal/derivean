#pragma once

#include "core/type/Vec.hpp"

#include <array>

namespace derivean::sim {

inline constexpr int RoseSectors = 16;

// Unit vector a sector's sand travels along: sector 0 heads +X (east), sector 4 heads +Z (south).
inline Vec2 sectorHeading(int sector) {
    const double angle = sector * (6.283185307179586 / RoseSectors);
    return {std::cos(angle), std::sin(angle)};
}

// What the wind of a place can do to sand through a year: the sand rose. Each entry is the volume a
// dry, flat, sand-covered bed would pass towards that sector, so the entries already hold how often
// and how hard the wind blows from each side.
struct WindRegime {
    std::array<double, RoseSectors> drift{};   // m²/yr per metre of width, by sector of travel
    double potential{};                        // m²/yr, all sectors together
    Vec2 resultant;                            // m²/yr, their vector sum: where sand goes on balance
    Vec2 mean;                                 // m/s, annual mean wind
    double spread{};                           // m/s, standard deviation of the wind around its monthly mean
};

// 1 for a wind that always blows one way, 0 for one that cancels itself through the year.
inline double steadiness(const WindRegime& regime) { return regime.potential > 0 ? length(regime.resultant) / regime.potential : 0.0; }

} // namespace derivean::sim
