#include "measure/type/Mosaic.hpp"

#include "simulation/type/World.hpp"

namespace derivean::sim {

Mosaic gatherMosaic(World& world, TileKey first, int tiles) {
    Mosaic mosaic;
    mosaic.level = first.level;
    mosaic.first = first;
    mosaic.tiles = tiles;
    mosaic.cell = specOf(first.level).cell;
    mosaic.seaLevel = world.planet.seaLevel;
    const int side = tiles * TileCells;
    mosaic.ground = Field<float>(side, side);
    mosaic.water = Field<float>(side, side);
    mosaic.discharge = Field<float>(side, side);
    mosaic.cover = Field<float>(side, side);
    mosaic.datum = Field<float>(side, side);
    mosaic.coarse = Field<float>(side, side);
    for (Field<float>* field : {&mosaic.rain, &mosaic.warmth, &mosaic.ice, &mosaic.iceAge}) *field = Field<float>(side, side);
    mosaic.flowTo = Field<uint8_t>(side, side, uint8_t(255));
    mosaic.rock = Field<uint8_t>(side, side);
    for (int dz = 0; dz < tiles; ++dz)
        for (int dx = 0; dx < tiles; ++dx) world.atlas.request({first.level, first.x + dx, first.z + dz});
    for (int dz = 0; dz < tiles; ++dz) {
        for (int dx = 0; dx < tiles; ++dx) {
            const auto tile = world.atlas.require({first.level, first.x + dx, first.z + dz});
            for (int z = 0; z < TileCells; ++z) {
                for (int x = 0; x < TileCells; ++x) {
                    const size_t from = size_t(z) * TileCells + size_t(x);
                    const int tx = dx * TileCells + x, tz = dz * TileCells + z;
                    const float cover = tile->now.sediment[0][from] + tile->now.sediment[1][from] + tile->now.sediment[2][from];
                    mosaic.ground.at(tx, tz) = tile->now.rock[from] + cover;
                    mosaic.cover.at(tx, tz) = cover;
                    mosaic.coarse.at(tx, tz) = tile->now.sediment[0][from];
                    mosaic.datum.at(tx, tz) = tile->now.datum[from];
                    mosaic.rock.at(tx, tz) = tile->rock[from];
                    mosaic.rain.at(tx, tz) = tile->rain[from], mosaic.warmth.at(tx, tz) = tile->warmth[from];
                    mosaic.ice.at(tx, tz) = tile->ice[from], mosaic.iceAge.at(tx, tz) = tile->iceAge[from];
                    mosaic.water.at(tx, tz) = tile->water[from];
                    mosaic.discharge.at(tx, tz) = tile->discharge[from];
                    mosaic.flowTo.at(tx, tz) = tile->flowTo[from];
                }
            }
        }
    }
    return mosaic;
}

Mosaic gatherMosaic(World& world, Level level, Vec2 center, int tiles) {
    const TileKey middle = tileAt(level, center);
    return gatherMosaic(world, {level, middle.x - tiles / 2, middle.z - tiles / 2}, tiles);
}

Mosaic gatherAround(World& world, Level level, Vec2 center, double reach) {
    const TileKey low = tileAt(level, {center.x - reach, center.z - reach}), high = tileAt(level, {center.x + reach, center.z + reach});
    return gatherMosaic(world, low, std::max(high.x - low.x, high.z - low.z) + 1);
}

double riverThreshold(Level level) {
    // Chosen, not derived: about 3 m³/s at 1 km, a brook at 64 m and a trickle that already holds
    // one line at 4 m. A site tile is too small to gather more than that.
    return level == Level::Region ? 1e8 : level == Level::Local ? 2e6 : 6e4;
}

} // namespace derivean::sim
