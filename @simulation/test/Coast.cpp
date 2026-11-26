#include "coast/fn/Coast.hpp"
#include "coast/fn/Scenario.hpp"
#include "coast/fn/Survey.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Coast: " << message << " (" << value << ")\n";
    std::exit(1);
}

// m³ of loose cover on every cell and of broken rock in every surf.
double looseIn(const Domain& domain) {
    double sum = 0;
    for (size_t i = 0; i < domain.cells(); ++i) {
        sum += double(domain.ground.sediment[Coarse][i]) + domain.ground.sediment[Sand][i] + domain.ground.sediment[Fines][i];
        if (domain.shore && !domain.shore->pending[Sand].empty()) sum += double(domain.shore->pending[Sand][i]) + domain.shore->pending[Coarse][i];
    }
    return sum * domain.cell * domain.cell;
}

// Waves alone on a scene; checks on the way out that nothing was made or lost.
Domain worked(const CoastSpec& spec, int steps) {
    Domain domain = coastDomain(spec);
    const double before = looseIn(domain);
    workCoast(domain, steps);
    const CoastLedger& ledger = domain.shore->ledger;
    const double gained = ledger.cut - ledger.dissolved + ledger.entered - ledger.left, scale = std::max({before, ledger.cut, ledger.entered, 1.0});
    expect(std::abs(looseIn(domain) - before - gained) < 2e-4 * scale, "the waves' ledger does not close", (looseIn(domain) - before - gained) / scale);
    // What the coast booked on the tile's own cells is what those cells gained and lost.
    double booked = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) booked += double(domain.deposited[domain.index(x, z)]) - domain.eroded[domain.index(x, z)];
    const double area = domain.cell * domain.cell, turnover = std::max(1.0, domain.ledger.eroded + domain.ledger.deposited);
    expect(std::abs(booked * area - (domain.ledger.deposited - domain.ledger.eroded)) < 1e-3 * turnover, "the tile ledger disagrees with the cells the waves changed",
           (booked * area - (domain.ledger.deposited - domain.ledger.eroded)) / turnover);
    return domain;
}

// m the faces of one half of a straight scene have gone back, summed over its cells: the width of
// every column that came down and the cut in those that stand. `jointed` picks the joint zones or the rock between.
double retreat(const Domain& domain, bool east, int jointed = -1) {
    double sum = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = east ? domain.size / 2 + 1 : 0; x < (east ? domain.size : domain.size / 2); ++x) {
            const size_t i = domain.index(x, z);
            if (jointed >= 0 && (domain.weakness[i] > 0) != (jointed > 0)) continue;
            sum += (domain.shore->cut[i] > 0 ? float(domain.cell) : 0.0f) + domain.shore->notch[i];
        }
    return sum;
}

// m, the height of the train travelling east that leaves cell `x` of the middle row of a flat
// sea `depth` deep at high water (its mean depth is SeaSwing less), under a storm wind of `storm`
// m/s from the west, with a swell of `swell` m arriving over the western rim; the last cell before
// the eastern rim for a negative `x`.
float flatSea(Level level, float depth, double storm, double swell, int x) {
    CoastSpec spec;
    spec.level = level, spec.windFrom = 270, spec.swell = swell;
    spec.wind = storm / StormGain - StormCalm;
    Domain domain = coastDomain(spec);
    for (size_t i = 0; i < domain.cells(); ++i) {
        domain.ground.rock[i] = -depth + SeaSwing - 1.0f;
        for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] = c == Sand ? 1.0f : 0.0f;
    }
    FlowScratch scratch;
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    Shore& shore = shoreOf(domain);
    readStanding(domain, shore), spreadWater(domain, shore), raiseWaves(domain, shore);
    return std::sqrt(shore.wave[domain.index(x < 0 ? domain.size - 2 : x, domain.size / 2) * WaveDirections + 0]);
}

