#include "icesheet/fn/Continental.hpp"

#include "climate/type/Climate.hpp"
#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Dating.hpp"
#include "icesheet/fn/Flow.hpp"
#include "landscape/type/Level.hpp"
#include "planet/fn/Heat.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {
namespace {

constexpr double WidestGap = 1.5;     // °C at most between two stages
constexpr int FirstRounds = 1600;     // most the first stage is relaxed for at each resolution, from bare ground
constexpr int NextRounds = 800;       // and every colder one, from the stage before
constexpr float Settled = 2e-4f;      // share of the ice volume forty rounds may still move in a stage that counts as steady

// The same planet at twice the cell: the sheet finds its outline there four times as fast.
IceGrid halved(const IceGrid& fine) {
    IceGrid coarse;
    shapeIce(coarse, fine.width / 2, fine.height / 2, fine.cell * 2, fine.wraps);
    coarse.seaLevel = fine.seaLevel;
    for (int z = 0; z < coarse.height; ++z) {
        for (int x = 0; x < coarse.width; ++x) {
            const size_t to = coarse.index(x, z);
            for (int k = 0; k < 4; ++k) {
                const size_t from = fine.index(2 * x + (k & 1), 2 * z + (k >> 1));
                coarse.bed[to] += 0.25f * fine.bed[from], coarse.warmth[to] += 0.25f * fine.warmth[from], coarse.halfRange[to] += 0.25f * fine.halfRange[from];
                coarse.rain[to] += 0.25f * fine.rain[from], coarse.heat[to] += 0.25f * fine.heat[from];
            }
        }
    }
    return coarse;
}

// The first stage: its outline is found at twice the cell, then every cell starts from the ice
// surface found there, over its own ground.
void growFromBare(IceGrid& grid, double cooling, Workers* workers) {
    IceGrid coarse = halved(grid);
    coarse.cooling = grid.cooling = float(cooling);
    relaxIce(coarse, workers, FirstRounds, Settled, true);
    for (int z = 0; z < grid.height; ++z) {
        for (int x = 0; x < grid.width; ++x) {
            const size_t i = grid.index(x, z), from = coarse.index(x / 2, z / 2);
            grid.thickness[i] = coarse.thickness[from] <= 0 ? 0.0f : std::max(0.0f, coarse.bed[from] + coarse.thickness[from] - grid.bed[i]);
            grid.sink[i] = coarse.sink[from];
            grid.thaw[i] = coarse.thaw[from];
        }
    }
    relaxIce(grid, workers, FirstRounds, Settled, true);
}

void coolTo(IceGrid& grid, double cooling, Workers* workers) {
    grid.cooling = float(cooling);
    relaxIce(grid, workers, NextRounds, Settled, true);
}

} // namespace

IceGrid planetIce(const Planet& planet) {
    IceGrid grid;
    shapeIce(grid, PlanetCells, PlanetCells, PlanetCell, true);
    grid.seaLevel = float(planet.seaLevel);
    // The planet's temperature holds whatever the glacial cycle adds at its own date.
    const double glacial = glacialOffset(planet.seed, planet.time);
    for (int z = 0; z < PlanetCells; ++z) {
        const float halfRange = float(zonalClimate(planet.seed, planetCellCenter(0, z).z, planet.time).seasonal);
        for (int x = 0; x < PlanetCells; ++x) {
            const size_t i = grid.index(x, z);
            const float height = planet.elevation.at(x, z);
            grid.bed[i] = height;
            grid.warmth[i] = float(planet.temperature.at(x, z) - glacial + LapseRate * std::max(0.0, double(height) - planet.seaLevel));
            grid.halfRange[i] = halfRange;
            grid.rain[i] = planet.precipitation.at(x, z);
            grid.heat[i] = geothermAt(planet, planetCellCenter(x, z)).gradient;
        }
    }
    return grid;
}

IceStage stageOf(const IceGrid& grid, const Planet& planet) {
    IceStage stage;
    stage.cooling = grid.cooling;
    stage.thickness = Field<float>(grid.width, grid.height);
    stage.sink = Field<float>(grid.width, grid.height);
    size_t land = 0, covered = 0;
    for (size_t i = 0; i < grid.cells(); ++i) {
        const bool grounded = grid.thickness[i] >= LeastSheet && !afloat(grid, i);
        stage.thickness.cells[i] = grounded ? grid.thickness[i] : 0.0f;
        stage.sink.cells[i] = grid.sink[i];
        if (grounded) stage.volume += double(grid.thickness[i]) * grid.cell * grid.cell;
        if (planet.elevation.cells[i] <= planet.seaLevel) continue;
        ++land;
        covered += grounded;
    }
    stage.landShare = land ? double(covered) / double(land) : 0.0;
    return stage;
}

ContinentalIce solveContinental(const Planet& planet, double age, Workers* workers) {
    ContinentalIce ice;
    ice.present = lastTermination(age);
    IceGrid grid = planetIce(planet);
    ice.heat = Field<float>(grid.width, grid.height);
    ice.heat.cells = grid.heat;

    const double lastMaximum = maximumCooling(planet.seed, ice.present, ice.present);
    double deepest = lastMaximum;
    const Window window = windowAt(Level::Region, age);
    for (double time = window.start; time <= ice.present; time += RegionStep) deepest = std::min(deepest, maximumCooling(planet.seed, time, ice.present));
    const int colder = int(std::ceil((lastMaximum - deepest) / WidestGap - 1e-9));

    // Warmest first, each colder stage grown from the one before: the way a glaciation builds up.
    ice.stages.resize(size_t(2 + colder));
    growFromBare(grid, 0.0, workers);
    ice.stages[IceNow] = stageOf(grid, planet);
    for (int k = 0; k <= colder; ++k) {
        coolTo(grid, colder ? lastMaximum + (deepest - lastMaximum) * double(k) / double(colder) : lastMaximum, workers);
        ice.stages[size_t(IceLastMaximum + k)] = stageOf(grid, planet);
    }
    return ice;
}

} // namespace derivean::sim
