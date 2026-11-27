#include "planet/fn/Boundary.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Plan.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr double ConsumedShare = 0.5;   // of a footprint covered by overriding plates (Codex V2: a discrete choice)
constexpr int Behind = 8;               // pieces of its own plate a donor's surplus may back into
constexpr double ChurnAge = 10 * Myr;   // a piece consumed younger than this is churn (S3-design.md test 14)

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

// As the flat Crossing.cpp's markStacked, with the strike of the contact between the two pieces.
void markStacked(CrustPiece& piece, double time, float strike) {
    piece.column.orogenTime = piece.column.quietSince = float(time);
    piece.column.exhumed = 0;
    piece.column.fabric = strike;
    piece.column.basement = Basement::Orogen;
}

void book(Globe& globe, const Transfer& t, const std::vector<int>& global, std::vector<Cover>& covers, std::vector<int>& coverOf) {
    CrustPiece& from = globe.pieces[size_t(global[size_t(t.from)])];
    const double felsic = takeFelsic(from, t.felsic);
    const double basalt = std::clamp(t.basalt, 0.0, from.stock.basalt);
    from.stock.basalt -= basalt;
    if (t.to < 0) {
        globe.ledger.interior.felsic += felsic;
        globe.ledger.interior.basalt += basalt;
        globe.ledger.subducted += felsic;
        return;
    }
    CrustPiece& to = globe.pieces[size_t(global[size_t(t.to)])];
    giveFelsic(to, felsic);
    to.stock.basalt += basalt;
    const float gain = float(felsic / to.area);
    to.column.tectonicGain += gain;
    if (t.kind == TransferKind::Accrete) {
        addDeposit(to.record.cover, globe.time, gain, Facies{0, 60, 195, 0, 0, 0});
        globe.ledger.accreted += felsic;
        return;
    }
    // The donor's cover as it was before its first transfer of the substep goes along by share.
    if (felsic <= 0) return;
    Cover share = covers[size_t(coverOf[size_t(t.from)])];
    scaleCover(share, float(t.cover * from.area / to.area));
    stackCover(to.record.cover, share);
    const Vec3 at = centreOf(*globe.grid, to);
    markStacked(to, globe.time, flatStrikeOf(at, towards(at, centreOf(*globe.grid, from))));
    globe.motion[from.plate].resistance += felsic;
    globe.ledger.stacked += felsic;
}

} // namespace

void consumeInBand(Globe& globe, Band& band, const Substep& substep) {
    const SphereGrid& grid = *globe.grid;
    PlanInput input;
    std::vector<int> global, local(globe.pieces.size(), -1);
    const auto localOf = [&](int piece) {
        if (local[size_t(piece)] >= 0) return local[size_t(piece)];
        const CrustPiece& p = globe.pieces[size_t(piece)];
        local[size_t(piece)] = int(global.size());
        global.push_back(piece);
        input.key.push_back(p.key);
        input.plate.push_back(p.plate);
        input.felsic.push_back(p.stock.felsic);
        input.basalt.push_back(p.stock.basalt);
        input.sediment.push_back(std::min(p.stock.felsic, double(coverThickness(p.record.cover)) * p.area));
        input.room.push_back(std::max(0.0, double(StackLimit) * p.area - p.stock.felsic));
        return local[size_t(piece)];
    };
    std::vector<int> partners;
    std::vector<Overrider> overriders;
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
    }
    if (input.candidates.empty()) return;
    // Two oceans never stack: a chain that ends on an older ocean still sends the donor down.
    input.sinks = [&](int donor, int survivor) {
        const int d = global[size_t(donor)], s = global[size_t(survivor)];
        return sinksUnder(globe, d, s) || (!continental(globe.pieces[size_t(d)]) && !continental(globe.pieces[size_t(s)]));
    };
    const PlanResult plan = planTransfers(input);
    globe.stats.rounds += plan.rounds;
    globe.stats.cancelled += long(plan.cancelled.size());
    globe.stats.deferred += long(plan.deferred.size());

    // Covers are read as they stood before any transfer, so a donor's share does not depend on order.
    std::vector<Cover> covers;
    std::vector<int> coverOf(global.size(), -1);
    for (const int d : plan.consumed) coverOf[size_t(d)] = int(covers.size()), covers.push_back(globe.pieces[size_t(global[size_t(d)])].record.cover);
    for (const Transfer& t : plan.transfers) book(globe, t, global, covers, coverOf);
    for (const int d : plan.consumed) globe.ledger.consumedFelsic += input.felsic[size_t(d)];
    for (const int d : plan.consumed) {
        const int k = global[size_t(d)];
        CrustPiece& gone = globe.pieces[size_t(k)];
        // What rounding of the shares leaves on a consumed piece goes to the interior with it.
        globe.ledger.interior.felsic += gone.stock.felsic;
        globe.ledger.interior.basalt += gone.stock.basalt;
        globe.ledger.subducted += gone.stock.felsic;
        gone.stock = {};
        band.dead[size_t(k)] = 1;
        globe.ledger.consumed += gone.area;
        ++globe.stats.consumptions;
        if (gone.key.step >= 0 && globe.time + (substep.index + 1) * substep.duration - gone.bornAt < ChurnAge) globe.ledger.churned += gone.area, ++globe.stats.churned;
    }
    // A cancelled donor jams its plate by what it could not place in the round that cancelled it (§4;
    // Codex DREV-CLAUDE-005-02), once per settled plan and per planet step of the substep: the donor
    // stays and meets the same shortfall in every substep, so the brake does not grow with their count.
    for (size_t i = 0; i < plan.cancelled.size(); ++i)
        globe.motion[size_t(input.plate[size_t(plan.cancelled[i])])].resistance += plan.shortfall[i] * substep.duration / PlanetStep;
}

} // namespace derivean::sim
