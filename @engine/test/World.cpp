#include "granular/type/Sand.hpp"
#include "simulation/type/JoltRuntime.hpp"
#include "voxel/type/Mesh.hpp"
#include "water/type/Water.hpp"

#include <cstdlib>
#include <iostream>

using namespace derivean;

namespace {

void expect(bool passed, const char* message) {
    if (passed) return;
    std::cerr << message << '\n';
    std::exit(1);
}

void checkBalance(const WaterState& water) {
    uint64_t cells = 0, parcels = 0;
    for (const auto& [point, amount] : water.cells) {
        expect(amount > 0 && amount <= WaterUnits, "Overfilled or empty water cell");
        cells += amount;
    }
    for (const auto& parcel : water.parcels) parcels += parcel.amount;
    expect(cells == water.budget.bulk && parcels == water.budget.airborne, "Water ledger disagrees with stored state");
    expect(cells + parcels + water.budget.escaped == water.budget.initial, "Water volume changed");
}

void simulate(WaterState& water, const VoxelWorld& world, int steps, const JoltRuntime* runtime = nullptr) {
    for (int step = 0; step < steps; ++step) {
        advanceWater(water, world, 1.0 / 120.0, runtime, runtime ? 0.0 : 1e100);
        checkBalance(water);
    }
}

uint64_t below(const WaterState& water, int level) {
    uint64_t count = 0;
    for (const auto& [cell, amount] : water.cells)
        if (cell.y < level) count += amount;
    return count;
}

void sparseTerrain() {
    VoxelWorld world;
    const Cell floor{-1, -1025, -1};
    const Cell sky{-1, 1025, -1};
    expect(chunkOf(floor).y == -33 && chunkOf(sky).y == 32, "Chunks must be addressed along Y");
    expect(materialAt(world, floor) == Material::Air, "Unexpected material in empty world");
    setVoxel(world, floor, Material::Bedrock);
    setVoxel(world, sky, Material::Shale);
    expect(sampleWorld(world, chunkBounds(chunkOf(floor)))[floor] == Material::Bedrock, "Deep tile did not round trip");
    expect(sampleWorld(world, chunkBounds(chunkOf(sky)))[sky] == Material::Shale, "Sky tile did not round trip");
    expect(!belowTerrain(world, -125), "A remote floor must still count as material");
    expect(carve(world, floor, 1, BrushShape::Cube).size() == 1, "Lowest authored material should be destructible");
    expect(belowTerrain(world, 0), "Empty space beneath the remaining island should be open");
    removeVoxel(world, sky);
    expect(world.tiles.empty() && world.chunks.empty() && world.occupiedLevels.empty(), "Empty tiles survived removal");
}

void trough(VoxelWorld& world, int floor, int width) {
    fillBox(world, {{-width, floor, -4}, {width + 1, floor + 1, 5}}, Material::Basalt);
    fillBox(world, {{-width, floor + 1, -4}, {-width + 1, floor + 8, 5}}, Material::Basalt);
    fillBox(world, {{width, floor + 1, -4}, {width + 1, floor + 8, 5}}, Material::Basalt);
    fillBox(world, {{-width + 1, floor + 1, -4}, {width, floor + 8, -3}}, Material::Basalt);
    fillBox(world, {{-width + 1, floor + 1, 4}, {width, floor + 8, 5}}, Material::Basalt);
}

void independentTroughsAndVoid() {
    VoxelWorld world;
    WaterState water;
    constexpr int upper = 65, lower = -66;
    trough(world, upper, 4);
    trough(world, lower, 5);
    seedWater(water, world, {{-2, upper + 1, -2}, {3, upper + 3, 3}});
    seedWater(water, world, {{-2, lower + 1, -2}, {3, lower + 2, 3}});
    const auto firstBelow = below(water, 0);
    simulate(water, world, 120);
    expect(below(water, 0) == firstBelow && water.parcels.empty() && water.budget.escaped == 0,
           "Vertically aligned volumes should remain separate");
    expect(!meshWater(water, world, chunkOf({0, upper + 1, 0})).empty() &&
           !meshWater(water, world, chunkOf({0, lower + 1, 0})).empty(),
           "Both water sheets should be projected independently");

    const auto upperBreach = carve(world, {0, upper, 0}, 3, BrushShape::Cube);
    wakeWater(water, upperBreach);
    simulate(water, world, 110);
    expect(water.budget.airborne > 0 && water.budget.escaped == 0, "Water should still be flying toward the lower catch");
    simulate(water, world, 510);
    expect(below(water, 0) > firstBelow && water.budget.escaped == 0, "Water did not reach the lower channel");

    const auto lowerBreach = carve(world, {0, lower, 0}, 3, BrushShape::Cube);
    wakeWater(water, lowerBreach);
    simulate(water, world, 450);
    expect(water.budget.escaped > 0, "Opening the lower floor should permit permanent escape");
}

void damAndDisplacement() {
    VoxelWorld world;
    WaterState water;
    constexpr int floor = 136;
    trough(world, floor, 5);
    fillBox(world, {{0, floor + 1, -3}, {1, floor + 8, 4}}, Material::Clay);
    seedWater(water, world, {{-4, floor + 1, -3}, {0, floor + 4, 4}});
    const auto downstream = [&] {
        uint64_t count = 0;
        for (const auto& [point, amount] : water.cells) if (point.x > 0) count += amount;
        return count;
    };
    simulate(water, world, 180);
    expect(downstream() == 0 && water.budget.escaped == 0, "Full-height dam leaked");
    const auto cut = carve(world, {0, floor + 1, 0}, 1, BrushShape::Cube);
    wakeWater(water, cut);
    simulate(water, world, 250);
    expect(downstream() > 0, "A low breach should release stored water");
    const Cell plug{0, floor + 1, 0};
    expect(displaceWater(water, world, plug), "Placing in water should displace the volume");
    setVoxel(world, plug, Material::Clay);
    wakeWater(water, plug);
    checkBalance(water);
    const auto downstreamBefore = downstream();
    simulate(water, world, 160);
    expect(waterAt(water, plug) == 0 &&
           downstream() <= downstreamBefore + WaterUnits * 4, "Rebuilt plug allowed more water to cross");
}

void roofedPipe() {
    VoxelWorld world;
    WaterState water;
    fillBox(world, {{-2, -100, -2}, {9, -91, 3}}, Material::Basalt);
    fillBox(world, {{-1, -99, 0}, {8, -98, 1}}, Material::Air);
    fillBox(world, {{-1, -98, 0}, {0, -91, 1}}, Material::Air);
    fillBox(world, {{7, -98, 0}, {8, -91, 1}}, Material::Air);
    seedWater(water, world, {{-1, -99, 0}, {8, -98, 1}});
    seedWater(water, world, {{-1, -98, 0}, {0, -94, 1}});
    simulate(water, world, 420);
    expect(waterAt(water, {7, -97, 0}) > 0, "Pressure should pass through a roofed, full conduit");
    expect(!water.budget.escaped, "Sealed pipe lost water");
}

void remoteCatch() {
    VoxelWorld world;
    WaterState water;
    trough(world, -512, 4);
    seedWater(water, world, {{0, 40, 0}, {1, 41, 1}});
    simulate(water, world, 900);
    expect(below(water, -500) && !water.budget.escaped, "Water failed to find terrain far outside the local chunks");
}

void weakBearingFails() {
    VoxelWorld world;
    fillBox(world, {{-40, 0, -2}, {41, 1, 3}}, Material::Basalt);
    fillBox(world, {{-1, 6, -1}, {2, 8, 2}}, Material::Basalt);
    fillBox(world, {{0, 1, 0}, {1, 6, 1}}, Material::Leaf);
    fillBox(world, {{1, 1, 0}, {2, 6, 1}}, Material::Timber);

    auto cut = carve(world, {1, 5, 0}, 1, BrushShape::Cube);
    const auto detached = collapse(world, cut);
    expect(detached.pieces.size() == 1 && detached.pieces.front().mass > 100,
           "A leaf cannot support the slab after its timber brace is cut");
    expect(materialAt(world, {0, 6, 0}) == Material::Air, "Detached rock remained in the terrain");
}

void contactsAndSediment() {
    VoxelWorld world;
    WaterState water;
    JoltRuntime physics;
    physics.returnPlayer({0.5, 0.025, 1.7});
    const auto hit = physics.sweepMoving({0.5, 2.2, 1.7}, {0.5, 0.7, 1.7}, 0.5 * VoxelSize);
    expect(hit && hit->normal[1] > 0.6, "The physical player capsule should be hit from above");
    fillBox(world, {{-3, -4, -3}, {3, -3, 3}}, Material::Shale);
    SandState sand;
    const Cell positions[]{{0, 14, 0}, {10, 14, 10}};
    scatterSand(sand, positions);
    std::vector<Cell> landed;
    for (int i = 0; i < 300; ++i) stepSand(sand, world, water, 1.0 / 120.0, landed);
    expect(sand.deposited == 1 && sand.escaped == 1 && sand.grains.empty(),
           "Sand should settle on contact and be lost across the void");
    expect(landed.size() == 1 && materialAt(world, landed.front()) == Material::Sand,
           "Settled sand must become world material");
}

} // namespace

int main() {
    sparseTerrain();
    independentTroughsAndVoid();
    damAndDisplacement();
    roofedPipe();
    remoteCatch();
    weakBearingFails();
    contactsAndSediment();
}
