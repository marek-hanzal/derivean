#include "strata/fn/Window.hpp"

#include "landscape/type/Level.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

// m the planet took off a column since the region window opened, less what it laid on.
double removedInWindow(const Planet& planet, const CrustColumn& column) {
    const double start = std::min(windowAt(Level::Region, planet.time).start, planet.time);
    if (planet.time - start <= 0) return 0;
    const ColumnRecord& record = planet.records.cells[size_t(&column - planet.columns.cells.data())];
    const PastMark& past = record.past[size_t(pastMarkIndex(start))];
    return (column.eroded - past.eroded) - (column.deposited - past.deposited);
}

// Puts back on top, as the youngest unit present, what the window removed, and takes off what it
// laid down, youngest first. Where no cover is left the basement simply reaches higher.
void restoreRemoved(StrataColumn& column, float removed) {
    column.restored = removed;
    float left = -removed;
    for (int back = 0; back < StrataSlots; ++back) {
        float& thickness = column.thickness[size_t((column.newest - back + StrataSlots) % StrataSlots)];
        if (thickness <= 0) continue;
        if (removed > 0) {
            thickness += removed;
            column.coverTotal += removed;
            return;
        }
        const float taken = std::min(left, thickness);
        thickness -= taken, column.coverTotal -= taken, left -= taken;
        if (left <= 0) return;
    }
    if (column.older <= 0) return;
    const float change = removed > 0 ? removed : -std::min(left, column.older);
    column.older += change, column.coverTotal += change;
}

} // namespace

void restoreWindow(const Planet& planet, const ColumnBlend& around, StrataColumn& column) {
    double removed = 0;
    for (int corner = 0; corner < 4; ++corner) removed += around.weight[corner] * removedInWindow(planet, *around.column[corner]);
    restoreRemoved(column, float(removed));
}

} // namespace derivean::sim
