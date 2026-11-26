#include "chronology/type/Calendar.hpp"
#include "hydrology/fn/Water.hpp"
#include "hydrology/fn/Ocean.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Hydrology: " << message << " (" << value << ")\n";
    std::exit(1);
}

bool close(double a, double b, double scale) { return std::abs(a - b) <= 1e-9 * std::max(scale, 1e-30); }

// Lets time pass and integrates it at once, so that every call is a step of its own.
void drain(Ocean& ocean, double seconds) {
    drainOcean(ocean, seconds);
    settleOcean(ocean);
}

// A sea on a small wrapping grid: two bowls joined by a trench whose bed is the sill between them.
// The western bowl is the deeper one.
Ocean smallSea(double sill) {
    Ocean ocean;
    WaterGrid& grid = ocean.grid;
    shapeGrid(grid, 32, 16, 1000.0);
    grid.periodic = true;
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 32; ++x) {
            const double west = std::hypot(x - 8.0, z - 8.0), east = std::hypot(x - 24.0, z - 8.0);
            double height = std::min({-400.0 + 60.0 * west, -250.0 + 60.0 * east, 0.0});
            if (x >= 8 && x <= 24 && std::abs(z - 8) <= 1) height = std::min(height, sill);
            grid.ground[size_t(z) * 32 + size_t(x)] = float(height);
        }
    }
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        grid.depth[cell] = std::max(0.0, -20.0 - double(grid.ground[cell]));
        grid.salt[cell] = 30.0 * grid.depth[cell];
    }
    ocean.basins = buildBasins(grid);
    ocean.initial = stockOf(grid);
    WaterFlows unused;
    ocean.lakes = settle(grid, ocean.basins, {}, unused);
    return ocean;
}

} // namespace

void testSmallSea() {
    const int32_t deepest = 8 * 32 + 8;
    Ocean sea = smallSea(-120.0);
    expect(seaBodies(sea).size() == 1, "the two bowls do not start as one sea", double(seaBodies(sea).size()));
    expect(close(stockOf(sea.grid).water, sea.initial.water, sea.initial.water), "settling a level sea moved water");
    const double head = -20.0 - double(sea.grid.ground[size_t(deepest)]);
    sea.openings.push_back({deepest, 50.0, -1e9});
    drain(sea, 3600.0);
    const double expected = OrificeCoefficient * 50.0 * std::sqrt(2 * Gravity * head) * 3600.0;
    expect(std::abs(sea.escaped.water - expected) < 1e-3 * expected, "the opening does not follow the orifice law", sea.escaped.water / expected);

    // The sea falls until the sill parts it. The eastern bowl is cut off there and keeps its water
    // and its salt at full strength; the holed bowl goes on to run dry.
    Ocean stepped = smallSea(-120.0), once = smallSea(-120.0);
    stepped.openings.push_back({deepest, 2000.0, -1e9});
    once.openings = stepped.openings;
    int steps = 0;
    while (seaBodies(stepped).size() < 2 && steps < 2000) drain(stepped, 1800.0), ++steps;
    drain(stepped, 1800.0), ++steps;
    drain(once, steps * 1800.0);
    expect(std::abs(once.escaped.water - stepped.escaped.water) < 1e-9 * once.escaped.water, "draining in steps differs from one jump", once.escaped.water - stepped.escaped.water);
    std::vector<SeaBody> bodies = seaBodies(once);
    expect(bodies.size() == 2 && bodies[0].draining != bodies[1].draining, "the falling sea did not part at the sill", double(bodies.size()));
    const SeaBody kept = bodies[0].draining ? bodies[1] : bodies[0];
    expect(std::abs(kept.level + 120.0) < 1e-6, "the cut-off sea does not stand at its sill", kept.level);
    expect(std::abs(kept.stock.salt / kept.stock.water - 30.0) < 1e-9, "the cut-off sea changed its salinity");
    Tally left = stockOf(once.grid);
    expect(close(left.water + once.escaped.water, once.initial.water, once.initial.water), "sea water is not stock plus escaped");
    expect(close(left.salt + once.escaped.salt, once.initial.salt, once.initial.salt), "sea salt is not stock plus escaped");

    drain(once, 400 * DaySeconds);
    bodies = seaBodies(once);
    expect(bodies.size() == 1 && !bodies[0].draining && close(bodies[0].stock.water, kept.stock.water, kept.stock.water), "the holed bowl did not run dry beside the cut-off one", double(bodies.size()));
    left = stockOf(once.grid);
    expect(close(left.water + once.escaped.water, once.initial.water, once.initial.water), "the dry bowl broke the water budget");
    expect(close(left.salt + once.escaped.salt, once.initial.salt, once.initial.salt), "the dry bowl broke the salt budget");

    // Ticks are gathered, not integrated one by one: a frame's worth of drainage is less than a
    // large stock can resolve. An hour of ticks is an hour.
    Ocean ticked = smallSea(-120.0), hour = smallSea(-120.0);
    ticked.openings.push_back({deepest, 50.0, -1e9});
    hour.openings = ticked.openings;
    for (int tick = 0; tick < 3600 * 20; ++tick) drainOcean(ticked, 0.05);
    settleOcean(ticked);
    drain(hour, 3600.0);
    expect(std::abs(ticked.escaped.water - hour.escaped.water) < 1e-9 * hour.escaped.water, "ticks and an hour drain differently", ticked.escaped.water / hour.escaped.water);
    // Two openings in different places share the sea by turns, in rounds fixed in time: however
    // the days are fed in, the same turns are taken.
    Ocean both = smallSea(-120.0), walked = smallSea(-120.0);
    for (Ocean* sea : {&both, &walked}) {
        openVoid(*sea, {deepest, 800.0, -1e9});
        openVoid(*sea, {8 * 32 + 24, 800.0, -1e9});
    }
    drain(both, 20 * DaySeconds);
    for (int step = 0; step < 500; ++step) drainOcean(walked, 20 * DaySeconds / 500);
    settleOcean(walked);
    expect(both.escaped.water > 0 && std::abs(both.escaped.water - walked.escaped.water) < 1e-9 * both.escaped.water, "two openings depend on how time is fed in", walked.escaped.water / both.escaped.water);
    expect(close(stockOf(both.grid).water + both.escaped.water, both.initial.water, both.initial.water), "two openings broke the water budget");

    // With the trench as deep as the opening, nothing is cut off and the whole sea drains.
    Ocean whole = smallSea(-400.0);
    whole.openings.push_back({deepest, 2000.0, -1e9});
    drain(whole, 400 * DaySeconds);
    expect(stockOf(whole.grid).water < 1e-9 * whole.initial.water, "a sea without a sill did not drain completely", stockOf(whole.grid).water);
    expect(close(whole.escaped.water, whole.initial.water, whole.initial.water) && close(whole.escaped.salt, whole.initial.salt, whole.initial.salt), "the drained sea is not all in the void");
}
