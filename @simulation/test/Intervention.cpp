#include "InterventionShared.hpp"

#include "hydrology/constant/Water.hpp"
#include "hydrology/fn/Ocean.hpp"
#include "landscape/fn/Forcing.hpp"
#include "SeaFixture.hpp"

#include <memory>

using namespace derivean::sim;

namespace {

// The planet's ocean is a finite stock in bodies that straits connect. A strait from the ocean to
// a dry depression above it moves nothing: water does not climb; nor does a strait within the
// ocean. The planet's sea fills everything under its level until S3e and so has no dry depression
// under it; a fixture on the planet grid carries the flow through the same edits and replay.
void testOcean(World& world) {
    const ScenarioReport report = runStrait(world);
    expectLive(report.failure.empty() && report.stages.size() == 3, "the strait scenario did not run", double(report.stages.size()));
    expectClosed(report);
    const auto value = [&](size_t stage, const char* measure) { return valueOf(report, stage, measure); };
    const WorldBudget& start = report.stages[0].budget;
    const WorldBudget& last = report.stages[2].budget;
    expectLive(start.ocean.water > 0.99 * world.planet.surfaceWater && start.ocean.water < 1.01 * world.planet.surfaceWater,
               "the ocean does not start as the planet's surface water", start.ocean.water / world.planet.surfaceWater);
    expectLive(std::abs(start.ocean.salt / start.ocean.water - world.ocean.salinity) < 1e-9 && world.ocean.salinity > 20 && world.ocean.salinity < 36, "the ocean's salinity is off", world.ocean.salinity);
    expectLive(value(0, "depression above the sea") > 0, "the depression does not lie above the sea", value(0, "depression above the sea"));
    expectLive(world.ocean.straits.size() == 2 && world.ocean.straits[0].a == world.ocean.anchor && world.ocean.straits[1].a == world.ocean.anchor, "the straits do not start at the ocean's anchor");
    expectLive(last.moved.water == 0 && value(2, "changes of the sea") == 0, "water ran up a strait or within one body", last.moved.water);
    expectLive(last.ocean.water == start.ocean.water && last.others.water == start.others.water && value(2, "sea level") == value(0, "sea level"), "the bodies changed");
    expectLive(value(2, "open water cells") == value(0, "open water cells"), "the open water changed");
    int open = 0;
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) open += openWaterAt(world.ocean, x, z);
    expectLive(open == int(value(2, "open water cells")), "the open-water accessor disagrees with the sea");

    // The same history replayed in one jump ends in the same sea.
    const std::vector<double> depth = world.ocean.grid.depth, salt = world.ocean.grid.salt;
    const double now = world.interventions.calendar.seconds;
    jumpTo(world, 0);
    // The straits were cut at the start, so they are cut again there.
    expectLive(world.interventions.calendar.seconds == 0 && world.ocean.straits.size() == 2 && world.ocean.changes == 0 && world.ocean.grid.depth == depth, "a jump back to the start did not rebuild the untouched sea");
    jumpTo(world, now);
    expectLive(world.ocean.straits.size() == 2 && world.ocean.grid.depth == depth && world.ocean.grid.salt == salt, "the replayed sea differs");
}

// A sea and a dry bowl below it on the planet grid, in the world: the strait edit lets the sea into
// the bowl, the budget closes, and a jump back and forward again replays the same sea.
void testStraitWorld(World& world) {
    forgetInterventions(world);
    Ocean fixture = sea::boxes(PlanetCells, PlanetCells, 100.0, {{20, 20, 60, 60, -1000.0, -20.0}, {100, 20, 110, 30, -300.0}}, PlanetCell);
    fixture.pristineLevel = -20.0;
    fixture.salinity = sea::Salinity;
    fixture.start = std::make_shared<const Ocean>(fixture);
    world.ocean = fixture;
    const auto at = [&](int x, int z) { return worldPoint(world.options.seed, planetCellCenter(x, z)); };
    intervene(world, strait(at(40, 40), at(105, 25), 1e7, -290.0));
    advance(world, YearSeconds);
    const WorldBudget budget = budgetOf(world);
    const size_t bowl = size_t(25 * PlanetCells + 105);
    expectLive(world.ocean.moved[0].water > 1e12 && world.ocean.grid.depth[bowl] > 0 && world.ocean.changes > 0, "the strait edit did not let the sea into the bowl", world.ocean.moved[0].water);
    expectLive(budget.others.water > 0 && waterImbalance(budget) <= Closed && saltImbalance(budget) <= Closed, "the strait lost water or salt in the world", saltImbalance(budget));
    const std::vector<double> depth = world.ocean.grid.depth, salt = world.ocean.grid.salt;
    const double now = world.interventions.calendar.seconds;
    jumpTo(world, 0);
    expectLive(world.ocean.straits.size() == 1 && world.ocean.changes == 0 && world.ocean.grid.depth == fixture.grid.depth, "a jump back did not rebuild the fixture's sea");
    jumpTo(world, now);
    expectLive(world.ocean.grid.depth == depth && world.ocean.grid.salt == salt, "the replayed fixture's sea differs");
    forgetInterventions(world);
}

