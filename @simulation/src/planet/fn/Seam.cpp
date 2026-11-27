#include "planet/fn/Seam.hpp"

#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Lattice.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "planet/fn/Transfers.hpp"

#include <algorithm>
#include <map>

namespace derivean::sim {
namespace {

constexpr double SeamShare = 0.5;   // of a piece its own plate covers when the seam consumes it (as at a boundary)
constexpr int Around = 8;           // pieces of its plate around a donor that its surplus may go to
constexpr int SeamRounds = 8;       // of sutures and fills

bool continental(const CrustPiece& piece) { return piece.column.crust >= float(OceanicLimit); }

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

// The live pieces of k's plate over k's cells and their rings that do not overlap it, nearest first
// (the lower key on a tie): where a donor's surplus goes when the pieces over it are full.
std::vector<int> aroundOf(const Globe& globe, const Band& band, size_t b, const std::vector<int>& skip) {
    const int k = band.pieces[b];
    const CrustPiece& piece = globe.pieces[size_t(k)];
    const Vec3 x = centreOf(*globe.grid, piece);
    std::vector<std::pair<double, int>> found;
    const auto consider = [&](int l, double) {
        const CrustPiece& other = globe.pieces[size_t(l)];
        if (l == k || other.plate != piece.plate || std::find(skip.begin(), skip.end(), l) != skip.end()) return;
        for (const auto& f : found)
            if (f.second == l) return;
        const Vec3 d = centreOf(*globe.grid, other) - x;
        found.push_back({dot(d, d), l});
    };
    for (int32_t s = band.weightStart[b]; s < band.weightStart[b + 1]; ++s) {
        const int cell = band.weights[size_t(s)].cell;
        forLive(band, cell, consider);
        for (const int32_t other : (*globe.rings)[size_t(cell)])
            if (other >= 0) forLive(band, other, consider);
    }
    std::sort(found.begin(), found.end(), [&](const auto& a, const auto& c) {
        return a.first != c.first ? a.first < c.first : globe.pieces[size_t(a.second)].key < globe.pieces[size_t(c.second)].key;
    });
    std::vector<int> around;
    for (size_t i = 0; i < found.size() && int(i) < Around; ++i) around.push_back(found[i].second);
    return around;
}

size_t suture(Globe& globe, Band& band, const Substep& substep) {
    const SphereGrid& grid = *globe.grid;
    PlanPieces plan{{}, {}, std::vector<int>(globe.pieces.size(), -1)};
    std::vector<int> over, receivers;
    std::vector<double> overlap;
    const size_t coupled = band.weightStart.size() - 1;
    for (size_t b = 0; b < coupled; ++b) {
        const int k = band.pieces[b];
        if (band.dead[size_t(k)]) continue;
        const CrustPiece& piece = globe.pieces[size_t(k)];
        bool deciding = false;
        over.clear();
        for (int32_t s = band.weightStart[b]; s < band.weightStart[b + 1]; ++s) {
            const int cell = band.weights[size_t(s)].cell;
            deciding = deciding || band.decide[size_t(cell)];
            forLive(band, cell, [&](int l, double) {
                if (l != k && globe.pieces[size_t(l)].plate == piece.plate && std::find(over.begin(), over.end(), l) == over.end()) over.push_back(l);
            });
        }
        if (!deciding || over.empty()) continue;
        std::sort(over.begin(), over.end(), [&](int a, int c) { return globe.pieces[size_t(a)].key < globe.pieces[size_t(c)].key; });
        const SpherePolygon footprint = footprintOf(grid, piece);
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
        candidate.piece = plan.localOf(globe, k);
        for (size_t i = 0; i < receivers.size(); ++i) candidate.receivers.push_back({plan.localOf(globe, receivers[i]), overlap[i] / sum});
        for (const int l : aroundOf(globe, band, b, receivers)) candidate.behind.push_back(plan.localOf(globe, l));
        plan.input.candidates.push_back(std::move(candidate));
    }
    if (plan.input.candidates.empty()) return 0;
    plan.input.sinks = [&](int donor, int survivor) {
        const int d = plan.global[size_t(donor)], s = plan.global[size_t(survivor)];
        return sinksUnder(globe, d, s) || (!continental(globe.pieces[size_t(d)]) && !continental(globe.pieces[size_t(s)]));
    };
    const PlanResult result = planTransfers(plan.input);
    globe.stats.rounds += result.rounds;
    globe.stats.deferred += long(result.deferred.size());
    globe.stats.sutured += long(result.consumed.size());
    globe.stats.seamsKept += long(result.cancelled.size());
    applyPlan(globe, band, plan, result, substep);
    return result.consumed.size();
}

// The plate that covers most of a cell and its ring among the live pieces (the lower index on a tie).
int plateAround(const Globe& globe, const Band& band, int cell) {
    std::map<int, double> cover;
    const auto add = [&](int k, double area) { cover[globe.pieces[size_t(k)].plate] += area; };
    forLive(band, cell, add);
    for (const int32_t other : (*globe.rings)[size_t(cell)])
        if (other >= 0) forLive(band, other, add);
    int plate = -1;
    double most = 0;
    for (const auto& [p, area] : cover)
        if (area > most) plate = p, most = area;
    return plate;
}

size_t fill(Globe& globe, Band& band, const Substep& substep, int round) {
    size_t filled = 0;
    const SphereGrid& grid = *globe.grid;
    for (const int32_t cell : band.cells) {
        const double area = grid.area[size_t(cell)], half = BornShare * area;
        const Over over = overOf(globe, band, cell, -1, {}, {});
        if (over.sum > half && (over.most > half || unionInCell(globe, cell, over.pieces, over.sum, globe.stats.unions) > half)) continue;
        const int plate = plateAround(globe, band, cell);
        if (plate < 0) continue;
        if (!fillCell(globe, band, cell, plate, {substep.step, cell, -1 - round}, globe.time)) continue;
        ++globe.stats.fossil, ++filled;
    }
    return filled;
}

} // namespace

void reorganisePlates(Globe& globe, int step) {
    Band band;
    markBand(globe, 1, band);
    enterGlobeEpoch(globe);
    closeSeams(globe, band, step);
}

void closeSeams(Globe& globe, Band& band, int step) {
    const Substep seam{step, -1, 0.0};
    coupleBand(globe, band);
    // A suture can leave a cycle for the next round and a fossil can cover a neighbour by the rest of
    // its half, so both repeat until neither changes anything (a few rounds; bounded for safety).
    for (int round = 0; round < SeamRounds; ++round) {
        size_t changed = 0;
        for (size_t consumed = 1; consumed > 0;) changed += consumed = suture(globe, band, seam);
        changed += fill(globe, band, seam, round);
        if (changed == 0) break;
    }
    settleBand(globe, band);
}

} // namespace derivean::sim
