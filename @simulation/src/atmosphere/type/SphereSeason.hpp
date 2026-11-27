#pragma once

#include <vector>

namespace derivean::sim {

// The season of the sphere's circulation by bands of latitude (S4.3b; S4 plan, point 3; the flat
// world's seasonOf with rows become bands): what the belts hold on a day from the zonal temperature
// of each band. Values per band sit at the band's centre, those per boundary (count + 1, south pole
// to north, LatitudeBands::boundary) on the circle that cuts the bands' areas apart.
struct SphereSeason {
    double declination{};             // degrees, of the sun on the day
    std::vector<double> eastward;     // m/s per boundary: the belts' zonal wind the jet is built from
    std::vector<double> jet;          // 1/s per band: its vorticity by Stokes (SphereClosures::jet)
    std::vector<double> northward;    // m/s per boundary: the overturning cells' surface branch
    std::vector<double> overturning;  // m per band: the lifting the cells force, from their convergence
    std::vector<double> growth;       // 1/s per band: the rate the temperature contrast feeds weather systems
    std::vector<double> shear;        // m/s per band: the westerly shear of the thermal wind
};

} // namespace derivean::sim