// The cascade: an upper lake is let into a lower one, which reaches its sill and floods a third.
void testCascade(World& world, const StreamSite& site) {
    const ScenarioReport report = runLakeCascade(world, site);
    expectClosed(report);
    const auto value = [&](size_t stage, const char* measure) { return valueOf(report, stage, measure); };
    enum { Untouched, Sealed, Full, Cut, Later };
    expectLive(value(Full, "upper full") == 1 && value(Full, "upper volume") > 5000 && value(Full, "lower full") == 0, "the upper lake is not full over a lower lake with room");
    expectLive(value(Cut, "upper level") < value(Full, "upper level") - 1.0, "the cut did not lower the upper lake", value(Cut, "upper level") - value(Full, "upper level"));
    expectLive(value(Cut, "lower full") == 1 && std::abs(value(Cut, "lower level") - value(Cut, "upper level")) < 1e-6, "the lower lake did not rise to its sill and to the upper lake");
    expectLive(value(Cut, "third volume") > value(Full, "third volume") + 500, "the lower lake did not overflow into the third basin", value(Cut, "third volume") - value(Full, "third volume"));
    // Where the upper lake's water went: every cubic metre is in another basin, elsewhere on the
    // tiles, or in one of the flows across their edge.
    const WaterFlows& before = report.stages[Full].budget.flows;
    const WaterFlows& after = report.stages[Cut].budget.flows;
    const double net = (after.inflow.water - before.inflow.water) + (after.rain.water - before.rain.water) + (after.fromBody.water - before.fromBody.water) -
                       (after.evaporation.water - before.evaporation.water) - (after.outflow.water - before.outflow.water);
    double gained = 0;
    for (const char* store : {"upper volume", "lower volume", "third volume", "elsewhere"}) gained += value(Cut, store) - value(Full, store);
    expectLive(std::abs(gained - net) < 1e-6, "the water of the upper lake is not all found again", gained - net);
    // And directly: but for what the stream brought in the meantime, what the upper lake lost is
    // what the two basins below it gained.
    const double released = value(Full, "upper volume") - value(Cut, "upper volume");
    const double caught = value(Cut, "lower volume") - value(Full, "lower volume") + value(Cut, "third volume") - value(Full, "third volume");
    expectLive(released > 2000 && std::abs(caught - released) < 0.15 * released, "what the upper lake released is not in the basins below", caught / released);
    expectLive(std::abs(value(Later, "third level") - value(Later, "upper level")) < 1e-6 && value(Later, "upper level") > value(Cut, "upper level"), "the works did not end as one lake");
    expectLive(report.stages[Later].budget.dug > 1e4 && report.stages[Later].budget.filled > 1e3, "the works moved no ground");

    // The same cut followed wave by wave: the event solver keeps the budget and, once the water
    // has come to rest and time moves on, ends where the quiet solver ended.
    const double later = world.interventions.calendar.seconds, cut = world.interventions.history.back().time;
    jumpTo(world, 0);
    jumpTo(world, cut);
    for (int step = 0; step < 150; ++step) advance(world, 2.0);
    expectLive(world.interventions.overlay.sets.front().awake > 0, "five minutes after a dam is cut the water is already called calm");
    const WorldBudget waves = budgetOf(world);
    expectLive(waterImbalance(waves) <= Closed && saltImbalance(waves) <= Closed, "the event solver lost water or salt on the site", waterImbalance(waves));
    advance(world, later - world.interventions.calendar.seconds);
    const WorldBudget rest = budgetOf(world);
    expectLive(std::abs(rest.live.water - report.stages[Later].budget.live.water) < 0.01 * rest.live.water, "waves and quiet steps end in different lakes",
               rest.live.water / report.stages[Later].budget.live.water);
}

// The clock is driven from outside: paused, at a rate, or by a jump.
void testClock(World& world) {
    forgetInterventions(world);
    world.interventions.paused = true;
    tick(world, 5.0);
    expectLive(world.interventions.calendar.seconds == 0, "a paused clock moved");
    world.interventions.paused = false;
    world.interventions.rate = 3600.0;
    tick(world, 2.0);
    expectLive(world.interventions.calendar.seconds == 7200.0 && world.interventions.calendar.era == world.options.age, "the clock does not run at its rate from the world's era");
    forgetInterventions(world);
}

} // namespace

void testIntervention() {
    World world({42, 1.2e9, 0});
    testClock(world);
    testOcean(world);
    testStraitWorld(world);
    const StreamSite site = findStream(world);
    expectLive(site.found, "the world has no stream to work on");
    testCascade(world, site);
    testDiversion(world, site);
    testEruption(world, site);
}
