#include "planet/fn/Lattice.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {

double unionInCell(const Globe& globe, int cell, const std::vector<int>& pieces, double sum, long& unions) {
    const SphereGrid& grid = *globe.grid;
    if (pieces.size() <= 1) return sum;
    const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(cell)]);
    const PlanePolygon own = projected(plane, polygonOf(grid, cell));
    std::vector<PlanePolygon> parts;
    for (const int k : pieces) parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(k)])), own));
    ++unions;
    return unionArea(parts).area;
}

Over overOf(const Globe& globe, const Band& band, int cell, int plate, Vec3 x, Vec3 normal) {
    Over over;
    forLive(band, cell, [&](int k, double area) {
        const CrustPiece& piece = globe.pieces[size_t(k)];
        if (plate >= 0 && (piece.plate != plate || dot(centreOf(*globe.grid, piece) - x, normal) >= 0)) return;
        over.pieces.push_back(k), over.sum += area, over.most = std::max(over.most, area);
    });
    return over;
}

// How far to move a new footprint from its cell along `normal` (rad) to weld it onto its own plate:
// the share of the cell the plate's live pieces behind it (centres on the far side of `normal`) cover
// together, as a length across the cell (Codex DREV-CLAUDE-005-02: the union, since a sum of overlaps
// within the plate pushes the piece too far). For a straight trailing edge that is flush; for a ragged
// one it leaves about as much gap as overlap. The pieces beside the cell along the ridge share edges
// with it and no move along `normal` takes it off them.
double weldTurn(Globe& globe, const Band& band, int cell, int plate, Vec3 x, Vec3 normal) {
    const double area = globe.grid->area[size_t(cell)];
    const Over behind = overOf(globe, band, cell, plate, x, normal);
    const double kept = unionInCell(globe, cell, behind.pieces, behind.sum, globe.stats.unions);
    return std::min(kept, area) / std::sqrt(area) / PlanetRadius;
}

// A slot of a plate's lattice next to the gap: the cell beside the birth cell of one of its pieces T
// near the gap, across T's edge that faces the gap, turned by T's rotation. A piece born there tiles
// with T exactly, so ridge crust goes on in its plate's lattice instead of in the mantle grid's, where
// the plate's pieces stand shifted by fractions of a cell and every birth leaves a hole or an overlap
// (Codex DREV-CLAUDE-005-01). A slot is open when the union of the live pieces over it leaves ½ of it or
// more uncovered and the pieces of T's own plate cover under SlotClash of it; the most open one is taken
// (the lower key of T, then the lower edge, on a tie; a slot whose overlaps sum to ½ or less is ranked
// by that sum, a bound of its union, since it is open either way). It may graze the cell and leave it
// open, so a cell takes up to three births in a substep (Spread.cpp); ranking by the cover of the cell
// instead walks the ridge's axis by 3.8 cells in 200 Myr. A hole of one slot that straddles mantle cells
// leaves each of them under half open, so slots are looked for wherever the cell is not covered. With
// a zero `normal` (a seam inside one plate, planet/fn/Seam.hpp) a slot may lie across any edge of T.
bool openSlot(Globe& globe, const Band& band, int cell, int plate, Vec3 normal, bool cellOpen, Slot& slot) {
    const SphereGrid& grid = *globe.grid;
    const PreparedPolygon home = preparedOf(polygonOf(grid, cell));
    std::vector<int> near;
    const auto gather = [&](int c) {
        forLive(band, c, [&](int k, double) {
            if (std::find(near.begin(), near.end(), k) == near.end()) near.push_back(k);
        });
    };
    gather(cell);
    for (const int32_t other : (*globe.rings)[size_t(cell)])
        if (other >= 0) gather(other);
    std::sort(near.begin(), near.end(), [&](int a, int b) { return globe.pieces[size_t(a)].key < globe.pieces[size_t(b)].key; });
    std::vector<Vec3> nearCentre(near.size());
    std::vector<SpherePolygon> nearFootprint(near.size());
    for (size_t i = 0; i < near.size(); ++i)
        nearCentre[i] = centreOf(grid, globe.pieces[size_t(near[i])]), nearFootprint[i] = footprintOf(grid, globe.pieces[size_t(near[i])]);
    double best = 0, bestCover = 0;   // the most open slot; else, in an open cell, the fill covering most of it
    Slot fill;
    std::vector<int> over;
    for (const int t : near) {
        const CrustPiece& template_ = globe.pieces[size_t(t)];
        if (template_.plate != plate) continue;
        const Vec3 from = centreOf(grid, template_);
        for (int k = 0; k < 4; ++k) {
            const int next = neighbourOf(grid, template_.birth, k);
            const Vec3 centre = rotate(template_.rotation, grid.centre[size_t(next)]);
            if (dot(normal, normal) > 0 && dot(centre - from, normal) <= 0) continue;
            const SpherePolygon footprint = rotated(polygonOf(grid, next), template_.rotation);
            const PreparedPolygon ready = preparedOf(footprint);
            const double cover = overlapArea(home, footprint);
            if (cover <= 0) continue;
            const double area = grid.area[size_t(next)], reach = 2.5 * std::sqrt(area) / PlanetRadius;
            double own = 0, sum = 0;
            over.clear();
            for (size_t i = 0; i < near.size(); ++i) {
                const int l = near[i];
                const Vec3 d = nearCentre[i] - centre;
                if (dot(d, d) >= reach * reach) continue;
                const double o = overlapArea(ready, nearFootprint[i]);
                if (o <= 0) continue;
                over.push_back(int(i)), sum += o;
                if (globe.pieces[size_t(l)].plate == plate) own += o;
            }
            if (own > SlotClash * area) continue;
            double covered = sum;
            if (over.size() > 1 && sum > 0.5 * area) {
                const TangentPlane plane = tangentPlaneAt(centre);
                const PlanePolygon clip = projected(plane, footprint);
                std::vector<PlanePolygon> parts;
                for (const int i : over) parts.push_back(clipped(projected(plane, nearFootprint[size_t(i)]), clip));
                ++globe.stats.unions;
                covered = unionArea(parts).area;
            }
            const double open = (area - covered) / area;
            // Below ½ the union was taken, so a fill is measured exactly; one that the other plate
            // already covers by more than 1 − SlotFill leaves the cell to the weld.
            if (cellOpen && open >= SlotFill && cover > bestCover) bestCover = cover, fill = {next, template_.rotation};
            if (open < BornShare || open <= best) continue;
            best = open, slot = {next, template_.rotation};
        }
    }
    if (best <= 0 && bestCover > 0) slot = fill;
    return best > 0 || bestCover > 0;
}

} // namespace derivean::sim
