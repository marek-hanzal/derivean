#include "geometry/fn/Candidates.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"

namespace derivean::sim {
namespace {

// Chords on the unit sphere are compared with this margin over their rounding (about 1e-16).
constexpr double ReachMargin = 1e-12;

// A plain loop: the lists hold about a dozen cells, where a library search costs more than it finds.
bool contains(const std::vector<int>& list, int cell) {
    for (const int c : list)
        if (c == cell) return true;
    return false;
}

} // namespace

void candidateCells(const SphereGrid& grid, const SpherePolygon& footprint, std::vector<int>& cells, std::vector<int>& work) {
    candidateCells(grid, capOf(footprint), cells, work);
}

void candidateCells(const SphereGrid& grid, const PolygonCap& cap, std::vector<int>& cells, std::vector<int>& work) {
    cells.clear();
    work.clear();
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
