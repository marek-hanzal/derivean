#include "game/type/Player.hpp"
#include "simulation/fn/Query.hpp"

#include <cmath>

namespace derivean {
namespace {

constexpr float WalkSpeed = 4.6f, SprintSpeed = 7.2f;   // m/s
constexpr float BoostOnFoot = 2.5f;
constexpr float FlySpeed = 12.0f, BoostFlySpeed = 250.0f;
constexpr double BoostHeight = 200.0;      // m above ground that add the boost speed once more
constexpr double BoostLimit = 20.0;
constexpr double FloatDepth = 1.0;         // m of water over the feet at which the body starts to float
constexpr double FloatRange = 0.4;         // m further down it is carried entirely
constexpr float MaxLift = 1.6f;
constexpr double BedDepth = 1.0;           // m under the bed of natural water down to which a body still counts as in it
constexpr double SettledClearance = 10.0;  // m above surveyed ground at which the player is plainly in the air
constexpr double RoughClearance = 150.0;   // the same while only coarse tiles describe the ground
constexpr double BodySlack = 0.04;         // m of overlap the character controller leaves at rest
constexpr double StandClearance = 0.02;    // m above a voxel top the feet are put

// Voxel distance to the capsule's axis decides the overlap; the box around the body would also
// catch the steps of a slope the rounded foot clears.
bool overlaps(const VoxelWorld& world, Position feet) {
    const double radius = PlayerRadius, reach = radius - BodySlack;
    const double bottom = feet[1] + radius, top = feet[1] + PlayerHeight - radius;
    const Cell min = cellAt({feet[0] - radius, feet[1], feet[2] - radius});
    const Cell max = cellAt({feet[0] + radius, feet[1] + PlayerHeight, feet[2] + radius});
    for (int y = min.y; y <= max.y; ++y)
        for (int z = min.z; z <= max.z; ++z)
            for (int x = min.x; x <= max.x; ++x) {
                if (materialAt(world, {x, y, z}) == Material::Air) continue;
                const double dx = std::max({x * double(VoxelSize) - feet[0], feet[0] - (x + 1) * double(VoxelSize), 0.0});
                const double dz = std::max({z * double(VoxelSize) - feet[2], feet[2] - (z + 1) * double(VoxelSize), 0.0});
                const double dy = std::max({y * double(VoxelSize) - top, bottom - (y + 1) * double(VoxelSize), 0.0});
                if (dx * dx + dy * dy + dz * dz < reach * reach) return true;
            }
    return false;
}

// 1 when every chunk the body touches is authoritative, 0 when one is rock waiting below a known
// surface, -1 when nothing is known about one yet.
int bodyKnowledge(const VoxelWorld& world, Position feet) {
    const Cell min = chunkOf(cellAt({feet[0] - PlayerRadius, feet[1], feet[2] - PlayerRadius}));
    const Cell max = chunkOf(cellAt({feet[0] + PlayerRadius, feet[1] + PlayerHeight, feet[2] + PlayerRadius}));
    int knowledge = 1;
    for (int y = min.y; y <= max.y; ++y)
        for (int z = min.z; z <= max.z; ++z)
            for (int x = min.x; x <= max.x; ++x) {
                if (known(world, {x, y, z})) continue;
                if (!world.streamed->sky.contains({x, 0, z})) return -1;
                knowledge = 0;
            }
    return knowledge;
}

bool clearOfGround(const Game& game, Position feet) {
    double highest = -1e30;
    bool surveyed = true;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            const sim::SurfaceSample sample = sim::surfaceAt(game.nature.world, {feet[0] + dx * ChunkMeters, feet[2] + dz * ChunkMeters});
            if (!sample.found) return false;
            surveyed &= sample.level >= sim::Level::Site;
            highest = std::max(highest, double(sample.ground));
        }
    return feet[1] - highest > (surveyed ? SettledClearance : RoughClearance);
}

} // namespace

Footing footingAt(const Game& game, Position point) {
    Footing footing;
    const sim::SurfaceSample sample = sim::surfaceAt(game.nature.world, {point[0], point[2]}, sim::Level::Ground);
    if (!sample.found) return footing;
    footing.ground = sample.ground;
    if (sample.water > sample.ground) footing.water = sample.water;
    return footing;
}

