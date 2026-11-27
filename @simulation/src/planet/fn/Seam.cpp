#include "planet/fn/Seam.hpp"

#include "core/fn/Listed.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Around.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Interior.hpp"
#include "planet/fn/Lattice.hpp"
#include "planet/fn/Lay.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "planet/fn/Transfers.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr double SeamShare = 0.5;   // of a piece its own plate covers when the seam consumes it (as at a boundary)
constexpr int SeamRounds = 8;       // of sutures and fills

// How much of k the given pieces cover together, in k's tangent plane.
double unionOverPiece(Globe& globe, int k, const std::vector<int>& pieces) {
    const SphereGrid& grid = *globe.grid;
    const TangentPlane plane = tangentPlaneAt(centreOf(grid, globe.pieces[size_t(k)]));
    const PlanePolygon own = projected(plane, footprintOf(grid, globe.pieces[size_t(k)]));
    std::vector<PlanePolygon> parts;
    for (const int l : pieces) parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(l)])), own));
    ++globe.stats.unions;
    return unionArea(parts).area;
}

size_t suture(Globe& globe, Band& band, const Substep& substep) {
    const SphereGrid& grid = *globe.grid;
    PlanPieces plan{{}, {}, std::vector<int>(globe.pieces.size(), -1)};
    std::vector<int> over, receivers, cells, work, scratch;
    std::vector<double> overlap;
    std::vector<CellWeight> weights;
    std::vector<std::vector<int>> cellsOf;   // per candidate
    // The pieces coupled at the substep's start, then those born in it (planet/fn/Band.hpp).
    const size_t coupled = band.weightStart.size() - 1;
    for (size_t b = 0; b < band.pieces.size(); ++b) {
        const int k = band.pieces[b];
        if (band.dead[size_t(k)]) continue;
        const CrustPiece& piece = globe.pieces[size_t(k)];
        cells.clear();
        if (b < coupled) {
            for (int32_t s = band.weightStart[b]; s < band.weightStart[b + 1]; ++s) cells.push_back(band.weights[size_t(s)].cell);
        } else {
            exactWeights(grid, piece, weights, scratch, work);
            for (const CellWeight& w : weights) cells.push_back(w.cell);
        }
        bool deciding = false;
        over.clear();
        for (const int cell : cells) {
            deciding = deciding || band.decide[size_t(cell)];
            forLive(band, cell, [&](int l, double) {
                if (l != k && globe.pieces[size_t(l)].plate == piece.plate && !tiles(globe.pieces[size_t(l)], piece) &&
                    !listed(over, l))
                    over.push_back(l);
            });
        }
        if (!deciding || over.empty()) continue;
        std::sort(over.begin(), over.end(), [&](int a, int c) { return globe.pieces[size_t(a)].key < globe.pieces[size_t(c)].key; });
        const PreparedPolygon footprint = preparedOf(footprintOf(grid, piece));
        receivers.clear(), overlap.clear();
        double sum = 0, most = 0;
        for (const int l : over) {
            const double o = overlapArea(footprint, footprintOf(grid, globe.pieces[size_t(l)]));
            if (o <= 0) continue;
            receivers.push_back(l), overlap.push_back(o);
            sum += o, most = std::max(most, o);
        }
        const double half = SeamShare * piece.area;
        if (sum < half || (most < half && unionOverPiece(globe, k, receivers) < half)) continue;
        PlanCandidate candidate;
        candidate.piece = localOf(plan, globe, k);
        for (size_t i = 0; i < receivers.size(); ++i) candidate.receivers.push_back({localOf(plan, globe, receivers[i]), overlap[i] / sum});
        for (const int l : aroundOf(globe, band, k, cells, receivers)) candidate.behind.push_back(localOf(plan, globe, l));
        plan.input.candidates.push_back(std::move(candidate));
        cellsOf.push_back(cells);
    }
    if (plan.input.candidates.empty()) return 0;
    plan.input.sinks = [&](int donor, int survivor) { return sinksInPlan(globe, plan.global[size_t(donor)], plan.global[size_t(survivor)]); };
    // A donor the plan refuses for want of room spreads its crust farther through its plate, and what finds
    // no room there either is the doubled root that founders into the mantle (Kay & Kay 1993, docs/credits),
    // so the seam closes with the stock held by the interior (lead, 27 Nov). Only a widened donor
    // delaminates, and the plan is made again until none is refused: a widened reach may take the room
    // another donor had, which is then widened in turn. Every pass widens one donor more, so at most N.
    PlanResult result;
    for (;;) {
        result = planTransfers(plan.input);
        globe.stats.rounds += result.rounds;
        if (result.cancelled.empty()) break;
        globe.stats.seamsRefused += long(result.cancelled.size());
        for (size_t c = 0; c < plan.input.candidates.size(); ++c) {
            PlanCandidate& candidate = plan.input.candidates[c];
            if (std::find(result.cancelled.begin(), result.cancelled.end(), candidate.piece) == result.cancelled.end()) continue;
            std::vector<int> skip;
            for (const PlanShare& r : candidate.receivers) skip.push_back(plan.global[size_t(r.piece)]);
            candidate.behind.clear();
            for (const int l : wideAroundOf(globe, band, plan.global[size_t(candidate.piece)], cellsOf[c], skip, substep)) candidate.behind.push_back(localOf(plan, globe, l));
            candidate.delaminate = true;
        }
    }
    globe.stats.deferred += long(result.deferred.size());
    globe.stats.sutured += long(result.consumed.size());
    applyPlan(globe, band, plan, result, substep, false);
    return result.consumed.size();
}

