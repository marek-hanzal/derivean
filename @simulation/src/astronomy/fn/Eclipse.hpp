#pragma once

#include "astronomy/type/Sky.hpp"
#include "geometry/type/Vec3.hpp"

namespace derivean::sim {

// Eclipses follow from the positions of the three bodies; nothing is scheduled. The shadow is described
// on the fundamental plane as in NASA's canons (Espenak & Meeus 2006, 2009). Discs are uniform
// (limb darkening left out). `planetRadius` is the shadow-casting Earth's (geometry's sphere; NASA's
// canons use the equatorial 6 378 137 m).

// Share [0, 1] of the Sun's disc that the Moon hides from an observer (Earth frame, m).
double sunCoverage(const Sky& sky, Vec3 observer);

// The Moon's shadow on the fundamental plane, through the Earth's centre across the shadow's axis.
// Somewhere on the planet sees the Sun eclipsed while `axisDistance` < planetRadius + `penumbraRadius`;
// the eclipse is central where it is below planetRadius, total where `umbraRadius` > 0 and annular where
// it is negative (the cone's tip falls short). While the Moon stands beyond the Earth, the distance is
// infinite.
struct SolarShadow {
    double axisDistance{};      // m, from the Earth's centre to the axis through Sun and Moon
    double penumbraRadius{};    // m
    double umbraRadius{};       // m
    bool north{};               // the axis passes north of the Earth's centre
};
SolarShadow solarShadow(const Sky& sky);

// The Earth's shadow at the Moon, enlarged by 2 % for the air (Chauvenet 1863, as in NASA's canons).
// Magnitudes are the share of the Moon's diameter inside the shadow (negative: outside by that much);
// `umbralCoverage` is the share of its disc inside the umbra, which dims moonlight.
struct LunarShadow {
    double penumbralMagnitude{};
    double umbralMagnitude{};
    double umbralCoverage{};
};
LunarShadow lunarShadow(const Sky& sky, double planetRadius);

} // namespace derivean::sim
