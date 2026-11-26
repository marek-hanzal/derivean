#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"
#include "aeolian/fn/Wind.hpp"
#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Plane.hpp"
#include "planet/type/Planet.hpp"

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
    // An escarpment across the wind, too steep for sand to climb: what reaches the stall before it stays there.
    for (int z = 1; z < domain.size - 1; ++z)
        for (int x = 80; x < 83; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.rock[i] = domain.surface[i] = domain.level[i] = 700.0f;
        }
    const int row = domain.size / 2;
    const auto at = [&](int x) { return domain.index(x, row); };
    expect(mobileSand(domain, at(50)) == 2.0f && std::abs(mobileSand(domain, at(105)) - LagArmour * 2.0f / 0.5f) < 1e-5f, "the wind's share of a cover is wrong", mobileSand(domain, at(105)));
    expect(heldByWater(domain, at(145)) && !heldByWater(domain, at(50)), "water was not told from dry ground");

    // Sand on the ground and parked beside it: it changes only by what crosses the edge.
    const auto stock = [&] {
        double sum = 0;
        for (size_t i = 0; i < domain.cells(); ++i)
            if (domain.hold[i] == Free) sum += double(domain.ground.sediment[Sand][i]) + domain.aeolian.parked[i];
        return sum * domain.cell * domain.cell;
    };
    const double start = stock();
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
    double parked = 0;
    for (const float metres : domain.aeolian.parked) parked += metres;
    expect(parked > 0, "a step before the last did not park the wind's sand", parked);
    expect(std::abs(stock() - start - ledger.sandIn + ledger.sandOut) < 1e-4 * ledger.sandLifted, "parking made or lost sand", stock() - start);
    const double sand = ledger.sandIn + ledger.sandLifted - ledger.sandLaid - ledger.sandDrowned - ledger.sandOut;
    const double dust = ledger.dustRaised + ledger.dustIn - ledger.dustSettled - ledger.dustDrowned - ledger.dustOut;
    expect(ledger.sandLifted > 0 && std::abs(sand) < 1e-4 * ledger.sandLifted, "the sand ledger does not close", sand / ledger.sandLifted);
    expect(ledger.dustRaised > 0 && std::abs(dust) < 1e-4 * ledger.dustRaised, "the dust ledger does not close", dust / ledger.dustRaised);
}

// A hollow in dry country whose water never reaches its rim: what flows in leaves as vapour, and
// its salt lies level over the floor of the hollow and nowhere else, never faster than salt pans
// grow. One step is all the hollow has: the model lays the sediment of a whole step in it as if a
// lake stood to its rim, and it is full after that. A step shortened to `years` brings in salt
// enough to raise the floor faster than pans grow; when `capped`, the floor must stop at that pace.
void checkBasin(double years, bool capped) {
    PlaneSpec spec;
    spec.level = Level::Region;
    spec.slope = 0.002;
    spec.rain = 0.15;
    spec.rock = Material::Limestone;
    spec.steps = 1;
    Domain domain = planeDomain(spec);
    if (years > 0) domain.step = years, domain.endTime = domain.startTime + years;
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
    const auto top = [&](size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); };
    // The lake the salt lies in: its cells and the flooded ones they touch. Other pools are not its floor.
    std::vector<uint8_t> lake(domain.cells(), 0);
    std::vector<size_t> open;
    for (size_t i = 0; i < domain.cells(); ++i)
        if (labels.salt[i] > 0) lake[i] = 1, open.push_back(i);
    while (!open.empty()) {
        const size_t i = open.back();
        open.pop_back();
        for (int k = 0; k < 8; ++k) {
            const int nx = int(i % size_t(domain.size)) + NeighborX[k], nz = int(i / size_t(domain.size)) + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= domain.size || nz >= domain.size) continue;
            const size_t next = domain.index(nx, nz);
            if (!lake[next] && underFlood(domain, next)) lake[next] = 1, open.push_back(next);
        }
    }
    float thickest = 0, step = 0, floor = -1e30f, bare = 1e30f;
    int pan = 0, astray = 0;
    for (int z = 1; z < domain.size - 1; ++z)
        for (int x = 1; x < domain.size - 1; ++x) {
            const size_t i = domain.index(x, z);
            if (labels.salt[i] <= 0) {
                if (lake[i]) bare = std::min(bare, top(i));
                continue;
            }
            ++pan, thickest = std::max(thickest, labels.salt[i]), floor = std::max(floor, top(i));
            astray += std::hypot(x - middle, z - middle) > 90.0;
            // Salt cells that touch lie in one pool and share its floor.
            for (const size_t next : {domain.index(x + 1, z), domain.index(x, z + 1)})
                if (labels.salt[next] > 0) step = std::max(step, std::abs(top(next) - top(i)));
        }
    expect(pan >= 4 && astray == 0, "a drying basin left no salt on its floor, or left it outside", pan);
    expect(floor <= bare + 0.01f, "salt lies above lake floor it left bare", floor - bare);
    expect(thickest > 0.01f, "the crust is a film", thickest);
    const float most = float(EvaporiteRate * domain.step);
    expect(thickest <= most * 1.001f, "salt grew faster than salt pans do", thickest);
    expect(!capped || thickest >= 0.99f * most, "the floor of a short step did not reach the pans' pace", thickest / most);
    expect(step < 0.01f, "the salt floor is not level", step);
    expect(domain.aeolian.ledger.salt > 0 && domain.aeolian.ledger.salt <= domain.ledger.dissolved * 1.0001 + 1.0, "more salt was laid than rock gave to solution", domain.aeolian.ledger.salt);
}

