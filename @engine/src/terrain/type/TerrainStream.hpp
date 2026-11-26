#pragma once

#include "nature/type/Mailbox.hpp"
#include "terrain/type/Column.hpp"

#include <deque>

namespace derivean {

struct Nature;

inline constexpr int LoadRadius = 13;        // chunk columns materialised around the player
inline constexpr int KeepRadius = 15;        // columns farther than this are dropped
inline constexpr double LoadHeight = 64.0;   // m above a column's ground within which its voxels matter
inline constexpr double KeepHeight = 96.0;
inline constexpr int ReachRadius = 2;        // chunks around the player kept materialised at any depth: what a brush can touch

struct StreamColumn {
    uint64_t ticket{};           // the task whose result this column accepts; 0 when none is in flight
    bool resolved{};             // the ground band is in the world and the column's sky is known
    int low{}, high{-1};         // the ground band
    float ground{};              // m, near the column's middle
    std::vector<int> levels;     // loaded chunk levels
};

struct LoadedChunk {
    Cell key;
    uint8_t openFaces{};
    bool occupied{};
    bool meshed{};               // `faces` came with the chunk, built against the untouched world around it
    FaceMesh faces;
};

// Keeps the voxel world materialised around the player from the simulated one. Columns are
// generated off the game thread and enter the world here, a bounded amount per frame.
struct TerrainStream {
    std::unordered_map<Cell, StreamColumn, CellHash> columns;   // by {x, 0, z}
    std::shared_ptr<Mailbox<ColumnResult>> finished;
    std::deque<ColumnResult> arrived;        // generated, waiting for their turn to enter the world
    std::vector<Cell> backlog;               // columns the last plan wants, farthest first
    std::vector<LoadedChunk> loaded;         // entered the world since the projection last drained this
    std::vector<Cell> dropped;               // left the world since the projection last drained this
    Cell center{};
    bool planned{};                          // false while the last plan left work for the next frame
    int calls{};
    size_t flying{};                         // tasks in flight
    size_t missing{};                        // columns wanted and not yet resolved
    uint64_t tickets{};
    double generationMs{};                   // background time of the columns that entered this frame
    size_t builtColumns{}, builtChunks{};    // totals since the start, with the background time they took
    double builtMs{};
};

// One frame of streaming around the player's feet. `speed` is the player's in m/s.
void advanceStream(TerrainStream& stream, Nature& nature, VoxelWorld& world, const EditStore& edits,
                   Position feet, double speed);

// Tells the stream that an edit materialised or touched `chunk`, so it is tracked with its column.
void noteEdited(TerrainStream& stream, Cell chunk);

} // namespace derivean
