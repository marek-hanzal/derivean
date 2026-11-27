#include "planet/fn/Interior.hpp"

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {

bool sameLattice(const CrustPiece& a, const CrustPiece& b) {
    return a.plate == b.plate && a.rotation.w == b.rotation.w && a.rotation.x == b.rotation.x && a.rotation.y == b.rotation.y && a.rotation.z == b.rotation.z;
}

bool tiles(const CrustPiece& a, const CrustPiece& b) { return sameLattice(a, b) && a.birth != b.birth; }

LatticeSlot slotOf(const Globe& globe, int t, int k) {
    const SphereGrid& grid = *globe.grid;
    const CrustPiece& piece = globe.pieces[size_t(t)];
    LatticeSlot slot;
    slot.birth = neighbourOf(grid, piece.birth, k);
    slot.footprint = rotated(polygonOf(grid, slot.birth), piece.rotation);
    slot.cell = indexOf(grid, cellOf(rotate(piece.rotation, grid.centre[size_t(slot.birth)]), grid.level));
    slot.area = grid.area[size_t(slot.birth)];
    return slot;
}

double interiorOpening(const Globe& globe, int t, const LatticeSlot& slot, const std::vector<int>& near, long& unions) {
    const SphereGrid& grid = *globe.grid;
    const int plate = globe.pieces[size_t(t)].plate;
    std::vector<int> over;
    double sum = 0;
    for (const int l : near)
        if (globe.pieces[size_t(l)].plate != plate) return -1;
    for (const int l : near) {
        const double o = overlapArea(slot.footprint, footprintOf(grid, globe.pieces[size_t(l)]));
        if (o > 0) over.push_back(l), sum += o;
    }
    double covered = sum;
    if (over.size() > 1) {
        const TangentPlane plane = tangentPlaneAt(rotate(globe.pieces[size_t(t)].rotation, grid.centre[size_t(slot.birth)]));
        const PlanePolygon clip = projected(plane, slot.footprint);
        std::vector<PlanePolygon> parts;
        for (const int l : over) parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(l)])), clip));
        ++unions;
        covered = unionArea(parts).area;
    }
    return std::max(0.0, slot.area - covered) / slot.area;
}

bool interiorSlot(Globe& globe, Band& band, int cell, InteriorSlot& found) {
    std::vector<int> templates, near;
    const auto gather = [&](std::vector<int>& into, int c) {
        forLive(band, c, [&](int k, double) {
            if (std::find(into.begin(), into.end(), k) == into.end()) into.push_back(k);
        });
    };
    gather(templates, cell);
    for (const int32_t other : (*globe.rings)[size_t(cell)])
        if (other >= 0) gather(templates, other);
    std::sort(templates.begin(), templates.end(), [&](int a, int b) { return globe.pieces[size_t(a)].key < globe.pieces[size_t(b)].key; });
    double best = 0;
    const SphereGrid& grid = *globe.grid;
    for (const int t : templates) {
        const CrustPiece& piece = globe.pieces[size_t(t)];
        for (int k = 0; k < 4; ++k) {
            // A slot that a piece of the same lattice holds is closed (most slots inside a plate).
            const int birth = neighbourOf(grid, piece.birth, k);
            bool held = false;
            for (const int l : templates) held = held || (globe.pieces[size_t(l)].birth == birth && sameLattice(globe.pieces[size_t(l)], piece));
            if (held) continue;
            const size_t at = size_t(t) * 4 + size_t(k);
            if (band.slotStamp.size() < globe.pieces.size() * 4) band.slotStamp.resize(globe.pieces.size() * 4, 0), band.slotOpen.resize(globe.pieces.size() * 4, 0);
            if (band.slotStamp[at] == band.slotGeneration && band.slotOpen[at] < float(BornShare)) continue;   // closed when last measured
            const LatticeSlot s = slotOf(globe, t, k);
            bool deciding = band.decide[size_t(s.cell)], listed = true;
            for (const int32_t other : (*globe.rings)[size_t(s.cell)]) {
                if (other < 0) continue;
                deciding = deciding || band.decide[size_t(other)];
                listed = listed && band.region[size_t(other)] >= 0;
                for (const int32_t far : (*globe.rings)[size_t(other)]) listed = listed && (far < 0 || band.region[size_t(far)] >= 0);
            }
            band.slotStamp[at] = band.slotGeneration, band.slotOpen[at] = -1;
            if (!deciding || !listed) continue;
            near.clear();
            gather(near, s.cell);
            for (const int32_t other : (*globe.rings)[size_t(s.cell)])
                if (other >= 0) gather(near, other);
            const double open = interiorOpening(globe, t, s, near, globe.stats.unions);
            band.slotOpen[at] = float(open);
            if (open < BornShare || open <= best) continue;
            best = open, found = {{s.birth, globe.pieces[size_t(t)].rotation}, globe.pieces[size_t(t)].plate, t, s.cell};
        }
    }
    return best > 0;
}

} // namespace derivean::sim
