#include "game/type/WorldProjection.hpp"

namespace derivean {
namespace {

constexpr int CollisionRadius = 2;
constexpr int DebrisUrgency = 1;   // chunks around a moving body whose collider cannot wait for a background build

int distance(Cell a, Cell b) {
    return std::max({std::abs(a.x - b.x), std::abs(a.y - b.y), std::abs(a.z - b.z)});
}

bool within(const std::vector<Cell>& centers, Cell key, int radius) {
    return std::any_of(centers.begin(), centers.end(), [&](Cell center) { return distance(key, center) <= radius; });
}

void collectShapes(Game& game) {
    ColliderQueue& queue = game.colliders;
    for (ShapeResult& result : queue.finished->collect()) {
        const auto pending = queue.pending.find(result.key);
        if (pending == queue.pending.end() || pending->second != result.ticket) continue;
        queue.pending.erase(pending);
        if (game.terrain.contains(result.key) || !game.world.chunks.contains(result.key)) continue;
        game.terrain.emplace(result.key, game.physics.addTerrain(result.shape, chunkBounds(result.key).min));
    }
}

void requestShape(Game& game, Cell key) {
    ColliderQueue& queue = game.colliders;
    const uint64_t ticket = ++queue.tickets;
    queue.pending[key] = ticket;
    game.nature.crew->submit([box = queue.finished, key, ticket, voxels = sampleWorld(game.world, chunkBounds(key))] {
        box->post({key, ticket, terrainShape(voxels)});
    }, 1);
}

} // namespace

void streamColliders(Game& game) {
    collectShapes(game);
    std::vector<Cell> debris;
    for (const auto& body : game.bodies)
        if (!body.suspended) debris.push_back(chunkOf(cellAt(game.physics.pose(body.id).position)));
    std::vector<Cell> centers = debris;
    centers.push_back(chunkOf(cellAt(game.physics.eye())));
    std::sort(centers.begin(), centers.end());
    centers.erase(std::unique(centers.begin(), centers.end()), centers.end());
    if (game.dirtyColliders.empty() && !game.colliders.stale && centers == game.colliderCenters) return;
    game.colliderCenters = centers;
    game.colliders.stale = false;

    for (auto it = game.terrain.begin(); it != game.terrain.end();) {
        if (game.dirtyColliders.contains(it->first) || !game.world.chunks.contains(it->first) || !within(centers, it->first, CollisionRadius)) {
            game.physics.removeBody(it->second);
            it = game.terrain.erase(it);
        } else {
            ++it;
        }
    }
    std::erase_if(game.colliders.pending, [&](const auto& entry) {
        return game.dirtyColliders.contains(entry.first) || !within(centers, entry.first, CollisionRadius);
    });

    for (Cell center : centers)
        for (int y = -CollisionRadius; y <= CollisionRadius; ++y)
            for (int z = -CollisionRadius; z <= CollisionRadius; ++z)
                for (int x = -CollisionRadius; x <= CollisionRadius; ++x) {
                    const Cell key = center + Cell{x, y, z};
                    if (!game.world.chunks.contains(key) || game.terrain.contains(key)) continue;
                    // Streamed ground arrives ahead of whoever walks onto it, so its collider can be
                    // built off the game thread. An edit or a body in motion needs the collider now.
                    const bool background = !game.dirtyColliders.contains(key) && !within(debris, key, DebrisUrgency);
                    if (!background) game.terrain.emplace(key, game.physics.addTerrain(sampleWorld(game.world, chunkBounds(key))));
                    else if (!game.colliders.pending.contains(key)) requestShape(game, key);
                }
    game.dirtyColliders.clear();
}

} // namespace derivean
