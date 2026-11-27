#include "planet/fn/Boundary.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/constant/Globe.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Lattice.hpp"
#include "planet/fn/Pieces.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr int BirthsPerCell = 3;      // births one deciding cell may take in a substep
constexpr float StretchShare = 0.4f;  // of a continental neighbour's crust and cover that necks into the gap (Crossing.cpp)

// The nearest live piece of each of the two nearest plates to a cell, among the band's pieces over the
// cells within two rings of it as the substep started (nearest by chord from the cell's centre; the
// lower key on a tie). This substep's births are left out: one would make its own plate the nearer for
// the next cell across the axis too, and that plate would take the whole gap, so the axis would wander
// off the symmetry of the spreading. Coverage, slots and necks read them.
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
    for (const int32_t c : near)
        forListed(band, c, [&](int k, double) {
            const CrustPiece& piece = globe.pieces[size_t(k)];
            const Vec3 d = centreOf(*globe.grid, piece) - x;
            const double distance = dot(d, d);
            const auto at = std::find(plates.begin(), plates.end(), int(piece.plate));
            if (at == plates.end()) {
                plates.push_back(piece.plate);
                best.push_back({distance, k});
                return;
            }
            auto& [seen, holder] = best[size_t(at - plates.begin())];
            if (distance < seen || (distance == seen && piece.key < globe.pieces[size_t(holder)].key)) seen = distance, holder = k;
        });
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
        forLive(band, other, [&](int k, double area) {
            if (globe.pieces[size_t(k)].plate == plate && area > most) best = k, most = area;
        });
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

// One birth over a deciding cell, if it needs one; whether a piece was laid. `order` is the key's.
bool bearCell(Globe& globe, Band& band, int cell, const Substep& substep, int order) {
    const SphereGrid& grid = *globe.grid;
    {
        const double area = grid.area[size_t(cell)];
        // Open where ½ or more is uncovered, the union at most ½. The union is under the sum and over
        // the largest overlap, so a sum of ½ or less is open and one overlap strictly over ½ is not;
        // between them the union decides (at exactly ½ it may be exactly ½ although the sum is more).
        // A closed cell still looks for a slot while its overlaps sum to under 1 − SlotHint.
        const Over over = overOf(globe, band, cell, -1, {}, {});
        const double half = BornShare * area;
        const bool open = !(over.sum > half && (over.most > half || unionInCell(globe, cell, over.pieces, over.sum, globe.stats.unions) > half));
        if (!open && over.sum >= (1 - SlotHint) * area) return false;
        const Nearest nearest = nearestPlates(globe, band, cell);
        if (nearest.first < 0) return false;
        if (nearest.second < 0)
            return open && fillCell(globe, band, cell, globe.pieces[size_t(nearest.first)].plate, {substep.step, cell, order}, globe.time + (substep.index + 1) * substep.duration);
        // The plate whose edge here is the older bears if it has an open slot, else the other; the nearer
        // plate on a tie. A ridge's floor so goes to its two plates by turns and its axis stays in the
        // middle of the gap: the nearer plate alone lets the axis walk by half a cell at every birth,
        // the cell order deciding the ties.
        const bool older = globe.pieces[size_t(nearest.second)].bornAt < globe.pieces[size_t(nearest.first)].bornAt;
        const int choice[2] = {older ? nearest.second : nearest.first, older ? nearest.first : nearest.second};
        const Vec3 x = grid.centre[size_t(cell)];
        const Vec3 apart = centreOf(grid, globe.pieces[size_t(nearest.second)]) - centreOf(grid, globe.pieces[size_t(nearest.first)]);
        const Vec3 normal = towards(x, x + apart);   // from the nearer plate towards the other
        const int near = globe.pieces[size_t(nearest.first)].plate, far = globe.pieces[size_t(nearest.second)].plate;
        if (closingAt(globe.applied[near], globe.applied[far], x, normal).convergence >= 0)
            return open && fillCell(globe, band, cell, near, {substep.step, cell, order}, globe.time + (substep.index + 1) * substep.duration);

        CrustPiece fresh;
        int bearer = -1;
        Slot slot;
        for (int c = 0; c < 2 && bearer < 0; ++c) {
            const int plate = globe.pieces[size_t(choice[c])].plate;
            if (openSlot(globe, band, cell, plate, plate == near ? normal : normal * -1.0, open, slot))
                bearer = c, fresh.rotation = slot.rotation, fresh.birth = slot.birth;
        }
        if (bearer < 0 && !open) return false;
        if (bearer < 0) {
            // No slot of either lattice fits: the cell moved away from the nearer plate's pieces welds onto
            // its trailing edge (weldTurn; deviation from S3-design.md §4's Q = I).
            bearer = choice[0] == nearest.first ? 0 : 1;
            fresh.rotation = rotationAbout(cross(x, normal), weldTurn(globe, band, cell, near, x, normal));
            fresh.birth = cell;
            ++globe.stats.welded;
        }
        const CrustPiece& p = globe.pieces[size_t(choice[bearer])];
        const CrustPiece& q = globe.pieces[size_t(choice[1 - bearer])];
        fresh.key = {substep.step, cell, order};
        fresh.area = grid.area[size_t(fresh.birth)];
        fresh.bornAt = globe.time + (substep.index + 1) * substep.duration;   // births follow the substep's turn
        fresh.plate = p.plate;
        // A plate behind that keeps pace fills the gap itself; only a real parting stretches crust.
        const Vec3 own = velocityAt(globe.applied[p.plate], x);
        const double speed = length(own);
        const bool stretch = speed > 0 && dot(velocityAt(globe.applied[q.plate], x), own) / speed < 0.5 * speed;
        layNewPiece(globe, band, fresh, cell, stretch);
    }
    return true;
}

} // namespace

// A cell still open after its birth (a slot that straddles it) gets another, up to BirthsPerCell in a
// substep; the key's order is substep · BirthsPerCell + the birth's number.
void bearInBand(Globe& globe, Band& band, const Substep& substep) {
    for (const int32_t cell : band.cells)
        for (int n = 0; n < BirthsPerCell && bearCell(globe, band, cell, substep, substep.index * BirthsPerCell + n); ++n) {
        }
}


bool fillCell(Globe& globe, Band& band, int cell, int plate, PieceKey key, double bornAt) {
    CrustPiece fresh;
    Slot slot;
    fresh.birth = cell;
    if (openSlot(globe, band, cell, plate, {}, true, slot)) fresh.rotation = slot.rotation, fresh.birth = slot.birth;
    else ++globe.stats.welded;
    fresh.key = key;
    fresh.area = globe.grid->area[size_t(fresh.birth)];
    fresh.bornAt = bornAt;
    fresh.plate = uint16_t(plate);
    layNewPiece(globe, band, fresh, cell, true);
    ++globe.stats.filled;
    return true;
}

void layNewPiece(Globe& globe, Band& band, CrustPiece fresh, int cell, bool stretch) {
    if (!stretch || !neck(globe, band, cell, fresh.plate, fresh)) {
        fresh.column = CrustColumn{};
        fresh.column.quietSince = float(globe.time);
        fresh.column.basement = Basement::Oceanic;
        fresh.stock.basalt = OceanicCrust * fresh.area;
        globe.ledger.interior.basalt -= fresh.stock.basalt;
    }
    refreshColumn(fresh);
    resetMarks(fresh);
    globe.pieces.push_back(fresh);
    joinBand(globe, band, int(globe.pieces.size()) - 1);
    globe.ledger.born += fresh.area;
    ++globe.stats.births;
}

} // namespace derivean::sim
