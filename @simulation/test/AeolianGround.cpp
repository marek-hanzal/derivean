#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"
#include "aeolian/fn/Wind.hpp"
#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Plane.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Aeolian: " << message << " (" << value << ")\n";
    std::exit(1);
}

// A level, dry Region domain under one steady wind towards +X.
Domain dryPlain() {
    Domain domain;
    domain.key = {Level::Region, 0, 0};
    domain.cell = specOf(Level::Region).cell, domain.step = specOf(Level::Region).step;
    domain.origin = {-Halo * domain.cell, -Halo * domain.cell};
    domain.endTime = 8 * domain.step;
    domain.seaLevel = -100;
    allocateBare(domain);
    domain.unit.assign(domain.cells(), UnitCache{-1e30f, 1e30f, traitsOf(Material::Sandstone)});
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.rock[i] = domain.ground.datum[i] = domain.surface[i] = 100.0f;
            domain.level[i] = 100.0f;
            domain.rain[i] = 0.05f, domain.warmthNow[i] = 20.0f;
            if (x == 0 || z == 0 || x == last || z == last) domain.hold[i] = Rim;
        }
    allocateWind(domain);
    const double heading = 0, speed = 10, share = 1;
    domain.aeolian.regime = regimeOf(1, &heading, &speed, &share);
    return domain;
}

void cover(Domain& domain, int fromX, int toX, float coarse, float sand, float fines) {
    for (int z = 1; z < domain.size - 1; ++z)
        for (int x = fromX; x < toX; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.sediment[Coarse][i] = coarse, domain.ground.sediment[Sand][i] = sand, domain.ground.sediment[Fines][i] = fines;
        }
}

// Clean sand blows away; sand among stones blows until the stones it leaves close the surface;
// sand under water stays; and the dust raised on the way settles downwind, thinner with distance.
void checkBudget() {
    Domain domain = dryPlain();
    FlowScratch scratch;
    cover(domain, 40, 60, 0.0f, 2.0f, 1.0f);      // sand with silt between the grains
    cover(domain, 100, 110, 0.5f, 2.0f, 0.0f);    // gravelly sand
    cover(domain, 140, 150, 0.0f, 2.0f, 0.0f);    // sand under a lake
    for (int z = 1; z < domain.size - 1; ++z)
        for (int x = 140; x < 150; ++x) domain.level[domain.index(x, z)] = 105.0f, domain.discharge[domain.index(x, z)] = 1e9f;
    const int row = domain.size / 2;
    const auto at = [&](int x) { return domain.index(x, row); };
    expect(mobileSand(domain, at(50)) == 2.0f && std::abs(mobileSand(domain, at(105)) - LagArmour * 2.0f / 0.5f) < 1e-5f, "the wind's share of a cover is wrong", mobileSand(domain, at(105)));
    expect(heldByWater(domain, at(145)) && !heldByWater(domain, at(50)), "water was not told from dry ground");

    blowLoose(domain, scratch);
    const WindLabels& labels = domain.aeolian.labels;
    expect(domain.ground.sediment[Sand][at(50)] < 0.01f, "clean dry sand in a strong wind stayed", domain.ground.sediment[Sand][at(50)]);
    expect(labels.lag[at(105)] > 0.99f * LagArmour && domain.ground.sediment[Sand][at(105)] > 1.5f, "a gravel lag did not stop the wind", domain.ground.sediment[Sand][at(105)]);
    expect(domain.ground.sediment[Sand][at(145)] >= 2.0f, "the wind took sand from under water", domain.ground.sediment[Sand][at(145)]);
    blowLoose(domain, scratch);
    expect(domain.ground.sediment[Sand][at(105)] > 1.5f, "an armoured surface was winnowed again", domain.ground.sediment[Sand][at(105)]);

    expect(labels.loess[at(20)] == 0, "dust settled upwind of its source", labels.loess[at(20)]);
    expect(labels.loess[at(70)] > labels.loess[at(130)] && labels.loess[at(130)] > labels.loess[at(250)] && labels.loess[at(250)] > 0,
           "dust does not thin out downwind of its source", labels.loess[at(130)]);
    expect(labels.loess[at(70)] > 0.005f && labels.loess[at(70)] < 1.0f, "the loess next to its source is not a mantle", labels.loess[at(70)]);

    const WindLedger& ledger = domain.aeolian.ledger;
    const double sand = ledger.sandIn + ledger.sandLifted - ledger.sandLaid - ledger.sandDrowned - ledger.sandOut;
    const double dust = ledger.dustRaised + ledger.dustIn - ledger.dustSettled - ledger.dustDrowned - ledger.dustOut;
    expect(ledger.sandLifted > 0 && std::abs(sand) < 1e-4 * ledger.sandLifted, "the sand ledger does not close", sand / ledger.sandLifted);
    expect(ledger.dustRaised > 0 && std::abs(dust) < 1e-4 * ledger.dustRaised, "the dust ledger does not close", dust / ledger.dustRaised);
}

// A hollow in dry country whose water never reaches its rim: what flows in leaves as vapour, and
// its salt lies level on the lowest ground and nowhere else. One step is all the hollow has: the
// model lays the sediment of a whole step in it as if a lake stood to its rim, and it is full after that.
void checkBasin() {
    PlaneSpec spec;
    spec.level = Level::Region;
    spec.slope = 0.002;
    spec.rain = 0.15;
    spec.rock = Material::Limestone;
    spec.steps = 1;
    Domain domain = planeDomain(spec);
    const int middle = domain.size / 2;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const double away = std::hypot(x - middle, z - middle) / 45.0;
            domain.ground.rock[i] -= float(160.0 * std::exp(-away * away));
            domain.ground.datum[i] = domain.trend[i] = domain.ground.rock[i];
            domain.warmth[i] += 15.0f;
        }
    domain.settledFrom = domain.settledTo = domain.ground;
    Planet planet;
    planet.seaLevel = domain.seaLevel, planet.time = domain.endTime;
    Backdrop backdrop;
    const Setting setting{spec.seed, domain.endTime, &planet, &backdrop};
    FlowScratch scratch;
    for (int step = 0; step < spec.steps; ++step) stepDomain(domain, scratch, setting);

    const WindLabels& labels = domain.aeolian.labels;
    float low = 1e9f, high = -1e9f, thickest = 0;
    int pan = 0, astray = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            if (labels.salt[i] <= 0) continue;
            const float top = domain.ground.rock[i] + domain.sedimentAt(i);
            ++pan, low = std::min(low, top), high = std::max(high, top), thickest = std::max(thickest, labels.salt[i]);
            astray += std::hypot(x - middle, z - middle) > 45.0;
        }
    expect(pan >= 4 && astray == 0, "a drying basin left no salt on its floor, or left it outside", pan);
    expect(thickest > 0.01f, "the crust is a film", thickest);
    expect(high - low < 0.5f, "the salt floor is not level", high - low);
    expect(domain.aeolian.ledger.salt > 0 && domain.aeolian.ledger.salt <= domain.ledger.dissolved * 1.0001 + 1.0, "more salt was laid than rock gave to solution", domain.aeolian.ledger.salt);
}

} // namespace

void testAeolianGround() {
    checkBudget();
    checkBasin();
}
