#include "atmosphere/fn/Kernels.hpp"
#include "climate/constant/Moisture.hpp"
#include "core/service/Workers.hpp"
#include "planet/type/Planet.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double Era = 4.6e9;

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Haze: " << message << " (" << value << ")\n";
    std::exit(1);
}

Calendar onDay(double day) { return {Era, day * DaySeconds}; }

// Sea in the west; in the east a desert plain to the north of the equator's row band and green
// country south of it, both warm.
Planet hazePlanet() {
    Planet planet;
    planet.seed = 42;
    planet.time = Era;
    planet.seaLevel = 0;
    planet.surfaceWater = 1.2e18;
    planet.elevation = Field<float>(PlanetCells, PlanetCells);
    planet.precipitation = Field<float>(PlanetCells, PlanetCells, 0.8f);
    planet.temperature = Field<float>(PlanetCells, PlanetCells, 24.0f);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            const bool land = x >= PlanetCells / 2;
            planet.elevation.at(x, z) = land ? 150.0f : -3000.0f;
            if (land) planet.precipitation.at(x, z) = z < PlanetCells / 2 ? 0.02f : 2.0f;
        }
    return planet;
}

double sum(const Field<double>& field) {
    double all = 0;
    for (double value : field.cells) all += value;
    return all;
}

// Mean of a field over the cells of the eastern half in rows [from, to).
double eastMean(const Field<double>& field, int from, int to) {
    double all = 0;
    int count = 0;
    for (int z = from; z < to; ++z)
        for (int x = AirCells / 2 + 4; x < AirCells - 4; ++x) all += field.at(x, z), ++count;
    return all / count;
}

} // namespace

