#include "intervention/fn/Scenario.hpp"

#include "hydrology/fn/Ocean.hpp"

namespace derivean::sim {

// The opening is made where the world ocean is deepest. First a shaft the size of a mine, which
// the sea hardly notices in a century; then the floor around it gives way.
ScenarioReport runOceanDrain(World& world) {
    ScenarioReport report;
    report.title = "Ocean: a passage opens from the deepest sea floor into the void";
    report.measures = {"sea level", "seas", "cut-off seas", "open water cells", "head over the mouth"};
    forgetInterventions(world);
    const Ocean& ocean = world.ocean;
    int32_t deepest = 0;
    for (size_t cell = 0; cell < ocean.grid.cells(); ++cell)
        if (ocean.grid.depth[cell] > ocean.grid.depth[size_t(deepest)]) deepest = int32_t(cell);
    if (ocean.grid.depth[size_t(deepest)] <= 0) {
        report.failure = "the planet has no sea";
        return report;
    }
    const Vec2 floor = planetCellCenter(deepest % PlanetCells, deepest / PlanetCells);
    const auto measure = [&](const std::string& label) {
        const WorldBudget budget = budgetOf(world);
        const std::vector<SeaBody> bodies = seaBodies(ocean);
        double wet = 0, cutOff = 0;
        for (const double depth : ocean.grid.depth) wet += depth > 0;
        for (const SeaBody& body : bodies) cutOff += !body.draining;
        const double mouth = ocean.openings.empty() ? 0.0 : double(ocean.grid.ground[size_t(ocean.openings.front().cell)]);
        report.stages.push_back({label, budget, {budget.seaLevel, double(bodies.size()), ocean.openings.empty() ? double(bodies.size()) - 1 : cutOff, wet,
                                                 ocean.openings.empty() ? 0.0 : std::max(0.0, budget.seaLevel - mouth)}});
    };
    measure("untouched ocean");
    intervene(world, voidOpening(floor, 100.0, -1e9));
    advance(world, 100 * YearSeconds);
    measure("100 yr through a 100 m2 shaft");
    intervene(world, voidOpening(floor, 1e6, -1e9));
    advance(world, 100 * YearSeconds);
    measure("100 yr more through a 1 km2 breach");
    advance(world, 9800 * YearSeconds);
    measure("10 000 yr after the first opening");
    return report;
}

} // namespace derivean::sim
