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

// A valley falling east with three hollows in a row, each behind a sill lower than the last, and a
// river entering above the first. Cells are 4 m; the outer ring is the rim.
constexpr int Wide = 72, Deep = 24;
constexpr int PitX[3] = {14, 34, 54};

WaterGrid cascadeGrid(double riverPerYear) {
    WaterGrid grid;
    shapeGrid(grid, Wide, Deep, 4.0);
    for (int z = 0; z < Deep; ++z) {
        for (int x = 0; x < Wide; ++x) {
            const size_t cell = size_t(z) * Wide + size_t(x);
            double height = 40.0 - 0.1 * x + 0.5 * std::abs(z - Deep / 2);
            for (const int centre : PitX) {
                const double away = std::hypot(double(x - centre), double(z - Deep / 2));
                height -= 3.0 * std::max(0.0, 1.0 - away / 7.0);
            }
            grid.solid.rock[cell] = grid.ground[cell] = grid.held[cell] = float(height);
            if (x == 0 || z == 0 || x == Wide - 1 || z == Deep - 1) grid.kind[cell] = uint8_t(CellKind::Rim);
        }
    }
    grid.sources.push_back({int32_t(size_t(Deep / 2) * Wide + 3), riverPerYear, 0.0});
    grid.sourceSalinity = RiverSalinity;
    return grid;
}

double balanceOf(const WaterFlows& flows) {
    return flows.inflow.water + flows.rain.water + flows.fromBody.water - flows.evaporation.water - flows.outflow.water;
}

double saltBalanceOf(const WaterFlows& flows) {
    return flows.inflow.salt + flows.rain.salt + flows.fromBody.salt - flows.evaporation.salt - flows.outflow.salt;
}

double levelAt(const WaterGrid& grid, int pit) {
    const size_t cell = size_t(Deep / 2) * Wide + size_t(PitX[pit]);
    return double(grid.ground[cell]) + grid.depth[cell];
}

// The same grid after a run long enough to fill every hollow.
WaterGrid full(WaterGrid grid, const BasinTree& tree) {
    WaterFlows spare;
    fillAndSpill(grid, tree, 50.0, 0.0, spare);
    return grid;
}

void testCascade() {
    WaterGrid grid = cascadeGrid(4000.0);
    const BasinTree tree = buildBasins(grid);
    WaterFlows flows;
    // Sills: the level each hollow overflows at, read off a run long enough to fill all three.
    WaterGrid full = grid;
    WaterFlows spare;
    fillAndSpill(full, tree, 50.0, 0.0, spare);
    const double sill[3] = {levelAt(full, 0), levelAt(full, 1), levelAt(full, 2)};
    expect(sill[0] > sill[1] && sill[1] > sill[2], "the three sills do not step down");
    expect(spare.outflow.water > 0, "a full cascade does not overflow across the rim");

    // The same river in small steps: the hollows fill strictly one after the other.
    bool secondStarted = false, thirdStarted = false;
    for (int step = 0; step < 200; ++step) {
        fillAndSpill(grid, tree, 0.05, 0.0, flows);
        const Tally stock = stockOf(grid);
        expect(close(stock.water, balanceOf(flows), flows.inflow.water), "water is lost or made while lakes fill", stock.water - balanceOf(flows));
        expect(close(stock.salt, saltBalanceOf(flows), flows.inflow.salt), "salt is lost or made while lakes fill", stock.salt - saltBalanceOf(flows));
        const bool second = grid.depth[size_t(Deep / 2) * Wide + size_t(PitX[1])] > 0, third = grid.depth[size_t(Deep / 2) * Wide + size_t(PitX[2])] > 0;
        if (second && !secondStarted) expect(std::abs(levelAt(grid, 0) - sill[0]) < 1e-6, "the second hollow fills before the first is full", levelAt(grid, 0));
        if (third && !thirdStarted) expect(std::abs(levelAt(grid, 1) - sill[1]) < 1e-6, "the third hollow fills before the second is full", levelAt(grid, 1));
        secondStarted = second, thirdStarted = third;
    }
    expect(thirdStarted && flows.outflow.water > 0, "the cascade never reached the rim");
    // One long step and many short ones end in the same lakes.
    for (size_t cell = 0; cell < grid.cells(); ++cell)
        expect(std::abs(grid.depth[cell] - full.depth[cell]) < 1e-6, "a jump and small steps disagree on a depth", grid.depth[cell] - full.depth[cell]);

    // Cutting the first sill joins the two upper hollows: the first drains into the second, and
    // since the second was full, the water goes on into the third and across the rim.
    const double before[3] = {levelAt(grid, 0), levelAt(grid, 1), levelAt(grid, 2)};
    const Tally stood = stockOf(grid);
    for (int x = PitX[0]; x < PitX[1]; ++x) {
        const size_t cell = size_t(Deep / 2) * Wide + size_t(x);
        const float bed = float(sill[1] - 0.5);
        if (grid.ground[cell] > bed) grid.solid.rock[cell] = grid.ground[cell] = bed;
    }
    const BasinTree cut = buildBasins(grid);
    WaterFlows after;
    settle(grid, cut, {}, after);
    expect(levelAt(grid, 0) < before[0] - 0.5 && std::abs(levelAt(grid, 0) - levelAt(grid, 1)) < 1e-6, "the cut did not drain the upper lake to the lower one", levelAt(grid, 0));
    expect(std::abs(levelAt(grid, 1) - before[1]) < 1e-6 && std::abs(levelAt(grid, 2) - before[2]) < 1e-6, "the lower lakes did not stay at their sills");
    expect(after.outflow.water > 1.0, "the released water did not reach the rim", after.outflow.water);
    expect(close(stood.water - stockOf(grid).water, after.outflow.water, stood.water), "the released water is not what the lakes lost");
    expect(close(stood.salt - stockOf(grid).salt, after.outflow.salt, stood.salt), "the released salt is not what the lakes lost");
}

