#pragma once

#include "simulation/type/World.hpp"
#include "voxel/type/Mesh.hpp"

namespace derivean {

// The far terrain is the voxel world at coarser cube sizes. Level k holds cubes of edge
// VoxelSize · 2^k on the lattice the voxels lie on, so a cube of one level is eight of the level
// below, and a level is drawn where its cubes are a few pixels wide.
//
// The resampling rule, the same wherever a level is made: a cube is solid if the world is solid at
// its centre, and is made of what is there. No average shaves a crest and nothing is fattened: a
// wall stays where it stands, and a feature thinner than a cube shows or vanishes by where the
// centres fall. For a column of ground this puts the top on the whole cube nearest the surface.
// Two things come before it. The top cube of a column is made of what covers the surface, however
// thin, because that is what the ground is from above. And standing water keeps its shore: a
// column under water ends below the water level and the dry column beside it at or above it, so
// rounding never floods land or lifts a lake bed out of its lake.
inline constexpr int CubeSide = 32;          // cubes along every edge of a chunk
inline constexpr int FinestCubes = 2;       // 0.5 m; the levels below are the voxels themselves
inline constexpr int CoarsestCubes = 13;    // 1024 m, the cell of the simulation's coarsest data
inline constexpr int CubeLevelCount = CoarsestCubes - FinestCubes + 1;

inline constexpr double edgeOf(int level) { return double(VoxelSize) * double(1 << level); }

// The chunks of a level are cut a column at a time: [x, x + 1) × [z, z + 1) chunk sizes, at every height.
struct CubeKey {
    int level{};
    int32_t x{}, z{};
    bool operator==(const CubeKey&) const = default;
};

// Standing water of one level inside a column: top faces in the layout of FaceMesh, to be drawn at
// `level` wherever their plane lies.
struct WaterSheet {
    double level{};   // m
    FaceMesh faces;
};

// One chunk: CubeSide cubes of its level along every edge, from `at` chunk sizes. Its faces count
// cubes from that corner, so no coordinate in them exceeds CubeSide.
struct CubeChunk {
    uint32_t id{};
    int level{};
    Cell at{};
    FaceMesh faces;
    std::vector<WaterSheet> water;   // of the whole column; carried by its highest chunk
    uint8_t dataLevel{};             // coarsest level any column was read from: 0 planet, then region, local, site, ground
};

struct CubeColumn {
    CubeKey key;
    uint64_t ticket{};
    std::vector<CubeChunk> chunks;   // those that hold a face, from the lowest
    uint64_t detail{};               // which data was there to read, as cubeDetail reports it
    size_t faces{};
    double milliseconds{};           // time the build took
};

// Finest simulation level the cubes of a level are cut from: the one whose cells they can still
// tell apart.
sim::Level cubeSource(int level);

// The data a column built now would read: the level found at the corners of the square its ground
// is read from, and on a grid over the square its water is read from. It differs from a built
// column's once finer tiles have finished anywhere the column reads.
uint64_t cubeDetail(const sim::World& world, CubeKey key);

// True when `detail` says the column's ground was read from its source level all over: the corners
// of the square it reads touch every tile under it, and with all of them finished nothing is eased
// towards coarser data.
bool cubeFinal(int level, uint64_t detail);

// Metres beyond its square that a column of chunks of `level` reads its source level's tiles over:
// the cubes its faces, shores and water sample, the cells easing looks across and, for the levels
// cut from the ground level, how far a stone reaches beyond the tile that holds it.
double cubeReadMargin(int level);

// Cuts a column of chunks from the finished tiles, and from the planet's own grid where there are
// none yet; never blocks. Safe off the game thread. A column of cubes depends on the world alone,
// so columns of chunks that share a border agree on it.
CubeColumn buildCubes(const sim::World& world, CubeKey key);

} // namespace derivean
