#include "planet/fn/Transfers.hpp"

#include "chronology/constant/Timeline.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr double ChurnAge = 10 * Myr;   // a piece consumed younger than this is churn (S3-design.md test 14)

// As the flat Crossing.cpp's markStacked, with the strike of the contact between the two pieces.
void markStacked(CrustPiece& piece, double time, float strike) {
    piece.column.orogenTime = piece.column.quietSince = float(time);
    piece.column.exhumed = 0;
    piece.column.fabric = strike;
    piece.column.basement = Basement::Orogen;
}

void book(Globe& globe, const Transfer& t, const std::vector<int>& global, std::vector<Cover>& covers, std::vector<int>& coverOf, bool boundary) {
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
    if (boundary) {
        const Vec3 at = centreOf(*globe.grid, to);
        markStacked(to, globe.time, flatStrikeOf(at, towards(at, centreOf(*globe.grid, from))));
        globe.motion[from.plate].resistance += felsic;
    }
    globe.ledger.stacked += felsic;
}

} // namespace

int localOf(PlanPieces& plan, const Globe& globe, int piece) {
    std::vector<int>& local = plan.local;
    PlanInput& input = plan.input;
    if (local[size_t(piece)] >= 0) return local[size_t(piece)];
    const CrustPiece& p = globe.pieces[size_t(piece)];
    local[size_t(piece)] = int(plan.global.size());
    plan.global.push_back(piece);
    input.key.push_back(p.key);
    input.plate.push_back(p.plate);
    input.felsic.push_back(p.stock.felsic);
    input.basalt.push_back(p.stock.basalt);
    input.sediment.push_back(std::min(p.stock.felsic, double(coverThickness(p.record.cover)) * p.area));
    input.room.push_back(std::max(0.0, double(StackLimit) * p.area - p.stock.felsic));
    return local[size_t(piece)];
}

void applyPlan(Globe& globe, Band& band, const PlanPieces& pieces, const PlanResult& plan, const Substep& substep, bool boundary) {
    const std::vector<int>& global = pieces.global;
    // Covers are read as they stood before any transfer, so a donor's share does not depend on order.
    std::vector<Cover> covers;
    std::vector<int> coverOf(global.size(), -1);
    for (const int d : plan.consumed) coverOf[size_t(d)] = int(covers.size()), covers.push_back(globe.pieces[size_t(global[size_t(d)])].record.cover);
    for (const Transfer& t : plan.transfers) book(globe, t, global, covers, coverOf, boundary);
    for (const int d : plan.consumed) globe.ledger.consumedFelsic += pieces.input.felsic[size_t(d)];
    const double now = globe.time + (substep.index + 1) * substep.duration;
    for (const int d : plan.consumed) {
        const int k = global[size_t(d)];
        CrustPiece& gone = globe.pieces[size_t(k)];
        // What rounding of the shares leaves on a consumed piece goes to the interior with it.
        globe.ledger.interior.felsic += gone.stock.felsic;
        globe.ledger.interior.basalt += gone.stock.basalt;
        globe.ledger.subducted += gone.stock.felsic;
        gone.stock = {};
        band.dead[size_t(k)] = 1;
        ++band.slotGeneration;
        globe.ledger.consumed += gone.area;
        ++globe.stats.consumptions;
        if (gone.key.step >= 0 && now - gone.bornAt < ChurnAge) globe.ledger.churned += gone.area, ++globe.stats.churned;
    }
}

} // namespace derivean::sim
