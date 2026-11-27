#include "planet/fn/Regions.hpp"

#include "core/fn/Listed.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

// sr a plate covers in the given cells: per cell the union of its pieces' real footprints clipped to the
// cell. The pieces are those its own and its ring's coupling lists hold: a footprint of about a cell
// reaches a cell only from a piece centred within its ring, and a cloud-in-cell piece is listed in the
// cell under its centre, so also where the coupling's weights are cloud-in-cell (lead, 27 Nov).
double unionCover(const Globe& globe, const int32_t* begin, const int32_t* end, int32_t plate) {
    const SphereGrid& grid = *globe.grid;
    const Coupling& coupling = globe.coupling;
    std::vector<int32_t> pieces;
    std::vector<PlanePolygon> parts;
    double cover = 0;
    for (const int32_t* c = begin; c != end; ++c) {
        pieces.clear();
        const auto gather = [&](int32_t cell) {
            for (int32_t s = coupling.cellStart[size_t(cell)]; s < coupling.cellStart[size_t(cell) + 1]; ++s) {
                const int32_t k = coupling.byCell[size_t(s)].piece;
                if (globe.pieces[size_t(k)].plate == plate && !listed(pieces, k)) pieces.push_back(k);
            }
        };
        gather(*c);
        for (const int32_t m : (*globe.rings)[size_t(*c)])
            if (m >= 0) gather(m);
        const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(*c)]);
        const PlanePolygon cell = projected(plane, polygonOf(grid, *c));
        parts.clear();
        for (const int32_t k : pieces) {
            PlanePolygon part = clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(k)])), cell);
            if (part.count >= 3) parts.push_back(std::move(part));
        }
        if (!parts.empty()) cover += std::min(unionArea(parts).area, grid.area[size_t(*c)]);
    }
    return cover / (PlanetRadius * PlanetRadius);
}

} // namespace

PlateRegions plateRegionsOf(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const Coupling& coupling = globe.coupling;
    const size_t n = size_t(grid.cells);
    PlateRegions regions;
    regions.dominant.assign(n, -1);
    regions.owner.assign(n, -1);
    // m², by the owner's pieces, at most the cell: their summed overlaps (at least the union) and the
    // largest of them (at most the union).
    std::vector<double> covered(n, 0), least(n, 0);
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
        covered[c] = std::min(own, grid.area[c]), least[c] = std::min(most, grid.area[c]);
    }
    regions.region.assign(n, -1);
    std::vector<int32_t> stack, through(n, -1);   // through: the region whose search last crossed a free cell
    std::vector<int32_t> members, start;           // each region's owned cells, region after region
    std::vector<double> lower;                     // per region, sr of the largest overlap per cell
    for (size_t c = 0; c < n; ++c) {
        if (regions.region[c] >= 0 || regions.owner[c] < 0) continue;
        const int32_t id = int32_t(regions.plate.size()), plate = regions.owner[c];
        regions.plate.push_back(plate), regions.area.push_back(0), regions.cells.push_back(0);
        lower.push_back(0), start.push_back(int32_t(members.size()));
        regions.region[c] = id, stack.assign(1, int32_t(c));
        while (!stack.empty()) {
            const int32_t x = stack.back();
            stack.pop_back();
            if (regions.owner[size_t(x)] == plate) {
                regions.area[size_t(id)] += covered[size_t(x)] / (PlanetRadius * PlanetRadius), ++regions.cells[size_t(id)];
                lower[size_t(id)] += least[size_t(x)] / (PlanetRadius * PlanetRadius);
                members.push_back(x);
            }
            for (const int32_t m : (*globe.rings)[size_t(x)]) {
                if (m < 0) continue;
                if (regions.owner[size_t(m)] == plate && regions.region[size_t(m)] < 0) regions.region[size_t(m)] = id, stack.push_back(m);
                else if (regions.owner[size_t(m)] < 0 && through[size_t(m)] != id) through[size_t(m)] = id, stack.push_back(m);
            }
        }
    }
    start.push_back(int32_t(members.size()));
    const size_t count = regions.plate.size();
    std::vector<uint8_t> exact(count, 0);
    const auto measure = [&](size_t r) {
        if (!exact[r]) regions.area[r] = unionCover(globe, members.data() + start[r], members.data() + start[r + 1], regions.plate[r]), exact[r] = 1;
    };
    // The largest: the region of its plate's best lower bound (the first of equal ones), unless another
    // region's union reaches that bound; only one whose upper bound does can, so only those are measured,
    // and where one reaches it the union decides among them all (the first of equal ones).
    regions.largest.assign(size_t(plates), -1);
    for (size_t r = 0; r < count; ++r) {
        int32_t& holder = regions.largest[size_t(regions.plate[r])];
        if (holder < 0 || lower[r] > lower[size_t(holder)]) holder = int32_t(r);
    }
    const std::vector<int32_t> holders = regions.largest;
    std::vector<uint8_t> contested(size_t(plates), 0);
    for (size_t r = 0; r < count; ++r) {
        const size_t plate = size_t(regions.plate[r]);
        const double reach = lower[size_t(holders[plate])];
        if (int32_t(r) == holders[plate] || regions.area[r] < reach) continue;
        measure(r);
        contested[plate] = contested[plate] || regions.area[r] >= reach;
    }
    for (size_t p = 0; p < size_t(plates); ++p)
        if (contested[p]) measure(size_t(holders[p])), regions.largest[p] = -1;
    for (size_t r = 0; r < count; ++r) {
        int32_t& largest = regions.largest[size_t(regions.plate[r])];
        if (contested[size_t(regions.plate[r])] && exact[r] && (largest < 0 || regions.area[r] > regions.area[size_t(largest)])) largest = int32_t(r);
    }
    // A fragment whose bound reaches a microplate is measured: its overlaps may count crust twice.
    for (size_t r = 0; r < count; ++r)
        if (regions.largest[size_t(regions.plate[r])] != int32_t(r) && regions.cells[r] >= MicroplateCells && regions.area[r] >= MicroplateArea) measure(r);
    return regions;
}

} // namespace derivean::sim
