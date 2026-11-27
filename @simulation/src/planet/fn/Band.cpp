#include "planet/fn/Band.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// Widens a set of cells by `rings` rings, in place; `cells` lists the set.
void widen(const std::vector<CellRing>& rings, std::vector<uint8_t>& set, std::vector<int32_t>& cells, int count) {
    std::vector<int32_t> frontier(cells), next;
    for (int ring = 0; ring < count; ++ring) {
        next.clear();
        for (const int32_t cell : frontier)
            for (const int32_t other : rings[size_t(cell)])
                if (other >= 0 && !set[size_t(other)]) {
                    set[size_t(other)] = 1;
                    next.push_back(other);
                    cells.push_back(other);
                }
        frontier.swap(next);
    }
    std::sort(cells.begin(), cells.end());
}

// The cells within `count` rings of one cell (a few dozen).
void around(const std::vector<CellRing>& rings, int cell, int count, std::vector<int32_t>& out) {
    out.assign(1, cell);
    for (int ring = 0, from = 0; ring < count; ++ring) {
        const int to = int(out.size());
        for (int i = from; i < to; ++i)
            for (const int32_t other : rings[size_t(out[size_t(i)])])
                if (other >= 0 && std::find(out.begin(), out.end(), other) == out.end()) out.push_back(other);
        from = to;
    }
}

// The band round a marked core: its deciding cells, its region and the pieces listed there.
void fillBand(const Globe& globe, int sweep, const std::vector<uint8_t>& core, const std::vector<int32_t>& coreCells, Band& band) {
    const SphereGrid& grid = *globe.grid;
    const std::vector<CellRing>& rings = *globe.rings;
    const Coupling& coupling = globe.coupling;
    band.sweep = sweep;
    band.decide = core;
    band.cells = coreCells;
    widen(rings, band.decide, band.cells, sweep + 1);
    std::vector<uint8_t> reach = core;
    std::vector<int32_t> reached = coreCells;
    widen(rings, reach, reached, 2 * sweep + 3);
    band.regionCells = reached;
    band.region.assign(size_t(grid.cells), -1);
    for (size_t r = 0; r < reached.size(); ++r) band.region[size_t(reached[r])] = int32_t(r);

    band.member.assign(globe.pieces.size(), 0);
    band.dead.assign(globe.pieces.size(), 0);
    band.pieces.clear();
    for (const int32_t cell : reached)
        for (int32_t k = coupling.cellStart[size_t(cell)]; k < coupling.cellStart[size_t(cell) + 1]; ++k) {
            const int32_t piece = coupling.byCell[size_t(k)].piece;
            if (!band.member[size_t(piece)]) band.member[size_t(piece)] = 1, band.pieces.push_back(piece);
        }
    std::sort(band.pieces.begin(), band.pieces.end());
}

} // namespace

void markBand(const Globe& globe, int sweep, Band& band) {
    const SphereGrid& grid = *globe.grid;
    const std::vector<CellRing>& rings = *globe.rings;
    const Coupling& coupling = globe.coupling;
    // A cell's plate, or -2 where two plates already share it, -1 where nothing covers it.
    std::vector<int32_t> plate(size_t(grid.cells), -1);
    for (int cell = 0; cell < grid.cells; ++cell)
        for (int32_t k = coupling.cellStart[size_t(cell)]; k < coupling.cellStart[size_t(cell) + 1]; ++k) {
            const int32_t own = globe.pieces[size_t(coupling.byCell[size_t(k)].piece)].plate;
            int32_t& seen = plate[size_t(cell)];
            seen = seen == -1 || seen == own ? own : -2;
        }
    std::vector<uint8_t> core(size_t(grid.cells), 0);
    std::vector<int32_t> coreCells;
    for (int cell = 0; cell < grid.cells; ++cell) {
        int32_t first = plate[size_t(cell)];
        bool mixed = first == -2;
        for (const int32_t other : rings[size_t(cell)]) {
            if (other < 0 || mixed) continue;
            const int32_t there = plate[size_t(other)];
            if (there == -1) continue;
            if (there == -2 || (first >= 0 && there != first)) mixed = true;
            if (first < 0) first = there;
        }
        if (mixed) core[size_t(cell)] = 1, coreCells.push_back(cell);
    }
    fillBand(globe, sweep, core, coreCells, band);
}

void markBandAt(const Globe& globe, int sweep, const std::vector<int32_t>& cells, Band& band) {
    std::vector<uint8_t> core(size_t(globe.grid->cells), 0);
    std::vector<int32_t> coreCells;
    for (const int32_t cell : cells)
        if (!core[size_t(cell)]) core[size_t(cell)] = 1, coreCells.push_back(cell);
    std::sort(coreCells.begin(), coreCells.end());
    fillBand(globe, sweep, core, coreCells, band);
}

