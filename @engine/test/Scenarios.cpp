#include "granular/type/Sand.hpp"
#include "structure/type/Collapse.hpp"

#include "WaterChecks.hpp"

using namespace derivean;

namespace {

// The water of an upper channel, released through its floor, is caught whole by the channel below
// and leaves the world only when that one is opened too.
void channelDrainsThroughChannel() {
    VoxelWorld world;
    world.voidLevel = -2;
    WaterState water;
    constexpr int upper = 40, lower = 0;
    trough(world, upper, 12);
    // The catch stands wider than the stream and its walls higher than any splash.
    fillBox(world, {{-16, lower, -8}, {17, lower + 1, 9}}, Material::Basalt);
    fillBox(world, {{-16, lower + 1, -8}, {17, lower + 30, -7}}, Material::Basalt);
    fillBox(world, {{-16, lower + 1, 8}, {17, lower + 30, 9}}, Material::Basalt);
    fillBox(world, {{-16, lower + 1, -7}, {-15, lower + 30, 8}}, Material::Basalt);
    fillBox(world, {{16, lower + 1, -7}, {17, lower + 30, 8}}, Material::Basalt);
    seedWater(water, world, {{-11, upper + 1, -3}, {12, upper + 5, 4}});
    simulate(water, world, 60);
    expect(water.budget.airborne == 0 && water.budget.escaped == 0, "A closed channel must hold its water");

    wakeWater(water, carve(world, {0, upper, 0}, 2, BrushShape::Cube));
    uint64_t peakFalling = 0;
    bool foam = false;
    for (int step = 0; step < 900; ++step) {
        simulate(water, world, 1);
        peakFalling = std::max(peakFalling, water.budget.airborne);
        foam |= !water.foam.empty();
    }
    expect(peakFalling > 10'000 && foam, "Released water should fall as a stream and foam where it lands");
    expect(water.budget.escaped == 0 && below(water, upper) > 100'000, "The lower channel must catch the upper one's water");

    wakeWater(water, carve(world, {0, lower, 0}, 4, BrushShape::Cube));
    simulate(water, world, 1800);
    expect(water.budget.escaped > 100'000, "Opening the lower channel should drain the water out of the world");
}

// A breach one voxel wide lets water fall through without lifting any of it above its source.
void narrowBreachKeepsItsLevel() {
    VoxelWorld world;
    WaterState water;
    constexpr int floor = 24, depth = 4;
    trough(world, floor, 6);
    seedWater(water, world, {{-5, floor + 1, -3}, {6, floor + 1 + depth, 4}});
    wakeWater(water, carve(world, {0, floor, 0}, 1, BrushShape::Cube));
    bool fell = false;
    for (int step = 0; step < 360; ++step) {
        simulate(water, world, 1);
        for (const auto& [cell, amount] : water.cells)
            expect(cell.y < floor + 1 + depth, "Water rose above its original source");
        for (const auto& parcel : water.parcels) fell |= parcel.position[1] < floor * double(VoxelSize);
    }
    expect(fell && water.budget.escaped == 0, "Water did not fall through the opening");
}

// A bank of sand on a ledge, undercut at its foot, runs out and comes to rest: every grain that left
// its place settles again, including those that land on grains deposited a moment earlier.
void sandBankSettles() {
    VoxelWorld world;
    WaterState water;
    SandState sand;
    fillBox(world, {{-72, -6, -27}, {72, 0, 35}}, Material::Basalt);
    fillBox(world, {{26, 0, -15}, {53, 2, -5}}, Material::Shale);
    fillBox(world, {{27, 2, -15}, {52, 8, -5}}, Material::Sand);
    std::vector<Cell> cut = carve(world, {38, 1, -6}, 5, BrushShape::Cube);
    releaseSand(sand, world, cut);
    for (const auto& [cell, material] : collapse(world, cut).loose) scatterSand(sand, std::span<const Cell>(&cell, 1), material);
    const uint64_t released = sand.grains.size() + sand.deposited;
    expect(released > 100, "Undercutting a sand bank should release it");
    std::vector<Cell> landed;
    for (int step = 0; step < 960; ++step) stepSand(sand, world, water, 1.0 / 120.0, landed);
    expect(sand.grains.empty() && sand.escaped == 0 && sand.deposited == released, "Released sand must come to rest, every grain of it");
    expect(landed.size() == released, "Every settled grain must be reported as changed ground");
    for (const auto& [tile, content] : world.tiles) expect(content.occupied > 0, "An emptied tile survived");
}

} // namespace

// Whole sequences a player can set off, built from voxels in code.
void testScenarios() {
    channelDrainsThroughChannel();
    narrowBreachKeepsItsLevel();
    sandBankSettles();
}
