#pragma once

#include "world/type/VoxelWorld.hpp"

#include <optional>

namespace derivean {

Cell cellAt(Position position);
Position cellCenter(Cell cell);
Position interpolate(Position from, Position to, double fraction);
bool touchesTerrain(const VoxelWorld& world, Position point, double halfSize);
// True where the cube reaches into ground that is not loaded: there is no surface there to land on yet.
bool reachesUnloaded(const VoxelWorld& world, Position point, double halfSize);
std::optional<Position> recoverFromTerrain(const VoxelWorld& world, Position point, double halfSize);
int sweepSamples(Position from, Position to);

struct MovingContact {
    double fraction;
    Position normal;
    Position velocity;
};

Position rebound(Position velocity, const MovingContact& contact, double restitution);

} // namespace derivean