void coupleBand(const Globe& globe, Band& band) {
    const SphereGrid& grid = *globe.grid;
    const size_t n = band.pieces.size();
    band.weightStart.assign(n + 1, 0);
    band.weights.clear();
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    for (size_t b = 0; b < n; ++b) {
        const int32_t piece = band.pieces[b];
        if (!band.dead[size_t(piece)]) {
            const CrustPiece& crust = globe.pieces[size_t(piece)];
            const double sum = exactWeights(grid, crust, weights, cells, work);
            require(std::abs(sum - crust.area) <= 1e-9 * crust.area, "exact overlaps of a band footprint miss its area", sum / crust.area - 1);
            for (CellWeight& w : weights) band.weights.push_back({w.cell, w.area * crust.area / sum});
        }
        band.weightStart[b + 1] = int32_t(band.weights.size());
    }
    band.listStart.assign(band.regionCells.size() + 1, 0);
    for (const CellWeight& w : band.weights)
        if (band.region[size_t(w.cell)] >= 0) ++band.listStart[size_t(band.region[size_t(w.cell)]) + 1];
    for (size_t r = 0; r < band.regionCells.size(); ++r) band.listStart[r + 1] += band.listStart[r];
    band.lists.resize(size_t(band.listStart.back()));
    std::vector<int32_t> fill(band.listStart.begin(), band.listStart.end() - 1);
    for (size_t b = 0; b < n; ++b)
        for (int32_t s = band.weightStart[b]; s < band.weightStart[b + 1]; ++s) {
            const CellWeight& w = band.weights[size_t(s)];
            const int32_t r = band.region[size_t(w.cell)];
            if (r >= 0) band.lists[size_t(fill[size_t(r)]++)] = {band.pieces[b], w.area};
        }
    band.born.assign(band.regionCells.size(), {});
    ++band.slotGeneration;
}

std::vector<int32_t> nearBand(const Globe& globe, const Band& band) {
    const SphereGrid& grid = *globe.grid;
    std::vector<uint8_t> near = band.decide;
    std::vector<int32_t> cells = band.cells;
    widen(*globe.rings, near, cells, band.sweep + GuardRings + 1);
    std::vector<int32_t> pieces;
    for (size_t k = 0; k < band.member.size(); ++k)
        if (!band.member[k] && near[size_t(indexOf(grid, cellOf(centreOf(grid, globe.pieces[k]), grid.level)))]) pieces.push_back(int32_t(k));
    return pieces;
}

void requireBandHeld(const Globe& globe, const Band& band, const std::vector<int32_t>& near, double dt, double shortest) {
    const SphereGrid& grid = *globe.grid;
    const double between = 0.25 * shortest / PlanetRadius;   // of a point of the arc from the nearest sample
    std::vector<int32_t> cells;
    for (const int32_t k : near) {
        const CrustPiece& piece = globe.pieces[size_t(k)];
        const Vec3 euler = globe.applied[piece.plate];
        const int samples = std::max(1, int(std::ceil(length(euler) * PlanetRadius * dt / (0.5 * shortest))));
        for (int j = 0; j <= samples; ++j) {
            const Rotation back = j == 0 ? piece.rotation : compose(rotationOf(euler, -dt * j / samples), piece.rotation);
            const Vec3 at = rotate(back, grid.centre[size_t(piece.birth)]);
            around(*globe.rings, indexOf(grid, cellOf(at, grid.level)), GuardRings, cells);
            for (const int32_t c : cells) {
                if (!band.decide[size_t(c)]) continue;
                const Vec3 d = at - grid.centre[size_t(c)];
                const double apart = grid.reach[size_t(piece.birth)] + grid.reach[size_t(c)] + between;
                require(dot(d, d) >= apart * apart, "a piece outside the band reaches a deciding cell", double(k));
            }
        }
    }
}

void joinBand(const Globe& globe, Band& band, int piece) {
    band.member.resize(globe.pieces.size(), 0);
    band.dead.resize(globe.pieces.size(), 0);
    band.member[size_t(piece)] = 1;
    ++band.slotGeneration;
    band.pieces.push_back(piece);
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    const CrustPiece& crust = globe.pieces[size_t(piece)];
    const double sum = exactWeights(*globe.grid, crust, weights, cells, work);
    require(std::abs(sum - crust.area) <= 1e-9 * crust.area, "exact overlaps of a new footprint miss its area", sum / crust.area - 1);
    for (const CellWeight& w : weights) {
        const int32_t r = band.region[size_t(w.cell)];
        require(r >= 0, "a new footprint reaches out of the band's region", double(w.cell));
        band.born[size_t(r)].push_back({piece, w.area * crust.area / sum});
    }
}

BandList listOf(const Band& band, int cell) {
    const int32_t r = band.region[size_t(cell)];
    if (r < 0) return {};
    return {band.lists.data() + band.listStart[size_t(r)], band.lists.data() + band.listStart[size_t(r) + 1]};
}

} // namespace derivean::sim
