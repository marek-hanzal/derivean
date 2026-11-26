#include "game/type/TerrainEdits.hpp"
#include "game/type/WorldProjection.hpp"

namespace derivean {
namespace {

void spawnDebris(Game& game, Frame& frame, const VoxelPiece& piece) {
    const Bounds bounds = piece.voxels.bounds;
    Voxels halo(bounds.padded(1));
    for (int y = bounds.min.y; y < bounds.max.y; ++y)
        for (int z = bounds.min.z; z < bounds.max.z; ++z)
            for (int x = bounds.min.x; x < bounds.max.x; ++x)
                halo[{x, y, z}] = piece.voxels[{x, y, z}];
    const auto body = game.physics.addDebris(piece);
    game.bodies.push_back({body, piece.mass});
    frame.spawned.push_back({body.GetIndexAndSequenceNumber(), meshFaces(halo, bounds)});
}

void dig(Game& game, Frame& frame, Cell center, int radius, BrushShape shape) {
    auto changed = carve(game.world, center, radius, shape, &game.excavatedKg);
    if (changed.empty()) return;
    releaseSand(game.sand, game.world, changed);
    auto detached = collapse(game.world, changed);
    for (const auto& [cell, material] : detached.loose) scatterSand(game.sand, std::span<const Cell>(&cell, 1), material);
    markTerrainChanged(game, changed);
    for (const auto& piece : detached.pieces) spawnDebris(game, frame, piece);
}

void place(Game& game, Cell point) {
    if (!game.buildStock || materialAt(game.world, point) != Material::Air ||
        game.physics.overlapsMovingVoxel(point) || !displaceWater(game.water, game.world, point)) return;
    setVoxel(game.world, point, Material::Basalt);
    --game.buildStock;
    markTerrainChanged(game, std::span<const Cell>(&point, 1));
}

} // namespace

void editTerrain(Game& game, Frame& frame, const RayHit& hit, const Command& command) {
    if (!hit.terrain) return;
    if (command.dig) dig(game, frame, hit.voxel, std::clamp(command.radius, 1, MaxBrushRadius), command.shape);
    else if (command.place && hit.face != Cell{}) place(game, hit.voxel + hit.face);
}

} // namespace derivean
