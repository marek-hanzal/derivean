#include "intervention/fn/Scenario.hpp"

#include "hydrology/fn/Ocean.hpp"
#include "landscape/fn/Forcing.hpp"

#include <cmath>

namespace derivean::sim {

// One strait runs from the ocean's anchor to the pit of the lowest closed depression whose floor
// lies above the sea, with its bed at that floor: the sea does not climb to it. Another runs from
// the anchor to the shallowest cell of the same ocean, a strait within one body: it moves nothing.
// Until the ocean is a connected body on the sphere (S3e) the planet's sea fills everything under
// its level, so a dry depression below the sea, the case where water does run, is the fixtures'.
ScenarioReport runStrait(World& world) {
    ScenarioReport report;
    report.title = "Ocean: straits from the deepest sea floor to a depression above the sea and within the ocean";
    report.measures = {"sea level", "seas", "moved between bodies", "open water cells", "depression above the sea", "changes of the sea"};
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
    const auto lakeOf = [&](int32_t cell) { return ocean.lakes.lakeOf[size_t(tree.basinOf[size_t(cell)])]; };
    int32_t shore = -1;
    for (int32_t cell = 0; cell < int32_t(ocean.grid.cells()); ++cell)
        if (cell != ocean.anchor && ocean.grid.depth[size_t(cell)] > 0 && lakeOf(cell) == lakeOf(ocean.anchor) &&
            (shore < 0 || ocean.grid.depth[size_t(cell)] < ocean.grid.depth[size_t(shore)]))
            shore = cell;
    if (shore < 0) {
        report.failure = "the ocean is a single cell";
        return report;
    }
    const auto measure = [&](const std::string& label) {
        const WorldBudget budget = budgetOf(world);
        double wet = 0;
        for (const double depth : ocean.grid.depth) wet += depth > 0;
        report.stages.push_back({label, budget, {budget.seaLevel, double(seaBodies(ocean).size()), budget.moved.water, wet, floor - budget.seaLevel, double(ocean.changes)}});
    };
    measure("untouched ocean");
    // Edits are made in the world; the world point over a planet cell's centre lands in that cell.
    const auto at = [&](int32_t cell) { return worldPoint(world.options.seed, planetCellCenter(cell % PlanetCells, cell / PlanetCells)); };
    intervene(world, strait(at(ocean.anchor), at(pit), 1e6, floor));
    intervene(world, strait(at(ocean.anchor), at(shore), 1e6, double(tree.height[size_t(ocean.anchor)])));
    advance(world, 100 * YearSeconds);
    measure("100 yr after the straits");
    advance(world, 9900 * YearSeconds);
    measure("10 000 yr after the straits");
    return report;
}

} // namespace derivean::sim