PlayerMove playerMove(const Game& game, const Command& command, double scale, const Footing& footing) {
    PlayerMove move;
    move.jump = game.jumpQueued;
    const Position feet = game.physics.feet();
    if (game.flying) {
        move.mode = PlayerMove::Mode::Fly;
        const Position look = command.direction;
        const double length = std::hypot(look[0], look[1], look[2]), flat = std::hypot(look[0], look[2]);
        Position thrust{0, double(command.ascend), 0};
        if (length > 1e-6 && flat > 1e-6) {
            // The command's x and z are world axes; split them into along and across the view.
            const double ahead = (command.x * look[0] + command.z * look[2]) / flat;
            const double across = (command.x * -look[2] + command.z * look[0]) / flat;
            for (int axis = 0; axis < 3; ++axis) thrust[axis] += ahead * look[axis] / length;
            thrust[0] += across * -look[2] / flat;
            thrust[2] += across * look[0] / flat;
        }
        const double height = footing.ground ? std::max(0.0, feet[1] - *footing.ground) : 0.0;
        const double speed = command.boost ? BoostFlySpeed * std::min(BoostLimit, 1 + height / BoostHeight) : FlySpeed;
        const double gain = speed / std::max(1.0, std::hypot(thrust[0], thrust[1], thrust[2]));
        for (int axis = 0; axis < 3; ++axis) move.velocity[size_t(axis)] = float(thrust[axis] * gain);
        return move;
    }

    if (game.held) move.mode = PlayerMove::Mode::Hold;
    const Position wading{feet[0], feet[1] + 0.035, feet[2]};
    double depth = waterDepth(game.water, wading).value_or(0);
    // Natural water stands over its bed. A body well under the bed is in a dug-out space, which the sea does not enter.
    if (footing.water && feet[1] > *footing.ground - BedDepth) {
        const double natural = std::max(0.0, *footing.water - feet[1]);
        depth = std::max(depth, natural);
        move.lift = float(std::clamp((natural - FloatDepth) / FloatRange, 0.0, double(MaxLift)));
    }
    const float drag = std::max(0.4f, 1 - 0.5f * float(depth));
    const float speed = (command.sprint ? SprintSpeed : WalkSpeed) * (command.boost ? BoostOnFoot : 1.0f);
    move.velocity = {float(command.x / scale) * drag * speed, 0, float(command.z / scale) * drag * speed};
    return move;
}

bool awaitsGround(const Game& game) {
    if (game.flying) return false;
    const Position feet = game.physics.feet();
    const Cell here = chunkOf(cellAt({feet[0], feet[1] + 0.5 * PlayerHeight, feet[2]}));
    bool settled = true;
    for (int y = -1; y <= 1 && settled; ++y)
        for (int z = -1; z <= 1 && settled; ++z)
            for (int x = -1; x <= 1 && settled; ++x) {
                const Cell key = here + Cell{x, y, z};
                settled = known(game.world, key) && (!game.world.chunks.contains(key) || game.terrain.contains(key));
            }
    return !settled && !clearOfGround(game, feet);
}

void resolveOverlap(Game& game) {
    Position feet = game.physics.feet();
    if (bodyKnowledge(game.world, feet) != 1 || !overlaps(game.world, feet)) return;
    // Straight up, one voxel top at a time: the first place the body fits is in the nearest
    // open space above, which is the pocket the player is in before it is the surface.
    while (true) {
        feet[1] = (std::floor(feet[1] / VoxelSize) + 1) * VoxelSize + StandClearance;
        const int knowledge = bodyKnowledge(game.world, feet);
        if (knowledge < 0) return;
        if (knowledge == 0) {
            feet[1] = (std::floor(feet[1] / ChunkMeters) + 1) * ChunkMeters - VoxelSize;
            continue;
        }
        if (overlaps(game.world, feet)) continue;
        game.physics.returnPlayer(feet);
        return;
    }
}

} // namespace derivean
