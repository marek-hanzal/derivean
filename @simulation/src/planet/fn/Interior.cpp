#include "planet/fn/Interior.hpp"

#include "core/fn/Listed.hpp"
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

namespace {

// A slot without its footprint, which only slots that are measured take.
LatticeSlot slotAt(const Globe& globe, int t, int k) {
    const SphereGrid& grid = *globe.grid;
    const CrustPiece& piece = globe.pieces[size_t(t)];
    LatticeSlot slot;
    slot.birth = neighbourOf(grid, piece.birth, k);
    slot.cell = indexOf(grid, cellOf(rotate(piece.rotation, grid.centre[size_t(slot.birth)]), grid.level));
    slot.area = grid.area[size_t(slot.birth)];
    return slot;
}

void setFootprint(const Globe& globe, int t, LatticeSlot& slot) {
    slot.footprint = rotated(polygonOf(*globe.grid, slot.birth), globe.pieces[size_t(t)].rotation);
}

// Whether a piece of another plate than t's is among `near`.
bool otherPlateNear(const Globe& globe, int t, const std::vector<int>& near) {
    const int plate = globe.pieces[size_t(t)].plate;
    for (const int l : near)
        if (globe.pieces[size_t(l)].plate != plate) return true;
    return false;
}

// interiorOpening of a slot with no other plate near, with near[i]'s footprint and cap from of(i), so
// that a caller measuring many slots takes each piece's once. `over` is scratch. Where only closed or
// open is asked (`closedOnly`), the first overlap that leaves the slot under BornShare open by the
// union's bound ends the measure with that bound: the union covers its largest overlap to
// UnionShortfall, and with no other overlap the slot's openness is that overlap's own.
template <class Of>
double openingOf(const Globe& globe, int t, const LatticeSlot& slot, const std::vector<int>& near, Of of, bool closedOnly, std::vector<size_t>& over, long& unions) {
    const SphereGrid& grid = *globe.grid;
    over.clear();
    double sum = 0, most = 0;
    const PreparedPolygon ready = preparedOf(slot.footprint);
    for (size_t i = 0; i < near.size(); ++i) {
        const auto [footprint, cap] = of(i);
        const double o = overlapArea(ready, *footprint, *cap);
        if (o <= 0) continue;
        over.push_back(i), sum += o, most = std::max(most, o);
        const double bound = (slot.area - o) / slot.area + UnionShortfall;
        if (closedOnly && bound < BornShare) return bound;
    }
    // The union covers the largest overlap (to UnionShortfall), so a slot that one piece already leaves
    // under BornShare open is closed whatever the union gives: its bound is returned instead.
    const double atMost = (slot.area - most) / slot.area + UnionShortfall;
    if (over.size() > 1 && atMost < BornShare) return atMost;
    double covered = sum;
    if (over.size() > 1) {
        const TangentPlane plane = tangentPlaneAt(rotate(globe.pieces[size_t(t)].rotation, grid.centre[size_t(slot.birth)]));
        const PlanePolygon clip = projected(plane, slot.footprint);
        std::vector<PlanePolygon> parts;
        parts.reserve(over.size());
        for (const size_t i : over) parts.push_back(clipped(projected(plane, *of(i).first), clip));
        ++unions;
        covered = unionArea(parts).area;
    }
    return std::max(0.0, slot.area - covered) / slot.area;
}

} // namespace

LatticeSlot slotOf(const Globe& globe, int t, int k) {
    LatticeSlot slot = slotAt(globe, t, k);
    setFootprint(globe, t, slot);
    return slot;
}

double interiorOpening(const Globe& globe, int t, const LatticeSlot& slot, const std::vector<int>& near, long& unions) {
    std::vector<SpherePolygon> footprints;
    std::vector<PolygonCap> caps;
    for (const int l : near) footprints.push_back(footprintOf(*globe.grid, globe.pieces[size_t(l)])), caps.push_back(capOf(footprints.back()));
    if (otherPlateNear(globe, t, near)) return -1;
    std::vector<size_t> over;
    return openingOf(globe, t, slot, near, [&](size_t i) { return std::pair{&footprints[i], &caps[i]}; }, false, over, unions);
}

bool interiorSlot(Globe& globe, Band& band, int cell, InteriorSlot& found) {
    std::vector<int> templates, near;
    std::vector<size_t> over;
    const auto gather = [&](std::vector<int>& into, int c) {
        forLive(band, c, [&](int k, double) {
            if (!listed(into, k)) into.push_back(k);
        });
    };
    gather(templates, cell);
    for (const int32_t other : (*globe.rings)[size_t(cell)])
        if (other >= 0) gather(templates, other);
    std::sort(templates.begin(), templates.end(), [&](int a, int b) { return globe.pieces[size_t(a)].key < globe.pieces[size_t(b)].key; });
    // The templates by birth cell, so a slot's holder is looked up rather than searched for.
    std::vector<std::pair<int, int>> byBirth;
    byBirth.reserve(templates.size());
    for (const int l : templates) byBirth.push_back({globe.pieces[size_t(l)].birth, l});
    std::sort(byBirth.begin(), byBirth.end());
    double best = 0;
    const SphereGrid& grid = *globe.grid;
    if (band.slotStamp.size() < globe.pieces.size() * 4) band.slotStamp.resize(globe.pieces.size() * 4, 0), band.slotOpen.resize(globe.pieces.size() * 4, 0);
    if (band.footprintStamp.size() < globe.pieces.size())
        band.footprintStamp.resize(globe.pieces.size(), 0), band.footprint.resize(globe.pieces.size()), band.footprintCap.resize(globe.pieces.size());
    if (++band.footprintCall == 0) std::fill(band.footprintStamp.begin(), band.footprintStamp.end(), 0), band.footprintCall = 1;
    const auto footprintAt = [&](size_t i) {
        const size_t l = size_t(near[i]);
        if (band.footprintStamp[l] != band.footprintCall) {
            band.footprint[l] = footprintOf(grid, globe.pieces[l]);
            band.footprintCap[l] = capOf(band.footprint[l]);
            band.footprintStamp[l] = band.footprintCall;
        }
        return std::pair{&band.footprint[l], &band.footprintCap[l]};
    };
    for (const int t : templates) {
        const CrustPiece& piece = globe.pieces[size_t(t)];
        for (int k = 0; k < 4; ++k) {
            // A slot that a piece of the same lattice holds is closed (most slots inside a plate).
            const int birth = neighbourOf(grid, piece.birth, k);
            bool held = false;
            for (auto at = std::lower_bound(byBirth.begin(), byBirth.end(), std::pair{birth, -1}); at != byBirth.end() && at->first == birth && !held; ++at)
                held = sameLattice(globe.pieces[size_t(at->second)], piece);
            if (held) continue;
            const size_t at = size_t(t) * 4 + size_t(k);
            if (band.slotStamp[at] == band.slotGeneration && band.slotOpen[at] < float(BornShare)) continue;   // closed when last measured
            LatticeSlot s = slotAt(globe, t, k);
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
            if (otherPlateNear(globe, t, near)) continue;   // closed: another plate is near
            setFootprint(globe, t, s);
            const double open = openingOf(globe, t, s, near, footprintAt, true, over, globe.stats.unions);
            band.slotOpen[at] = float(open);
            if (open < BornShare || open <= best) continue;
            best = open, found = {{s.birth, globe.pieces[size_t(t)].rotation}, globe.pieces[size_t(t)].plate, t, s.cell};
        }
    }
    return best > 0;
}

} // namespace derivean::sim