// The event solver ends where the quiet solver puts the water. A river runs for a while and stops;
// what the event solver still holds above the sills then drains over them ever more slowly, so the
// two states are compared by the share of the water that stands in a different place.
void testSurgeAgrees() {
    const double river = 0.72 * YearSeconds;   // 0.72 m³/s
    WaterGrid quiet = cascadeGrid(river), live = quiet;
    const BasinTree tree = buildBasins(quiet);
    WaterFlows quietFlows, liveFlows;
    const double seconds = 1500.0;
    fillAndSpill(quiet, tree, seconds / YearSeconds, 0.0, quietFlows);
    expect(levelAt(quiet, 1) > levelAt(full(quiet, tree), 1) - 1e-6 && quiet.depth[size_t(Deep / 2) * Wide + size_t(PitX[2])] > 0.5 && quietFlows.outflow.water == 0,
           "the test river does not end part-way into the third hollow");
    const auto misplaced = [&] {
        double moved = 0;
        for (size_t cell = 0; cell < live.cells(); ++cell) moved += std::abs(live.depth[cell] - quiet.depth[cell]);
        return moved * 16.0 / stockOf(quiet).water;
    };
    SurgeScratch scratch;
    for (double done = 0; done < seconds; done += 5.0) surge(live, 5.0, liveFlows, scratch);
    live.sources.clear();
    double early = 0, unrest = 0;
    for (double done = 0; done < 4000.0; done += 5.0) {
        unrest = surge(live, 5.0, liveFlows, scratch);
        if (done == 500.0) early = misplaced();
    }
    const Tally stock = stockOf(live);
    expect(close(stock.water, balanceOf(liveFlows), liveFlows.inflow.water), "the event solver loses or makes water", stock.water - balanceOf(liveFlows));
    expect(close(stock.salt, saltBalanceOf(liveFlows), liveFlows.inflow.salt), "the event solver loses or makes salt");
    expect(close(liveFlows.inflow.water, quietFlows.inflow.water, quietFlows.inflow.water), "the two solvers were fed different rivers");
    expect(misplaced() < 0.08 && misplaced() < 0.5 * early, "the event solver does not approach the quiet solution", misplaced());
    for (int pit = 0; pit < 2; ++pit)
        expect(levelAt(live, pit) > levelAt(quiet, pit) - 1e-3 && levelAt(live, pit) < levelAt(quiet, pit) + 0.03, "a lake does not come to rest at its sill", levelAt(live, pit) - levelAt(quiet, pit));
    expect(unrest < 1e-4, "the event never calmed down", unrest);
    for (double depth : live.depth) expect(depth >= 0 && std::isfinite(depth), "the event solver left a bad depth", depth);
}

