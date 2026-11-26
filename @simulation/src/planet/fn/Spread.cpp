#include "planet/fn/Boundary.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/constant/Globe.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Pieces.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr double BornShare = 0.5;     // of a cell left uncovered by the union of its pieces
constexpr float StretchShare = 0.4f;  // of a continental neighbour's crust and cover that necks into the gap (Crossing.cpp)

// The nearest live piece of each of the two nearest plates to a cell, among the band's pieces listed
// within two rings of it (nearest by chord from the cell's centre; the lower key on a tie).
struct Nearest {
    int first{-1}, second{-1};
};

Nearest nearestPlates(const Globe& globe, const Band& band, int cell) {
    const std::vector<CellRing>& rings = *globe.rings;
    std::vector<int32_t> near{cell};
    for (int ring = 0; ring < 2; ++ring)
        for (size_t i = 0, n = near.size(); i < n; ++i)
            for (const int32_t other : rings[size_t(near[i])])
                if (other >= 0 && std::find(near.begin(), near.end(), other) == near.end()) near.push_back(other);
    const Vec3 x = globe.grid->centre[size_t(cell)];
    std::vector<std::pair<double, int>> best;   // per plate seen: distance, piece
    std::vector<int> plates;
    for (const int32_t c : near) {
        const BandList list = listOf(band, c);
        for (const PieceWeight* w = list.begin; w != list.end; ++w) {
            if (band.dead[size_t(w->piece)]) continue;
            const CrustPiece& piece = globe.pieces[size_t(w->piece)];
            const Vec3 d = centreOf(*globe.grid, piece) - x;
            const double distance = dot(d, d);
            const auto at = std::find(plates.begin(), plates.end(), int(piece.plate));
            if (at == plates.end()) {
                plates.push_back(piece.plate);
                best.push_back({distance, w->piece});
                continue;
            }
            auto& [seen, holder] = best[size_t(at - plates.begin())];
            if (distance < seen || (distance == seen && piece.key < globe.pieces[size_t(holder)].key)) seen = distance, holder = w->piece;
        }
    }
    std::vector<size_t> order(plates.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        if (best[a].first != best[b].first) return best[a].first < best[b].first;
        return plates[a] < plates[b];
    });
    Nearest nearest;
    if (order.size() >= 1) nearest.first = best[order[0]].second;
    if (order.size() >= 2) nearest.second = best[order[1]].second;
    return nearest;
}

// The area of a cell its live pieces cover together, with the pieces born earlier in this substep,
// which the band's lists do not hold yet.
double coveredArea(const Globe& globe, const Band& band, int cell, const std::vector<int>& born, long& unions) {
    const SphereGrid& grid = *globe.grid;
    const double area = grid.area[size_t(cell)];
    const Vec3 x = grid.centre[size_t(cell)];
    const SpherePolygon home = polygonOf(grid, cell);
    const BandList list = listOf(band, cell);
    std::vector<int> pieces;
    double sum = 0, most = 0;
    for (const PieceWeight* w = list.begin; w != list.end; ++w)
        if (!band.dead[size_t(w->piece)]) sum += w->area, most = std::max(most, w->area), pieces.push_back(w->piece);
    for (const int f : born) {
        const Vec3 d = centreOf(grid, globe.pieces[size_t(f)]) - x;
        if (dot(d, d) * PlanetRadius * PlanetRadius > 9 * area) continue;
        const double w = overlapArea(home, footprintOf(grid, globe.pieces[size_t(f)]));
        if (w > 0) sum += w, most = std::max(most, w), pieces.push_back(f);
    }
    if (most >= BornShare * area || sum < BornShare * area) return std::max(sum, most);
    const TangentPlane plane = tangentPlaneAt(x);
    const PlanePolygon own = projected(plane, home);
    std::vector<PlanePolygon> parts;
    for (const int k : pieces) parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(k)])), own));
    ++unions;
    return unionArea(parts).area;
}

// How far to move a new footprint from its cell along `normal` (rad) to weld it onto its own plate:
// the share of the cell the plate's live pieces behind it (centres on the far side of `normal`) still
// cover, as a length across the cell. For a straight trailing edge that is flush; for a ragged one it
// leaves about as much gap as overlap, so the area the plate covers stays what it was given. The pieces
// beside the cell along the ridge share edges with it and no move along `normal` takes it off them.
double weldTurn(const Globe& globe, const Band& band, int cell, int plate, Vec3 x, Vec3 normal) {
    const SphereGrid& grid = *globe.grid;
    const double area = grid.area[size_t(cell)];
    std::vector<int32_t> seen;
    const SpherePolygon home = polygonOf(grid, cell);
    double kept = 0;
    const auto gather = [&](int c) {
        const BandList list = listOf(band, c);
        for (const PieceWeight* w = list.begin; w != list.end; ++w) {
            if (band.dead[size_t(w->piece)] || globe.pieces[size_t(w->piece)].plate != plate) continue;
            if (std::find(seen.begin(), seen.end(), w->piece) != seen.end()) continue;
            seen.push_back(w->piece);
            const CrustPiece& piece = globe.pieces[size_t(w->piece)];
            if (dot(centreOf(grid, piece) - x, normal) < 0) kept += overlapArea(home, footprintOf(grid, piece));
        }
    };
    gather(cell);
    for (const int32_t other : (*globe.rings)[size_t(cell)])
        if (other >= 0) gather(other);
    return std::min(kept, area) / std::sqrt(area) / PlanetRadius;
}

