#pragma once

#include "game/type/Game.hpp"

namespace derivean {

void streamColliders(Game& game);
void markTerrainChanged(Game& game, std::span<const Cell> changed);

// Brings what the renderer holds up to date: chunks that entered or left the world, chunks an edit
// touched, water meshes near the eye, and the box the voxels fill.
void projectScene(Game& game, Frame& frame);

// Fills the frame's clock, sun, weather at the player and, when there is a new one, the cloud
// snapshot. `ground` is the simulated ground under the player in metres, if known.
void projectSky(Game& game, Frame& frame, std::optional<double> ground);

} // namespace derivean
