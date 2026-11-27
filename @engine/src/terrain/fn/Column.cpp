#include "terrain/type/Column.hpp"
#include "nature/type/SurfaceReader.hpp"
#include "planet/constant/Scale.hpp"
#include "simulation/fn/Query.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <map>

namespace derivean {
namespace {

using Slab = std::vector<Material>;   // one chunk of voxels indexed (y, z, x); empty stands for all air

constexpr double SurfaceReach = 2 * 0.25;   // m: the surface interpolation taps two ground cells past a point
// Stones stand on the ground surface, the largest nearly four metres high, and ledges and towers
// rise out of it. The band starts that much higher, and is still only trusted once it is closed.
constexpr double FormRise = 4.0;           // m
constexpr int MaxBandGrowth = 64;

int levelOf(double y) { return int(std::floor(y / ChunkMeters)); }

size_t slabIndex(Cell p) { return size_t((p.y * ChunkSide + p.z) * ChunkSide + p.x); }

// Untouched voxels of a box given in voxels, indexed (y, z, x).
Slab sample(sim::World& world, Bounds box) {
    const Cell size = box.size();
    Slab voxels(size_t(size.x) * size.y * size.z);
    sim::sampleBlock(world, {box.min.x * double(VoxelSize), box.min.y * double(VoxelSize), box.min.z * double(VoxelSize),
                             VoxelSize, size.x, size.y, size.z}, voxels.data());
    return voxels;
}

bool layerIs(const Slab& voxels, int y, bool air) {
    if (voxels.empty()) return air;
    const auto begin = voxels.begin() + size_t(y) * ChunkSide * ChunkSide;
    return std::all_of(begin, begin + ChunkSide * ChunkSide, [air](Material m) { return (m == Material::Air) == air; });
}

bool borderOpen(const Slab& voxels, Cell normal) {
    const int axis = normal.x ? 0 : normal.y ? 1 : 2;
    for (int v = 0; v < ChunkSide; ++v)
        for (int u = 0; u < ChunkSide; ++u) {
            Cell p{};
            p[axis] = normal[axis] > 0 ? ChunkSide - 1 : 0;
            p[(axis + 1) % 3] = u;
            p[(axis + 2) % 3] = v;
            if (voxels[slabIndex(p)] == Material::Air) return true;
        }
    return false;
}

ChunkVoxels compress(Cell key, const Slab& voxels) {
    ChunkVoxels chunk{key, {}, 0, 0x3f};
    if (voxels.empty()) return chunk;
    constexpr int side = ChunkSide / VoxelsPerMeter;
    for (int ty = 0; ty < side; ++ty)
        for (int tz = 0; tz < side; ++tz)
            for (int tx = 0; tx < side; ++tx) {
                TerrainTile::Detail detail;
                for (int y = 0; y < VoxelsPerMeter; ++y)
                    for (int z = 0; z < VoxelsPerMeter; ++z)
                        std::copy_n(voxels.begin() + slabIndex({tx * VoxelsPerMeter, ty * VoxelsPerMeter + y, tz * VoxelsPerMeter + z}),
                                    VoxelsPerMeter, detail.begin() + (y * VoxelsPerMeter + z) * VoxelsPerMeter);
                int occupied = 0;
                bool uniform = true;
                for (Material m : detail) {
                    occupied += m != Material::Air;
                    uniform &= m == detail.front();
                }
                if (!occupied) continue;
                TerrainTile tile;
                tile.occupied = occupied;
                if (uniform) tile.uniform = detail.front();
                else tile.detail = std::make_unique<TerrainTile::Detail>(detail);
                chunk.tiles.emplace_back(key * side + Cell{tx, ty, tz}, std::move(tile));
                chunk.occupied += size_t(occupied);
            }
    chunk.openFaces = 0;
    for (size_t face = 0; face < Neighbors.size(); ++face)
        if (borderOpen(voxels, Neighbors[face])) chunk.openFaces |= uint8_t(1u << face);
    return chunk;
}

// The untouched world a chunk's face reads beyond it: one voxel thick, and on the sides a voxel
// wider than the chunk, because the occlusion of a face at an edge looks at the cells diagonally
// across it.
Bounds faceBox(Bounds area, Cell normal) {
    Bounds face = area;
    const int axis = normal.x ? 0 : normal.y ? 1 : 2;
    face.min[axis] = normal[axis] > 0 ? area.max[axis] : area.min[axis] - 1;
    face.max[axis] = face.min[axis] + 1;
    if (!normal.y) {
        --face.min.y;
        ++face.max.y;
        if (normal.x) --face.min.z, ++face.max.z;
    }
    return face;
}

// The faces beside each meshed chunk, by level and in the order of the sideways Neighbors. A side
// has one footprint up the whole column, so each is read in one sampleBlocks call; every chunk's
// part is the block its face alone would read.
constexpr std::array<size_t, 4> Sideways{0, 1, 4, 5};
using Sides = std::map<int, std::array<Slab, Sideways.size()>>;
size_t sideOf(size_t neighbor) { return size_t(std::find(Sideways.begin(), Sideways.end(), neighbor) - Sideways.begin()); }

Sides sampleSides(sim::World& world, int x, int z, const std::vector<int>& levels) {
    Sides sides;
    if (levels.empty()) return sides;
    std::vector<sim::BlockSpan> spans(levels.size());
    for (size_t side = 0; side < Sideways.size(); ++side) {
        const Cell normal = Neighbors[Sideways[side]];
        for (size_t i = 0; i < levels.size(); ++i) {
            const Bounds face = faceBox(chunkBounds({x, levels[i], z}), normal);
            const Cell size = face.size();
            Slab& voxels = sides[levels[i]][side];
            voxels.resize(size_t(size.x) * size.y * size.z);
            spans[i] = {face.min.y * double(VoxelSize), size.y, voxels.data()};
        }
        const Bounds face = faceBox(chunkBounds({x, levels.front(), z}), normal);
        const Cell size = face.size();
        sim::sampleBlocks(world, {face.min.x * double(VoxelSize), 0, face.min.z * double(VoxelSize), VoxelSize, size.x, 0, size.z}, spans);
    }
    return sides;
}

// Faces of one chunk against what lies around it: the levels made here above and below, the
// untouched world everywhere else.
FaceMesh mesh(sim::World& world, const std::map<int, Slab>& slabs, const Sides& sides, Cell key, const Slab& voxels) {
    const Bounds area = chunkBounds(key);
    Voxels padded(area.padded(1));
    for (int y = 0; y < ChunkSide; ++y)
        for (int z = 0; z < ChunkSide; ++z)
            std::copy_n(voxels.begin() + slabIndex({0, y, z}), ChunkSide, padded.cells.begin() + padded.index(area.min + Cell{0, y, z}));
    for (size_t index = 0; index < Neighbors.size(); ++index) {
        const Cell normal = Neighbors[index];
        const Bounds face = faceBox(area, normal);
        const auto beside = normal.y ? slabs.find(key.y + normal.y) : slabs.end();
        if (beside != slabs.end()) {
            if (beside->second.empty()) continue;
            const int layer = normal.y > 0 ? 0 : ChunkSide - 1;
            for (int z = 0; z < ChunkSide; ++z)
                std::copy_n(beside->second.begin() + slabIndex({0, layer, z}), ChunkSide,
                            padded.cells.begin() + padded.index({area.min.x, face.min.y, area.min.z + z}));
            continue;
        }
        const Slab sampled = normal.y ? sample(world, face) : Slab{};
        const Slab& outside = normal.y ? sampled : sides.at(key.y)[sideOf(index)];
        const Cell size = face.size();
        for (int y = 0; y < size.y; ++y)
            for (int z = 0; z < size.z; ++z)
                std::copy_n(outside.begin() + size_t(y * size.z + z) * size.x, size.x,
                            padded.cells.begin() + padded.index(face.min + Cell{0, y, z}));
    }
    return meshFaces(padded, area);
}

} // namespace

ColumnResult generateColumn(sim::World& world, const ColumnRequest& request) {
    const auto start = std::chrono::steady_clock::now();
    const double x0 = request.x * ChunkMeters, z0 = request.z * ChunkMeters;
    for (double dz : {-SurfaceReach, ChunkMeters + SurfaceReach})
        for (double dx : {-SurfaceReach, ChunkMeters + SurfaceReach}) sim::surveyAt(world, {x0 + dx, z0 + dz}, sim::Level::Ground);

    ColumnResult result;
    result.x = request.x;
    result.z = request.z;
    result.ticket = request.ticket;
    result.surface = request.surface;
    std::map<int, Slab> slabs;
    const int floor = WorldFloorLevel;
    const auto untouched = [&](int level) { return sample(world, chunkBounds({request.x, level, request.z})); };
    if (request.surface) {
        SurfaceReader reader = surfaceReader(world, sim::Level::Ground);
        double lowest = 1e30, highest = -1e30;
        for (int j = 0; j < ChunkSide; ++j)
            for (int i = 0; i < ChunkSide; ++i) {
                const double ground = surfaceAt(reader, {x0 + (i + 0.5) * VoxelSize, z0 + (j + 0.5) * VoxelSize}).ground;
                lowest = std::min(lowest, ground);
                highest = std::max(highest, ground);
            }
        result.low = std::max(floor, levelOf(lowest) - 1);
        result.high = std::max(result.low, levelOf(highest + FormRise));
        for (int level = result.low; level <= result.high; ++level) slabs[level] = untouched(level);
        // The band is only right once it is closed by air above and rock below.
        for (int grown = 0; grown < MaxBandGrowth && !layerIs(slabs[result.high], ChunkSide - 1, true); ++grown) {
            ++result.high;
            slabs[result.high] = untouched(result.high);
        }
        for (int grown = 0; grown < MaxBandGrowth && result.low > floor && !layerIs(slabs[result.low], 0, false); ++grown) {
            --result.low;
            slabs[result.low] = untouched(result.low);
        }
        result.sky = result.high + 1;
    }

    const auto ensure = [&](int level) -> Slab& {
        const auto found = slabs.find(level);
        if (found != slabs.end()) return found->second;
        const bool open = level < floor || (request.surface && level >= result.sky);
        return slabs[level] = open ? Slab{} : untouched(level);
    };
    for (int level = request.low; level <= request.high; ++level) ensure(level);
    for (const auto& [level, edits] : request.edits) {
        if (!request.surface && (level < request.low || level > request.high)) continue;
        Slab& voxels = ensure(level);
        if (voxels.empty()) voxels.assign(ChunkVolume, Material::Air);
        for (size_t i = 0; i < edits.size(); ++i)
            if (edits[i] != Untouched) voxels[i] = Material(edits[i]);
    }

    std::vector<int> meshed;
    for (const auto& [level, voxels] : slabs) {
        result.chunks.push_back({compress({request.x, level, request.z}, voxels), request.mesh, {}});
        if (request.mesh && result.chunks.back().voxels.occupied) meshed.push_back(level);
    }
    const Sides sides = sampleSides(world, request.x, request.z, meshed);
    for (ColumnChunk& chunk : result.chunks)
        if (sides.contains(chunk.voxels.key.y)) chunk.faces = mesh(world, slabs, sides, chunk.voxels.key, slabs.at(chunk.voxels.key.y));
    result.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return result;
}

} // namespace derivean
