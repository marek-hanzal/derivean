#pragma once

#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/type/Globe.hpp"

#include <algorithm>
#include <vector>

namespace derivean::sim::globe_test {

// Whether the plates hold together (lead, 27 Nov): each cell belongs to the plate of the piece that
// covers most of it (exact coupling), and the cells of one plate joined across cell edges make its
// regions. On Earth a plate is one region; the small plates are plates of their own (Bird 2003,
// docs/credits). `stray` counts the cells outside their plate's largest region; two regions of one size
// both count as largest. A bare cell belongs to no plate and joins no region.
struct Cohesion {
    long regions{}, plates{};   // regions of all plates; plates with any cell
    long stray{}, cells{};      // cells outside their plate's largest region; cells with an owner
    long largestStray{};        // cells of the largest region that is not its plate's largest
};

inline Cohesion measureCohesion(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    Coupling exact;
    couple(grid, globe.pieces, {}, true, exact);
    const size_t n = grid.area.size();
    std::vector<int> owner(n, -1);
    for (size_t c = 0; c < n; ++c) {
        double most = 0;
        for (int32_t w = exact.cellStart[c]; w < exact.cellStart[c + 1]; ++w)
            if (exact.byCell[size_t(w)].area > most) most = exact.byCell[size_t(w)].area, owner[c] = globe.pieces[size_t(exact.byCell[size_t(w)].piece)].plate;
    }
    std::vector<int> region(n, -1), stack;
    std::vector<long> size;
    for (size_t c = 0; c < n; ++c) {
        if (region[c] >= 0 || owner[c] < 0) continue;
        const int id = int(size.size());
        size.push_back(0);
        region[c] = id, stack.assign(1, int(c));
        while (!stack.empty()) {
            const int x = stack.back();
            stack.pop_back();
            ++size[size_t(id)];
            for (int e = 0; e < 4; ++e) {
                const int m = neighbourOf(grid, x, e);
                if (region[size_t(m)] < 0 && owner[size_t(m)] == owner[c]) region[size_t(m)] = id, stack.push_back(m);
            }
        }
    }
    std::vector<long> largest(globe.motion.size(), 0);   // per plate
    for (size_t c = 0; c < n; ++c)
        if (owner[c] >= 0) largest[size_t(owner[c])] = std::max(largest[size_t(owner[c])], size[size_t(region[c])]);
    Cohesion cohesion;
    cohesion.regions = long(size.size());
    for (const long l : largest) cohesion.plates += l > 0;
    for (size_t c = 0; c < n; ++c) {
        if (owner[c] < 0) continue;
        ++cohesion.cells;
        const long s = size[size_t(region[c])];
        if (s == largest[size_t(owner[c])]) continue;
        ++cohesion.stray;
        cohesion.largestStray = std::max(cohesion.largestStray, s);
    }
    return cohesion;
}

} // namespace derivean::sim::globe_test
