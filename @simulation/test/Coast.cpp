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
    // The weak half goes back faster than the strong one, and joint zones faster than the rock
    // between them, which is thirteen times as wide.
    expect(retreat(exposed, true) > 3 * retreat(exposed, false) && retreat(exposed, false) > 0, "chalk does not go back faster than limestone", retreat(exposed, true) / retreat(exposed, false));
    expect(retreat(exposed, false, 1) * 13 > 2 * retreat(exposed, false, 0), "waves do not take the joint zones of a rock first", retreat(exposed, false, 1) / retreat(exposed, false, 0));

    // The same shore in the lee of its own land, with no sea beyond: the chop of a few kilometres
    // of water cannot touch the limestone, and takes of the chalk a small part of what the open sea did.
    spec.windFrom = 180, spec.swell = 0;
    const Domain sheltered = worked(spec, spec.steps);
    expect(retreat(sheltered, false) == 0, "a sheltered shore of limestone was cut", retreat(sheltered, false));
    expect(retreat(sheltered, true) < 0.25 * retreat(exposed, true), "sheltered chalk goes back like exposed chalk", retreat(sheltered, true) / retreat(exposed, true));
    expect(surveyShore(shoreGridOf(sheltered)).platformWidth.high < grid.cell, "a sheltered shore grew a platform", surveyShore(shoreGridOf(sheltered)).platformWidth.high);
}

void spit() {
    CoastSpec spec;
    spec.scene = CoastScene::Bay;
    spec.rock = spec.other = Material::Granite;
    spec.windFrom = 285, spec.sand = 6, spec.shelf = 6;
    spec.steps = 40;
    const Domain start = coastDomain(spec);
    const Domain bay = worked(spec, spec.steps);
    // Sand that stands at high water or above where the bay's mouth was open water.
    const int middle = bay.size / 2;
    int raised = 0, farthest = 0;
    for (int z = middle - 4; z < middle + 30; ++z)
        for (int x = middle - 34; x < middle + 34; ++x) {
            const size_t i = bay.index(x, z);
            if (start.ground.rock[i] + start.sedimentAt(i) > -2.0f || bay.ground.rock[i] + bay.sedimentAt(i) < 0.5f || bay.ground.sediment[Sand][i] < 1.0f) continue;
            ++raised, farthest = std::max(farthest, x - (middle - 35));
        }
    expect(raised >= 6 && farthest >= 4, "oblique waves built no spit into the mouth of the bay", raised);
}

void lakeBeach() {
    CoastSpec spec;
    spec.scene = CoastScene::Lake, spec.level = Level::Site;
    spec.rock = Material::Granite;
    spec.sand = 1.0, spec.gravel = 0.6;
    spec.steps = 30;
    const Domain lake = worked(spec, spec.steps);
    // On the downwind shore the swash has taken gravel up the beach and left the sand below the waterline.
    const int middle = lake.size / 2;
    double beach[2] = {}, shallows[2] = {}, calm[2] = {};
    for (int z = 0; z < lake.size; ++z)
        for (int x = middle - 30; x <= middle + 30; ++x) {
            const size_t i = lake.index(x, z);
            const float water = lake.shore->still[i], coarse = lake.ground.sediment[Coarse][i], sand = lake.ground.sediment[Sand][i];
            if (water == NoWater || coarse + sand <= 0) continue;
            const float above = lake.ground.rock[i] + lake.sedimentAt(i) - water;
            double* into = z < middle ? (above > 0 && above < 0.5f ? calm : nullptr) : above > 0 && above < 0.5f ? beach : above > -0.6f && above <= 0 ? shallows : nullptr;
            if (into) into[0] += coarse, into[1] += coarse + sand;
        }
    expect(beach[1] > 0 && shallows[1] > 0, "a lake has no shore to measure", beach[1]);
    expect(beach[0] / beach[1] > shallows[0] / shallows[1] + 0.08, "gravel does not lie above sand on a lake beach", beach[0] / beach[1] - shallows[0] / shallows[1]);
    expect(std::abs(calm[0] / calm[1] - 0.375) < 0.02, "the upwind shore of a lake was sorted by waves it does not have", calm[0] / calm[1]);
    float highest = 0;
    for (size_t i = 0; i < lake.cells() * WaveDirections; ++i) highest = std::max(highest, lake.shore->wave[i]);
    expect(highest > 0.01f && highest < 0.25f, "a lake 560 m across carries waves of the open sea", std::sqrt(highest));
}

// The whole step loop, rivers and slopes included, gives the same shore whatever pool its rows are spread over.
void threads() {
    CoastSpec spec;
    spec.rock = Material::Sandstone, spec.other = Material::Chalk;
    spec.windFrom = 40, spec.sand = 2, spec.river = 3e7;
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

void testCoast() {
    rockCoasts();
    spit();
    lakeBeach();
    threads();
}
