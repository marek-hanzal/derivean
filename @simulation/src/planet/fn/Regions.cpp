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
// cell, also where the coupling's weights are cloud-in-cell (lead, 27 Nov). The pieces are those the
// cell's and its ring's coupling lists hold: an exactly coupled piece is listed in every cell it
// overlaps, and a cloud-in-cell piece's 2 × 2 stencil reaches towards its centre's nearer neighbours,
// so some cell of it lies in the ring of any cell its footprint reaches. The tight case is a cube
// corner, where the stencil keeps to the centre's face: there the largest footprint's reach over the
// nearest unlisted cell's distance is 0.984 on L5, 0.992 on L6, 0.996 on L7 (independent reviewer).
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
    std::vector<double> covered(n, 0);   // m², the owner's pieces' summed overlaps, at most the cell
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
    std::vector<int32_t> members, start;           // each region's owned cells, region after region
    for (size_t c = 0; c < n; ++c) {
        if (regions.region[c] >= 0 || regions.owner[c] < 0) continue;
        const int32_t id = int32_t(regions.plate.size()), plate = regions.owner[c];
        regions.plate.push_back(plate), regions.area.push_back(0), regions.cells.push_back(0);
        start.push_back(int32_t(members.size()));
        regions.region[c] = id, stack.assign(1, int32_t(c));
        while (!stack.empty()) {
            const int32_t x = stack.back();
            stack.pop_back();
            if (regions.owner[size_t(x)] == plate) {
                regions.area[size_t(id)] += covered[size_t(x)] / (PlanetRadius * PlanetRadius), ++regions.cells[size_t(id)];
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
    // A plate in two regions or more is measured whole, every region by the union of its real footprints,
    // before its largest is chosen and its fragments weighed: the summed overlaps count crust twice, and
    // with cloud-in-cell weights they are no bound (Codex DREV-CLAUDE-008-05, lead 27 Nov). A plate in one
    // region decides nothing by its area and keeps the sum.
    const size_t count = regions.plate.size();
    std::vector<int32_t> parts(size_t(plates), 0);
    for (const int32_t plate : regions.plate) ++parts[size_t(plate)];
    regions.largest.assign(size_t(plates), -1);
    for (size_t r = 0; r < count; ++r) {
        const size_t plate = size_t(regions.plate[r]);
        if (parts[plate] > 1) regions.area[r] = unionCover(globe, members.data() + start[r], members.data() + start[r + 1], regions.plate[r]);
        int32_t& largest = regions.largest[plate];
        if (largest < 0 || regions.area[r] > regions.area[size_t(largest)]) largest = int32_t(r);
    }
    return regions;
}

} // namespace derivean::sim