void resetMarks(CrustPiece& piece) {
    const CrustColumn& c = piece.column;
    piece.record.past.fill({columnElevation(c), c.crust, c.tectonicGain, c.eroded, c.deposited, c.melt, c.erupted});
}

// The new piece of a stretched continent: 40 % of the crust, basalt and cover of each continental
// neighbour of its plate across the cell's edges, weighted by the length of the shared edge, so the
// neighbours give 40 % between them as one neighbour does on the flat grid.
bool neck(Globe& globe, const Band& band, int cell, int plate, CrustPiece& fresh) {
    const SphereGrid& grid = *globe.grid;
    // A piece across two of the cell's edges gives once, by both edges' length.
    std::array<int, 4> giver{-1, -1, -1, -1};
    std::array<double, 4> edge{};
    int givers = 0;
    double total = 0;
    for (int k = 0; k < 4; ++k) {
        const int other = neighbourOf(grid, cell, k);
        int best = -1;
        double most = 0;
        const BandList list = listOf(band, other);
        for (const PieceWeight* w = list.begin; w != list.end; ++w)
            if (!band.dead[size_t(w->piece)] && globe.pieces[size_t(w->piece)].plate == plate && w->area > most) best = w->piece, most = w->area;
        if (best < 0 || globe.pieces[size_t(best)].column.crust < 0.5f * float(ContinentalLimit)) continue;
        const double length = grid.edge[size_t(grid.edges[size_t(cell)][size_t(k)])].length;
        int slot = 0;
        while (slot < givers && giver[size_t(slot)] != best) ++slot;
        if (slot == givers) giver[size_t(givers++)] = best;
        edge[size_t(slot)] += length;
        total += length;
    }
    if (total <= 0) return false;
    int widest = -1;
    for (int k = 0; k < givers; ++k) {
        CrustPiece& from = globe.pieces[size_t(giver[size_t(k)])];
        if (widest < 0 || edge[size_t(k)] > edge[size_t(widest)]) widest = k;
        const double share = StretchShare * edge[size_t(k)] / total;
        const double felsic = takeFelsic(from, share * from.stock.felsic);
        const double basalt = share * from.stock.basalt;
        from.stock.basalt -= basalt;
        fresh.stock.felsic += felsic;
        fresh.stock.basalt += basalt;
        Cover part = from.record.cover;
        scaleCover(part, float(share * from.area / fresh.area));
        stackCover(fresh.record.cover, part);
        scaleCover(from.record.cover, float(1 - share));
        from.column.tectonicGain -= float(felsic / from.area);
        from.column.quietSince = float(globe.time);
    }
    const CrustColumn& like = globe.pieces[size_t(giver[size_t(widest)])].column;
    fresh.column = like;
    fresh.column.oceanAge = 0;
    fresh.column.load = 0;
    fresh.column.tectonicGain = fresh.column.eroded = fresh.column.deposited = 0;
    fresh.column.driftX = fresh.column.driftZ = 0;
    fresh.column.quietSince = float(globe.time);
    return true;
}

} // namespace

void bearInBand(Globe& globe, Band& band, const Substep& substep) {
    const SphereGrid& grid = *globe.grid;
    std::vector<int> born;   // this substep's new pieces
    for (const int32_t cell : band.cells) {
        const double area = grid.area[size_t(cell)];
        if (area - coveredArea(globe, band, cell, born, globe.stats.unions) < BornShare * area) continue;
        const Nearest nearest = nearestPlates(globe, band, cell);
        if (nearest.second < 0) continue;
        const CrustPiece& p = globe.pieces[size_t(nearest.first)];
        const CrustPiece& q = globe.pieces[size_t(nearest.second)];
        const Vec3 x = grid.centre[size_t(cell)];
        const Vec3 normal = towards(x, x + (centreOf(grid, q) - centreOf(grid, p)));
        if (closingAt(globe.applied[p.plate], globe.applied[q.plate], x, normal).convergence >= 0) continue;

        CrustPiece fresh;
        // New crust welds onto the trailing edge of its plate: the footprint is the cell moved away from
        // the plate's own pieces, towards the other plate (weldTurn). A footprint left on the cell would
        // overlap its own plate for good, by up to half a cell at every birth (deviation from
        // S3-design.md §4's Q = I, docs/handoff/planet-globe.md).
        fresh.rotation = rotationAbout(cross(x, normal), weldTurn(globe, band, cell, p.plate, x, normal));
        fresh.key = {substep.step, cell, substep.index};
        fresh.birth = cell;
        fresh.area = area;
        fresh.plate = p.plate;
        // A plate behind that keeps pace fills the gap itself; only a real parting stretches crust.
        const Vec3 own = velocityAt(globe.applied[p.plate], x);
        const double speed = length(own);
        const bool stretch = speed > 0 && dot(velocityAt(globe.applied[q.plate], x), own) / speed < 0.5 * speed;
        if (!stretch || !neck(globe, band, cell, fresh.plate, fresh)) {
            fresh.column = CrustColumn{};
            fresh.column.quietSince = float(globe.time);
            fresh.column.basement = Basement::Oceanic;
            fresh.stock.basalt = OceanicCrust * area;
            globe.ledger.interior.basalt -= fresh.stock.basalt;
        }
        refreshColumn(fresh);
        resetMarks(fresh);
        globe.pieces.push_back(fresh);
        joinBand(globe, band, int(globe.pieces.size()) - 1);
        born.push_back(int(globe.pieces.size()) - 1);
        globe.ledger.born += area;
        ++globe.stats.births;
    }
}

} // namespace derivean::sim
