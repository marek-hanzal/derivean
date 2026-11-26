#include "volcanic/fn/Emplace.hpp"

#include "landscape/type/Terrain.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr double GulpSheets = 4.0;   // flow units the vent cell takes at once
constexpr double MostGulps = 8.0;    // an eruption is fed in at most this many parts

} // namespace

void pourLava(Emplacement& field, const Rheology& rheology, int x, int z, double volume) {
    if (volume <= 0 || !field.contains(x, z)) return;
    Worklist work;
    const size_t vent = field.index(x, z);
    double left = volume / (field.cell * field.cell);
    const double gulp = std::max(GulpSheets * rheology.sheet, left / MostGulps);
    while (left > 0) {
        const double part = std::min(left, gulp);
        feed(field, work, vent, float(part));
        left -= part;
        relax(field, rheology, work);
    }
    const std::vector<int32_t> reached = work.touched;
    set(field, work);
    // What set steeper than its own rubble lies comes down as rubble.
    const float holds = RubbleRest * float(field.cell);
    const Rheology rubble{RubbleRest * 0.3f, RubbleRest * 0.3f, 0.3f, 1e30f};
    for (const int32_t at : reached) {
        const size_t i = size_t(at);
        if (field.laid[i] <= 0 || field.rest[i] == Rest::Closed) continue;
        float lowest = field.ground[i];
        for (int k = 0; k < 8; ++k) {
            const int nx = at % field.width + NeighborX[k], nz = at / field.width + NeighborZ[k];
            if (field.contains(nx, nz)) lowest = std::min(lowest, field.ground[field.index(nx, nz)] + holds * NeighborReach[k]);
        }
        const float loose = std::min(field.laid[i], field.ground[i] - lowest);
        if (loose <= 0) continue;
        field.laid[i] -= loose, field.ground[i] -= loose;
        feed(field, work, i, loose);
    }
    relax(field, rubble, work);
    set(field, work);
}

} // namespace derivean::sim
