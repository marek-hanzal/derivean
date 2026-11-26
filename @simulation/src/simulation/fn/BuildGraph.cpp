#include "simulation/fn/BuildGraph.hpp"

#include "groundwater/fn/Reads.hpp"
#include "landscape/fn/Build.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

std::vector<BuildKey> buildReads(BuildKey key) {
    std::vector<BuildKey> reads;
    switch (key.product) {
    case BuildProduct::Terrain:
        for (const TileKey tile : dependencies(key.tile)) reads.push_back({BuildProduct::Terrain, tile});
        for (const TileKey tile : groundwaterRead(key.tile)) reads.push_back({BuildProduct::LocalWater, tile});
        break;
    case BuildProduct::RegionalWater:
        for (const TileKey tile : aquiferTerrain(key.tile)) reads.push_back({BuildProduct::Terrain, tile});
        break;
    case BuildProduct::LocalWater:
        for (const TileKey tile : aquiferTerrain(key.tile)) reads.push_back({BuildProduct::Terrain, tile});
        for (const TileKey tile : aquiferRegions(key.tile)) reads.push_back({BuildProduct::RegionalWater, tile});
        break;
    }
    return reads;
}

bool built(const World& world, BuildKey key) {
    switch (key.product) {
    case BuildProduct::Terrain: return bool(world.atlas.find(key.tile));
    case BuildProduct::RegionalWater: return bool(world.groundwater.findRegional(key.tile));
    case BuildProduct::LocalWater: return bool(world.groundwater.find(key.tile));
    }
    return false;
}

void buildFrontier(const World& world, BuildKey key, BuildKeys& visited, std::vector<BuildKey>& ready) {
    if (built(world, key) || !visited.insert(key).second) return;
    bool inputsReady = true;
    for (const BuildKey input : buildReads(key)) {
        if (built(world, input)) continue;
        inputsReady = false;
        buildFrontier(world, input, visited, ready);
    }
    if (inputsReady) ready.push_back(key);
}

void buildReady(World& world, BuildKey key) {
    switch (key.product) {
    case BuildProduct::Terrain: world.atlas.require(key.tile, BuildMode::Prepared); break;
    case BuildProduct::RegionalWater: world.groundwater.requireRegional(world.atlas, key.tile, BuildMode::Prepared); break;
    case BuildProduct::LocalWater: world.groundwater.require(world.atlas, key.tile, BuildMode::Prepared); break;
    }
}

} // namespace derivean::sim
