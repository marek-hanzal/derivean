#include "simulation/type/Simulation.hpp"
#include "demo/type/ProvingGrounds.hpp"
#include "simulation/type/TerrainEdits.hpp"
#include "simulation/type/WorldProjection.hpp"

#include <chrono>
#include <cmath>

namespace derivean {
namespace {

constexpr double FixedStep = 1.0 / 120.0;

Position feet(const Simulation& game) {
    Position point = game.physics.eye();
    point[1] -= PlayerEyeHeight - 0.035;
    return point;
}

bool canStand(const VoxelWorld& world, Position position) {
    if (materialAt(world, cellAt({position[0], position[1] - VoxelSize, position[2]})) == Material::Air)
        return false;
    const Cell min = cellAt({position[0] - PlayerRadius, position[1], position[2] - PlayerRadius});
    const Cell max = cellAt({position[0] + PlayerRadius, position[1] + PlayerHeight, position[2] + PlayerRadius});
    for (int y = min.y; y <= max.y; ++y)
        for (int z = min.z; z <= max.z; ++z)
            for (int x = min.x; x <= max.x; ++x)
                if (materialAt(world, {x, y, z}) != Material::Air) return false;
    return true;
}

std::optional<Position> standingAbove(const VoxelWorld& world, Cell floor) {
    Position position = cellCenter(floor);
    position[1] = (floor.y + 1) * VoxelSize + Spawn[1];
    return canStand(world, position) ? std::optional(position) : std::nullopt;
}

std::optional<Position> respawnPosition(const VoxelWorld& world) {
    const Cell floor = cellAt({Spawn[0], Spawn[1] - VoxelSize, Spawn[2]});
    if (canStand(world, Spawn)) return Spawn;
    for (int radius = 4; radius <= 24; radius += 4) {
        for (int z = -radius; z <= radius; z += 4)
            for (int x = -radius; x <= radius; x += 4) {
                if (std::max(std::abs(x), std::abs(z)) != radius) continue;
                if (auto position = standingAbove(world, floor + Cell{x, 0, z})) return position;
            }
    }
    if (world.occupiedLevels.empty()) return std::nullopt;
    const int top = world.occupiedLevels.rbegin()->first;
    const int tileY = floorDiv(top, VoxelsPerMeter);
    for (const auto& [tile, _] : world.tiles) {
        if (tile.y != tileY) continue;
        for (int z = 0; z < VoxelsPerMeter; ++z)
            for (int x = 0; x < VoxelsPerMeter; ++x) {
                const Cell point = tile * VoxelsPerMeter + Cell{x, 0, z};
                if (auto position = standingAbove(world, {point.x, top, point.z})) return position;
            }
    }
    return std::nullopt;
}

void retireDebris(Simulation& game, Frame& frame) {
    const double playerY = feet(game)[1];
    std::erase_if(game.bodies, [&](const DebrisBody& body) {
        const double top = game.physics.verticalExtent(body.id).second;
        if (!belowTerrain(game.world, top) || top >= playerY) return false;
        frame.retired.push_back(body.id.GetIndexAndSequenceNumber());
        game.physics.removeBody(body.id);
        game.debrisEscapedKg += body.mass;
        return true;
    });
}

void move(Simulation& game, const Command& command, double scale) {
    streamColliders(game);
    const auto submerged = waterDepth(game.water, feet(game));
    const float drag = 1 - 0.5f * submerged.value_or(0);
    game.physics.movePlayer(float(FixedStep), float(command.x / scale) * std::max(0.4f, drag),
                            float(command.z / scale) * std::max(0.4f, drag), game.jumpQueued, command.sprint);
    game.jumpQueued = false;

    double lowestMoving = feet(game)[1];
    for (const auto& body : game.bodies) lowestMoving = std::min(lowestMoving, game.physics.verticalExtent(body.id).first);
    advanceWater(game.water, game.world, FixedStep, &game.physics, lowestMoving);
    stepSand(game.sand, game.world, game.water, FixedStep, game.landed, &game.physics, lowestMoving);
    if (!game.landed.empty()) {
        markTerrainChanged(game, game.landed);
        game.landed.clear();
    }
    game.accumulator -= FixedStep;
}

Bounds particleView(Position eye) {
    const Cell center = chunkOf(cellAt(eye));
    const Cell spread{Visibility, Visibility, Visibility};
    return {chunkBounds(center - spread).min, chunkBounds(center + spread).max};
}

} // namespace

Simulation::Simulation() {
    buildProvingGrounds(world, water);
    physics.returnPlayer(Spawn);
}

Frame advance(Simulation& game, const Command& command) {
    const auto start = std::chrono::steady_clock::now();
    Frame frame{};
    streamColliders(game);
    if (command.active) {
        if ((command.dig || command.place)) {
            if (const auto hit = game.physics.aim(command.direction)) editTerrain(game, frame, *hit, command);
            streamColliders(game);
        }
        game.jumpQueued |= command.jump;
        game.accumulator += std::clamp(command.elapsed, 0.0, 0.1);
        const double scale = std::max(1.0, std::hypot(double(command.x), double(command.z)));
        while (game.accumulator + 1e-9 >= FixedStep) move(game, command, scale);
        const double eyeY = game.physics.eye()[1];
        bool playerLost = belowTerrain(game.world, eyeY + 2);
        for (const auto& body : game.bodies) {
            const auto [bottom, top] = game.physics.verticalExtent(body.id);
            if (!belowTerrain(game.world, top) && bottom < eyeY) playerLost = false;
        }
        if (playerLost) {
            if (const auto position = respawnPosition(game.world)) game.physics.returnPlayer(*position);
        }
        retireDebris(game, frame);
        streamColliders(game);
        if (const auto hit = game.physics.aim(command.direction); hit && hit->terrain)
            frame.target = Target{hit->voxel, materialAt(game.world, hit->voxel)};
    } else {
        game.jumpQueued = false;
        game.accumulator = 0;
    }

    projectScene(game, frame);
    frame.eye = game.physics.eye();
    const Bounds view = particleView(frame.eye);
    projectWaterParticles(frame.particles, game.water, view);
    projectSand(frame.particles, game.sand, view);
    frame.water = game.water.budget;
    frame.waterDepth = waterDepth(game.water, feet(game));
    for (const auto& body : game.bodies)
        frame.debris.push_back({body.id.GetIndexAndSequenceNumber(), game.physics.pose(body.id)});
    frame.editedTiles = game.editedTiles.size();
    frame.visibleChunks = game.visible.size();
    frame.bodies = game.bodies.size();
    frame.sandAirborne = game.sand.grains.size();
    frame.sandDeposited = game.sand.deposited;
    frame.sandEscaped = game.sand.escaped;
    frame.debrisEscapedKg = game.debrisEscapedKg;
    frame.excavatedKg = game.excavatedKg;
    frame.buildStock = game.buildStock;
    frame.nativeMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return frame;
}

} // namespace derivean
