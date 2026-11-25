#include "world/type/VoxelWorld.hpp"

namespace derivean {
namespace {

size_t tileIndex(Cell offset) {
    return size_t((offset.y * VoxelsPerMeter + offset.z) * VoxelsPerMeter + offset.x);
}

Material at(const TerrainTile& tile, Cell offset) {
    return tile.detail ? (*tile.detail)[tileIndex(offset)] : tile.uniform;
}

template<typename Map, typename Key>
void dropCell(Map& counts, const Key& key) {
    const auto counter = counts.find(key);
    if (--counter->second == 0) counts.erase(counter);
}

} // namespace

Material materialAt(const VoxelWorld& world, Cell point) {
    const Cell key = tileOf(point);
    const auto tile = world.tiles.find(key);
    return tile == world.tiles.end() ? Material::Air : at(tile->second, point - key * VoxelsPerMeter);
}

Voxels sampleWorld(const VoxelWorld& world, Bounds bounds) {
    Voxels result(bounds);
    const Cell first = tileOf(bounds.min);
    const Cell last = tileOf(bounds.max - Cell{1, 1, 1});
    for (int y = first.y; y <= last.y; ++y)
        for (int z = first.z; z <= last.z; ++z)
            for (int x = first.x; x <= last.x; ++x) {
                const Cell key{x, y, z};
                const auto it = world.tiles.find(key);
                if (it == world.tiles.end()) continue;
                const Cell origin = key * VoxelsPerMeter;
                const Bounds region = intersection(bounds, {origin, origin + Cell{VoxelsPerMeter, VoxelsPerMeter, VoxelsPerMeter}});
                for (int rowY = region.min.y; rowY < region.max.y; ++rowY)
                    for (int rowZ = region.min.z; rowZ < region.max.z; ++rowZ) {
                        const Cell start{region.min.x, rowY, rowZ};
                        auto output = result.cells.begin() + result.index(start);
                        const int count = region.max.x - region.min.x;
                        if (it->second.detail)
                            std::copy_n(it->second.detail->begin() + tileIndex(start - origin), count, output);
                        else
                            std::fill_n(output, count, it->second.uniform);
                    }
            }
    return result;
}

void setVoxel(VoxelWorld& world, Cell point, Material material) {
    const Cell key = tileOf(point);
    auto found = world.tiles.find(key);
    if (found == world.tiles.end()) {
        if (material == Material::Air) return;
        found = world.tiles.try_emplace(key).first;
    }

    auto& tile = found->second;
    const Cell offset = point - key * VoxelsPerMeter;
    const Material before = at(tile, offset);
    if (before == material) return;
    if (before == Material::Air) {
        ++world.chunks[chunkOf(point)];
        ++world.occupiedLevels[point.y];
        ++tile.occupied;
    } else if (material == Material::Air) {
        dropCell(world.chunks, chunkOf(point));
        dropCell(world.occupiedLevels, point.y);
        if (--tile.occupied == 0) {
            world.tiles.erase(found);
            return;
        }
    }

    if (!tile.detail) {
        tile.detail = std::make_unique<TerrainTile::Detail>();
        tile.detail->fill(tile.uniform);
    }
    (*tile.detail)[tileIndex(offset)] = material;
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
    if (materialAt(world, point) == Material::Air) return false;
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
                const Material material = materialAt(world, point);
                if (material == Material::Air) continue;
                if (excavatedKg) *excavatedKg += materialInfo(material).density * double(VoxelVolume);
                setVoxel(world, point, Material::Air);
                changed.push_back(point);
            }
    return changed;
}

bool belowTerrain(const VoxelWorld& world, double highestPoint) {
    return world.occupiedLevels.empty() || highestPoint < world.occupiedLevels.begin()->first * double(VoxelSize);
}

} // namespace derivean
