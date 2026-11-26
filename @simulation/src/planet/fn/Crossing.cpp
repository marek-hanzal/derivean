#include "planet/fn/Crossing.hpp"

namespace derivean::sim {
namespace {

constexpr float StretchShare = 0.4f;
constexpr float OffscrapeShare = 0.25f;
constexpr float StackLimit = 64000.0f;    // m of crust a collision can stack before it backs up

void resetMarks(Parcel& parcel) {
    parcel.record.past.fill({columnElevation(parcel.column), parcel.column.crust, parcel.column.tectonicGain,
                             parcel.column.eroded, parcel.column.deposited, parcel.column.melt, parcel.column.erupted});
}

} // namespace

// Fills the cell a plate leaves behind. Where nothing follows the plate, continental crust necks and
// thins into the gap; otherwise the gap is a new spreading contact that a follower may take over.
Parcel openGap(CrustColumn& departing, ColumnRecord& record, double time, bool stretch) {
    Parcel fresh;
    fresh.column.quietSince = float(time);
    if (!stretch || departing.crust < 0.5f * float(ContinentalLimit)) {
        fresh.column.basement = Basement::Oceanic;
        resetMarks(fresh);
        return fresh;
    }
    fresh.column = departing;
    fresh.record.cover = record.cover;
    fresh.column.oceanAge = 0;
    fresh.column.crust = departing.crust * StretchShare;
    fresh.column.load = departing.load * StretchShare;
    fresh.column.tectonicGain = fresh.column.eroded = fresh.column.deposited = 0;
    fresh.column.driftX = fresh.column.driftZ = 0;
    scaleCover(fresh.record.cover, StretchShare);
    resetMarks(fresh);
    const float kept = 1 - StretchShare;
    departing.tectonicGain -= departing.crust * StretchShare;
    departing.crust *= kept;
    departing.load *= kept;
    departing.quietSince = float(time);
    scaleCover(record.cover, kept);
    return fresh;
}

namespace {

double volumeOf(const CrustColumn& column) { return double(column.crust + column.load) * PlanetCellArea; }

void markStacked(CrustColumn& column, double time, float strike) {
    column.orogenTime = column.quietSince = float(time);
    column.exhumed = 0;
    column.fabric = strike;
    column.basement = Basement::Orogen;
}

} // namespace

// Material pushed off the leading edge of a plate enters the cell across the contact.
//  - A gap the neighbour has just opened is simply taken over: no relative motion, no deformation.
//  - A thin oceanic column sinks under whatever it meets and leaves a scraped wedge.
//  - A continent rides over old ocean floor, which sinks beneath it.
//  - Two continents cannot sink: they stack up to StackLimit and the surplus backs up into the
//    arriving plate, which is what widens a collision belt into a plateau and brakes the plate.
// The cell changes hands only when the arriving crust comes to lie on top: a plate advances over
// what sinks under it and is itself consumed where it sinks.
void crossContact(Line line, const Parcel& carry, int ahead, int lead, int direction, int length, int plate, PlateMotion& motion, float strike) {
    Planet& planet = line.planet;
    CrustColumn& target = line.column(ahead);
    const bool carryOceanic = carry.column.crust < float(OceanicLimit);
    const bool targetOceanic = target.crust < float(OceanicLimit);
    if (targetOceanic && (!carryOceanic || target.oceanAge < 3 * float(PlanetStep))) {
        planet.ledger.subducted += volumeOf(target);
        line.put(ahead, carry);
        line.claim(ahead, plate);
        if (!carryOceanic) target.quietSince = float(planet.time);
        return;
    }
    Cover& cover = line.record(ahead).cover;
    if (carryOceanic) {
        const float sediment = coverThickness(carry.record.cover) + carry.column.load;
        const float scraped = std::min(carry.column.crust + carry.column.load, OffscrapeShare * sediment);
        if (scraped > 0) {
            addDeposit(cover, planet.time, scraped, Facies{0, 60, 195, 0, 0, 0});
            target.crust += scraped;
            target.tectonicGain += scraped;
        }
        planet.ledger.accreted += double(scraped) * PlanetCellArea;
        planet.ledger.subducted += volumeOf(carry.column) - double(scraped) * PlanetCellArea;
        return;
    }
    target.crust += carry.column.crust;
    target.tectonicGain += carry.column.crust;
    target.load += carry.column.load;
    stackCover(cover, carry.record.cover);
    markStacked(target, planet.time, strike);
    motion.resistance += carry.column.crust;

    float surplus = target.crust - StackLimit;
    if (surplus <= 0) return;
    // The rock that backs up carries its share of the strata with it.
    Cover backed = cover;
    scaleCover(backed, 1.0f / target.crust);
    scaleCover(cover, StackLimit / target.crust);
    target.crust = StackLimit;
    target.tectonicGain -= surplus;
    int cell = lead;
    for (int step = 0; step < length && surplus > 0; ++step) {
        CrustColumn& behind = line.column(cell);
        const float room = step + 1 == length ? surplus : std::max(0.0f, StackLimit - behind.crust);
        const float taken = std::min(surplus, room);
        if (taken > 0) {
            behind.crust += taken;
            behind.tectonicGain += taken;
            Cover share = backed;
            scaleCover(share, taken);
            stackCover(line.record(cell).cover, share);
            markStacked(behind, planet.time, strike);
        }
        surplus -= taken;
        cell = wrapIndex(cell - direction, PlanetCells);
    }
}

} // namespace derivean::sim
