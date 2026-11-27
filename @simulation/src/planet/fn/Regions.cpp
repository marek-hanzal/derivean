#include "planet/fn/Regions.hpp"

#include "geometry/constant/Sphere.hpp"
#include "planet/fn/Coupling.hpp"

#include <algorithm>

namespace derivean::sim {

PlateRegions plateRegionsOf(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const Coupling& coupling = globe.coupling;
    const size_t n = size_t(grid.cells);
    PlateRegions regions;
    regions.dominant.assign(n, -1);
    regions.owner.assign(n, -1);
    std::vector<double> covered(n, 0);   // m², by the owner's pieces, at most the cell
    int32_t plates = 0;
    for (size_t c = 0; c < n; ++c) {
        const int k = regions.dominant[c] = dominantPiece(coupling, int(c));
        if (k < 0) continue;
        double most = 0, own = 0;
        const int32_t plate = globe.pieces[size_t(k)].plate;
        for (int32_t s = coupling.cellStart[c]; s < coupling.cellStart[c + 1]; ++s) {
            const PieceWeight& w = coupling.byCell[size_t(s)];
            most = std::max(most, w.area);
            if (globe.pieces[size_t(w.piece)].plate == plate) own += w.area;
        }
        if (most < OwnedShare * grid.area[c]) continue;
        regions.owner[c] = plate, plates = std::max(plates, plate + 1);
        covered[c] = std::min(own, grid.area[c]);
    }
    regions.region.assign(n, -1);
    std::vector<int32_t> stack, through(n, -1);   // through: the region whose search last crossed a free cell
    for (size_t c = 0; c < n; ++c) {
        if (regions.region[c] >= 0 || regions.owner[c] < 0) continue;
        const int32_t id = int32_t(regions.plate.size()), plate = regions.owner[c];
        regions.plate.push_back(plate), regions.area.push_back(0), regions.cells.push_back(0);
        regions.region[c] = id, stack.assign(1, int32_t(c));
        while (!stack.empty()) {
            const int32_t x = stack.back();
            stack.pop_back();
            if (regions.owner[size_t(x)] == plate)
                regions.area[size_t(id)] += covered[size_t(x)] / (PlanetRadius * PlanetRadius), ++regions.cells[size_t(id)];
            for (const int32_t m : (*globe.rings)[size_t(x)]) {
                if (m < 0) continue;
                if (regions.owner[size_t(m)] == plate && regions.region[size_t(m)] < 0) regions.region[size_t(m)] = id, stack.push_back(m);
                else if (regions.owner[size_t(m)] < 0 && through[size_t(m)] != id) through[size_t(m)] = id, stack.push_back(m);
            }
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
