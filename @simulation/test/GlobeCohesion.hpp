#pragma once

#include "planet/fn/Cohesion.hpp"

#include <algorithm>

namespace derivean::sim::globe_test {

// Whether the plates hold together, by the model's own regions (planet/fn/Cohesion): `stray` counts the
// cells outside their plate's largest region, `fragment` is the largest such region of MicroplateCells in
// sr, which keepPlatesWhole keeps under MicroplateArea (Bird 2003, docs/credits).
struct Cohesion {
    long stray{}, cells{};      // cells outside their plate's largest region; cells with an owner
    double fragment{};          // sr
};

inline Cohesion measureCohesion(const Globe& globe) {
    const PlateRegions regions = plateRegionsOf(globe);
    Cohesion cohesion;
    for (size_t c = 0; c < regions.owner.size(); ++c) {
        if (regions.owner[c] < 0) continue;
        ++cohesion.cells;
        cohesion.stray += regions.largest[size_t(regions.owner[c])] != regions.region[c];
    }
    for (size_t r = 0; r < regions.plate.size(); ++r)
        if (regions.largest[size_t(regions.plate[r])] != int32_t(r) && regions.cells[r] >= MicroplateCells) cohesion.fragment = std::max(cohesion.fragment, regions.area[r]);
    return cohesion;
}

} // namespace derivean::sim::globe_test
