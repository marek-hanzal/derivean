#pragma once

#include "game/type/Game.hpp"

namespace derivean {

// Natural water and ground under the player from the finished simulation tiles; never blocks.
struct Footing {
    std::optional<double> ground;      // m
    std::optional<double> water;       // m, surface of water standing over the ground
};

Footing footingAt(const Game& game, Position point);

// What the player's body does this step, from the command and the world around it.
PlayerMove playerMove(const Game& game, const Command& command, double scale, const Footing& footing);

// True while a player on foot must wait: the ground around is unknown or its colliders are missing.
bool awaitsGround(const Game& game);

// Lifts a player whose body overlaps solid voxels straight up to the first place it fits.
void resolveOverlap(Game& game);

// Where a new player stands: at the wished x and z, else at the simulation's landing on a shore, or,
// where that lies under water, on the first dry ground a widening search round it finds. Stands on
// the site level, which can lie a metre off the ground level the voxels bring in later. Blocks on
// the simulation tiles it reads, down to the site level.
Position findSpawn(sim::World& world, const std::optional<std::array<double, 2>>& wish);

} // namespace derivean
