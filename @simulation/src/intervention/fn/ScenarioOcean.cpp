#include "intervention/fn/Scenario.hpp"

#include "hydrology/fn/Ocean.hpp"
#include "landscape/fn/Forcing.hpp"

#include <cmath>

namespace derivean::sim {

// The strait runs from the ocean's anchor to the pit of the lowest closed depression whose floor
// lies above the sea, with its bed at that floor. The sea does not climb to it. Until the ocean is
// a connected body on the sphere (S3e) the planet's sea fills everything under its level, so a dry
// depression below the sea, the case where water does run, is the hydrology fixture's.
ScenarioReport runStrait(World& world) {
    ScenarioReport report;
    report.title = "Ocean: a strait cut from the deepest sea floor to the lowest depression above the sea";
    report.measures = {"sea level", "seas", "moved between bodies", "open water cells", "depression above the sea"};
    forgetInterventions(world);
    const Ocean& ocean = world.ocean;
    if (ocean.anchor < 0) {
        report.failure = "the planet has no sea";
        return report;
    }
    const BasinTree& tree = ocean.basins;
    int32_t pit = -1;
    for (size_t id = 1; id < tree.basins.size(); ++id) {
        const Basin& basin = tree.basins[id];
        if (basin.child >= 0 || basin.count == 0 || basin.base <= ocean.pristineLevel) continue;
        const int32_t cell = tree.members[size_t(basin.first)];
        if (ocean.grid.depth[size_t(cell)] <= 0 && (pit < 0 || tree.height[size_t(cell)] < tree.height[size_t(pit)])) pit = cell;
    }
    if (pit < 0) {
        report.failure = "no closed depression lies above the sea";
        return report;
    }
    const double floor = double(tree.height[size_t(pit)]);
    const auto measure = [&](const std::string& label) {
        const WorldBudget budget = budgetOf(world);
        double wet = 0;
        for (const double depth : ocean.grid.depth) wet += depth > 0;
        report.stages.push_back({label, budget, {budget.seaLevel, double(seaBodies(ocean).size()), budget.moved.water, wet, floor - budget.seaLevel}});
    };
    measure("untouched ocean");
    // Edits are made in the world; the world point over a planet cell's centre lands in that cell.
    const Vec2 from = worldPoint(world.options.seed, planetCellCenter(ocean.anchor % PlanetCells, ocean.anchor / PlanetCells));
    const Vec2 to = worldPoint(world.options.seed, planetCellCenter(pit % PlanetCells, pit / PlanetCells));
    intervene(world, strait(from, to, 1e6, floor));
    advance(world, 100 * YearSeconds);
    measure("100 yr after the strait");
    advance(world, 9900 * YearSeconds);
    measure("10 000 yr after the strait");
    return report;
}

} // namespace derivean::sim