// Wave growth against the fetch-limited forecast of the Shore Protection Manual (1984, eq. 3-39):
// 100 km of deep water under 15 m/s raise 2.3 m; 20 km of water 3.2 m deep under 20 m/s 0.83 m (a
// lake that shallow, as Balaton, sees storm waves of about a metre), and no more than the 0.90 m
// such water holds however long its fetch. A swell higher than that, broken to 1.6 m where it
// enters the shelf, loses height to the bed between the JONSWAP rates (dH/dx = −Cf·H/(2·(g·h)^1.5),
// Cf 0.038 to 0.09 m²/s³: 1.39 to 1.15 m after 1.3 km) and is worn down to what the water holds.
void waveGrowth() {
    const float open = flatSea(Level::Region, 200, 15, 0, 98);
    expect(open > 2.2f && open < 2.5f, "100 km of deep sea under 15 m/s do not raise 2.3 m", open);
    const float shallow = flatSea(Level::Local, 3.2f, 20, 0, -1);
    expect(shallow > 0.78f && shallow < 0.88f, "20 km of water 3.2 m deep under 20 m/s do not raise 0.83 m", shallow);
    const float midway = flatSea(Level::Local, 3.2f, 20, 3, 20), worn = flatSea(Level::Local, 3.2f, 20, 3, -1);
    expect(midway > 1.15f && midway < 1.4f, "the bed of a shallow shelf does not wear a swell at the rate it is measured to", midway);
    expect(worn > 0.88f && worn < 0.95f, "a swell over a shallow shelf is not worn down to what the depth holds", worn);
}

void rockCoasts() {
    CoastSpec spec;
    spec.rock = Material::Limestone, spec.other = Material::Chalk;
    spec.joints = 0.8, spec.windFrom = 30;
    spec.steps = 30;
    const Domain exposed = worked(spec, spec.steps);
    const ShoreGrid grid = shoreGridOf(exposed);
    const ShoreSurvey survey = surveyShore(grid);
    expect(survey.cliffHeight.count > 40 && survey.cliffHeight.p50 >= 10, "an exposed coast of weak rock grew no cliff", survey.cliffHeight.p50);
    expect(survey.benched > 5000 && survey.platformWidth.high >= 2 * grid.cell, "an exposed cliff has no platform at its foot", survey.platformWidth.high);
    // The weak half goes back faster than the strong one. Sound limestone withstands these storms;
    // only its joint zones are cut.
    expect(retreat(exposed, true) > 3 * retreat(exposed, false) && retreat(exposed, false) > 0, "chalk does not go back faster than limestone", retreat(exposed, true) / retreat(exposed, false));
    expect(retreat(exposed, false, 0) == 0, "storms of three metres cut sound limestone", retreat(exposed, false, 0));
    // A rock the storms cut between its joints too loses its joint zones faster than the rock
    // between them, which is thirteen times as wide.
    CoastSpec soft = spec;
    soft.rock = soft.other = Material::Shale;
    const Domain shale = worked(soft, soft.steps);
    expect(retreat(shale, false, 0) > 0 && retreat(shale, false, 1) * 13 > 2 * retreat(shale, false, 0), "waves do not take the joint zones of a rock first",
           retreat(shale, false, 1) / std::max(1.0, retreat(shale, false, 0)));

    // The same shore in the lee of its own land, with no sea beyond: the chop of a few kilometres
    // of water cannot touch the limestone, and takes of the chalk a small part of what the open sea did.
    spec.windFrom = 180, spec.swell = 0;
    const Domain sheltered = worked(spec, spec.steps);
    expect(retreat(sheltered, false) == 0, "a sheltered shore of limestone was cut", retreat(sheltered, false));
    expect(retreat(sheltered, true) < 0.25 * retreat(exposed, true), "sheltered chalk goes back like exposed chalk", retreat(sheltered, true) / retreat(exposed, true));
}

