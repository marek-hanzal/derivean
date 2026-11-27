#include "planet/fn/Cohesion.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Band.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Seam.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr int WholePasses = 9;     // of splits, else joins, in a round; a join can leave a fragment next to another
constexpr int ClosingRounds = 3;   // of joins and their seams in a step; a closing's births can cut a fragment off

int centreCellOf(const SphereGrid& grid, const CrustPiece& piece) { return indexOf(grid, cellOf(centreOf(grid, piece), grid.level)); }

// Where each region goes: a plate, or -1 where it stays.
std::vector<int32_t> splitsOf(Globe& globe, const PlateRegions& regions) {
    std::vector<int32_t> to(regions.plate.size(), -1);
    for (size_t r = 0; r < to.size(); ++r) {
        const int32_t parent = regions.plate[r];
        if (regions.largest[size_t(parent)] == int32_t(r) || regions.area[r] < MicroplateArea || regions.cells[r] < MicroplateCells) continue;
        const size_t child = globe.epoch.plates.size();
        require(child < 0xffff, "a split makes more plates than a piece can name", double(child));
        const EulerPlate plate = globe.epoch.plates[size_t(parent)];
        GlobeMotion motion = globe.motion[size_t(parent)];
        motion.id = globe.platesMade++;
        const Vec3 applied = globe.applied[size_t(parent)];
        globe.epoch.plates.push_back(plate), globe.motion.push_back(motion), globe.applied.push_back(applied);
        to[r] = int32_t(child);
    }
    return to;
}

// A cell's plate once the regions go where `to` says (empty: nowhere).
int32_t ownerAfter(const PlateRegions& regions, const std::vector<int32_t>& to, int32_t cell) {
    const int32_t r = regions.region[size_t(cell)];
    return r >= 0 && !to.empty() && to[size_t(r)] >= 0 ? to[size_t(r)] : regions.owner[size_t(cell)];
}

// The plate of the cell nearest `from` across rings outside region `skip`, or -1 if none.
int32_t nearestPlateOf(const Globe& globe, const PlateRegions& regions, const std::vector<int32_t>& to, const std::vector<int32_t>& from, int32_t skip) {
    std::vector<uint8_t> seen(regions.owner.size(), 0);
    std::vector<int32_t> front = from;
    for (const int32_t c : from) seen[size_t(c)] = 1;
    for (size_t i = 0; i < front.size(); ++i) {
        const int32_t plate = ownerAfter(regions, to, front[i]);
        if (plate >= 0 && regions.region[size_t(front[i])] != skip) return plate;
        for (const int32_t m : (*globe.rings)[size_t(front[i])])
            if (m >= 0 && !seen[size_t(m)]) seen[size_t(m)] = 1, front.push_back(m);
    }
    return -1;
}

// The plate a fragment joins: the one whose largest region shares the longest border with it (metres of
// cell edges, SphereEdge::length; lead, 27 Nov), else the one sharing the longest border at all (a
// fragment among other fragments), so a join does not feed another plate's fragment; the lower plate on an
// exact tie. One meeting other plates at corners only counts corners, one meeting none the plate of the
// nearest cell (staying if that is its own). Of two fragments joining no largest region that would each
// take the other's plate only the lower goes, else they would swap for ever.
std::vector<int32_t> joinsOf(const Globe& globe, const PlateRegions& regions) {
    const SphereGrid& grid = *globe.grid;
    const size_t plates = globe.epoch.plates.size();
    std::vector<int32_t> to(regions.plate.size(), -1);
    std::vector<uint8_t> aside(regions.plate.size(), 0);   // joins no plate's largest region
    std::vector<double> shared(2 * plates, 0);   // m (corners: their count) with a plate's largest region, then with any of it
    std::vector<std::vector<int32_t>> cells(regions.plate.size());
    for (int c = 0; c < grid.cells; ++c)
        if (regions.region[size_t(c)] >= 0) cells[size_t(regions.region[size_t(c)])].push_back(c);
    const auto count = [&](size_t r, int32_t m, double length) {
        const int32_t other = m >= 0 ? regions.owner[size_t(m)] : -1;
        if (other < 0 || other == regions.plate[r]) return;
        shared[plates + size_t(other)] += length;
        if (regions.largest[size_t(other)] == regions.region[size_t(m)]) shared[size_t(other)] += length;
    };
    for (size_t r = 0; r < to.size(); ++r) {
        if (regions.largest[size_t(regions.plate[r])] == int32_t(r)) continue;
        std::fill(shared.begin(), shared.end(), 0);
        for (const int32_t c : cells[r])
            for (int e = 0; e < 4; ++e) count(r, neighbourOf(grid, c, e), grid.edge[size_t(grid.edges[size_t(c)][size_t(e)])].length);
        if (std::all_of(shared.begin(), shared.end(), [](double n) { return n == 0; }))
            for (const int32_t c : cells[r])
                for (const int32_t m : (*globe.rings)[size_t(c)]) count(r, m, 1);
        double most = 0;
        for (size_t p = 0; p < shared.size() && (most == 0 || p < plates); ++p)
            if (shared[p] > most) most = shared[p], to[r] = int32_t(p % plates), aside[r] = p >= plates;
        if (most == 0) {
            const int32_t plate = nearestPlateOf(globe, regions, {}, cells[r], int32_t(r));
            to[r] = plate == regions.plate[r] ? -1 : plate, aside[r] = 1;
        }
        for (size_t s = 0; s < r && aside[r]; ++s)
            if (aside[s] && to[s] == regions.plate[r] && regions.plate[s] == to[r]) to[r] = -1;
    }
    return to;
}

