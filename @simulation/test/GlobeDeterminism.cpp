#include "GlobeFixture.hpp"

#include "chronology/constant/Timeline.hpp"
#include "core/service/Workers.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/GlobeEvolve.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <numeric>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

constexpr int Level = 4;
constexpr int64_t Start = 20;   // planet steps before the comparison; a climate solve falls at 32
constexpr int64_t Steps = 16;

template <class T> bool sameField(const SphereField<T>& a, const SphereField<T>& b) {
    return a.value.size() == b.value.size() && std::equal(a.value.begin(), a.value.end(), b.value.begin(), [](const T& x, const T& y) { return sameBits(x, y); });
}

// Columns and strata slots carry padding, so they compare member by member.
bool sameColumn(const CrustColumn& a, const CrustColumn& b) {
    const float fa[] = {a.crust, a.oceanAge, a.load, a.tectonicGain, a.orogenTime, a.quietSince, a.exhumed, a.meltTime, a.melt, a.erupted, a.eroded, a.deposited, a.fabric};
    const float fb[] = {b.crust, b.oceanAge, b.load, b.tectonicGain, b.orogenTime, b.quietSince, b.exhumed, b.meltTime, b.melt, b.erupted, b.eroded, b.deposited, b.fabric};
    return sameBits(fa, fb) && a.meltSource == b.meltSource && a.driftX == b.driftX && a.driftZ == b.driftZ && a.basement == b.basement;
}
bool sameFacies(const Facies& a, const Facies& b) {
    return a.gravel == b.gravel && a.sand == b.sand && a.mud == b.mud && a.carbonate == b.carbonate && a.volcanic == b.volcanic && a.evaporite == b.evaporite;
}
bool sameRecord(const ColumnRecord& a, const ColumnRecord& b) {
    if (!sameBits(a.past, b.past) || !sameBits(a.cover.older, b.cover.older) || !sameFacies(a.cover.olderFacies, b.cover.olderFacies)) return false;
    for (size_t i = 0; i < a.cover.slots.size(); ++i)
        if (!sameBits(a.cover.slots[i].thickness, b.cover.slots[i].thickness) || !sameFacies(a.cover.slots[i].facies, b.cover.slots[i].facies)) return false;
    return true;
}

bool sameContact(const GlobeContact& a, const GlobeContact& b) {
    return sameBits(a.distance, b.distance) && sameBits(a.convergence, b.convergence) && sameBits(a.shear, b.shear) && a.across == b.across && sameBits(a.normal, b.normal);
}

// Bit for bit: every piece, the fields of the mantle grid, the contacts, the plates' motion, the sea and
// the ledger.
void expectSame(const Globe& a, const Globe& b, const char* what) {
    expect(a.pieces.size() == b.pieces.size(), what, double(a.pieces.size()) - double(b.pieces.size()));
    for (size_t k = 0; k < a.pieces.size(); ++k) {
        const CrustPiece &p = a.pieces[k], &q = b.pieces[k];
        expect(p.key == q.key && p.birth == q.birth && p.plate == q.plate, what, double(k));
        expect(sameBits(p.stock, q.stock), what, p.stock.felsic - q.stock.felsic);
        expect(sameColumn(p.column, q.column), what, double(p.column.crust - q.column.crust));
        expect(sameRecord(p.record, q.record), what, double(k) + 0.5);
        expect(sameBits(p.rotation, q.rotation) && sameBits(p.area, q.area) && sameBits(p.bornAt, q.bornAt), what, double(k) + 0.25);
    }
    expect(sameField(a.elevation, b.elevation) && sameField(a.meltHeat, b.meltHeat) && sameField(a.erosion, b.erosion) &&
               sameField(a.deposition, b.deposition) && sameField(a.temperature, b.temperature) && sameField(a.precipitation, b.precipitation) &&
               sameField(a.owner, b.owner) && sameField(a.swell, b.swell),
           what);
    expect(a.contactZone == b.contactZone && a.contact.value.size() == b.contact.value.size() &&
               std::equal(a.contact.value.begin(), a.contact.value.end(), b.contact.value.begin(), sameContact),
           what);
    expect(a.motion.size() == b.motion.size() && a.applied.size() == b.applied.size() && a.epoch.plates.size() == b.epoch.plates.size() && a.splits.size() == b.splits.size() && a.plateEnds.size() == b.plateEnds.size(), what);
    for (size_t e = 0; e < a.plateEnds.size(); ++e) expect(sameBits(a.plateEnds[e].time, b.plateEnds[e].time) && a.plateEnds[e].id == b.plateEnds[e].id, what, double(e));
    for (size_t s = 0; s < a.splits.size(); ++s) {
        const PlateSplit &p = a.splits[s], &q = b.splits[s];
        expect(sameBits(p.time, q.time) && p.parent == q.parent && p.child == q.child && sameBits(p.area, q.area) && p.pieces == q.pieces, what, double(s));
    }
    for (size_t p = 0; p < a.motion.size(); ++p) expect(sameBits(a.motion[p].resistance, b.motion[p].resistance) && sameBits(a.motion[p].oceanAge, b.motion[p].oceanAge) && a.motion[p].id == b.motion[p].id && sameBits(a.applied[p], b.applied[p]), what, double(p));
    expect(sameBits(a.seaLevel, b.seaLevel) && a.seaLevels == b.seaLevels && sameBits(a.ledger, b.ledger), what);
}

// Test 10 (serial; stage 5 adds thread counts and plate order): the same history twice, and a history
// whose storage was shuffled, give bit for bit the same globe.
void checkDeterminism() {
    Workers workers;
    const double time = TectonicsStart + double(Start + Steps) * PlanetStep;
    const Globe once = evolveGlobe(42, time, Level, PlateSpeeds::Today, workers);
    const Globe twice = evolveGlobe(42, time, Level, PlateSpeeds::Today, workers);
    expect(once.stats.births > 0 && once.stats.consumptions > 0, "the history moves no boundary");
    expectSame(once, twice, "the same history twice differs");

    Globe kept = evolveGlobe(42, TectonicsStart + double(Start) * PlanetStep, Level, PlateSpeeds::Today, workers);
    Globe shuffled = kept;
    std::vector<size_t> order(shuffled.pieces.size());
    std::iota(order.begin(), order.end(), size_t{0});
    Random random{7};
    for (size_t i = order.size(); i > 1; --i) std::swap(order[i - 1], order[size_t(random.next() * double(i))]);
    for (size_t i = 0; i < order.size(); ++i) shuffled.pieces[i] = kept.pieces[order[i]];
    // A shuffled store has lost which pieces were coupled exactly (the band's), so it is coupled exactly
    // when it is put back in order (restoreKeyOrder); the kept one is too, and the two then run the
    // same history. The history itself never leaves key order, so it never takes this path.
    couple(*kept.grid, kept.pieces, {}, true, kept.coupling);
    for (int64_t step = Start; step < Start + Steps; ++step) {
        stepGlobe(kept, step, workers);
        stepGlobe(shuffled, step, workers);
    }
    expectSame(kept, shuffled, "a shuffled store gives another history");
}

} // namespace

void testGlobeDeterminism() { checkDeterminism(); }
