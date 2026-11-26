#include "InterventionShared.hpp"

#include "hydrology/constant/Water.hpp"
#include "hydrology/fn/Ocean.hpp"

using namespace derivean::sim;

namespace {

// A finite ocean with a hole in its floor: the stock falls, what is lost is in the ledger, and
// seas behind sills are left behind with their water and their salt.
void testOcean(World& world) {
    const ScenarioReport report = runOceanDrain(world);
    expectClosed(report);
    const auto value = [&](size_t stage, const char* measure) { return valueOf(report, stage, measure); };
    const WorldBudget& start = report.stages[0].budget;
    const WorldBudget& shaft = report.stages[1].budget;
    const WorldBudget& breach = report.stages[2].budget;
    const WorldBudget& last = report.stages[3].budget;
    expectLive(start.escaped.water == 0 && start.ocean.water > 0.99 * world.planet.surfaceWater && start.ocean.water < 1.01 * world.planet.surfaceWater,
               "the ocean does not start as the planet's surface water", start.ocean.water / world.planet.surfaceWater);
    expectLive(std::abs(start.ocean.salt / start.ocean.water - world.ocean.salinity) < 1e-9 && world.ocean.salinity > 20 && world.ocean.salinity < 36, "the ocean's salinity is off", world.ocean.salinity);

    // A century through a mine shaft: the head hardly changes, so the loss is the orifice law.
    const double expected = OrificeCoefficient * 100.0 * std::sqrt(2 * Gravity * value(1, "head over the mouth")) * 100 * YearSeconds;
    expectLive(std::abs(shaft.escaped.water - expected) < 0.01 * expected, "the shaft does not drain by the orifice law", shaft.escaped.water / expected);
    expectLive(value(1, "sea level") < value(0, "sea level") - 0.05 && value(1, "sea level") > value(0, "sea level") - 1.0, "a century of the shaft did not lower the sea by decimetres");
    expectLive(std::abs(shaft.escaped.salt / shaft.escaped.water - world.ocean.salinity) < 1e-9, "water left without its salt");

    // The breach: the sea falls past sills, and what lies behind them stays.
    expectLive(breach.ocean.water < 0.8 * shaft.ocean.water && last.ocean.water < breach.ocean.water, "the breach does not drain the ocean");
    expectLive(value(2, "cut-off seas") >= 1 && breach.cutOff.water > 0 && last.cutOff.water >= breach.cutOff.water * (1 - 1e-12), "no sea was cut off behind a sill");
    expectLive(std::abs(last.cutOff.salt / last.cutOff.water - world.ocean.salinity) < 1e-9, "the cut-off seas changed their salinity");
    expectLive(value(3, "head over the mouth") < 1e-6 && last.ocean.water < 1e-3 * start.ocean.water, "the connected ocean did not drain to the opening", value(3, "head over the mouth"));
    expectLive(last.cutOff.water > 1e-4 * start.ocean.water, "nothing worth the name was left behind the sills", last.cutOff.water / start.ocean.water);
    // What depended on the water changes with it: the open water the air can draw on shrinks.
    expectLive(value(3, "open water cells") < 0.95 * value(0, "open water cells"), "the open water did not shrink");
    int open = 0;
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) open += openWaterAt(world.ocean, x, z);
    expectLive(open == int(value(3, "open water cells")), "the open-water accessor disagrees with the sea");

    // The same history replayed in one jump ends in the same sea.
    const double escaped = world.ocean.escaped.water, now = world.interventions.calendar.seconds;
    jumpTo(world, 0);
    expectLive(world.ocean.escaped.water == 0 && world.interventions.calendar.seconds == 0, "a jump back to the start left the sea drained");
    jumpTo(world, now);
    expectLive(std::abs(world.ocean.escaped.water - escaped) <= Closed * escaped, "the replayed sea differs", world.ocean.escaped.water - escaped);
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
    const StreamSite site = findStream(world);
    expectLive(site.found, "the world has no stream to work on");
    testCascade(world, site);
    testDiversion(world, site);
    testEruption(world, site);
}
