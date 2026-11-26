#include "atmosphere/type/Cloud.hpp"
#include "atmosphere/type/Column.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace derivean::sim;

namespace {

void expect(bool passed, const std::string& message, double value = 0) {
    if (passed) return;
    std::cerr << "Clouds: " << message << " (" << value << ")\n";
    std::exit(1);
}

// A controlled column: one kind of air under one kind of forcing. The humidities are what the
// wind keeps supplying; the cloud is whatever the column's own physics makes of it.
struct Scene {
    double air, lapse, warmth, lift, glide;
    double humidity[AirBands];
    int hours;
    double climb = 0;         // m of terrain lift where the column stands
    double upwindClimb = 0;   // and where its air comes from
};

struct Outcome {
    AirColumn column;
    ColumnProfile profile;
    CloudReport clouds;
    double rain{};   // mm/h
};

Outcome play(const Scene& scene) {
    Outcome outcome;
    AirColumn& column = outcome.column;
    column.airTemperature = scene.air;
    column.upperTemperature = scene.air - scene.lapse * UpperLevel;
    column.surfaceWarmth = scene.warmth;
    column.lift = scene.lift;
    column.glide = scene.glide;
    column.climb = scene.climb;
    const ColumnRates rates = columnRates(3600.0);
    for (int hour = 0; hour < scene.hours; ++hour) {
        AirColumn upwind = column;
        upwind.climb = scene.upwindClimb;
        const ColumnProfile supply = profileOf(upwind);
        for (size_t band = 0; band < AirBands; ++band) column.vapour[band] = std::max(column.vapour[band], scene.humidity[band] * supply.capacity[band]);
        outcome.rain = stepColumn(column, profileOf(column), rates).precipitation;
    }
    outcome.profile = profileOf(column);
    outcome.clouds = describeClouds(column, outcome.profile, outcome.rain / 3600.0);
    return outcome;
}

double water(const AirColumn& column) {
    double sum = 0;
    for (size_t band = 0; band < AirBands; ++band) sum += column.vapour[band] + column.liquid[band] + column.ice[band] + column.falling[band];
    return sum;
}

void expectGenus(const char* scene, const Outcome& outcome, Genus genus) {
    expect(outcome.clouds.genus == genus, std::string(scene) + " gave " + genusName(outcome.clouds.genus) + " instead of " + genusName(genus));
    expect(outcome.clouds.share[size_t(genus)] > 0.1, std::string(scene) + " covers too little sky", outcome.clouds.share[size_t(genus)]);
    expect(!explainClouds(outcome.column, outcome.profile, outcome.clouds).empty(), "no explanation");
}

// One scene per genus of the WMO atlas, each with the base and top its kind of cloud has.
void testGenera() {
    const Outcome cumulus = play({22, 4.5e-3, 2.0, 0, 0, {0.82, 0.25, 0.2}, 4});
    expectGenus("fair-weather heating under a stable layer", cumulus, Genus::Cumulus);
    expect(cumulus.clouds.layer[Low].base > 400 && cumulus.clouds.layer[Low].base < 1500 && cumulus.clouds.convectiveTop <= 2500, "cumulus is not a low heap", cumulus.clouds.layer[Low].base);
    expect(cumulus.clouds.layer[Middle].cover < 0.05 && cumulus.rain < 0.1, "fair-weather cumulus reaches the middle band or rains");

    const Outcome storm = play({28, 6.5e-3, 2.0, 100, 0, {0.88, 0.7, 0.5}, 5});
    expectGenus("deep moist instability", storm, Genus::Cumulonimbus);
    expect(storm.clouds.layer[Low].base < 1000 && storm.clouds.convectiveTop > 10000, "the storm does not span the troposphere", storm.clouds.convectiveTop);
    expect(storm.column.ice[High] > 0.005 && storm.rain > 1.0 && storm.clouds.layer[High].genus == Genus::Cumulonimbus, "the storm has no ice anvil or no rain", storm.rain);

    const Outcome stratus = play({4, 4.0e-3, -1.0, 0, 0, {0.9, 0.3, 0.2}, 4});
    expectGenus("moist still air over a cool surface", stratus, Genus::Stratus);
    expect(stratus.clouds.layer[Low].base < 700 && stratus.clouds.layer[Low].top <= 2000, "stratus is not a low sheet", stratus.clouds.layer[Low].base);

    const Outcome fog = play({2, 4.0e-3, -4.0, 0, 0, {0.97, 0.3, 0.2}, 2});
    expectGenus("a cold night over moist ground", fog, Genus::Stratus);
    expect(fog.clouds.layer[Low].base == 0, "the night's stratus does not touch the ground", fog.clouds.layer[Low].base);

    const Outcome rolls = play({12, 3.0e-3, 0.5, 0, 0, {0.9, 0.3, 0.2}, 4});
    expectGenus("stirred air under a lid", rolls, Genus::Stratocumulus);
    expect(rolls.clouds.layer[Low].top <= 2000 && rolls.clouds.convectiveTop == 0, "stratocumulus grew through its lid");

    const Outcome rainLayer = play({8, 5.5e-3, -1.5, 500, 600, {0.95, 0.95, 0.8}, 5});
    expectGenus("deep steady lifting", rainLayer, Genus::Nimbostratus);
    expect(rainLayer.rain > 0.5 && rainLayer.clouds.layer[Middle].base <= 2000 && rainLayer.clouds.layer[Middle].top > 4000, "nimbostratus is not a thick raining layer", rainLayer.rain);

    const Outcome cells = play({15, 6.0e-3, -2.5, 0, 0, {0.5, 0.87, 0.3}, 2});
    expectGenus("a thin moist layer aloft", cells, Genus::Altocumulus);
    const Outcome sheet = play({5, 6.0e-3, -2.5, 0, 300, {0.5, 0.88, 0.4}, 3});
    expectGenus("gentle upglide at mid-level", sheet, Genus::Altostratus);
    for (const Outcome* outcome : {&cells, &sheet})
        expect(outcome->clouds.layer[Middle].base >= 2000 && outcome->clouds.layer[Middle].top < 8000 && outcome->clouds.layer[Low].cover < 0.05, "middle cloud is out of its band", outcome->clouds.layer[Middle].base);
    expect(sheet.column.ice[Middle] > sheet.column.liquid[Middle] && cells.column.liquid[Middle] > 0, "altostratus is not icy or altocumulus has no droplets");

    const Outcome streaks = play({15, 6.5e-3, -2.5, 0, 0, {0.4, 0.3, 0.88}, 3});
    expectGenus("thin moist air near the tropopause", streaks, Genus::Cirrus);
    const Outcome veil = play({15, 6.5e-3, -2.5, 0, 800, {0.4, 0.5, 0.97}, 5});
    expectGenus("broad upglide near the tropopause", veil, Genus::Cirrostratus);
    const Outcome ripples = play({15, 6.5e-3, -2.5, 0, 150, {0.4, 0.3, 0.95}, 1});
    expectGenus("cloud that formed aloft within the hour", ripples, Genus::Cirrocumulus);
    for (const Outcome* outcome : {&streaks, &veil, &ripples}) {
        expect(outcome->clouds.layer[High].base > 6000 && outcome->clouds.layer[High].top < 13500, "high cloud is out of its band", outcome->clouds.layer[High].base);
        expect(outcome->profile.celsius[High] < -20, "high cloud is not in freezing air");
    }
    expect(veil.clouds.layer[High].cover > 0.9 && streaks.clouds.layer[High].cover < 0.6, "cirrostratus is not a veil or cirrus is one");
    expect(ripples.column.liquid[High] > 0 && veil.column.ice[High] > 5 * veil.column.liquid[High], "ripples lack droplets or the veil is not ice");

    // Cloud heights follow the column: the same high cloud sits lower in cold air.
    const Outcome polar = play({-12, 6.5e-3, -1.0, 0, 800, {0.4, 0.5, 0.97}, 5});
    expect(polar.clouds.layer[High].cover > 0.5 && polar.clouds.layer[High].base < veil.clouds.layer[High].base - 2000, "high cloud does not come down in cold air", polar.clouds.layer[High].base);
}

void testTransitions() {
    // The same heated moist air under ever steeper lapse rates: cumulus grows into cumulonimbus by degrees.
    double towering = 0, top = 0, development = 0;
    Genus first = Genus::Cumulus, last = Genus::Cumulus;
    for (int stage = 0; stage <= 10; ++stage) {
        const Outcome outcome = play({24, 4.2e-3 + 0.25e-3 * stage, 2.0, 0, 0, {0.85, 0.6, 0.4}, 5});
        const double share = outcome.clouds.share[size_t(Genus::Cumulonimbus)];
        expect(share >= towering - 0.02 && outcome.profile.convection >= development - 1e-12, "more instability gave less of a storm", share);
        towering = share;
        top = outcome.clouds.convectiveTop;
        development = outcome.profile.convection;
        if (stage == 0) first = outcome.clouds.genus;
        last = outcome.clouds.genus;
    }
    expect(first == Genus::Cumulus && last == Genus::Cumulonimbus && top > 10000, "cumulus did not grow into cumulonimbus", top);

    // Layers of different kinds over one another.
    const Outcome stacked = play({10, 4.0e-3, -1.0, 0, 0, {0.9, 0.3, 0.9}, 3});
    expect(stacked.clouds.layer[Low].cover > 0.3 && stacked.clouds.layer[High].cover > 0.3 && stacked.clouds.layer[Middle].cover < 0.05, "low and high cloud do not coexist");
    expect(stacked.clouds.layer[Low].genus == Genus::Stratus && stacked.clouds.layer[High].genus == Genus::Cirrus, "the stacked layers are misnamed");
    expect(stacked.clouds.layer[High].base > stacked.clouds.layer[Low].top + 3000, "the layers are not apart");

    // Terrain: the same moist air holds less after it climbed, so it clouds over and rains there.
    const Outcome plain = play({14, 5.5e-3, -1.0, 0, 0, {0.84, 0.4, 0.3}, 3});
    const Outcome slope = play({14, 5.5e-3, -1.0, 0, 0, {0.84, 0.4, 0.3}, 3, 250.0, 0.0});
    expect(plain.clouds.cover < 0.05 && slope.clouds.layer[Low].cover > 0.3, "air that climbed terrain did not cloud over", slope.clouds.layer[Low].cover);

    // Night and day over the same moist land: fog under the cold ground, heaps under the sun.
    const Outcome night = play({12, 5.5e-3, -4.0, 0, 0, {0.84, 0.3, 0.2}, 2});
    const Outcome day = play({12, 5.5e-3, 3.0, 0, 0, {0.84, 0.3, 0.2}, 2});
    expect(night.clouds.genus == Genus::Stratus && night.profile.convection == 0 && day.profile.convection > 0.3, "the daily cycle does not switch between layer and heap");
    expect(day.clouds.layer[Low].base > night.clouds.layer[Low].base + 300, "cloud base does not rise with the day's heating", day.clouds.layer[Low].base);
}

// Whatever a column does, its water only leaves as precipitation.
void testColumnWater() {
    const Scene scenes[] = {{28, 6.5e-3, 2.0, 100, 0, {0.88, 0.7, 0.5}, 1}, {8, 5.5e-3, -1.5, 500, 600, {0.95, 0.95, 0.8}, 1},
                            {-15, 6.0e-3, -2.0, -300, 400, {0.9, 0.9, 0.9}, 1}};
    const ColumnRates rates = columnRates(3600.0);
    for (const Scene& scene : scenes) {
        AirColumn column = play(scene).column;
        const double start = water(column);
        double fallen = 0;
        for (int hour = 0; hour < 72; ++hour) {
            fallen += stepColumn(column, profileOf(column), rates).precipitation;
            for (size_t band = 0; band < AirBands; ++band)
                expect(column.vapour[band] >= 0 && column.liquid[band] >= 0 && column.ice[band] >= 0 && column.falling[band] >= 0, "a column store went negative");
        }
        expect(std::abs(water(column) + fallen - start) < 1e-12 * start, "a column lost or made water", water(column) + fallen - start);
        expect(fallen > 0, "a saturated column never precipitated");
    }
}

} // namespace

void testClouds() {
    testGenera();
    testTransitions();
    testColumnWater();
}
