#pragma once

#include "simulation/type/BuildKey.hpp"
#include "voxel/type/Grid.hpp"

#include <array>
#include <cstdint>
#include <future>
#include <vector>

namespace derivean {

struct Nature;

// The ground level feeds the voxels as well as the finest far cubes: it is wanted at least this far
// around the eye, and up to this clearance above the ground, above which nobody stands on the voxels soon.
inline constexpr double GroundReach = 96.0, GroundCeiling = 150.0;   // m

struct TileRequest {
    sim::BuildKey key;
    uint64_t ticket;
    std::future<void> completion;
};

// Simulation tiles the engine asked for and is still waiting on. Only tiles whose dependencies
// are all finished are asked for, and ground tiles only once their groundwater exists, so each
// request is one build. The current plan's budget counts flying requests. After replanning, queued
// work no longer needed is removed from the worker pool. Claimed work finishes with its key and
// future in retired. If a later plan wants the build, it counts against that plan again.
struct TileRequests {
    std::vector<TileRequest> flying, retired;
    std::array<bool, sim::LevelCount> ready{};   // by sim::Level: every tile wanted at that level is finished
    std::array<uint32_t, sim::LevelCount> wanted{}, finished{};   // by sim::Level: tiles the view wants and how many of them are built
    // By sim::Level: metres around the eye over which tiles are wanted. The game sets them to what the
    // far cubes read (cubeDataReach), so readiness waits for no tile the view does not read and every
    // tile it reads is asked for.
    std::array<double, sim::LevelCount> reach{};
    // By sim::Level: metres of clearance above the ground over which the level is not worth building.
    // The region is wanted from any height; the game sets the finer levels to where the far cubes cut
    // from them rest (cubeCeiling).
    std::array<double, sim::LevelCount> ceiling{0, 1e9, 0, 0, 0};
    Position planned{};
    int calls{};
};

// How many requests may be in flight on a pool of `workers`: the player's own place may use every
// worker, the rest of the view leaves a share of them free, but never the only one.
struct RequestBudget {
    size_t own, shared;
};
RequestBudget requestBudget(size_t workers);

// Asks for the tiles the view around `eye` gains most from. `velocity` is the eye's in m/s: what
// lies ahead of a fast flight counts as nearer. `clearance` is the height above ground in metres;
// fine tiles are pointless from far above.
void requestTiles(Nature& nature, Position eye, Position velocity, double clearance);

} // namespace derivean
