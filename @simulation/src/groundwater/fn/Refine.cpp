#include "groundwater/fn/Refine.hpp"

#include "groundwater/constant/Hydraulics.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {
namespace {

float groundOf(const Tile& tile, size_t cell) {
    return tile.now.rock[cell] + tile.now.sediment[Coarse][cell] + tile.now.sediment[Sand][cell] + tile.now.sediment[Fines][cell];
}

} // namespace

void cellOf(const GroundwaterTile& tile, size_t index, int64_t& x, int64_t& z) {
    const int32_t cell = tile.records[index].cell;
    x = int64_t(tile.key.x) * TileCells + cell % TileCells, z = int64_t(tile.key.z) * TileCells + cell / TileCells;
}

bool ventNear(const GroundwaterTile& tile, size_t index, Vec2 low, Vec2 high, double margin) {
    int64_t x, z;
    cellOf(tile, index, x, z);
    return double(x + 1) * WaterCell > low.x - margin && double(x) * WaterCell < high.x + margin && double(z + 1) * WaterCell > low.z - margin && double(z) * WaterCell < high.z + margin;
}

Spring refineVent(World& world, const GroundwaterTile& tile, size_t index) {
    Spring spring = tile.springs[index];
    const SpringRecord& record = tile.records[index];
    const double fine = specOf(Level::Site).cell;
    int64_t localX, localZ;
    cellOf(tile, index, localX, localZ);
    const int64_t firstX = localX * Refinement, firstZ = localZ * Refinement;
    const std::shared_ptr<const Tile> site = world.atlas.require(tileAt(Level::Site, {(double(firstX) + 0.5) * fine, (double(firstZ) + 0.5) * fine}));
    const int64_t tileX = int64_t(site->key.x) * TileCells, tileZ = int64_t(site->key.z) * TileCells;
    const auto cellAt = [&](int64_t x, int64_t z) { return size_t(z - tileZ) * TileCells + size_t(x - tileX); };
    if (record.kind == SpringKind::Karst) {
        const int64_t x = std::clamp(int64_t(std::floor(spring.position.x / fine)), firstX, firstX + Refinement - 1);
        const int64_t z = std::clamp(int64_t(std::floor(spring.position.z / fine)), firstZ, firstZ + Refinement - 1);
        spring.elevation = groundOf(*site, cellAt(x, z));
        return spring;
    }
    // The lowest ground of the cell, or for water a bed turns out the ground nearest the height of
    // the bed's lip; of equals the one nearest to where the local level put the vent.
    double best = 1e30;
    for (int64_t z = firstZ; z < firstZ + Refinement; ++z)
        for (int64_t x = firstX; x < firstX + Refinement; ++x) {
            const float ground = groundOf(*site, cellAt(x, z));
            const Vec2 centre{(double(x) + 0.5) * fine, (double(z) + 0.5) * fine};
            const double misfit = (record.kind == SpringKind::Contact ? std::abs(double(ground) - tile.springs[index].elevation) : double(ground)) + 0.01 * length(centre - tile.springs[index].position);
            if (misfit >= best) continue;
            best = misfit;
            spring.position = centre, spring.elevation = ground;
        }
    return spring;
}

} // namespace derivean::sim
