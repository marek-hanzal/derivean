#include "simulation/type/WorldProjection.hpp"

#include <cmath>

namespace derivean {
namespace {

constexpr int CollisionRadius = 2;

int distance(Cell a, Cell b) {
    return std::max({std::abs(a.x - b.x), std::abs(a.y - b.y), std::abs(a.z - b.z)});
}

Position origin(Cell chunk) {
    constexpr double side = ChunkSide * double(VoxelSize);
    return {chunk.x * side, chunk.y * side, chunk.z * side};
}

} // namespace

void streamColliders(Simulation& game) {
    std::vector<Cell> centers{chunkOf(cellAt(game.physics.eye()))};
    for (const auto& body : game.bodies) centers.push_back(chunkOf(cellAt(game.physics.pose(body.id).position)));
    std::sort(centers.begin(), centers.end());
    centers.erase(std::unique(centers.begin(), centers.end()), centers.end());
    if (game.dirtyColliders.empty() && centers == game.colliderCenters) return;
    game.colliderCenters = centers;
    const auto needed = [&](Cell key) {
        return std::any_of(centers.begin(), centers.end(), [&](Cell center) {
            return distance(key, center) <= CollisionRadius;
        });
    };
    for (auto it = game.terrain.begin(); it != game.terrain.end();) {
        if (game.dirtyColliders.contains(it->first) || !game.world.chunks.contains(it->first) || !needed(it->first)) {
            game.physics.removeBody(it->second);
            it = game.terrain.erase(it);
        } else {
            ++it;
        }
    }
    for (const auto& [key, count] : game.world.chunks) {
        if (needed(key) && !game.terrain.contains(key))
            game.terrain.emplace(key, game.physics.addTerrain(sampleWorld(game.world, chunkBounds(key))));
    }
    game.dirtyColliders.clear();
}

void projectScene(Simulation& game, Frame& frame) {
    const Cell center = chunkOf(cellAt(game.physics.eye()));
    std::unordered_set<Cell, CellHash> wanted;
    for (const auto& [key, count] : game.world.chunks)
        if (distance(key, center) <= Visibility) wanted.insert(key);
    for (const auto& [key, count] : game.water.chunks)
        if (distance(key, center) <= Visibility) wanted.insert(key);
    for (Cell key : game.visible)
        if (!wanted.contains(key)) frame.unloaded.push_back(key);

    const bool waterDue = game.water.ticks >= game.lastWaterProjection + 2;
    for (Cell key : wanted) {
        const bool enters = !game.visible.contains(key);
        const bool dirty = game.dirtyTerrain.erase(key) != 0;
        if (enters || dirty) {
            FaceMesh faces;
            if (game.world.chunks.contains(key)) {
                const Bounds area = chunkBounds(key);
                faces = meshFaces(sampleWorld(game.world, area.padded(1)), area);
            }
            frame.chunks.push_back({key, origin(key), std::move(faces)});
        }
        if (enters || dirty || (waterDue && game.water.meshDirty.contains(key))) {
            frame.waterMeshes.push_back({key, origin(key), meshWater(game.water, game.world, key)});
            game.water.meshDirty.erase(key);
        }
    }
    if (waterDue) game.lastWaterProjection = game.water.ticks;
    game.visible = std::move(wanted);
    std::erase_if(game.dirtyTerrain, [&](Cell key) { return !game.visible.contains(key); });
    std::erase_if(game.water.meshDirty, [&](Cell key) { return !game.visible.contains(key); });
    frame.streamingReady = true;
}

void markTerrainChanged(Simulation& game, std::span<const Cell> changed) {
    if (changed.empty()) return;
    for (Cell point : changed) {
        game.editedTiles.insert(tileOf(point));
        const Cell key = chunkOf(point);
        game.dirtyColliders.insert(key);
        game.dirtyTerrain.insert(key);
        for (Cell normal : Neighbors) game.dirtyTerrain.insert(chunkOf(point + normal));
    }
    wakeWater(game.water, changed);
    game.physics.wakeDebris(boundsOf(changed).padded(1));
}

} // namespace derivean
