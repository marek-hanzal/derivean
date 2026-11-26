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
constexpr int Rounds = 6000;          // most a stage is relaxed for
constexpr float Settled = 0.25f;      // m a round may still change a stage that counts as steady

void relaxTo(IceGrid& grid, double cooling, Workers* workers) {
    grid.cooling = float(cooling);
    relaxIce(grid, workers, Rounds, Settled, true);
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

    ice.stages.resize(size_t(2 + colder));
    relaxTo(grid, lastMaximum, workers);
    ice.stages[IceLastMaximum] = stageOf(grid, planet);
    const std::vector<float> thickness = grid.thickness, sink = grid.sink;
    relaxTo(grid, 0.0, workers);
    ice.stages[IceNow] = stageOf(grid, planet);
    grid.thickness = thickness, grid.sink = sink;
    for (int k = 1; k <= colder; ++k) {
        relaxTo(grid, lastMaximum + (deepest - lastMaximum) * double(k) / double(colder), workers);
        ice.stages[size_t(IceLastMaximum + k)] = stageOf(grid, planet);
    }
    return ice;
}

} // namespace derivean::sim