constexpr int BirthsPerRound = 3;   // fossil births a cell may take in a closing round

// The live piece covering most of a cell (of its ring for a bare cell), and whether pieces of two
// plates lie over it or its ring.
struct CellPlates {
    int largest{-1};
    bool two{};
};
CellPlates platesAt(const Globe& globe, const Band& band, int cell) {
    CellPlates found;
    double most = 0, ring = 0;
    int plate = -1, nearest = -1;
    const auto look = [&](int k, double area) {
        const int p = globe.pieces[size_t(k)].plate;
        if (plate < 0) plate = p;
        found.two = found.two || p != plate;
        if (area > ring) ring = area, nearest = k;
    };
    forLive(band, cell, [&](int k, double area) {
        look(k, area);
        if (area > most) most = area, found.largest = k;
    });
    for (const int32_t other : (*globe.rings)[size_t(cell)])
        if (other >= 0) forLive(band, other, look);
    if (found.largest < 0) found.largest = nearest;   // a bare cell takes the plate of the ring
    return found;
}

// Every slot inside one plate next to a cell of the old band and open by half or more gets a new piece
// of that plate (fillSlot): the last spreading of a fossil ridge, ocean floor or crust necked from
// continental neighbours. Up to BirthsPerRound per cell and round, the key's order
// −1 − first − (round · BirthsPerRound + n). A cell of the old band where two plates now meet and that a
// suture left open by half takes a piece as at a closing boundary (fillCell, order
// −1 − first − (SeamRounds · BirthsPerRound + round)), so the new boundaries start with no cell open by half.
// `first` is a closing's SeamKeys · phase, so two closings of one step give their births distinct keys.
constexpr int SeamKeys = SeamRounds * (BirthsPerRound + 1);
size_t fill(Globe& globe, Band& band, const Substep& substep, int round, int first) {
    size_t filled = 0;
    for (const int32_t cell : band.cells) {
        for (int n = 0; n < BirthsPerRound && fillSlot(globe, band, cell, {substep.step, cell, -1 - first - (round * BirthsPerRound + n)}, globe.time); ++n)
            ++globe.stats.fossil, ++filled;
        const double half = BornShare * globe.grid->area[size_t(cell)];
        const Over over = overOf(globe, band, cell, -1, {}, {});
        if (coversOver(globe, cell, over, half, globe.stats.unions)) continue;
        const CellPlates at = platesAt(globe, band, cell);
        if (!at.two || at.largest < 0) continue;
        fillCell(globe, band, cell, at.largest, {substep.step, cell, -1 - first - (SeamRounds * BirthsPerRound + round)}, globe.time);
        ++globe.stats.fossil, ++filled;
    }
    return filled;
}

} // namespace

void reorganisePlates(Globe& globe, int step, Workers* workers) {
    Band band;
    markBand(globe, 1, band);
    enterGlobeEpoch(globe);
    closeSeams(globe, band, step, 0, workers);
}

size_t sutureBand(Globe& globe, Band& band, const Substep& substep) {
    size_t total = 0;
    for (size_t consumed = 1; consumed > 0;) total += consumed = suture(globe, band, substep);
    return total;
}

bool closeSeams(Globe& globe, Band& band, int step, int phase, Workers* workers) {
    const Substep seam{step, -1, 0.0};
    // A suture can leave a cycle for the next round and a fossil can cover a neighbour by the rest of
    // its half, so both repeat until neither changes anything (a few rounds; bounded for safety).
    int round = 0;
    for (; round < SeamRounds; ++round) {
        coupleBand(globe, band, workers);   // the last round's fossils become candidates of this one's sutures
        if (sutureBand(globe, band, seam) + fill(globe, band, seam, round, phase * SeamKeys) == 0) break;
    }
    const bool settled = round < SeamRounds;
    globe.stats.unsettled += !settled;
    settleBand(globe, band, phase > 0, workers);
    return settled;
}

} // namespace derivean::sim
