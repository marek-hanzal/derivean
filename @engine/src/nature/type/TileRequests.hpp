#pragma once

#include "landscape/type/Level.hpp"
#include "voxel/type/Grid.hpp"

#include <array>
#include <vector>

namespace derivean {

struct Nature;

// Simulation tiles the engine asked for and is still waiting on. As many are in flight as the
// simulation has workers and no more: a queued build cannot be cancelled, so what is queued must
// still be worth having when its turn comes.
struct TileRequests {
    std::vector<sim::TileKey> flying;
    std::array<bool, sim::LevelCount> ready{};   // by sim::Level: every tile wanted at that level is finished
    std::array<uint32_t, sim::LevelCount> wanted{}, finished{};   // by sim::Level: tiles the view wants and how many of them are built
    Position planned{};
    int calls{};
};

// How far around the eye each level is wanted, in metres: the region level over all that can be
// seen, each finer level over what its cells can still be told apart in.
inline constexpr double RegionReach = 330e3, LocalReach = 64e3, SiteReach = 3000.0, GroundReach = 96.0;

// Asks for the tiles the view around `eye` gains most from. `velocity` is the eye's in m/s: what
// lies ahead of a fast flight counts as nearer. `clearance` is the height above ground in metres;
// fine tiles are pointless from far above.
void requestTiles(Nature& nature, Position eye, Position velocity, double clearance);

} // namespace derivean
