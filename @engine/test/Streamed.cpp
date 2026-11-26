#include "granular/type/Sand.hpp"
#include "particles/type/VoxelContact.hpp"
#include "water/type/Water.hpp"

#include "Expect.hpp"

using namespace derivean;

namespace {

ChunkVoxels solidChunk(Cell key, Material material) {
    ChunkVoxels chunk{key, {}, 0, 0};
    constexpr int side = ChunkSide / VoxelsPerMeter;
    for (int y = 0; y < side; ++y)
        for (int z = 0; z < side; ++z)
            for (int x = 0; x < side; ++x) {
                TerrainTile tile;
                tile.uniform = material;
                tile.occupied = TerrainTile::CellCount;
                chunk.tiles.emplace_back(key * side + Cell{x, y, z}, std::move(tile));
                chunk.occupied += TerrainTile::CellCount;
            }
    return chunk;
}

VoxelWorld streamedWorld() {
    VoxelWorld world;
    world.streamed.emplace();
    world.voidLevel = -4;
    loadChunk(world, solidChunk({0, 0, 0}, Material::Granite));
    world.streamed->sky[{0, 0, 0}] = 1;
    return world;
}

void unloadedRockIsNotVoid() {
    VoxelWorld world = streamedWorld();
    const Cell loaded{3, 3, 3}, sky{3, ChunkSide + 3, 3}, deep{3, -3, 3}, beside{ChunkSide + 3, 3, 3}, abyss{3, -5 * ChunkSide, 3};
    expect(materialAt(world, loaded) == Material::Granite, "Loaded voxels must read as stored");
    expect(materialAt(world, sky) == Material::Air, "Space above a column's sky is open");
    expect(materialAt(world, deep) == Unloaded && materialAt(world, beside) == Unloaded, "Rock that is not loaded must read as solid");
    expect(materialAt(world, abyss) == Material::Air, "Only the space under the world floor is void");
    expect(!inVoid(world, -3 * double(ChunkSide) * VoxelSize) && inVoid(world, -5 * double(ChunkSide) * VoxelSize),
           "Matter is lost only below the world floor");

    const Voxels sample = sampleWorld(world, {{-2, -2, -2}, {ChunkSide + 2, ChunkSide + 2, ChunkSide + 2}});
    expect(sample[{-1, 3, 3}] == Unloaded && sample[{3, -1, 3}] == Unloaded && sample[{3, ChunkSide, 3}] == Material::Air &&
           sample[{3, 3, 3}] == Material::Granite, "Sampling must agree with single lookups across loaded and unloaded space");

    double excavated = 0;
    const auto carved = carve(world, {0, 3, 3}, 3, BrushShape::Cube, &excavated);
    expect(!carved.empty() && std::all_of(carved.begin(), carved.end(), [](Cell cell) { return cell.x >= 0; }),
           "A brush must not cut rock that is not loaded");
    setVoxel(world, beside, Material::Basalt);
    expect(materialAt(world, beside) == Unloaded && !world.chunks.contains(chunkOf(beside)), "Unloaded rock cannot be written");
    setVoxel(world, sky, Material::Basalt);
    expect(materialAt(world, sky) == Material::Basalt && world.streamed->loaded.contains(chunkOf(sky)),
           "Building into open sky must materialise its chunk");
    expect(world.streamed->journal.size() == carved.size() + 1, "Every voxel edit must be journalled");

    unloadChunk(world, {0, 0, 0});
    expect(materialAt(world, loaded) == Unloaded && !world.chunks.contains({0, 0, 0}), "An unloaded chunk turns back into unknown rock");
}

void looseMatterWaitsOutsideLoadedSpace() {
    VoxelWorld world = streamedWorld();
    WaterState water;
    SandState sand;
    // One grain over loaded ground, one over a column that is not loaded, one inside unloaded rock.
    const Cell cells[]{{3, ChunkSide + 20, 3}, {3 * ChunkSide, ChunkSide + 20, 3}, {3 * ChunkSide, 3, 3}};
    scatterSand(sand, cells);
    water.parcels.push_back({cellCenter({3 * ChunkSide, 8, 3}), {0, -1, 0}, WaterUnits});
    water.budget.airborne = water.budget.initial = WaterUnits;
    std::vector<Cell> landed;
    for (int step = 0; step < 600; ++step) {
        stepSand(sand, world, water, 1.0 / 120.0, landed);
        advanceWater(water, world, 1.0 / 120.0);
    }
    expect(sand.escaped == 0 && water.budget.escaped == 0, "Matter in space that is not loaded has not left the world");
    expect(sand.deposited == 1 && sand.grains.size() == 2, "Grains outside the loaded ground must wait where they are");
    expect(water.parcels.size() == 1 && water.budget.airborne == WaterUnits, "Water outside the loaded ground must wait where it is");

    // A grain drifting out of the open sky of a loaded column towards a column that is not loaded
    // must not settle against that unknown: what it left behind would depend on what was loaded.
    const size_t voxels = world.streamed->journal.size();
    sand.grains.push_back({{ChunkSide * double(VoxelSize) - 0.3, 2.5 * ChunkSide * VoxelSize, 0.5}, {3, 0, 0}, Material::Sand});
    for (int step = 0; step < 240; ++step) stepSand(sand, world, water, 1.0 / 120.0, landed);
    expect(sand.grains.size() == 3 && world.streamed->journal.size() == voxels, "A grain must not settle against ground that is not loaded");
    const SandGrain& waiting = sand.grains.back();
    expect(waiting.position[0] > ChunkSide * double(VoxelSize) - 0.3 && waiting.position[0] < ChunkSide * double(VoxelSize) &&
           waiting.position[1] > 2.4 * ChunkSide * VoxelSize && waiting.velocity[0] == 3, "A waiting grain must keep its place and its motion");
}

} // namespace

void testStreamedWorld() {
    unloadedRockIsNotVoid();
    looseMatterWaitsOutsideLoadedSpace();
}
