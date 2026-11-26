#pragma once

#include "simulation/type/World.hpp"

#include <cstdint>
#include <vector>

namespace derivean {

inline constexpr int PatchCells = 32;                       // quads per side of every patch
inline constexpr int PatchSide = PatchCells + 1;
inline constexpr int PatchVertices = PatchSide * PatchSide;
inline constexpr double FinestCell = 1.0;                   // m, cell of level 0; doubles per level
inline constexpr int CoarsestLevel = 10;                    // 1024 m cells, the simulation's coarsest data
inline constexpr double SplitCells = 125.0;                 // a patch gives way to its children nearer than this many of its cells
inline constexpr double MorphStart = 0.75;                  // share of a patch's far range at which it starts blending to its parent
inline constexpr double ViewDistance = 260e3;               // m
inline constexpr int ShadeDensity = 2;                      // shading samples per cell edge
inline constexpr int ShadeSide = PatchCells * ShadeDensity + 1;
// A patch is refined only while that would move its surface by more than this share of its
// distance from the eye: about half a pixel of a 1440 pixel wide view.
inline constexpr double ErrorShare = 0.0004;

inline constexpr double cellOf(int level) { return FinestCell * double(1 << level); }
inline constexpr double sizeOf(int level) { return PatchCells * cellOf(level); }

// A node of the quadtree over the world-aligned lattice: it covers [x, x + 1) × [z, z + 1) patch sizes of its level.
struct PatchKey {
    int level{};
    int32_t x{}, z{};
    bool operator==(const PatchKey&) const = default;
};

struct PatchKeyHash {
    size_t operator()(PatchKey key) const {
        const uint64_t mixed = uint64_t(uint32_t(key.x)) * 0x9e3779b185ebca87ULL + uint64_t(uint32_t(key.z)) * 0xc2b2ae3d27d4eb4fULL
                             + uint64_t(key.level) * 0x165667b19e3779f9ULL;
        return size_t(mixed ^ (mixed >> 29));
    }
};

inline PatchKey parentOf(PatchKey key) { return {key.level + 1, key.x >> 1, key.z >> 1}; }
inline PatchKey childOf(PatchKey key, int index) { return {key.level - 1, key.x * 2 + (index & 1), key.z * 2 + (index >> 1)}; }

// Height field of one node, vertices row-major (z then x). Quads are split along the diagonal from
// their minimum to their maximum corner; `morph` is the parent's surface under the same vertex, so a
// patch blended fully to `morph` meets a coarser neighbour without a gap. Heights are metres above
// `base`: single precision is too coarse for anything measured from the world's origin.
struct FarPatch {
    uint32_t id{};
    PatchKey key;
    uint64_t ticket{};
    double base{};                  // m, elevation the per-vertex heights are measured from
    std::vector<float> heights;     // m above base
    std::vector<float> morph;       // m above base
    std::vector<int8_t> normals;    // xyz × 127
    std::vector<uint8_t> colors;    // sRGB, albedo of the material the voxels put on top there
    std::vector<float> water;       // m above base: level of standing water; NaN away from any
    // A raster for shading only, finer than the geometry: ShadeSide² texels of four bytes, texel
    // (ShadeDensity · i, ShadeDensity · j) on vertex (i, j). Bytes: normal x and z as n · 127 + 128,
    // from one-sided slopes so an edge stays an edge; the material id a voxel there shows; how much
    // sky the place sees, 255 on an open plain.
    std::vector<uint8_t> shade;
    float skirt{};                  // m the border may be dropped by to hide what blending cannot
    float low{}, high{};            // m against the datum, range of the ground
    float error{};                  // m the next finer level would move the surface by, where it shows
    uint32_t detail{};              // level of the data found at the corners and the middle, 3 bits each; 0 is the planet grid
    uint8_t dataLevel{};            // coarsest level any vertex was read from: 0 planet, then region, local, site, ground
    double milliseconds{};          // time the build took
};

struct FarDelta {
    std::vector<FarPatch> added;
    std::vector<uint32_t> removed;
};

// Finest simulation level a patch of this level reads: the one whose cells its shading samples
// can still tell apart. Finer data would only alias.
sim::Level sourceFor(int level);

// The simulation levels available at the corners and the middle of a node, packed like FarPatch::detail.
uint32_t detailAt(const sim::World& world, PatchKey key);

// Samples the finished tiles, and the planet's own grid where there are none yet; never blocks.
// Safe off the game thread.
FarPatch buildPatch(const sim::World& world, PatchKey key);

} // namespace derivean
