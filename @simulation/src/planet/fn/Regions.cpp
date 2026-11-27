#include "planet/fn/Regions.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Coupling.hpp"
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

struct Scratch {
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    std::vector<int32_t> pieces;
};

// Fills piece k's real weights once.
void realOf(const Globe& globe, int32_t k, RealWeights& real, Scratch& scratch) {
    if (real.begin[size_t(k)] >= 0) return;
    const Coupling& coupling = globe.coupling;
    real.begin[size_t(k)] = int32_t(real.weights.size());
    if (coupling.exact[size_t(k)]) {
        real.weights.insert(real.weights.end(), coupling.byPiece.begin() + coupling.pieceStart[size_t(k)], coupling.byPiece.begin() + coupling.pieceStart[size_t(k) + 1]);
    } else {
        exactCoupling(*globe.grid, globe.pieces[size_t(k)], scratch.weights, scratch.cells, scratch.work);
        real.weights.insert(real.weights.end(), scratch.weights.begin(), scratch.weights.end());
    }
    real.end[size_t(k)] = int32_t(real.weights.size());
}

// Classifies cell c from its pieces' overlaps `[begin, end)` in key order; `covered` takes the owner's
// pieces' summed overlaps, at most the cell.
void classify(const Globe& globe, int32_t c, const PieceWeight* begin, const PieceWeight* end, PlateRegions& regions, std::vector<double>& covered) {
    const double area = globe.grid->area[size_t(c)];
    int32_t best = -1;
    double most = 0, own = 0;
    for (const PieceWeight* w = begin; w != end; ++w)
        if (w->area > most) most = w->area, best = w->piece;
    regions.dominant[size_t(c)] = best, regions.owner[size_t(c)] = -1, covered[size_t(c)] = 0;
    if (best < 0 || most < OwnedShare * area) return;
    const int32_t plate = globe.pieces[size_t(best)].plate;
    for (const PieceWeight* w = begin; w != end; ++w)
        if (globe.pieces[size_t(w->piece)].plate == plate) own += w->area;
    regions.owner[size_t(c)] = plate, covered[size_t(c)] = std::min(own, area);
}

