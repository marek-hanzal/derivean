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
// touchesTerrain, and where the cube touches, reachesUnloaded. Bedrock reads as Unloaded too, so a
// touch alone does not tell ground that is not loaded apart. Inline: it runs on every sample of
// every parcel's sweep, where a call of its own shows.
enum class Contact { Clear, Solid, Unloaded };
inline Contact probeTerrain(const VoxelWorld& world, Position point, double halfSize) {
    if (!touchesTerrain(world, point, halfSize)) return Contact::Clear;
    return reachesUnloaded(world, point, halfSize) ? Contact::Unloaded : Contact::Solid;
}
std::optional<Position> recoverFromTerrain(const VoxelWorld& world, Position point, double halfSize);
int sweepSamples(Position from, Position to);

struct MovingContact {
    double fraction;
    Position normal;
    Position velocity;
};

Position rebound(Position velocity, const MovingContact& contact, double restitution);

} // namespace derivean