void testHaze() {
    Workers workers;
    const Atmosphere start = startAtmosphere(hazePlanet(), onDay(0), workers);
    const Haze& haze = start.haze;

    // Plants grow where the rain keeps up with the air.
    expect(eastMean(haze.plants, 4, AirCells / 2 - 4) < 0.05, "the desert has plants", eastMean(haze.plants, 4, AirCells / 2 - 4));
    expect(eastMean(haze.plants, AirCells / 2 + 4, AirCells - 4) > 0.5, "the green country has none", eastMean(haze.plants, AirCells / 2 + 4, AirCells - 4));

    // Every kilogram raised is aloft, washed out or settled.
    for (int kind = 0; kind < HazeKinds; ++kind) {
        const double aloft = sum(haze.mass[size_t(kind)]) * AirCellArea;
        const double emitted = haze.totals.emitted[size_t(kind)];
        expect(emitted > 0, "a kind was never raised", kind);
        expect(std::abs(emitted - aloft - haze.totals.washed[size_t(kind)] - haze.totals.settled[size_t(kind)]) < 1e-9 * emitted, "the haze ledger does not close", kind);
    }

    // One step from clean air: the sea gives salt and sulphate, the desert dust, the plants organics.
    Atmosphere clean = start;
    for (Field<double>& field : clean.haze.mass) std::fill(field.cells.begin(), field.cells.end(), 0.0);
    std::fill(clean.rainRate.cells.begin(), clean.rainRate.cells.end(), 0.0);
    std::fill(clean.snowRate.cells.begin(), clean.snowRate.cells.end(), 0.0);
    Atmosphere wet = clean;
    for (size_t cell = 0; cell < wet.soil.size(); ++cell) wet.soil.cells[cell] = SoilCapacity * (1.0 - wet.openWater.cells[cell]);
    stepHaze(clean, workers);
    double westSalt = 0, eastSalt = 0;
    for (int z = 0; z < AirCells; ++z)
        for (int x = 0; x < AirCells / 2; ++x) westSalt += clean.haze.mass[SeaSalt].at(x, z), eastSalt += clean.haze.mass[SeaSalt].at(x + AirCells / 2, z);
    expect(westSalt > 0 && eastSalt < 1e-3 * westSalt, "salt came from land", eastSalt / std::max(westSalt, 1e-30));
    const double desertDust = eastMean(clean.haze.mass[MineralDust], 4, AirCells / 2 - 4), greenDust = eastMean(clean.haze.mass[MineralDust], AirCells / 2 + 4, AirCells - 4);
    expect(desertDust > 20 * greenDust, "green country raises as much dust as desert", greenDust / std::max(desertDust, 1e-30));
    const double desertOrganic = eastMean(clean.haze.mass[Organic], 4, AirCells / 2 - 4), greenOrganic = eastMean(clean.haze.mass[Organic], AirCells / 2 + 4, AirCells - 4);
    expect(greenOrganic > 20 * desertOrganic, "the desert gives off as many organics as plants", desertOrganic / std::max(greenOrganic, 1e-30));

    // Wet ground holds its dust.
    stepHaze(wet, workers);
    const double wetDust = eastMean(wet.haze.mass[MineralDust], 4, AirCells / 2 - 4);
    expect(wetDust < 0.2 * desertDust, "soaked desert raises dust", wetDust / std::max(desertDust, 1e-30));

    // Rain takes the haze down with the low band's water.
    Atmosphere rained = start, dry = start;
    for (Atmosphere* air : {&rained, &dry}) {
        std::fill(air->snowRate.cells.begin(), air->snowRate.cells.end(), 0.0);
        std::fill(air->rainRate.cells.begin(), air->rainRate.cells.end(), 0.0);
    }
    for (size_t cell = 0; cell < rained.rainRate.size(); ++cell) {
        const double water = rained.vapour[Low].cells[cell] + rained.liquid[Low].cells[cell] + rained.ice[Low].cells[cell];
        rained.rainRate.cells[cell] = water / AirStep;   // as much as the low band holds
    }
    stepHaze(rained, workers);
    stepHaze(dry, workers);
    for (int kind = 0; kind < HazeKinds; ++kind)
        expect(sum(rained.haze.mass[size_t(kind)]) < 0.6 * sum(dry.haze.mass[size_t(kind)]), "rain left the haze", kind);

    // Damp air swells the salt.
    const int seaX = AirCells / 4, seaZ = AirCells / 4;
    Atmosphere damp = start, parched = start;
    damp.vapour[Low].at(seaX, seaZ) = 0.94 / profileOf(columnAt(damp, seaX, seaZ)).perCapacity[Low];
    parched.vapour[Low].at(seaX, seaZ) = 0.3 / profileOf(columnAt(parched, seaX, seaZ)).perCapacity[Low];
    const double swollen = hazeDepths(damp, seaX, seaZ)[SeaSalt], shrunk = hazeDepths(parched, seaX, seaZ)[SeaSalt];
    expect(swollen > 3 * shrunk, "salt does not take up water", swollen / std::max(shrunk, 1e-30));

    // The renderer gets the haze of the low band and the veil above it, and the view along the ground shortens
    // as the haze thickens.
    const CloudField field = cloudField(start);
    for (size_t cell = 0; cell < field.hazeDepth.size(); ++cell) {
        const double split = double(field.hazeDepth[cell]) + field.aerosolDepth[cell] - start.haze.depth.cells[cell] - start.plume.depth.cells[cell];
        expect(std::abs(split) < 1e-5 * (1 + start.haze.depth.cells[cell]), "the published haze and veil do not add up", split);
    }
    const Vec2 sea{(seaX + 0.5) * AirCell, (seaZ + 0.5) * AirCell};
    Atmosphere thick = start;
    thick.haze.depth.at(seaX, seaZ) = start.haze.depth.at(seaX, seaZ) + 1.0;
    expect(viewDistance(thick, sea, 0) < 0.5 * viewDistance(start, sea, 0), "haze does not shorten the view", viewDistance(thick, sea, 0));
    expect(viewDistance(thick, sea, LowDepth + 500) == viewDistance(start, sea, LowDepth + 500), "haze above its band", viewDistance(thick, sea, LowDepth + 500));

    // Clean air alone lets one see 337 km, and the result does not depend on the number of workers.
    expect(std::abs(visibilityOf(RayleighExtinction) - 337e3) < 1e3, "clean air view", visibilityOf(RayleighExtinction));
    Workers one(1);
    Atmosphere alone = start, together = start;
    advanceAtmosphere(alone, onDay(1), one);
    advanceAtmosphere(together, onDay(1), workers);
    for (int kind = 0; kind < HazeKinds; ++kind)
        expect(alone.haze.mass[size_t(kind)].cells == together.haze.mass[size_t(kind)].cells, "the haze depends on the workers", kind);
    expect(alone.haze.depth.cells == together.haze.depth.cells, "the haze's depth depends on the workers");
}
