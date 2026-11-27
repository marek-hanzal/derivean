#include "simulation/fn/BuildGraph.hpp"

#include "groundwater/fn/Reads.hpp"
#include "landscape/fn/Build.hpp"
#include "simulation/constant/BuildPriority.hpp"
#include "simulation/type/World.hpp"

#include <atomic>
#include <exception>
#include <mutex>
#include <stdexcept>

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

void buildAll(World& world, const std::vector<BuildKey>& targets) {
    BuildKeys stalled;
    while (true) {
        BuildKeys visited;
        std::vector<BuildKey> ready;
        for (const BuildKey target : targets) buildFrontier(world, target, visited, ready);
        if (visited.empty()) return;
        if (ready.empty()) {
            // An input another thread published between a node's check and its own visit leaves
            // that node with nothing ready; the next plan sees it finished. Only a plan that comes
            // out the same twice has truly nothing to start.
            if (visited == stalled) throw std::logic_error("unfinished build graph has no ready node");
            stalled = std::move(visited);
            continue;
        }
        stalled.clear();
        // Another thread may finish or claim a node meanwhile: a finished one returns at once, a
        // claimed one is waited for, and its builder holds no claim it waits on (Prepared). Each
        // part takes the next node left, so one slow node does not hold up the ones cut with it.
        // A part waits for whatever its nodes find claimed, so it ranks below every build.
        std::atomic<size_t> next{0};
        std::mutex mutex;
        std::exception_ptr failure;
        world.workers.parallelFor(int(ready.size()), [&](int, int) {
            for (size_t i = next++; i < ready.size(); i = next++) try {
                    buildReady(world, ready[i]);
                } catch (...) {
                    std::lock_guard lock(mutex);
                    if (!failure) failure = std::current_exception();
                }
        }, 1, preparedPriority);
        if (failure) std::rethrow_exception(failure);
    }
}

} // namespace derivean::sim
