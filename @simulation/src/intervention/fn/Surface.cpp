#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Water.hpp"

namespace derivean::sim {

const EditedTile* editedTile(const World& world, TileKey key) {
    if (key.level != Level::Site) return nullptr;
    const auto& tiles = world.interventions.overlay.tiles;
    const auto found = tiles.find({key.z, key.x});
    return found == tiles.end() ? nullptr : found->second.get();
}

// The untouched sample is interpolated between cells; the live state is per cell. The sample keeps
// its interpolation and is moved by what changed in the cell the point lies in, so an untouched
// cell of a live tile still reads exactly as the atlas gives it.
void overlaySurface(const World& world, Vec2 position, SurfaceSample& sample) {
    if (!sample.found || sample.level != Level::Site) return;
    const double cell = specOf(Level::Site).cell;
    const int64_t gx = int64_t(std::floor(position.x / cell)), gz = int64_t(std::floor(position.z / cell));
    const TileKey key{Level::Site, int32_t(std::floor(double(gx) / TileCells)), int32_t(std::floor(double(gz) / TileCells))};
    const EditedTile* tile = editedTile(world, key);
    if (!tile) return;
    const size_t at = size_t(gz - int64_t(key.z) * TileCells) * TileCells + size_t(gx - int64_t(key.x) * TileCells);
    const Ground& was = tile->pristine->now;
    sample.rock += tile->now.rock[at] - was.rock[at];
    sample.ground = sample.rock;
    for (int c = 0; c < SedimentClasses; ++c) {
        sample.sediment[c] = std::max(0.0f, sample.sediment[c] + tile->now.sediment[c][at] - was.sediment[c][at]);
        sample.ground += sample.sediment[c];
    }
    sample.water = tile->depth[at] > OpenWater ? tile->water[at] : sample.ground - 1.0f;
    sample.discharge = tile->discharge[at];
}

SiteColumn siteColumn(World& world, int64_t cellX, int64_t cellZ) {
    const TileKey key{Level::Site, int32_t(std::floor(double(cellX) / TileCells)), int32_t(std::floor(double(cellZ) / TileCells))};
    const size_t at = size_t(cellZ - int64_t(key.z) * TileCells) * TileCells + size_t(cellX - int64_t(key.x) * TileCells);
    SiteColumn column;
    if (const EditedTile* tile = editedTile(world, key)) {
        column.rock = tile->now.rock[at];
        for (int c = 0; c < SedimentClasses; ++c) column.cover[c] = tile->now.sediment[c][at];
        column.ground = groundOf(tile->now, at);
        column.depth = tile->depth[at];
        column.salinity = tile->depth[at] > 0 ? tile->salt[at] / tile->depth[at] : 0.0;
        column.discharge = tile->discharge[at];
        column.live = true;
        return column;
    }
    const std::shared_ptr<const Tile> tile = world.atlas.require(key);
    column.rock = tile->now.rock[at];
    for (int c = 0; c < SedimentClasses; ++c) column.cover[c] = tile->now.sediment[c][at];
    column.ground = groundOf(tile->now, at);
    column.depth = std::max(0.0, double(tile->water[at]) - double(column.ground));
    column.salinity = column.depth <= 0 ? 0.0 : tile->water[at] <= float(tile->seaLevel) + 0.5f ? world.ocean.salinity : RiverSalinity;
    column.discharge = tile->discharge[at];
    return column;
}

} // namespace derivean::sim
