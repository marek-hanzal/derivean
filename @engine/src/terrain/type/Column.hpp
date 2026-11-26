#pragma once

#include "simulation/type/World.hpp"
#include "terrain/type/Edits.hpp"
#include "voxel/type/Mesh.hpp"

namespace derivean {

inline constexpr double ChunkMeters = ChunkSide * double(VoxelSize);

// Work order for one column of chunks. Everything the task reads travels with it.
struct ColumnRequest {
    int x{}, z{};                // chunk column
    uint64_t ticket{};
    bool surface{};              // materialise the band of chunks that holds the ground
    int low{}, high{-1};         // further chunk levels to materialise, inclusive; none when high < low
    bool mesh{};                 // nothing next to these chunks is edited: their faces can be built here too
    std::vector<std::pair<int, ChunkEdits>> edits;   // the column's player changes by chunk level; with the band, those levels are produced too
};

struct ColumnChunk {
    ChunkVoxels voxels;
    bool meshed{};
    FaceMesh faces;              // against the untouched world around the chunk, when `meshed`
};

struct ColumnResult {
    int x{}, z{};
    uint64_t ticket{};
    bool surface{};
    int low{}, high{-1};         // the ground band, when asked for
    int sky{};                   // first level from which the untouched column is air, when asked for the band
    std::vector<ColumnChunk> chunks;
    double milliseconds{};       // time the task took
};

// The untouched world's voxels for the request, with the request's edits applied. A pure function
// of the simulated world and the request: it waits for the site tiles its samples can reach, so the
// answer does not depend on what else happened to be built. Call it off the game thread.
ColumnResult generateColumn(sim::World& world, const ColumnRequest& request);

} // namespace derivean