// The dust of a world's deserts beyond a domain: none from a wet world, and from a world that is
// all desert some on every cell, never more than such ground raises.
void checkFarDust() {
    Domain domain = dryPlain();
    Planet planet;
    planet.seaLevel = 0;
    planet.elevation = Field<float>(PlanetCells, PlanetCells, 500.0f);
    planet.temperature = Field<float>(PlanetCells, PlanetCells, 25.0f);
    Backdrop backdrop;
    const Setting setting{7, 4.6e9, &planet, &backdrop};
    planet.precipitation = Field<float>(PlanetCells, PlanetCells, 2.0f);
    readFarDust(domain, setting);
    float most = 0;
    for (const float rate : domain.aeolian.farDust) most = std::max(most, rate);
    expect(domain.aeolian.farDust.size() == domain.cells() && most == 0, "a wet world raised dust", most);
    planet.precipitation = Field<float>(PlanetCells, PlanetCells, 0.0f);
    readFarDust(domain, setting);
    float least = 1e9f;
    for (const float rate : domain.aeolian.farDust) least = std::min(least, rate), most = std::max(most, rate);
    expect(least > 0 && most < 1e-4f, "far dust over a desert world is missing or more than deserts raise", least);
}

// Three pools at a Site: one fed by a desert wash, whose shallow rim is dry most of the year and
// whose deep middle keeps water below its spill; one on a river from wetter ground and one in a wet
// climate, both full to the spill.
void checkPools() {
    Domain domain = dryPlain();
    domain.key.level = Level::Site;
    const auto pool = [&](int cx, float discharge, float rain) {
        for (int z = cx - 2; z <= cx + 2; ++z)
            for (int x = cx - 2; x <= cx + 2; ++x) {
                const size_t i = domain.index(x, z);
                const bool middle = x == cx && z == cx;
                domain.surface[i] = domain.ground.rock[i] = middle ? 99.0f : 99.7f;
                domain.level[i] = 100.0f, domain.discharge[i] = discharge, domain.rain[i] = rain;
            }
    };
    pool(40, 1e3f, 0.05f);
    pool(80, 1e10f, 0.05f);
    pool(120, 1e3f, 1.5f);
    lowerEphemeralPools(domain);
    const auto depth = [&](int x, int z) { const size_t i = domain.index(x, z); return domain.level[i] - domain.surface[i]; };
    expect(depth(41, 40) == 0, "the rim of a desert pool stays wet all year", depth(41, 40));
    expect(depth(40, 40) > 0.3f && depth(40, 40) < 0.9f, "the middle of a desert pool does not keep what its wash leaves it", depth(40, 40));
    expect(std::abs(depth(81, 80) - 0.3f) < 1e-4f && std::abs(depth(121, 120) - 0.3f) < 1e-4f, "a pool on a perennial stream or in a wet climate fell", depth(81, 80));
}

} // namespace

void testAeolianGround() {
    checkBudget();
    checkBasin(0, false);
    checkBasin(2000, true);
    checkFarDust();
    checkPools();
}
