#include "volcanic/fn/Emplace.hpp"

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
    set(field, work);
}

} // namespace derivean::sim
