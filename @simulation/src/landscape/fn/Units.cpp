#include "landscape/fn/Process.hpp"

#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {

StrataColumn Domain::strataAtCell(size_t i) const {
    const int x = int(i % size_t(size)), z = int(i / size_t(size));
    const int nx = std::min(x / stride, nodes - 2), nz = std::min(z / stride, nodes - 2);
    const float tx = float(x - nx * stride) / float(stride), tz = float(z - nz * stride) / float(stride);
    const StrataColumn* corner[4] = {&strata[size_t(nz) * size_t(nodes) + size_t(nx)], &strata[size_t(nz) * size_t(nodes) + size_t(nx + 1)],
                                     &strata[size_t(nz + 1) * size_t(nodes) + size_t(nx)], &strata[size_t(nz + 1) * size_t(nodes) + size_t(nx + 1)]};
    const float weight[4] = {(1 - tx) * (1 - tz), tx * (1 - tz), (1 - tx) * tz, tx * tz};
    return blendThickness(strataOf(i), corner, weight);
}

// A region cell is far thicker than a bed, so it meets the unit average; finer levels meet single
// beds, which is what lets a resistant member stand out as a bench or a cliff.
void refreshUnits(Domain& domain, uint64_t seed) {
    const bool beds = domain.key.level != Level::Region;
    for (size_t i = 0; i < domain.cells(); ++i) {
        if (volcanicUnit(domain, i, seed)) continue;
        const float depth = std::max(0.0f, domain.ground.datum[i] - domain.ground.rock[i]);
        UnitCache& cache = domain.unit[i];
        if (depth >= cache.top && depth < cache.bottom) continue;
        const StrataColumn column = domain.strataAtCell(i);
        const StrataUnit unit = unitAt(column, std::max(0.0f, depth), domain.fold[i]);
        if (beds) {
            const Bed bed = bedAt(seed, column, unit, std::max(0.0f, depth), domain.materialOf(i));
            cache = {std::min(bed.top, depth), bed.bottom, traitsOf(bed.rock)};
        } else {
            cache = {std::min(unit.top, depth), unit.bottom, unitTraits(seed, column, unit, domain.materialOf(i))};
        }
    }
}

} // namespace derivean::sim
