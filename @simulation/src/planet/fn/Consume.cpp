#include "planet/fn/Boundary.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Lattice.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Plan.hpp"
#include "planet/fn/Transfers.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr double ConsumedShare = 0.5;   // of a footprint covered by overriding plates (Codex V2: a discrete choice)
constexpr int Behind = 8;               // pieces of its own plate a donor's surplus may back into

struct Overrider {
    int piece{};
    double overlap{};   // m², |footprint_k ∩ footprint_l|
};

bool continental(const CrustPiece& piece) { return piece.column.crust >= float(OceanicLimit); }

// Whether l's plate overrides k: k sinks under l, or both are continents and k's plate is the one
// arriving, with the larger component of its own motion against the other (the lower plate index on
// a tie). Only where the two plates close.
bool overrides(const Globe& globe, int l, int k) {
    const CrustPiece& own = globe.pieces[size_t(k)];
    const CrustPiece& other = globe.pieces[size_t(l)];
    const Vec3 x = centreOf(*globe.grid, own);
    const Vec3 normal = towards(x, centreOf(*globe.grid, other));
    if (closingAt(globe.applied[own.plate], globe.applied[other.plate], x, normal).convergence <= 0) return false;
    if (sinksUnder(globe, k, l)) return true;
    if (!continental(own) || !continental(other)) return false;
    const double arriving = dot(velocityAt(globe.applied[own.plate], x), normal);
    const double meeting = dot(velocityAt(globe.applied[other.plate], x), normal * -1.0);
    return arriving > meeting || (arriving == meeting && own.plate < other.plate);
}

// How much of k the given pieces cover together, in k's tangent plane.
double unionOver(const Globe& globe, int k, const std::vector<Overrider>& pieces, int plate, long& unions) {
    const TangentPlane plane = tangentPlaneAt(centreOf(*globe.grid, globe.pieces[size_t(k)]));
    const PlanePolygon own = projected(plane, footprintOf(*globe.grid, globe.pieces[size_t(k)]));
    std::vector<PlanePolygon> parts;
    for (const Overrider& o : pieces)
        if (plate < 0 || globe.pieces[size_t(o.piece)].plate == plate)
            parts.push_back(clipped(projected(plane, footprintOf(*globe.grid, globe.pieces[size_t(o.piece)])), own));
    ++unions;
    return unionArea(parts).area;
}

// The piece of `plate` with the largest weight in a cell, from the band where it reaches, else from
// the step's coupling.
int plateIn(const Globe& globe, const Band& band, int cell, int plate, const std::vector<int>& skip) {
    int best = -1;
    double most = 0;
    const auto consider = [&](int piece, double area) {
        if (band.dead[size_t(piece)] || globe.pieces[size_t(piece)].plate != plate || area <= most) return;
        if (std::find(skip.begin(), skip.end(), piece) != skip.end()) return;
        best = piece, most = area;
    };
    const BandList list = listOf(band, cell);
    if (list.begin) {
        for (const PieceWeight* w = list.begin; w != list.end; ++w) consider(w->piece, w->area);
    } else {
        const Coupling& c = globe.coupling;
        for (int32_t s = c.cellStart[size_t(cell)]; s < c.cellStart[size_t(cell) + 1]; ++s) consider(c.byCell[size_t(s)].piece, c.byCell[size_t(s)].area);
    }
    return best;
}

// Pieces of k's plate behind it, against its motion, one cell apart, nearest first.
std::vector<int> behindOf(const Globe& globe, const Band& band, int k) {
    const CrustPiece& piece = globe.pieces[size_t(k)];
    const Vec3 x = centreOf(*globe.grid, piece);
    const Vec3 velocity = velocityAt(globe.applied[piece.plate], x);
    std::vector<int> found{k};
    if (length(velocity) <= 0) return {};
    const Vec3 axis = normalized(cross(x, velocity * -1.0));
    const double step = std::sqrt(piece.area) / PlanetRadius;
    for (int i = 1; i <= 2 * Behind && int(found.size()) <= Behind; ++i) {
        const Vec3 at = rotate(rotationAbout(axis, i * step), x);
        const int other = plateIn(globe, band, indexOf(*globe.grid, cellOf(at, globe.grid->level)), piece.plate, found);
        if (other >= 0) found.push_back(other);
    }
    found.erase(found.begin());
    return found;
}

// Holds back every candidate over a cell that the candidates together keep at least half covered and
// the other live pieces do not (Codex DREV-CLAUDE-005-04): consuming them would open it by more than
// half at a trench, where the sinking plate has not yet moved into the room its consumed edge leaves.
// They wait until the overriding plate covers more of the cell; a hold is not a cancellation and
// brakes nothing. The cells' unions are measured once per substep.
void holdOpening(Globe& globe, const Band& band, PlanPieces& plan, const std::vector<size_t>& bandOf) {
    std::vector<PlanCandidate>& candidates = plan.input.candidates;
    if (candidates.empty()) return;
    std::vector<uint8_t> candidate(globe.pieces.size(), 0);
    for (const PlanCandidate& c : candidates) candidate[size_t(plan.global[size_t(c.piece)])] = 1;
    std::vector<int8_t> opens(band.regionCells.size(), -1);   // per region cell: unknown, no, yes
    std::vector<int> all, rest;
    const auto opening = [&](int cell) {
        int8_t& known = opens[size_t(band.region[size_t(cell)])];
        if (known >= 0) return known == 1;
        all.clear(), rest.clear();
        double sumAll = 0, sumRest = 0;
        forLive(band, cell, [&](int k, double area) {
            all.push_back(k), sumAll += area;
            if (!candidate[size_t(k)]) rest.push_back(k), sumRest += area;
        });
        const double half = BornShare * globe.grid->area[size_t(cell)];
        known = 0;
        if (sumRest < sumAll && sumAll >= half && unionInCell(globe, cell, rest, sumRest, globe.stats.unions) < half)
            known = unionInCell(globe, cell, all, sumAll, globe.stats.unions) >= half;
        return known == 1;
    };
    size_t kept = 0;
    for (size_t c = 0; c < candidates.size(); ++c) {
        bool hold = false;
        for (int32_t s = band.weightStart[bandOf[c]]; s < band.weightStart[bandOf[c] + 1] && !hold; ++s) {
            const int cell = band.weights[size_t(s)].cell;
            hold = band.region[size_t(cell)] >= 0 && opening(cell);
        }
        if (hold) {
            ++globe.stats.held;
            continue;
        }
        if (kept != c) candidates[kept] = std::move(candidates[c]);
        ++kept;
    }
    candidates.resize(kept);
}

} // namespace

