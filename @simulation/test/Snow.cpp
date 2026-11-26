#include "atmosphere/type/Atmosphere.hpp"
#include "core/service/Workers.hpp"
#include "planet/type/Planet.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double Era = 4.6e9;
constexpr int Massif = 96;   // first of the four air columns of the high ground

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Snow: " << message << " (" << value << ")\n";
    std::exit(1);
}

// Sea in the west, a plain in the east and a massif of 3 km on it.
Planet massifPlanet() {
    Planet planet;
    planet.seed = 42;
    planet.time = Era;
    planet.seaLevel = 0;
    planet.surfaceWater = 1.2e18;
    planet.elevation = Field<float>(PlanetCells, PlanetCells);
    planet.precipitation = Field<float>(PlanetCells, PlanetCells, 0.8f);
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x)
            planet.elevation.at(x, z) = x < PlanetCells / 2 ? -3000.0f : x >= 2 * Massif && x < 2 * Massif + 8 ? 3000.0f : 150.0f;
    return planet;
}

// Row nearest to a northern latitude on the near side of the pole.
int rowAt(const Atmosphere& air, double latitude) {
    int best = 0;
    for (int z = 0; z < AirCells / 2; ++z)
        if (std::abs(air.latitude[size_t(z)] - latitude) < std::abs(air.latitude[size_t(best)] - latitude)) best = z;
    return best;
}

} // namespace

void testSnow() {
    Workers workers;
    const Planet planet = massifPlanet();
    Atmosphere air = startAtmosphere(planet, {Era, 20 * DaySeconds}, workers);

    // Hypsometry: the rungs share out the land of a cell, and only where the cell has ground.
    for (int z = 0; z < AirCells; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            const SnowColumn& areas = air.snowArea.at(x, z);
            double sum = 0, weighed = 0;
            for (size_t rung = 0; rung < SnowRungs; ++rung) {
                expect(areas[rung] >= 0, "a rung stands for a negative share of its cell", areas[rung]);
                sum += areas[rung];
                weighed += areas[rung] * air.snowLadder.at(x, z)[rung];
            }
            const double land = 1 - air.openWater.at(x, z);
            expect(std::abs(sum - (land > 0 ? 1 : 0)) < 1e-12, "the rungs do not share out the land of their cell", sum);
            expect(std::abs(air.snow.at(x, z) - land * weighed) < 1e-9 * (1 + weighed), "the cell's snow is not its ladder weighed by area", air.snow.at(x, z));
            double height = 0;
            for (size_t rung = 0; rung < SnowRungs; ++rung) height += areas[rung] * SnowLadder[rung];
            // The massif keeps its ground high, also in its edge cells, and gives none to the plain beside it.
            if (x >= Massif && x < Massif + 4) expect(areas[0] + areas[1] + areas[2] + areas[3] + areas[4] == 0 && std::abs(height - 3000) < 400, "the massif's ladder is not at the massif's height", height);
            if (x == Massif - 1 || x == Massif + 4 || x == AirCells - 8) expect(areas[0] + areas[1] > 0.99 && height < 400, "the plain has high ground on its ladder", height);
        }
    }

    // Midwinter at 45° north: the plain is under snow, and there is more of it higher up.
    const int z = rowAt(air, 45), x = AirCells - 8;
    const Vec2 here{(x + 0.5) * AirCell, (z + 0.5) * AirCell};
    const SnowColumn& packs = air.snowLadder.at(x, z);
    expect(packs[0] > 5 && packs[8] > packs[0], "midwinter left the temperate plain bare or the heights with less snow", packs[0]);
    expect(std::abs(snowAt(air, here, planet.seaLevel + SnowLadder[3]) - packs[3]) < 1e-9, "snowAt at a cell centre on a rung is not that rung's pack");
    expect(std::abs(snowAt(air, here, planet.seaLevel + 0.5 * (SnowLadder[3] + SnowLadder[4])) - 0.5 * (packs[3] + packs[4])) < 1e-9, "snowAt is not linear between rungs");
    expect(snowAt(air, here, planet.seaLevel - 500) == packs[0] && snowAt(air, here, planet.seaLevel + 12000) == packs[SnowRungs - 1], "snowAt does not hold its end rungs beyond the ladder");
    expect(std::abs(weatherAt(air, here, planet.seaLevel + 2000, {Era, 20 * DaySeconds}).snowpack - packs[5]) < 1e-9, "the weather sample does not carry the pack of its ground");

    // No steps at cell borders, also across the planet's seam and along both axes.
    for (double height : {0.0, 700.0, 2600.0, 5200.0}) {
        for (double border : {double(x) * AirCell, 0.0, double(Massif) * AirCell}) {
            const double west = snowAt(air, {border - 1.0, here.z}, height), east = snowAt(air, {border + 1.0, here.z}, height);
            expect(std::abs(west - east) < 1e-3 * (1 + west), "snowAt jumps at a cell border", west - east);
        }
        const double north = snowAt(air, {here.x, z * AirCell - 1.0}, height), south = snowAt(air, {here.x, z * AirCell + 1.0}, height);
        expect(std::abs(north - south) < 1e-3 * (1 + north), "snowAt jumps at a cell border along Z", north - south);
    }

    // The snow line: where the packs above it thin out to nothing, and no snow below it.
    Atmosphere made = air;
    made.snowLadder.at(x, z) = {0, 0, 0, 10, 30, 60, 90, 120, 200, 300, 400, 500};
    expect(std::abs(snowLineOf(made, x, z) - 1000) < 1e-9, "the snow line is not where the packs above it run out", snowLineOf(made, x, z));
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) made.snowLadder.at(x + dx, z + dz) = made.snowLadder.at(x, z);
    expect(snowAt(made, here, 900) == 0 && std::abs(snowAt(made, here, 1100) - 5) < 1e-9 && std::abs(snowAt(made, here, 1400) - 20) < 1e-9, "snowAt does not start at the snow line", snowAt(made, here, 1100));

    // The renderer's copy is the same ladder.
    const CloudField field = cloudField(air);
    expect(field.snowLadder.size() == air.snow.size() * SnowRungs && field.snowLadder[air.snow.index(x, z) * SnowRungs + 8] == float(packs[8]) &&
           field.snowLine[air.snow.index(x, z)] == float(snowLineOf(air, x, z)) && field.seaLevel == planet.seaLevel, "the cloud field does not carry the ladder");

    // New heights re-weigh the rungs. Raising the land onto snowier rungs must not make snow, and
    // lowering it again gives the ocean what the cells then hold less; the ledger stays closed.
    const double water = total(waterLedger(air)), snowBefore = waterLedger(air).snow, oceanBefore = air.ocean;
    Field<float> raised = planet.elevation;
    for (float& height : raised.cells) height = height > 0 ? height + 900.0f : height;
    setSurface(air, raised, planet.seaLevel);
    expect(waterLedger(air).snow <= snowBefore * (1 + 1e-12) && air.ocean >= oceanBefore, "raising the land made snow at the ocean's expense", waterLedger(air).snow - snowBefore);
    expect(std::abs(total(waterLedger(air)) - water) < 1e-7 * (water - waterLedger(air).ocean), "re-weighing the snow broke the water ledger", total(waterLedger(air)) - water);
    Field<float> drowned(PlanetCells, PlanetCells, -100.0f);
    setSurface(air, drowned, planet.seaLevel);
    expect(waterLedger(air).snow == 0 && std::abs(total(waterLedger(air)) - water) < 1e-7 * (water - oceanBefore), "flooding the land lost its snow from the ledger", waterLedger(air).snow);
}
