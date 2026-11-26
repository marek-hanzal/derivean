#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "hydrology/fn/Ocean.hpp"
#include "hydrology/fn/Water.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Hydrology: " << message << " (" << value << ")\n";
    std::exit(1);
}

// Three hollows round one saddle cell on a level floor, with a river into the first and the way
// out beside the third: ties in height everywhere, and three pieces of ground meeting in one cell.
void testSaddle() {
    WaterGrid grid;
    shapeGrid(grid, 13, 9, 4.0);
    for (int z = 0; z < 9; ++z) {
        for (int x = 0; x < 13; ++x) {
            const size_t cell = size_t(z) * 13 + size_t(x);
            const bool hollow = (z == 4 && (x == 3 || x == 5 || x == 9)) || (x == 7 && z == 2), saddle = (x == 4 || x == 6 || x == 8) && z == 4, gap = (x == 7 && z == 3) || (x >= 10 && z == 4);
            grid.solid.rock[cell] = grid.ground[cell] = grid.held[cell] = hollow ? 8.0f : saddle || gap ? 9.0f : 10.0f;
            if (x == 0 || z == 0 || x == 12 || z == 8) grid.kind[cell] = uint8_t(CellKind::Rim);
        }
    }
    grid.held[4 * 13 + 12] = grid.ground[4 * 13 + 12] = grid.solid.rock[4 * 13 + 12] = 8.5f;
    grid.sources.push_back({4 * 13 + 3, 1000.0, 0.0});
    grid.sourceSalinity = 0.5;
    const BasinTree tree = buildBasins(grid);
    WaterFlows flows;
    Lakes lakes;
    for (int step = 0; step < 40; ++step) {
        lakes = fillAndSpill(grid, tree, 1.0, 0.0, flows);
        const Tally stock = stockOf(grid);
        expect(std::abs(stock.water - (flows.inflow.water - flows.outflow.water)) < 1e-9 * flows.inflow.water, "water is lost on level ground", stock.water - flows.inflow.water + flows.outflow.water);
        expect(std::abs(stock.salt - (flows.inflow.salt - flows.outflow.salt)) < 1e-9 * flows.inflow.salt, "salt is lost between lakes that stand level", stock.salt - flows.inflow.salt + flows.outflow.salt);
    }
    expect(flows.outflow.water > 0 && flows.outflow.salt > 0, "the hollows never overflowed");
    for (const int x : {3, 5, 9}) expect(std::abs(grid.depth[size_t(4 * 13 + x)] - 1.0) < 1e-9, "a hollow is not full to the saddle", grid.depth[size_t(4 * 13 + x)]);
    routeDischarge(grid, tree, lakes);
    expect(std::abs(grid.discharge[4 * 13 + 11] - 1000.0f) < 1.0f, "the river is not found below the chain of lakes", grid.discharge[4 * 13 + 11]);
}

// Random ground in steps of a quarter metre, with rain, evaporation, sea on part of the rim, laden
// rivers, pits dug into it and a hair of negative cover: every step must close all three budgets.
void testBudgets() {
    constexpr int Side = 56;
    for (uint64_t seed = 1; seed <= 10; ++seed) {
        WaterGrid grid;
        shapeGrid(grid, Side, Side, 4.0);
        for (int z = 0; z < Side; ++z) {
            for (int x = 0; x < Side; ++x) {
                const size_t cell = size_t(z) * Side + size_t(x);
                const double height = 30.0 - 0.15 * x + 6.0 * fractalNoise(seed, x / 14.0, z / 14.0, 3);
                grid.solid.rock[cell] = float(std::floor(height * 4.0) / 4.0 - 0.5);
                grid.solid.sediment[Sand][cell] = 0.25f;
                grid.solid.sediment[Fines][cell] = unit(hash(seed, x, z)) < 0.05 ? -2e-7f : 0.25f;
                grid.ground[cell] = grid.held[cell] = groundOf(grid.solid, cell);
                grid.runoff[cell] = 0.4f, grid.rain[cell] = 0.8f, grid.evaporation[cell] = seed % 2 ? 0.6f : 1.4f, grid.erodibility[cell] = 2e-5f;
                if (x != 0 && z != 0 && x != Side - 1 && z != Side - 1) continue;
                grid.kind[cell] = uint8_t(CellKind::Rim);
                grid.salinity[cell] = 20.0f;
                if (seed % 3 == 0 && x == Side - 1) grid.held[cell] += 1.0f;
            }
        }
        for (int source = 0; source < 4; ++source)
            grid.sources.push_back({int32_t((8 + int(hash(seed, source) % (Side - 16))) * Side + 2 + int(hash(seed, source, 1) % 12)), 1e5 * (1 + source), 2.0 * source});
        grid.sourceSalinity = 0.12;
        BasinTree tree = buildBasins(grid);
        WaterFlows water;
        SolidFlows solid;
        Lakes lakes = fillAndSpill(grid, tree, 3.0, 3.0, water);
        routeDischarge(grid, tree, lakes);
        for (size_t cell = 0; cell < grid.cells(); ++cell) grid.balance[cell] = grid.depth[cell] > 0 ? 0.0f : float(streamPower(grid, tree, cell));
        for (int pit = 0; pit < 3; ++pit) {
            const size_t cell = size_t(12 + int(hash(seed, pit, 7) % (Side - 24))) * Side + size_t(12 + int(hash(seed, pit, 9) % (Side - 24)));
            cutColumn(grid.solid, cell, 2.0);
            grid.ground[cell] = groundOf(grid.solid, cell);
        }
        for (int step = 0; step < 12; ++step) {
            tree = buildBasins(grid);
            lakes = fillAndSpill(grid, tree, 2.0, 2.0, water);
            routeDischarge(grid, tree, lakes);
            moveSediment(grid, tree, lakes, 5.0, solid);
            const Tally stock = stockOf(grid);
            const double gross = water.inflow.water + water.rain.water + water.fromBody.water;
            expect(std::abs(stock.water - (gross - water.evaporation.water - water.outflow.water)) < 1e-9 * gross, "water budget open on random ground", double(seed));
            expect(std::abs(stock.salt - (water.inflow.salt + water.rain.salt + water.fromBody.salt - water.outflow.salt)) < 1e-9 * (water.inflow.salt + water.rain.salt + water.fromBody.salt),
                   "salt budget open on random ground", double(seed));
            expect(std::abs(solid.carriedIn + solid.eroded - solid.deposited - solid.exported) < 1e-9 * (solid.carriedIn + solid.eroded + 1e-9) && solid.eroded >= 0,
                   "sediment budget open on random ground", double(seed));
            for (const double depth : grid.depth) expect(depth >= 0 && std::isfinite(depth), "a bad depth on random ground", depth);
        }
    }
}

} // namespace

void testHydrologyGround() {
    testSaddle();
    testBudgets();
}