void consumeInBand(Globe& globe, Band& band, const Substep& substep) {
    const SphereGrid& grid = *globe.grid;
    PlanPieces plan{{}, {}, std::vector<int>(globe.pieces.size(), -1)};
    PlanInput& input = plan.input;
    const auto localOf = [&](int piece) { return plan.localOf(globe, piece); };
    std::vector<int> partners;
    std::vector<Overrider> overriders;
    std::vector<size_t> bandOf;   // per candidate: its index among the band's pieces
    const size_t coupled = band.weightStart.size() - 1;
    for (size_t b = 0; b < coupled; ++b) {
        const int k = band.pieces[b];
        if (band.dead[size_t(k)]) continue;
        const CrustPiece& piece = globe.pieces[size_t(k)];
        bool deciding = false;
        partners.clear();
        for (int32_t s = band.weightStart[b]; s < band.weightStart[b + 1]; ++s) {
            const int cell = band.weights[size_t(s)].cell;
            deciding = deciding || band.decide[size_t(cell)];
            const BandList list = listOf(band, cell);
            for (const PieceWeight* w = list.begin; w != list.end; ++w)
                if (globe.pieces[size_t(w->piece)].plate != piece.plate && std::find(partners.begin(), partners.end(), w->piece) == partners.end())
                    partners.push_back(w->piece);
        }
        if (!deciding || partners.empty()) continue;
        std::sort(partners.begin(), partners.end(), [&](int a, int b) { return globe.pieces[size_t(a)].key < globe.pieces[size_t(b)].key; });
        overriders.clear();
        double sum = 0, most = 0;
        const SpherePolygon footprint = footprintOf(grid, piece);
        for (const int l : partners) {
            if (!overrides(globe, l, k)) continue;
            const double overlap = overlapArea(footprint, footprintOf(grid, globe.pieces[size_t(l)]));
            if (overlap <= 0) continue;
            overriders.push_back({l, overlap});
            sum += overlap, most = std::max(most, overlap);
        }
        const double half = ConsumedShare * piece.area;
        if (sum < half) continue;
        if (most < half && unionOver(globe, k, overriders, -1, globe.stats.unions) < half) continue;
        // The receiving plate is the one covering most of k, by its own union (the lower index on a tie).
        int plate = -1;
        double best = -1;
        for (const Overrider& o : overriders) {
            const int q = globe.pieces[size_t(o.piece)].plate;
            if (q == plate) continue;
            int count = 0;
            double single = 0;
            for (const Overrider& p : overriders)
                if (globe.pieces[size_t(p.piece)].plate == q) ++count, single = p.overlap;
            const double cover = count == 1 ? single : unionOver(globe, k, overriders, q, globe.stats.unions);
            if (cover > best || (cover == best && q < plate)) best = cover, plate = q;
        }
        for (const Overrider& o : overriders)
            if (globe.pieces[size_t(o.piece)].plate != globe.pieces[size_t(overriders[0].piece)].plate) {
                ++globe.stats.contested;
                break;
            }
        PlanCandidate candidate;
        candidate.piece = localOf(k);
        double total = 0;
        for (const Overrider& o : overriders)
            if (globe.pieces[size_t(o.piece)].plate == plate) total += o.overlap;
        for (const Overrider& o : overriders)
            if (globe.pieces[size_t(o.piece)].plate == plate) candidate.receivers.push_back({localOf(o.piece), o.overlap / total});
        for (const int other : behindOf(globe, band, k)) candidate.behind.push_back(localOf(other));
        input.candidates.push_back(std::move(candidate));
        bandOf.push_back(b);
    }
    holdOpening(globe, band, plan, bandOf);
    if (input.candidates.empty()) return;
    // Two oceans never stack: a chain that ends on an older ocean still sends the donor down.
    input.sinks = [&](int donor, int survivor) {
        const int d = plan.global[size_t(donor)], s = plan.global[size_t(survivor)];
        return sinksUnder(globe, d, s) || (!continental(globe.pieces[size_t(d)]) && !continental(globe.pieces[size_t(s)]));
    };
    const PlanResult result = planTransfers(input);
    globe.stats.rounds += result.rounds;
    globe.stats.cancelled += long(result.cancelled.size());
    globe.stats.deferred += long(result.deferred.size());
    applyPlan(globe, band, plan, result, substep);
    // A cancelled donor jams its plate by what it could not place in the round that cancelled it (§4;
    // Codex DREV-CLAUDE-005-02), once per settled plan and per planet step of the substep: the donor
    // stays and meets the same shortfall in every substep, so the brake does not grow with their count.
    for (size_t i = 0; i < result.cancelled.size(); ++i)
        globe.motion[size_t(input.plate[size_t(result.cancelled[i])])].resistance += result.shortfall[i] * substep.duration / PlanetStep;
}

} // namespace derivean::sim