// Deals the pieces of every region that goes somewhere to its plate; per region the pieces dealt, and
// the cells a join leaves (the regions' and the centres of their pieces). A piece of the plate that
// covers no cell most and is centred off its plate's cells goes with the moving region it covers most:
// left behind, it would cover the region most once the closing took the rest, and the births take its plate.
std::vector<int32_t> deal(Globe& globe, const PlateRegions& regions, const std::vector<int32_t>& to, std::vector<int32_t>& left) {
    const SphereGrid& grid = *globe.grid;
    const Coupling& coupling = globe.coupling;
    std::vector<int32_t> target(globe.pieces.size(), -1), dealt(to.size(), 0), from(globe.pieces.size(), -1);
    std::vector<uint8_t> dominant(globe.pieces.size(), 0);
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        const int32_t r = regions.region[size_t(centreCellOf(grid, globe.pieces[k]))];
        if (r >= 0 && to[size_t(r)] >= 0 && regions.plate[size_t(r)] == globe.pieces[k].plate) target[k] = to[size_t(r)], from[k] = r;
    }
    for (int c = 0; c < grid.cells; ++c) {
        const int k = dominantPiece(coupling, c);
        if (k >= 0) dominant[size_t(k)] = 1;
        const int32_t r = regions.region[size_t(c)];
        if (r < 0 || to[size_t(r)] < 0) continue;
        if (target[size_t(k)] < 0) target[size_t(k)] = to[size_t(r)], from[size_t(k)] = r;
        left.push_back(c);
    }
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        const CrustPiece& piece = globe.pieces[k];
        if (target[k] >= 0 || dominant[k] || regions.owner[size_t(centreCellOf(grid, piece))] == piece.plate) continue;
        double most = 0;
        for (int32_t w = coupling.pieceStart[k]; w < coupling.pieceStart[k + 1]; ++w) {
            const CellWeight& weight = coupling.byPiece[size_t(w)];
            const int32_t r = regions.region[size_t(weight.cell)];
            if (r >= 0 && to[size_t(r)] >= 0 && regions.plate[size_t(r)] == piece.plate && weight.area > most) most = weight.area, target[k] = to[size_t(r)], from[k] = r;
        }
    }
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        if (target[k] < 0) continue;
        globe.pieces[k].plate = uint16_t(target[k]);
        ++dealt[size_t(from[k])];
        left.push_back(centreCellOf(grid, globe.pieces[k]));
    }
    return dealt;
}

// The pieces of plates left with no cell go to the plate of the cell under their centre once the
// pass's joins `to` are dealt, or of the nearest cell that has one; their centres' cells are left to
// close. How many pieces moved.
long dealStranded(Globe& globe, const PlateRegions& regions, const std::vector<int32_t>& to, std::vector<int32_t>& left) {
    const SphereGrid& grid = *globe.grid;
    long moved = 0;
    for (CrustPiece& piece : globe.pieces) {
        if (piece.plate < regions.largest.size() && regions.largest[piece.plate] >= 0) continue;
        const int cell = centreCellOf(grid, piece);
        const int32_t plate = nearestPlateOf(globe, regions, to, {cell}, -1);
        if (plate < 0) continue;
        piece.plate = uint16_t(plate), ++moved;
        left.push_back(cell);
    }
    return moved;
}

