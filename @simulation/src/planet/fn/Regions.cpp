#include "planet/fn/Regions.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Owner.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

// Per cell classified by real overlaps, the pieces that reach it with their overlaps, in key order.
struct RealCells {
    std::vector<int32_t> slot;          // per cell: its index in `start`, or -1
    std::vector<int32_t> start;         // per such cell + 1
    std::vector<PieceWeight> entries;
};

// Each region's owned cells, region after region.
struct Members {
    std::vector<int32_t> cells, start;
};

// Classifies again by real overlaps every cell a piece of a `measured` plate really reaches or that
// such a plate owns, and the ring of each, from the real overlaps of every piece that can reach it
// (planet/fn/Owner), so the cells a fragment holds and those across its border, which `deal` and
// `joinsOf` read, carry real dominant pieces and covers. classifyCells has their owners already.
void classifyReal(const Globe& globe, const CellCandidates& candidates, const std::vector<uint8_t>& measured, PlateRegions& regions, std::vector<double>& covered, RealCells& at, OwnerScratch& scratch) {
    const size_t n = regions.owner.size();
    std::vector<uint8_t> reached(n, 0), set(n, 0);
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        if (!measured[globe.pieces[k].plate]) continue;
        realOf(globe, int32_t(k), regions.real, scratch);
        for (int32_t s = regions.real.begin[k]; s < regions.real.end[k]; ++s)
            if (regions.real.weights[size_t(s)].area > 0) reached[size_t(regions.real.weights[size_t(s)].cell)] = 1;
    }
    for (size_t c = 0; c < n; ++c) {
        if (!reached[c] && (regions.owner[c] < 0 || !measured[size_t(regions.owner[c])])) continue;
        set[c] = 1;
        for (const int32_t m : (*globe.rings)[c])
            if (m >= 0) set[size_t(m)] = 1;
    }
    at.slot.assign(n, -1), at.start.assign(1, 0), at.entries.clear();
    for (size_t c = 0; c < n; ++c) {
        if (!set[c]) continue;
        const size_t first = at.entries.size();
        realEntriesOf(globe, candidates, int32_t(c), regions.real, scratch, at.entries);
        at.slot[c] = int32_t(at.start.size() - 1), at.start.push_back(int32_t(at.entries.size()));
        const int32_t owner = regions.owner[c];
        classify(globe, int32_t(c), at.entries.data() + first, at.entries.data() + at.entries.size(), regions, covered);
        require(regions.owner[c] == owner, "real overlaps give a cell another owner than its classification", double(c));
    }
}

// Groups the owned cells into regions, each with its summed cover and its cells.
void group(const Globe& globe, const std::vector<double>& covered, PlateRegions& regions, Members& members) {
    const size_t n = regions.owner.size();
    regions.region.assign(n, -1), regions.plate.clear(), regions.area.clear(), regions.cells.clear();
    members.cells.clear(), members.start.clear();
    std::vector<int32_t> stack, through(n, -1);   // through: the region whose search last crossed a free cell
    for (size_t c = 0; c < n; ++c) {
        if (regions.region[c] >= 0 || regions.owner[c] < 0) continue;
        const int32_t id = int32_t(regions.plate.size()), plate = regions.owner[c];
        regions.plate.push_back(plate), regions.area.push_back(0), regions.cells.push_back(0);
        members.start.push_back(int32_t(members.cells.size()));
        regions.region[c] = id, stack.assign(1, int32_t(c));
        while (!stack.empty()) {
            const int32_t x = stack.back();
            stack.pop_back();
            if (regions.owner[size_t(x)] == plate) {
                regions.area[size_t(id)] += covered[size_t(x)] / (PlanetRadius * PlanetRadius), ++regions.cells[size_t(id)];
                members.cells.push_back(x);
            }
            for (const int32_t m : (*globe.rings)[size_t(x)]) {
                if (m < 0) continue;
                if (regions.owner[size_t(m)] == plate && regions.region[size_t(m)] < 0) regions.region[size_t(m)] = id, stack.push_back(m);
                else if (regions.owner[size_t(m)] < 0 && through[size_t(m)] != id) through[size_t(m)] = id, stack.push_back(m);
            }
        }
    }
    members.start.push_back(int32_t(members.cells.size()));
}