// Waves that reach a shore at half a right angle carry its sand along it, round the point where
// a bay opens and into the shelter behind: a hooked spit grows into the mouth of the bay.
void spit() {
    CoastSpec spec;
    spec.scene = CoastScene::Bay;
    spec.rock = spec.other = Material::Granite;
    spec.windFrom = 315, spec.sand = 6, spec.shelf = 6;
    spec.steps = 40;
    const Domain start = coastDomain(spec);
    const Domain bay = worked(spec, spec.steps);
    const int middle = bay.size / 2;
    int shoaled = 0, emerged = 0, farthest = 0;
    double sand = 0;
    for (int z = middle - 6; z < middle + 40; ++z)
        for (int x = middle - 34; x < middle + 34; ++x) {
            const size_t i = bay.index(x, z);
            const float was = start.ground.rock[i] + start.sedimentAt(i), now = bay.ground.rock[i] + bay.sedimentAt(i);
            if (was > -1.0f) continue;   // only what was open water of the bay
            sand += bay.ground.sediment[Sand][i];
            if (now > was + 1.5f) ++shoaled, farthest = std::max(farthest, x - (middle - 35));
            emerged += now >= 0.5f;
        }
    expect(sand > 100 && shoaled >= 20 && farthest >= 3, "oblique waves carried no sand into the mouth of the bay", sand);
    expect(emerged >= 1, "the sand in the mouth of the bay nowhere stands out of the water", emerged);
    // The same waves square to the shore move little of it along: only their trains at half a right
    // angle to either side, a quarter as strong, carry the western sand round the lip of the bay.
    spec.windFrom = 0;
    const Domain square = worked(spec, spec.steps);
    double across = 0;
    for (int z = middle - 6; z < middle + 40; ++z)
        for (int x = middle - 34; x < middle + 34; ++x)
            if (start.ground.rock[start.index(x, z)] + start.sedimentAt(start.index(x, z)) <= -1.0f) across += square.ground.sediment[Sand][square.index(x, z)];
    expect(across < 0.3 * sand, "waves square to a shore drift its sand along it", across / sand);
}

void lakeBeach() {
    CoastSpec spec;
    spec.scene = CoastScene::Lake, spec.level = Level::Site;
    spec.rock = Material::Granite;
    spec.sand = 1.0, spec.gravel = 0.6;
    spec.steps = 30;
    const Domain lake = worked(spec, spec.steps);
    // On the downwind shore the waves have sent sand down below the waterline and kept the gravel
    // at it; the upwind shore, which no wave reaches, lies as it was laid.
    const int middle = lake.size / 2;
    double waterline[2] = {}, below[2] = {}, calm[2] = {};
    for (int z = 1; z < lake.size - 1; ++z)
        for (int x = middle - 30; x <= middle + 30; ++x) {
            const size_t i = lake.index(x, z);
            const float water = lake.shore->still[i];
            const double coarse = lake.ground.sediment[Coarse][i], loose = coarse + lake.ground.sediment[Sand][i];
            if (water == NoWater || loose <= 0) continue;
            const float above = lake.ground.rock[i] + lake.sedimentAt(i) - water;
            if (above < -0.6f || above >= 0.4f) continue;
            double* into = z < middle ? calm : above >= -0.3f ? waterline : below;
            into[0] += coarse, into[1] += loose;
        }
    expect(waterline[1] > 0 && below[1] > 0 && calm[1] > 0, "a lake has no shore to measure", waterline[1]);
    expect(waterline[0] / waterline[1] > below[0] / below[1] + 0.03, "gravel does not lie above sand on a lake beach", waterline[0] / waterline[1] - below[0] / below[1]);
    expect(std::abs(calm[0] / calm[1] - 0.375) < 0.01, "the upwind shore of a lake was sorted by waves it does not have", calm[0] / calm[1]);
    float highest = 0;
    for (size_t i = 0; i < lake.cells() * WaveDirections; ++i) highest = std::max(highest, lake.shore->wave[i]);
    expect(highest > 0.01f && highest < 0.25f, "a lake 560 m across carries waves of the open sea", std::sqrt(highest));
}

// The whole step loop, rivers and slopes included, gives the same shore whatever pool its rows are spread over.
void threads() {
    CoastSpec spec;
    spec.rock = Material::Sandstone, spec.other = Material::Chalk;
    spec.windFrom = 40, spec.joints = 0.6, spec.river = 3e7;
    spec.steps = 6;
    const Domain alone = replayCoast(spec);
    Workers workers(4);
    const Domain pooled = replayCoast(spec, &workers);
    const auto same = [](const std::vector<float>& a, const std::vector<float>& b) { return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0; };
    bool identical = same(alone.ground.rock, pooled.ground.rock) && same(alone.shore->notch, pooled.shore->notch) && same(alone.shore->wave, pooled.shore->wave);
    for (int c = 0; c < SedimentClasses; ++c) identical = identical && same(alone.ground.sediment[c], pooled.ground.sediment[c]);
    expect(identical, "a shore depends on the number of threads");
    expect(alone.shore->ledger.cut > 0, "the full step loop cut no rock on an exposed chalk shore", alone.shore->ledger.cut);
}

} // namespace

void testCoastVoid();   // CoastVoid.cpp

void testCoast() {
    testCoastVoid();
    waveGrowth();
    rockCoasts();
    spit();
    lakeBeach();
    threads();
}
