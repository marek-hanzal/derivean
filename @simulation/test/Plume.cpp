#include "atmosphere/fn/Kernels.hpp"
#include "core/service/Workers.hpp"
#include "planet/type/Planet.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double Era = 4.6e9;

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Plume: " << message << " (" << value << ")\n";
    std::exit(1);
}

Calendar onDay(double day) { return {Era, day * DaySeconds}; }

// Sea in the west, a low plain in the east.
Planet plainPlanet() {
    Planet planet;
    planet.seed = 42;
    planet.time = Era;
    planet.seaLevel = 0;
    planet.surfaceWater = 1.2e18;
    planet.elevation = Field<float>(PlanetCells, PlanetCells);
    planet.precipitation = Field<float>(PlanetCells, PlanetCells, 0.8f);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) planet.elevation.at(x, z) = x < PlanetCells / 2 ? -3000.0f : 150.0f;
    return planet;
}

double apart(Vec2 a, Vec2 b) { return std::hypot(wrapDelta(a.x - b.x, PlanetSpan), wrapDelta(a.z - b.z, PlanetSpan)); }

double landMean(const Atmosphere& air, const AirField& field) {
    double sum = 0, land = 0;
    for (size_t cell = 0; cell < field.size(); ++cell) sum += (1 - air.openWater.cells[cell]) * field.cells[cell], land += 1 - air.openWater.cells[cell];
    return sum / land;
}

} // namespace

