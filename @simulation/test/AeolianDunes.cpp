#include "aeolian/fn/Dunes.hpp"
#include "aeolian/fn/Measure.hpp"
#include "aeolian/fn/Regime.hpp"
#include "core/service/Workers.hpp"
#include "landscape/fn/Process.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

void testAeolianGround();

namespace {

constexpr int Side = 192;
constexpr double Cell = 4.0;
constexpr double Degrees = 3.141592653589793 / 180.0;

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Aeolian: " << message << " (" << value << ")\n";
    std::exit(1);
}

SandBed flatBed(float sand, uint64_t seed = 7) {
    SandBed bed;
    bed.size = Side, bed.cell = Cell, bed.seed = seed, bed.reposeTan = ReposeTan[Sand];
    bed.base.assign(bed.cells(), 100.0f);
    bed.sand.assign(bed.cells(), sand);
    bed.kind.assign(bed.cells(), BedCell::Open);
    return bed;
}

WindRegime steady(std::initializer_list<double> headings) {
    std::vector<double> heading(headings), speed(heading.size(), 10.0), share(heading.size(), 1.0 / double(heading.size()));
    for (double& value : heading) value *= Degrees;
    return regimeOf(int(heading.size()), heading.data(), speed.data(), share.data());
}

double total(const SandBed& bed) {
    double sum = 0;
    for (size_t i = 0; i < bed.cells(); ++i)
        if (bed.kind[i] == BedCell::Open) sum += double(bed.sand[i]) * bed.cell * bed.cell;
    return sum;
}

std::vector<float> topOf(const SandBed& bed) {
    std::vector<float> top(bed.cells());
    for (size_t i = 0; i < bed.cells(); ++i) top[i] = bed.top(i);
    return top;
}

// Cells along +X by which the sand of `after` lies downwind of the sand of `before`.
int shiftAlongX(const SandBed& before, const SandBed& after) {
    int best = 0;
    double most = -1;
    for (int lag = -12; lag <= 12; ++lag) {
        double match = 0;
        for (int z = 0; z < Side; ++z)
            for (int x = 0; x < Side; ++x) match += double(before.sand[before.index(x, z)]) * after.sand[after.index((x + lag + Side) % Side, z)];
        if (match > most) most = match, best = lag;
    }
    return best;
}

// One steady wind over plenty of sand: ridges across the wind, lopsided, with slip faces at repose
// on the side away from the wind, moving downwind, and not a grain made or lost.
void checkOneWind() {
    SandBed bed = flatBed(5.0f);
    const WindRegime wind = steady({0.0});
    const double before = total(bed);
    blowBed(bed, wind, {300, 0.4f, 4});
    expect(std::abs(total(bed) - before) < 1e-4 * before, "a bed without edges gained or lost sand", total(bed) - before);
    const DuneStats dunes = measureDunes(Side, Cell, topOf(bed), bed.sand, wind.resultant, bed.reposeTan);
    expect(dunes.slipShare > 0.08, "one wind over deep sand built no slip faces", dunes.slipShare);
    expect(dunes.leeSlipShare > 0.9, "slip faces do not face away from the wind", dunes.leeSlipShare);
    expect(dunes.steepest < 1.1 * bed.reposeTan, "sand stands steeper than it can rest", dunes.steepest);
    expect(dunes.windward < 0.75 * dunes.lee, "dunes are not gentler on the side the wind comes from", dunes.windward / dunes.lee);
    expect(dunes.crestAngle > 60 && dunes.order > 0.4, "deep sand under one wind did not form ridges across the wind", dunes.crestAngle);
    expect(dunes.tallest > 5 && dunes.tallest < 60 && dunes.spacing > 40 && dunes.spacing < 600, "ridges are not the size of dunes", dunes.spacing);

    SandBed later = bed;
    blowBed(later, wind, {20, 0.4f, 4});
    expect(shiftAlongX(bed, later) > 0, "dunes did not move downwind", shiftAlongX(bed, later));

    // The same bed and seed give the same field, bit for bit, on the calling thread alone or spread over a pool.
    SandBed again = flatBed(5.0f);
    blowBed(again, wind, {300, 0.4f, 4});
    expect(again.sand == bed.sand, "the same bed gave two different fields");
    Workers pool(3);
    SandBed spread = flatBed(5.0f);
    spread.workers = &pool;
    blowBed(spread, wind, {300, 0.4f, 4});
    expect(spread.sand == bed.sand, "a pool changed the field");
}

