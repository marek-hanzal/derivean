#pragma once

#include "atmosphere/type/LatitudeBands.hpp"
#include "atmosphere/type/SphereDamping.hpp"
#include "atmosphere/type/StencilMatrix.hpp"

#include <vector>

namespace derivean::sim {

// The circulation's closures on the sphere (S4.3; S4 plan, point 3), one state the hour's forcing and
// its step check both read: the zonal mean of the level a step starts from relaxes per band of
// latitude to `jet` over `relaxation`, the friction rate per cell comes from the terrain, and the
// damping is the one whose stiffness the hour holds (`SphereHour::stiffest`), made from the K the
// flow inverts with, which it carries (SphereDamping::stiffness; Codex DREV-CLAUDE-007-06, 009-01).
// A damping put in its place must be made from that same K object (makeSphereDamping), or
// closuresFit fails and fillClosures and advanceClosedHour throw. Fixed for a window: the season sets
// `jet` between windows, with Σ_b B_b jet_b = 0 (a jet from zonalWindVorticity telescopes so), since
// the step takes ζ's area mean out whatever the forcing adds.
struct SphereClosures {
    LatitudeBands bands;
    SphereDamping damping;
    std::vector<double> friction;       // 1/s per cell, (1 + h / SphereRoughHeight) / SphereSurfaceDrag
    std::vector<double> jet;            // 1/s per band, the zonal-mean vorticity the belts hold; 0 until set
    double relaxation{};                // s
    std::vector<double> mean;           // work space, one call at a time
};

} // namespace derivean::sim