void testPlume() {
    // Two threads for the flat grid, as in testAtmosphere; the daily steps below take three.
    Workers workers(2);
    const Atmosphere clean = startAtmosphere(plainPlanet(), onDay(0), workers);
    int row = 0;
    for (int z = 0; z < AirCells / 2; ++z)
        if (std::abs(clean.latitude[size_t(z)] - 45) < std::abs(clean.latitude[size_t(row)] - 45)) row = z;
    const Vec2 vent{(AirCells * 0.7 + 0.3) * AirCell, (row + 0.4) * AirCell};

    // A sky nobody put anything into: a source that emits nothing changes nothing.
    Atmosphere control = clean, idle = clean;
    addSource(idle, {7, vent, 9000, 0, 5 * DaySeconds, {}, 0, 0, 0});
    advanceAtmosphere(control, onDay(3), workers);
    advanceAtmosphere(idle, onDay(3), workers);
    const PlumeLedger nothing = plumeLedger(idle);
    expect(control.airTemperature.cells == idle.airTemperature.cells && control.vapour[Low].cells == idle.vapour[Low].cells, "an empty source changed the weather");
    expect(nothing.ashEmitted == 0 && nothing.ashAloft == 0 && nothing.sulphurEmitted == 0 && ashFallAt(idle, vent).mass == 0 && landMean(idle, idle.plume.dimming) == 0, "ash or veil without a source");

    // Two days of eruption, reached in one jump and in daily steps: whole steps either way, so the
    // two states are the same to the last bit.
    const AirSource eruption{1, vent, 12000, 0.5 * DaySeconds, 2.5 * DaySeconds, {3e7, 6e7, 1e7}, 2e5, 1e6, 1e12};
    Atmosphere once = clean, daily = clean;
    addSource(once, eruption);
    addSource(daily, eruption);
    advanceAtmosphere(once, onDay(20), workers);
    Workers three(3);
    for (int day = 1; day <= 20; ++day) advanceAtmosphere(daily, onDay(day), three);
    bool same = once.plume.fallen.cells == daily.plume.fallen.cells && once.airTemperature.cells == daily.airTemperature.cells && once.plume.patches.size() == daily.plume.patches.size();
    for (size_t level = 0; level < PlumeLevels; ++level) same = same && once.plume.dust[level].cells == daily.plume.dust[level].cells && once.plume.veil[level].cells == daily.plume.veil[level].cells;
    expect(same, "one jump and daily steps on other threads gave different plumes");

    // Everything the source gave off is aloft or on the ground.
    const PlumeLedger ledger = plumeLedger(once);
    const double seconds = 2 * DaySeconds;
    expect(std::abs(ledger.ashEmitted - 1e8 * seconds) < 1e-9 * ledger.ashEmitted && std::abs(ledger.sulphurEmitted - 2e5 * seconds) < 1e-9 * ledger.sulphurEmitted, "the source did not emit its rates over its time", ledger.ashEmitted);
    expect(std::abs(ledger.ashAloft + ledger.ashFallen - ledger.ashEmitted) < 1e-9 * ledger.ashEmitted, "ash was lost or made", ledger.ashAloft + ledger.ashFallen - ledger.ashEmitted);
    expect(std::abs(ledger.sulphurGas + ledger.sulphurVeil + ledger.sulphurFallen - ledger.sulphurEmitted) < 1e-9 * ledger.sulphurEmitted, "sulphur was lost or made");
    expect(ledger.sulphurVeil > 0 && ledger.ashFallen > 0.9 * 9e7 * seconds && std::abs(ledger.vapourEmitted - 1e6 * seconds) < 1.0, "the plume's stores are not filled as expected", ledger.sulphurVeil);
    double patched = 0, dustDown = 0;
    for (const AshPatch& patch : once.plume.patches) patched += patch.mass;
    for (double fallen : once.plume.dustFallen.cells) dustDown += fallen * AirCellArea;
    expect(std::abs(patched + dustDown - ledger.ashFallen) < 1e-9 * ledger.ashFallen, "blankets and settled dust are not what fell", patched + dustDown - ledger.ashFallen);
    double booked = 0;
    for (double fallen : once.plume.fallen.cells) booked += fallen * AirCellArea;
    expect(std::abs(booked - ledger.ashFallen) < 1e-9 * ledger.ashFallen, "the cells were not booked what fell on them", booked - ledger.ashFallen);

    // Coarse ash falls first: lapilli nearest the vent, sand-size ash further, dust furthest, and the blanket thins outwards.
    double reach[AshClasses]{}, mass[AshClasses]{};
    Vec2 sandCentre{};
    for (const AshPatch& patch : once.plume.patches) {
        reach[patch.grain] += patch.mass * apart(patch.centre, vent), mass[patch.grain] += patch.mass;
        if (patch.grain == Grit) sandCentre = patch.centre;
    }
    for (int z = 0; z < AirCells; ++z)
        for (int x = 0; x < AirCells; ++x) reach[Dust] += once.plume.dustFallen.at(x, z) * apart({(x + 0.5) * AirCell, (z + 0.5) * AirCell}, vent), mass[Dust] += once.plume.dustFallen.at(x, z);
    for (int grain = 0; grain < AshClasses; ++grain) reach[grain] /= mass[grain];
    expect(reach[Lapilli] > 1000 && reach[Lapilli] < 0.3 * reach[Grit] && reach[Grit] < 1.0e6 && reach[Dust] > 2 * reach[Grit], "coarse ash did not fall before fine", reach[Grit]);
    const Vec2 heading = normalized({wrapDelta(sandCentre.x - vent.x, PlanetSpan), wrapDelta(sandCentre.z - vent.z, PlanetSpan)});
    const AshFall near = ashFallAt(once, vent + heading * reach[Lapilli]), further = ashFallAt(once, vent + heading * reach[Grit]), far = ashFallAt(once, vent + heading * (4 * reach[Grit]));
    expect(near.mass > 3 * further.mass && further.mass > 3 * far.mass, "the ash blanket does not thin downwind", further.mass);
    expect(ashFallAt(once, vent + heading * (-0.4 * PlanetSpan)).source == 0 || ashFallAt(once, vent + heading * (-0.4 * PlanetSpan)).mass >= 1e-6, "a place free of ash names a source");
    expect(near.source == 1 && near.since == eruption.start && near.mass > 1.0, "the blanket does not name its source", near.mass);
    const WeatherReport report = inspectWeather(once, vent + heading * reach[Lapilli], 150, onDay(20));
    expect(report.cause.find("ash from source 1") != std::string::npos && report.ash.mass == near.mass, "the inspector does not explain the ash");

    // A steady wind lays the blanket along itself, as far out as the fall takes.
    Atmosphere steady = clean;
    std::fill(steady.windX.cells.begin(), steady.windX.cells.end(), 12.0);
    std::fill(steady.windZ.cells.begin(), steady.windZ.cells.end(), 5.0);
    steady.season.drift.fill(0.0);
    addSource(steady, {2, vent, 1650, 0, 1e9, {1e6, 1e6, 0}, 0, 0, 0});
    stepPlume(steady, workers);
    expect(steady.plume.patches.size() == 2, "a steady source did not lay one blanket per grain", double(steady.plume.patches.size()));
    for (const AshPatch& patch : steady.plume.patches) {
        const Vec2 offset = patch.centre - vent;
        expect(std::abs(cross(offset, Vec2{12, 5})) < 1e-6 * length(offset) * 13 && dot(offset, Vec2{12, 5}) > 0, "the blanket is not laid along the wind");
        expect(std::abs(length(offset) - 13.0 * 1500 / AshSettling[size_t(patch.grain)]) < 1e-6 * length(offset), "the blanket is not where the fall ends", length(offset));
    }

    // A veil above the weather dims the sun and cools the ground under it.
    Atmosphere veiled = control;
    addSource(veiled, {3, vent, 30000, 3 * DaySeconds, 5 * DaySeconds, {}, 5e11 / (2 * DaySeconds), 0, 0});
    advanceAtmosphere(control, onDay(63), workers);
    advanceAtmosphere(veiled, onDay(63), workers);
    const double cooling = landMean(control, control.surfaceTemperature) - landMean(veiled, veiled.surfaceTemperature);
    expect(landMean(veiled, veiled.plume.depth) > 0.2 && landMean(veiled, veiled.plume.dimming) > 0.02 && landMean(veiled, veiled.plume.dimming) < 0.11, "the veil does not dim the sun as a veil does", landMean(veiled, veiled.plume.dimming));
    expect(cooling > 0.3 && cooling < 8, "the land under a heavy veil did not cool by a plausible amount", cooling);
    expect(plumeLedger(veiled).sulphurVeil > 0.5 * 5e11 && plumeLedger(control).sulphurEmitted == 0, "the veil did not form from the gas", plumeLedger(veiled).sulphurVeil);
}