// Classifies again by real overlaps every cell a piece of a `measured` plate really reaches or the
// coupling gives such a plate (`base`), and the ring of each, so the cells across a fragment's border are
// read as its own are (joinsOf), from the real overlaps of every piece that reaches it. Those are the
// pieces the coupling lists in the cell and its ring: an exactly coupled piece is listed in every cell it
// overlaps, and a cloud-in-cell piece's 2 × 2 stencil gives its centre's cell a quarter at least and
// reaches towards its centre's nearer neighbours, so some cell of it lies in the ring of any cell its
// footprint reaches. The tight case is a cube corner, where the stencil keeps to the centre's face: there
// the largest footprint's reach over the nearest unlisted cell's distance is 0.984 on L5, 0.992 on L6,
// 0.996 on L7 (independent reviewer); a measured plate's piece the lists miss is fatal. The set only
// grows with `measured`, and a cell's class does not depend on it.
void classifyReal(const Globe& globe, const std::vector<uint8_t>& measured, const std::vector<int32_t>& base, PlateRegions& regions, std::vector<double>& covered, RealCells& at, Scratch& scratch) {
    const Coupling& coupling = globe.coupling;
    const size_t n = regions.owner.size();
    std::vector<uint8_t> reached(n, 0), set(n, 0);
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        if (!measured[globe.pieces[k].plate]) continue;
        realOf(globe, int32_t(k), regions.real, scratch);
        for (int32_t s = regions.real.begin[k]; s < regions.real.end[k]; ++s)
            if (regions.real.weights[size_t(s)].area > 0) reached[size_t(regions.real.weights[size_t(s)].cell)] = 1;
    }
    for (size_t c = 0; c < n; ++c) {
        if (!reached[c] && (base[c] < 0 || !measured[size_t(base[c])])) continue;
        set[c] = 1;
        for (const int32_t m : (*globe.rings)[c])
            if (m >= 0) set[size_t(m)] = 1;
    }
    at.slot.assign(n, -1), at.start.assign(1, 0), at.entries.clear();
    for (size_t c = 0; c < n; ++c) {
        if (!set[c]) continue;
        std::vector<int32_t>& pieces = scratch.pieces;
        pieces.clear();
        const auto gather = [&](int32_t cell) {
            for (int32_t s = coupling.cellStart[size_t(cell)]; s < coupling.cellStart[size_t(cell) + 1]; ++s) pieces.push_back(coupling.byCell[size_t(s)].piece);
        };
        gather(int32_t(c));
        for (const int32_t m : (*globe.rings)[c])
            if (m >= 0) gather(m);
        std::sort(pieces.begin(), pieces.end());
        pieces.erase(std::unique(pieces.begin(), pieces.end()), pieces.end());
        const size_t first = at.entries.size();
        for (const int32_t k : pieces) {
            realOf(globe, k, regions.real, scratch);
            for (int32_t s = regions.real.begin[size_t(k)]; s < regions.real.end[size_t(k)]; ++s) {
                const CellWeight& w = regions.real.weights[size_t(s)];
                if (w.cell == int32_t(c) && w.area > 0) at.entries.push_back({k, w.area});
            }
        }
        at.slot[c] = int32_t(at.start.size() - 1), at.start.push_back(int32_t(at.entries.size()));
        classify(globe, int32_t(c), at.entries.data() + first, at.entries.data() + at.entries.size(), regions, covered);
    }
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        if (!measured[globe.pieces[k].plate]) continue;
        for (int32_t s = regions.real.begin[k]; s < regions.real.end[k]; ++s) {
            const CellWeight& w = regions.real.weights[size_t(s)];
            if (w.area <= 0) continue;
            const int32_t slot = at.slot[size_t(w.cell)];
            const auto listed = [&](const PieceWeight& e) { return e.piece == int32_t(k); };
            require(std::any_of(at.entries.begin() + at.start[size_t(slot)], at.entries.begin() + at.start[size_t(slot) + 1], listed),
                    "a piece of a plate in two regions reaches a cell beyond the coupling's lists round it", w.cell);
        }
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
    const Coupling& coupling = globe.coupling;
    const size_t n = size_t(grid.cells), pieces = globe.pieces.size();
    PlateRegions regions;
    regions.dominant.assign(n, -1), regions.owner.assign(n, -1);
    regions.real.begin.assign(pieces, -1), regions.real.end.assign(pieces, -1);
    std::vector<double> covered(n, 0);   // m², the owner's pieces' summed overlaps, at most the cell
    for (size_t c = 0; c < n; ++c)
        classify(globe, int32_t(c), coupling.byCell.data() + coupling.cellStart[c], coupling.byCell.data() + coupling.cellStart[c + 1], regions, covered);
    Members members;
    group(globe, covered, regions, members);
    // A plate in two regions or more is classified by real overlaps, and a plate that this cuts in two
    // in turn (lead, 27 Nov).
    size_t plates = 0;
    for (const CrustPiece& piece : globe.pieces) plates = std::max(plates, size_t(piece.plate) + 1);
    std::vector<uint8_t> measured(plates, 0);
    std::vector<int32_t> parts(plates);
    const std::vector<int32_t> base = regions.owner;
    RealCells at;
    Scratch scratch;
    for (;;) {
        std::fill(parts.begin(), parts.end(), 0);
        for (const int32_t plate : regions.plate) ++parts[size_t(plate)];
        bool more = false;
        for (size_t p = 0; p < plates; ++p)
            if (parts[p] > 1 && !measured[p]) measured[p] = 1, more = true;
        if (!more) break;
        classifyReal(globe, measured, base, regions, covered, at, scratch);
        group(globe, covered, regions, members);
    }
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