// A river that carries sediment into a lake builds a delta of exactly what it brought, the lakes
// below get none until the first has no room left, and a bed only cuts where it was disturbed.
void testSediment() {
    WaterGrid grid = cascadeGrid(4000.0);
    grid.sources.front().load = 12.0;
    const auto solidOf = [](const WaterGrid& of) {
        double volume = 0;
        for (size_t cell = 0; cell < of.cells(); ++cell) volume += double(of.solid.rock[cell]) + of.solid.sediment[0][cell] + of.solid.sediment[1][cell] + of.solid.sediment[2][cell];
        return volume * 16.0;
    };
    const auto raisedAround = [&](const WaterGrid& of, const WaterGrid& from, int pit) {
        double volume = 0;
        for (int z = 0; z < Deep; ++z)
            for (int x = PitX[pit] - 9; x <= PitX[pit] + 9; ++x) volume += (double(of.ground[size_t(z) * Wide + size_t(x)]) - from.ground[size_t(z) * Wide + size_t(x)]) * 16.0;
        return volume;
    };
    BasinTree tree = buildBasins(grid);
    WaterFlows water;
    Lakes lakes = fillAndSpill(grid, tree, 50.0, 0.0, water);
    routeDischarge(grid, tree, lakes);
    for (size_t cell = 0; cell < grid.cells(); ++cell) grid.balance[cell] = grid.depth[cell] > 0 ? 0.0f : float(streamPower(grid, tree, cell));
    const WaterGrid start = grid;
    const double firstLake = [&] {
        double volume = 0;
        for (int z = 0; z < Deep; ++z)
            for (int x = PitX[0] - 9; x <= PitX[0] + 9; ++x) volume += grid.depth[size_t(z) * Wide + size_t(x)] * 16.0;
        return volume;
    }();

    SolidFlows solid;
    expect(moveSediment(grid, tree, lakes, 1.0, solid), "a laden river changed nothing");
    expect(std::abs(solid.deposited - 12.0) < 1e-5 && solid.eroded == 0 && std::abs(solid.exported) < 1e-5, "a year's delta is not a year's load", solid.deposited);
    expect(std::abs(raisedAround(grid, start, 0) - 12.0) < 1e-5 && raisedAround(grid, start, 1) == 0, "the delta is not in the first lake");
    // Until the first lake is full of sediment nothing reaches the second; afterwards it does.
    const double span = 5.0;
    int years = 1;
    for (; years < 400 && raisedAround(grid, start, 1) == 0; years += int(span)) {
        tree = buildBasins(grid);
        lakes = settle(grid, tree, {}, water);
        routeDischarge(grid, tree, lakes);
        moveSediment(grid, tree, lakes, span, solid);
    }
    expect(raisedAround(grid, start, 1) > 0, "sediment never reached the second lake");
    expect(raisedAround(grid, start, 0) > 0.6 * firstLake && raisedAround(grid, start, 0) < 1.05 * firstLake, "the first lake did not fill with about its own volume of sediment",
           raisedAround(grid, start, 0) / firstLake);
    expect(close(solid.carriedIn, 12.0 * years, solid.carriedIn) && close(solid.carriedIn + solid.eroded, solid.deposited + solid.exported, solid.carriedIn),
           "sediment is lost or made on the way", solid.carriedIn + solid.eroded - solid.deposited - solid.exported);
    expect(close(solidOf(grid) - solidOf(start), solid.deposited - solid.eroded, solid.deposited), "the columns did not gain what was laid", solidOf(grid) - solidOf(start));

    // The same river without load on its untouched bed cuts nothing; a step dug into the bed does.
    WaterGrid clear = start;
    clear.sources.front().load = 0;
    tree = buildBasins(clear);
    lakes = settle(clear, tree, {}, water);
    routeDischarge(clear, tree, lakes);
    SolidFlows still;
    expect(!moveSediment(clear, tree, lakes, 100.0, still) && still.eroded == 0, "an untouched bed in balance was cut", still.eroded);
    const size_t step = size_t(Deep / 2) * Wide + 6;
    expect(clear.depth[step] == 0 && clear.depth[step - 1] == 0, "the test bed is under water");
    clear.solid.rock[step] -= 1.0f;
    clear.ground[step] -= 1.0f;
    clear.erodibility.assign(clear.cells(), 2e-5f);
    tree = buildBasins(clear);
    lakes = settle(clear, tree, {}, water);
    routeDischarge(clear, tree, lakes);
    moveSediment(clear, tree, lakes, 100.0, still);
    expect(still.eroded > 0 && clear.ground[step - 1] < start.ground[step - 1], "a step in the bed did not cut back upstream", still.eroded);
    expect(close(still.carriedIn + still.eroded, still.deposited + still.exported, still.eroded), "cut material is lost on the way");
}

} // namespace

void testSmallSea();          // HydrologySea.cpp
void testHydrologyGround();   // HydrologyGround.cpp

void testHydrology() {
    testCascade();
    testSurgeAgrees();
    testSediment();
    testSmallSea();
    testHydrologyGround();
}
