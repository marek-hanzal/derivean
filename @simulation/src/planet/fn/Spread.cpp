#include "planet/fn/Boundary.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/constant/Globe.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Interior.hpp"
#include "planet/fn/Lattice.hpp"
#include "planet/fn/Lay.hpp"
#include "planet/fn/Pieces.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr int BirthsPerCell = 3;      // births one deciding cell may take in a substep

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
        const bool open = !coversOver(globe, cell, over, half, globe.stats.unions);
        const bool closed = !open && over.sum >= (1 - SlotHint) * area;
        // Inside one plate a gap is a slot of its lattice open by half or more, not a mantle cell a seam
        // crosses (planet/fn/Interior.hpp); such a slot is filled first, wherever no other plate is near
        // it, and a cell with one plate within two rings takes nothing else. A seam's overlaps can sum a
        // cell to full beside a gap, so a cell on a seam (two lattices of one plate) is looked at even so.
        bool seam = false;
        for (size_t i = 0; i < over.pieces.size() && !seam; ++i)
            for (size_t j = 0; j < i && !seam; ++j) {
                const CrustPiece& a = globe.pieces[size_t(over.pieces[i])];
                const CrustPiece& b = globe.pieces[size_t(over.pieces[j])];
                seam = a.plate == b.plate && !sameLattice(a, b);
            }
        if (closed && !seam) return false;
        const double bornAt = globe.time + (substep.index + 1) * substep.duration;
        if (fillSlot(globe, band, cell, {substep.step, cell, order}, bornAt)) return ++globe.stats.filled, true;
        if (closed) return false;
        const Nearest nearest = nearestPlates(globe, band, cell);
        // One plate within two rings and no interior slot: nothing is laid. A weld there doubles the plate
        // over a seam faster than the sutures take it back and lifted Σa over 3 % (handoff, Open).
        if (nearest.second < 0) return false;
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
            return open && fillCell(globe, band, cell, nearest.first, {substep.step, cell, order}, bornAt);

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
        fresh.bornAt = bornAt;   // births follow the substep's turn
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
// round; the key's order is (substep · BearRounds + round) · BirthsPerCell + the birth's number.
void bearInBand(Globe& globe, Band& band, const Substep& substep, int round) {
    for (const int32_t cell : band.cells) {
        int n = 0;
        while (n < BirthsPerCell && bearCell(globe, band, cell, substep, (substep.index * BearRounds + round) * BirthsPerCell + n)) ++n;
    }
}

} // namespace derivean::sim
