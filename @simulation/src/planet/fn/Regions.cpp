#include "planet/fn/Regions.hpp"

#include "geometry/constant/Sphere.hpp"
#include "planet/fn/Coupling.hpp"

#include <algorithm>

namespace derivean::sim {

PlateRegions plateRegionsOf(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const size_t n = size_t(grid.cells);
    PlateRegions regions;
    regions.owner.assign(n, -1);
    int32_t plates = 0;
    for (size_t c = 0; c < n; ++c) {
        const int k = dominantPiece(globe.coupling, int(c));
        if (k >= 0) regions.owner[c] = globe.pieces[size_t(k)].plate, plates = std::max(plates, regions.owner[c] + 1);
    }
    regions.region.assign(n, -1);
    std::vector<int32_t> stack;
    for (size_t c = 0; c < n; ++c) {
        if (regions.region[c] >= 0 || regions.owner[c] < 0) continue;
        const int32_t id = int32_t(regions.plate.size());
        regions.plate.push_back(regions.owner[c]), regions.area.push_back(0), regions.cells.push_back(0);
        regions.region[c] = id, stack.assign(1, int32_t(c));
        while (!stack.empty()) {
            const int32_t x = stack.back();
            stack.pop_back();
            regions.area[size_t(id)] += grid.area[size_t(x)] / (PlanetRadius * PlanetRadius), ++regions.cells[size_t(id)];
            for (const int32_t m : (*globe.rings)[size_t(x)])
                if (m >= 0 && regions.region[size_t(m)] < 0 && regions.owner[size_t(m)] == regions.owner[c]) regions.region[size_t(m)] = id, stack.push_back(m);
        }
    }
    regions.largest.assign(size_t(plates), -1);
    for (size_t r = 0; r < regions.plate.size(); ++r) {
        int32_t& largest = regions.largest[size_t(regions.plate[r])];
        if (largest < 0 || regions.area[r] > regions.area[size_t(largest)]) largest = int32_t(r);
    }
    return regions;
}

} // namespace derivean::sim
