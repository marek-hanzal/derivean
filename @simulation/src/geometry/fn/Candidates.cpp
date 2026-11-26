#include "geometry/fn/Candidates.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

// Chords on the unit sphere are compared with this margin over their rounding (about 1e-16).
constexpr double ReachMargin = 1e-12;

bool contains(const std::vector<int>& list, int cell) { return std::find(list.begin(), list.end(), cell) != list.end(); }

} // namespace

void candidateCells(const SphereGrid& grid, const SpherePolygon& footprint, std::vector<int>& cells, std::vector<int>& work) {
    cells.clear();
    work.clear();
    const PolygonCap cap = capOf(footprint);
    cells.push_back(indexOf(grid, cellOf(cap.centre, grid.level)));
    for (size_t next = 0; next < cells.size(); ++next)
        for (int k = 0; k < 4; ++k) {
            const int cell = neighbourOf(grid, cells[next], k);
            if (contains(cells, cell) || contains(work, cell)) continue;
            const double reach = cap.reach + grid.reach[size_t(cell)] + ReachMargin;
            if (length(cap.centre - grid.centre[size_t(cell)]) <= reach) cells.push_back(cell);
            else work.push_back(cell);
        }
}

} // namespace derivean::sim
