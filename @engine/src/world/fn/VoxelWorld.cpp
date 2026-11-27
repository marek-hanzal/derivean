#include "world/type/VoxelWorld.hpp"

namespace derivean {
namespace {

constexpr int TileSide = TerrainChunk::TileSide;

size_t tileIndex(Cell offset) {
    return size_t((offset.y * VoxelsPerMeter + offset.z) * VoxelsPerMeter + offset.x);
}

Material at(const TerrainTile& tile, Cell offset) {
    return tile.detail ? (*tile.detail)[tileIndex(offset)] : tile.uniform;
}

// A point of a chunk: its tile there and its offset in that tile.
struct Spot {
    size_t tile;
    Cell offset;
};

Spot spotOf(Cell chunk, Cell point) {
    const Cell local = point - chunk * ChunkSide;
    const Cell tile{local.x / VoxelsPerMeter, local.y / VoxelsPerMeter, local.z / VoxelsPerMeter};
    return {size_t((tile.y * TileSide + tile.z) * TileSide + tile.x), local - tile * VoxelsPerMeter};
}

TerrainChunk* chunkAt(const VoxelWorld& world, Cell key) {
    // The world is read as const; the cache hands out what the caller is allowed to have.
    return cachedFind(world.found, const_cast<VoxelWorld&>(world).chunks, key);
}

void erase(VoxelWorld& world, Cell key) {
    forget(world.found, key);
    world.chunks.erase(key);
}

// The voxel as stored: air wherever no chunk holds matter, loaded or not.
Material storedAt(const VoxelWorld& world, Cell point) {
    const Cell key = chunkOf(point);
    const TerrainChunk* chunk = chunkAt(world, key);
    if (!chunk) return Material::Air;
    const Spot spot = spotOf(key, point);
    return at(chunk->tiles[spot.tile], spot.offset);
}

} // namespace

bool known(const VoxelWorld& world, Cell chunk) {
    if (!world.streamed || chunk.y < world.voidLevel) return true;
    const StreamedExtent& extent = *world.streamed;
    if (extent.loaded.contains(chunk)) return true;
    const auto sky = extent.sky.find({chunk.x, 0, chunk.z});
    return sky != extent.sky.end() && chunk.y >= sky->second;
}

Material materialAt(const VoxelWorld& world, Cell point) {
    const Cell key = chunkOf(point);
    const TerrainChunk* chunk = chunkAt(world, key);
    if (!chunk) return known(world, key) ? Material::Air : Unloaded;
    const Spot spot = spotOf(key, point);
    return at(chunk->tiles[spot.tile], spot.offset);
}

Voxels sampleWorld(const VoxelWorld& world, Bounds bounds) {
    Voxels result(bounds);
    if (world.streamed) {
        const Cell low = chunkOf(bounds.min), high = chunkOf(bounds.max - Cell{1, 1, 1});
        for (int y = low.y; y <= high.y; ++y)
            for (int z = low.z; z <= high.z; ++z)
                for (int x = low.x; x <= high.x; ++x) {
                    if (known(world, {x, y, z})) continue;
                    const Bounds region = intersection(bounds, chunkBounds({x, y, z}));
                    for (int rowY = region.min.y; rowY < region.max.y; ++rowY)
                        for (int rowZ = region.min.z; rowZ < region.max.z; ++rowZ)
                            std::fill_n(result.cells.begin() + result.index({region.min.x, rowY, rowZ}),
                                        region.max.x - region.min.x, Unloaded);
                }
    }
    const Cell low = chunkOf(bounds.min), high = chunkOf(bounds.max - Cell{1, 1, 1});
    for (int cy = low.y; cy <= high.y; ++cy)
        for (int cz = low.z; cz <= high.z; ++cz)
            for (int cx = low.x; cx <= high.x; ++cx) {
                const Cell key{cx, cy, cz};
                const auto chunk = world.chunks.find(key);
                if (chunk == world.chunks.end()) continue;
                const Bounds part = intersection(bounds, chunkBounds(key));
                const Cell first = tileOf(part.min), last = tileOf(part.max - Cell{1, 1, 1});
                for (int y = first.y; y <= last.y; ++y)
                    for (int z = first.z; z <= last.z; ++z)
                        for (int x = first.x; x <= last.x; ++x) {
                            const Cell origin = Cell{x, y, z} * VoxelsPerMeter;
                            const TerrainTile& tile = chunk->second.tiles[spotOf(key, origin).tile];
                            if (!tile.occupied) continue;
                            const Bounds region = intersection(part, {origin, origin + Cell{VoxelsPerMeter, VoxelsPerMeter, VoxelsPerMeter}});
                            for (int rowY = region.min.y; rowY < region.max.y; ++rowY)
                                for (int rowZ = region.min.z; rowZ < region.max.z; ++rowZ) {
                                    const Cell start{region.min.x, rowY, rowZ};
                                    auto output = result.cells.begin() + result.index(start);
                                    const int count = region.max.x - region.min.x;
                                    if (tile.detail)
                                        std::copy_n(tile.detail->begin() + tileIndex(start - origin), count, output);
                                    else
                                        std::fill_n(output, count, tile.uniform);
                                }
                        }
            }
    return result;
}

void setVoxel(VoxelWorld& world, Cell point, Material material) {
    const Cell key = chunkOf(point);
    TerrainChunk* found = chunkAt(world, key);
    if (!found) {
        // Rock that is not materialised cannot be edited; open sky becomes a loaded chunk on first use.
        if (material == Material::Air || !known(world, key)) return;
        if (world.streamed) world.streamed->loaded.insert(key);
        forget(world.found, key);
        found = &world.chunks.try_emplace(key).first->second;
    }

    TerrainChunk& chunk = *found;
    const Spot spot = spotOf(key, point);
    TerrainTile& tile = chunk.tiles[spot.tile];
    const Material before = at(tile, spot.offset);
    if (before == material) return;
    if (world.streamed) world.streamed->journal.push_back({point, before, material});
    if (before == Material::Air) {
        ++chunk.occupied;
        ++tile.occupied;
    } else if (material == Material::Air) {
        --chunk.occupied;
        if (--tile.occupied == 0) {
            if (chunk.occupied == 0) erase(world, key);
            else tile = TerrainTile{};
            return;
        }
    }

    if (!tile.detail) {
        tile.detail = std::make_unique<TerrainTile::Detail>();
        tile.detail->fill(tile.uniform);
    }
    (*tile.detail)[tileIndex(spot.offset)] = material;
    if (tile.occupied == TerrainTile::CellCount &&
        std::all_of(tile.detail->begin(), tile.detail->end(), [material](Material m) { return m == material; })) {
        tile.uniform = material;
        tile.detail.reset();
    }
}

void fillBox(VoxelWorld& world, Bounds area, Material material) {
    for (int y = area.min.y; y < area.max.y; ++y)
        for (int z = area.min.z; z < area.max.z; ++z)
            for (int x = area.min.x; x < area.max.x; ++x) setVoxel(world, {x, y, z}, material);
}

bool removeVoxel(VoxelWorld& world, Cell point) {
    if (storedAt(world, point) == Material::Air) return false;
    setVoxel(world, point, Material::Air);
    return true;
}

std::vector<Cell> carve(VoxelWorld& world, Cell center, int radius, BrushShape shape, double* excavatedKg) {
    std::vector<Cell> changed;
    const int inner = radius - 1;
    for (int y = -inner; y <= inner; ++y)
        for (int z = -inner; z <= inner; ++z)
            for (int x = -inner; x <= inner; ++x) {
                if (shape == BrushShape::Sphere && x * x + y * y + z * z >= radius * radius) continue;
                const Cell point = center + Cell{x, y, z};
                const Material material = storedAt(world, point);
                if (material == Material::Air) continue;
                if (excavatedKg) *excavatedKg += materialInfo(material).density * double(VoxelVolume);
                setVoxel(world, point, Material::Air);
                changed.push_back(point);
            }
    return changed;
}

bool inVoid(const VoxelWorld& world, double highestPoint) {
    return highestPoint < world.voidLevel * (ChunkSide * double(VoxelSize));
}

void loadChunk(VoxelWorld& world, ChunkVoxels&& chunk) {
    world.streamed->loaded.insert(chunk.key);
    if (!chunk.occupied) return;
    forget(world.found, chunk.key);
    TerrainChunk& into = world.chunks[chunk.key];
    into.occupied = chunk.occupied;
    for (auto& [key, tile] : chunk.tiles) into.tiles[spotOf(chunk.key, key * VoxelsPerMeter).tile] = std::move(tile);
}

void unloadChunk(VoxelWorld& world, Cell chunk) {
    world.streamed->loaded.erase(chunk);
    erase(world, chunk);
}

} // namespace derivean