// sr a plate covers in the given cells, all classified by real overlaps: per cell the union of the real
// footprints of its pieces that reach it, clipped to the cell.
double unionCover(const Globe& globe, const RealCells& at, const int32_t* begin, const int32_t* end, int32_t plate) {
    const SphereGrid& grid = *globe.grid;
    std::vector<PlanePolygon> parts;
    double cover = 0;
    for (const int32_t* c = begin; c != end; ++c) {
        const int32_t slot = at.slot[size_t(*c)];
        require(slot >= 0, "a cell of a plate in two regions is not classified by real overlaps", *c);
        const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(*c)]);
        const PlanePolygon cell = projected(plane, polygonOf(grid, *c));
        parts.clear();
        for (int32_t s = at.start[size_t(slot)]; s < at.start[size_t(slot) + 1]; ++s) {
            const CrustPiece& piece = globe.pieces[size_t(at.entries[size_t(s)].piece)];
            if (piece.plate != plate) continue;
            PlanePolygon part = clipped(projected(plane, footprintOf(grid, piece)), cell);
            if (part.count >= 3) parts.push_back(std::move(part));
        }
        if (!parts.empty()) cover += std::min(unionArea(parts).area, grid.area[size_t(*c)]);
    }
    return cover / (PlanetRadius * PlanetRadius);
}

} // namespace

PlateRegions plateRegionsOf(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const size_t n = size_t(grid.cells), pieces = globe.pieces.size();
    PlateRegions regions;
    regions.dominant.assign(n, -1), regions.owner.assign(n, -1);
    regions.real.begin.assign(pieces, -1), regions.real.end.assign(pieces, -1);
    std::vector<double> covered(n, 0);   // m², the owner's pieces' summed overlaps, at most the cell
    const CellCandidates candidates = candidatesOf(globe);
    OwnerScratch scratch;
    classifyCells(globe, candidates, regions, covered, scratch);
    Members members;
    group(globe, covered, regions, members);
    // A plate in two regions or more is classified by real overlaps (lead, 27 Nov); its cells keep their
    // owners, so its regions and their cells stay, and its regions' areas are their unions below.
    size_t plates = 0;
    for (const CrustPiece& piece : globe.pieces) plates = std::max(plates, size_t(piece.plate) + 1);
    std::vector<int32_t> parts(plates, 0);
    for (const int32_t plate : regions.plate) ++parts[size_t(plate)];
    std::vector<uint8_t> measured(plates, 0);
    bool any = false;
    for (size_t p = 0; p < plates; ++p) measured[p] = parts[p] > 1, any = any || parts[p] > 1;
    RealCells at;
    if (any) classifyReal(globe, candidates, measured, regions, covered, at, scratch);
    // Measured whole, every region of a plate in two by the union of its real footprints, before its
    // largest is chosen and its fragments weighed (Codex DREV-CLAUDE-008-05). A plate in one region
    // decides nothing by its area and keeps the sum.
    int32_t owners = 0;
    for (const int32_t plate : regions.plate) owners = std::max(owners, plate + 1);
    regions.largest.assign(size_t(owners), -1);
    for (size_t r = 0; r < regions.plate.size(); ++r) {
        const int32_t plate = regions.plate[r];
        if (parts[size_t(plate)] > 1) regions.area[r] = unionCover(globe, at, members.cells.data() + members.start[r], members.cells.data() + members.start[r + 1], plate);
        int32_t& largest = regions.largest[size_t(plate)];
        if (largest < 0 || regions.area[r] > regions.area[size_t(largest)]) largest = int32_t(r);
    }
    return regions;
}

} // namespace derivean::sim