// Drops the plates no piece is left on, in order, and renumbers the rest.
void dropEmptyPlates(Globe& globe) {
    const size_t n = globe.epoch.plates.size();
    std::vector<long> pieces(n, 0);
    for (const CrustPiece& piece : globe.pieces) ++pieces[piece.plate];
    if (std::find(pieces.begin(), pieces.end(), 0) == pieces.end()) return;
    std::vector<int32_t> to(n, -1);
    std::vector<EulerPlate> plates;
    std::vector<GlobeMotion> motion;
    std::vector<Vec3> applied;
    size_t sites = 0;
    for (size_t p = 0; p < n; ++p) {
        if (pieces[p] == 0) {
            globe.plateEnds.push_back({globe.time, globe.motion[p].id});
            continue;
        }
        to[p] = int32_t(plates.size());
        plates.push_back(globe.epoch.plates[p]), motion.push_back(globe.motion[p]), applied.push_back(globe.applied[p]);
        sites += p < globe.epoch.sites;
    }
    for (CrustPiece& piece : globe.pieces) piece.plate = uint16_t(to[piece.plate]);
    globe.epoch.plates = std::move(plates), globe.motion = std::move(motion), globe.applied = std::move(applied);
    if (globe.epoch.sites) globe.epoch.sites = sites;
}

// One pass: every region large enough splits off; if none does and `joins`, every other fragment joins
// the plate round it, and the pieces of a plate with no cell the plate under them. Whether anything moved.
bool regroup(Globe& globe, bool joins, std::vector<int32_t>& joined) {
    const PlateRegions regions = plateRegionsOf(globe);
    std::vector<int32_t> to = splitsOf(globe, regions);
    const bool split = std::any_of(to.begin(), to.end(), [](int32_t p) { return p >= 0; });
    if (split) {
        std::vector<int32_t> cells;
        const std::vector<int32_t> dealt = deal(globe, regions, to, cells);
        for (size_t r = 0; r < to.size(); ++r)
            if (to[r] >= 0) {
                globe.splits.push_back({globe.time, globe.motion[size_t(regions.plate[r])].id, globe.motion[size_t(to[r])].id, regions.area[r], dealt[r]});
                ++globe.stats.plateSplits;
            }
        return true;
    }
    if (!joins) return false;
    to = joinsOf(globe, regions);
    const long count = long(std::count_if(to.begin(), to.end(), [](int32_t p) { return p >= 0; }));
    if (count > 0) deal(globe, regions, to, joined);
    const long stranded = dealStranded(globe, regions, to, joined);
    globe.stats.fragmentsJoined += count, globe.stats.strandedPieces += stranded;
    return count > 0 || stranded > 0;
}

// Passes until nothing moves, WholePasses at most; keepPlatesWhole checks the end.
void settle(Globe& globe, bool joins, std::vector<int32_t>& joined) {
    for (int pass = 0; pass < WholePasses && regroup(globe, joins, joined); ++pass) {}
}

} // namespace

void keepPlatesWhole(Globe& globe, int step) {
    // Closing phase 0 is the reorganisation's (planet/fn/Seam.hpp); the joins take 1 to ClosingRounds and
    // the last round only splits.
    for (int phase = 1;; ++phase) {
        std::vector<int32_t> joined;
        settle(globe, phase <= ClosingRounds, joined);
        if (joined.empty()) break;
        Band band;
        markBandAt(globe, 1, joined, band);
        closeSeams(globe, band, step, phase);
    }
    dropEmptyPlates(globe);
    // Whole: every plate one region, whichever way the step got there.
    const PlateRegions regions = plateRegionsOf(globe);
    bool whole = regions.largest.size() == globe.epoch.plates.size();
    for (size_t r = 0; r < regions.plate.size() && whole; ++r) whole = regions.largest[size_t(regions.plate[r])] == int32_t(r);
    for (size_t p = 0; p < regions.largest.size() && whole; ++p) whole = regions.largest[p] >= 0;
    globe.stats.wholeUnsettled += !whole;
}

} // namespace derivean::sim