// Little sand under the same wind leaves separate dunes on a bare floor, not ridges.
void checkScarce() {
    SandBed bed = flatBed(1.0f);
    const WindRegime wind = steady({0.0});
    blowBed(bed, wind, {500, 0.2f, 4});
    const DuneStats dunes = measureDunes(Side, Cell, topOf(bed), bed.sand, wind.resultant, bed.reposeTan);
    expect(dunes.sandShare < 0.85 && dunes.order < 0.45, "scarce sand formed ridges instead of separate dunes", dunes.order);
    expect(dunes.tallest > 3, "scarce sand gathered into no dunes", dunes.tallest);
}

// Two winds a third of a circle apart: ridges along the direction sand goes on balance.
void checkTwoWinds() {
    SandBed bed = flatBed(3.0f);
    const WindRegime wind = steady({-60.0, 60.0});
    blowBed(bed, wind, {300, 0.4f, 4});
    const DuneStats dunes = measureDunes(Side, Cell, topOf(bed), bed.sand, wind.resultant, bed.reposeTan);
    expect(dunes.crestAngle < 30 && dunes.order > 0.35, "two winds did not form ridges along their resultant", dunes.crestAngle);
}

// Water takes sand and gives none; a finished neighbour's cells give and take without changing;
// and the bed's ledger says exactly where the sand of its open cells went.
void checkEdges() {
    SandBed bed = flatBed(3.0f);
    for (int z = 0; z < Side; ++z) {
        for (int x = 0; x < 8; ++x) bed.kind[bed.index(x, z)] = BedCell::Held;
        for (int x = 120; x < 126; ++x) bed.kind[bed.index(x, z)] = BedCell::Wet;
    }
    const std::vector<float> start = bed.sand;
    const double before = total(bed);
    blowBed(bed, steady({0.0}), {120, 0.4f, 4});
    for (int z = 0; z < Side; ++z)
        for (int x : {0, 7, 120, 125}) expect(bed.sand[bed.index(x, z)] == start[bed.index(x, z)], "a held or wet cell changed", x);
    const BedLedger& ledger = bed.ledger;
    expect(ledger.drowned > 0 && ledger.entered > 0 && ledger.left > 0, "nothing crossed into water or a neighbour", ledger.drowned);
    expect(std::abs(total(bed) - before - (ledger.entered - ledger.left - ledger.drowned)) < 1e-4 * before, "the bed's ledger does not close",
           total(bed) - before - (ledger.entered - ledger.left - ledger.drowned));
}

// What the climate knows of wind gives a rose: steady in the trades, turning with the seasons
// between the belts, and calm enough near 30° that little sand moves at all.
void checkRegime() {
    const WindRegime trades = regimeAtLatitude(14, -1, 0.4), westerlies = regimeAtLatitude(46, -1, 0.85), between = regimeAtLatitude(30, -1, 0.74);
    expect(trades.potential > 5 && trades.potential < 400, "the trades move no sand, or an absurd amount", trades.potential);
    expect(steadiness(trades) > 0.7 && trades.resultant.x < 0, "sand in the trades does not go west", steadiness(trades));
    expect(westerlies.resultant.x > 0, "sand in the westerlies does not go east", westerlies.resultant.x);
    expect(between.potential < 0.5 * trades.potential, "the calm between the belts moves as much sand as the trades", between.potential);
    expect(saturatedFlux(4.0) == 0 && saturatedFlux(12.0) > 4 * saturatedFlux(8.0), "sand flux does not rise steeply above a threshold", saturatedFlux(12.0));
    expect(dryShare(0.03, 22, 8) > 0.8 && dryShare(1.5, 10, 10) < 0.25 && dryShare(1.5, -4, 12) < dryShare(1.5, 10, 10), "wet or frozen ground blows like dry ground", dryShare(1.5, 10, 10));
}

} // namespace

void testAeolian() {
    checkRegime();
    checkOneWind();
    checkScarce();
    checkTwoWinds();
    checkEdges();
    testAeolianGround();
}
