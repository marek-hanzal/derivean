#include "planet/fn/GlobeCover.hpp"

#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {

GlobeCover measureCover(const Globe& globe, const std::vector<uint8_t>& skip) {
    const SphereGrid& grid = *globe.grid;
    Coupling exact;
    couple(grid, globe.pieces, {}, true, exact);
    constexpr double Levels[3] = {0.1, 0.2, 0.5};
    GlobeCover cover;
    double measured = 0;
    std::vector<PlanePolygon> parts;
    for (int cell = 0; cell < grid.cells; ++cell) {
        if (!skip.empty() && skip[size_t(cell)]) continue;
        const double area = grid.area[size_t(cell)];
        const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(cell)]);
        const PlanePolygon own = projected(plane, polygonOf(grid, cell));
        double sum = 0;
        parts.clear();
        for (int32_t k = exact.cellStart[size_t(cell)]; k < exact.cellStart[size_t(cell) + 1]; ++k) {
            sum += exact.byCell[size_t(k)].area;
            parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(exact.byCell[size_t(k)].piece)])), own));
        }
        const double united = parts.size() <= 1 ? sum : unionArea(parts).area;
        const double hole = std::max(0.0, area - united) / area, overlap = std::max(0.0, sum - united) / area;
        if (hole > cover.worstHole) cover.worstHole = hole, cover.worstCell = cell;
        cover.worstOverlap = std::max(cover.worstOverlap, overlap);
        for (int i = 0; i < 3; ++i) cover.holes[i] += hole > Levels[i], cover.overlaps[i] += overlap > Levels[i];
        cover.hole += hole * area, cover.overlap += overlap * area, measured += area;
        ++cover.cells;
    }
    if (measured > 0) cover.hole /= measured, cover.overlap /= measured;
    return cover;
}

} // namespace derivean::sim
