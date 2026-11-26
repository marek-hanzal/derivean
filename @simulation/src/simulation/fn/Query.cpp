#include "simulation/fn/Query.hpp"

#include "simulation/fn/Live.hpp"
#include "simulation/fn/Reader.hpp"

namespace derivean::sim {

namespace {

constexpr float RunningWater = 2.0e6f;   // m³/yr above which a cell holds a visible stream

} // namespace

void requireOver(World& world, Level level, double minX, double minZ, double maxX, double maxZ, double margin) {
    world.atlas.bind(world);
    const TileKey low = tileAt(level, {minX - margin, minZ - margin}), high = tileAt(level, {maxX + margin, maxZ + margin});
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x) world.atlas.require({level, x, z});
}

bool readSurface(TileReader& reader, Vec2 position, SurfaceSample& sample) {
    const double cell = specOf(reader.level).cell;
    const double fx = position.x / cell - 0.5, fz = position.z / cell - 0.5;
    const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
    const int64_t nearX = int64_t(std::floor(position.x / cell)), nearZ = int64_t(std::floor(position.z / cell));
    const int32_t homeX = int32_t(std::floor(double(nearX) / TileCells)), homeZ = int32_t(std::floor(double(nearZ) / TileCells));
    const Tile* home = reader.tile(homeX, homeZ);
    if (!home) return false;

    // A tap in a tile that is not finished falls back to the nearest cell of the home tile.
    const auto tap = [&](int64_t gx, int64_t gz, const Tile*& tile) {
        const int32_t tx = int32_t(std::floor(double(gx) / TileCells)), tz = int32_t(std::floor(double(gz) / TileCells));
        tile = (tx == homeX && tz == homeZ) ? home : reader.tile(tx, tz);
        if (!tile) {
            tile = home;
            gx = std::clamp(gx, int64_t(homeX) * TileCells, int64_t(homeX + 1) * TileCells - 1);
            gz = std::clamp(gz, int64_t(homeZ) * TileCells, int64_t(homeZ + 1) * TileCells - 1);
            return size_t(gz - int64_t(homeZ) * TileCells) * TileCells + size_t(gx - int64_t(homeX) * TileCells);
        }
        return size_t(gz - int64_t(tz) * TileCells) * TileCells + size_t(gx - int64_t(tx) * TileCells);
    };

    double wx[4], wz[4];
    const double tx = fx - double(ix), tz = fz - double(iz);
    if (reader.level == Level::Ground) {
        // Voxels are cut from the ground cells by straight interpolation; the surface answers alike.
        wx[0] = wx[3] = wz[0] = wz[3] = 0;
        wx[1] = 1 - tx, wx[2] = tx, wz[1] = 1 - tz, wz[2] = tz;
    } else {
        cubicWeights(tx, wx);
        cubicWeights(tz, wz);
    }
    double rock = 0, datum = 0, cover[SedimentClasses] = {};
    float low = 1e30f, high = -1e30f;
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            const Tile* tile;
            const size_t at = tap(ix - 1 + i, iz - 1 + j, tile);
            rock += wx[i] * wz[j] * tile->now.rock[at];
            datum += wx[i] * wz[j] * tile->now.datum[at];
            if (i < 1 || i > 2 || j < 1 || j > 2) continue;
            low = std::min(low, tile->now.rock[at]);
            high = std::max(high, tile->now.rock[at]);
            const double weight = (i == 1 ? 1 - tx : tx) * (j == 1 ? 1 - tz : tz);
            for (int c = 0; c < SedimentClasses; ++c) cover[c] += weight * tile->now.sediment[c][at];
        }
    }
    sample.rock = std::clamp(float(rock), low, high);
    sample.datum = std::max(sample.rock, float(datum));
    sample.ground = sample.rock;
    for (int c = 0; c < SedimentClasses; ++c) {
        sample.sediment[c] = std::max(0.0f, float(cover[c]));
        sample.ground += sample.sediment[c];
    }
    const Tile* tile;
    const size_t at = tap(nearX, nearZ, tile);
    sample.water = tile->water[at];
    sample.discharge = tile->discharge[at];
    // Running water: the depth a stream of this size has by the usual hydraulic geometry
    // (depth ∝ Q^0.4, Q in m³/s). Standing water already carries its level, and a ground tile
    // carries the level of its streams too, in the pools and over the steps of their beds.
    if (reader.level != Level::Ground && sample.water <= sample.ground && sample.discharge > RunningWater)
        sample.water = sample.ground + 0.25f * std::pow(sample.discharge / 3.156e7f, 0.4f);
    if (tile->detail) {
        sample.deposit = tile->detail->crust[at];
        sample.depositRock = sample.deposit <= 0 ? Material::Air : Crust(tile->detail->crustKind[at]) == Crust::Travertine ? Material::Travertine : Material::Sinter;
        sample.coarseAs = Material(tile->detail->coarseKind[at]);
        sample.ground += sample.deposit;
    }
    sample.rain = tile->rain[at];
    sample.warmth = tile->warmth[at];
    sample.erosionRate = tile->erosionRate[at];
    sample.depositionRate = tile->depositionRate[at];
    sample.bedrock = Material(tile->rock[at]);
    sample.level = reader.level;
    sample.found = true;
    return true;
}

SurfaceSample surfaceAt(const World& world, Vec2 position, Level finest) {
    SurfaceSample sample;
    for (int level = int(finest); level >= int(Level::Region); --level) {
        TileReader reader{world.atlas, Level(level)};
        if (readSurface(reader, position, sample)) break;
    }
    overlayLive(world, position, sample);
    return sample;
}

void requestAround(World& world, Vec2 center, double radius, Level level) {
    world.atlas.bind(world);
    const double span = tileSpan(level);
    const TileKey low = tileAt(level, {center.x - radius, center.z - radius}), high = tileAt(level, {center.x + radius, center.z + radius});
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x) {
            const Vec2 middle{(x + 0.5) * span, (z + 0.5) * span};
            world.atlas.request({level, x, z}, -int(length(middle - center) / span * 4.0) - int(level) * 64);
        }
}

bool readyAround(const World& world, Vec2 center, double radius, Level level) {
    const TileKey low = tileAt(level, {center.x - radius, center.z - radius}), high = tileAt(level, {center.x + radius, center.z + radius});
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x)
            if (!world.atlas.find({level, x, z})) return false;
    return true;
}

SurfaceSample surveyAt(World& world, Vec2 position, Level finest) {
    // The interpolation reads two cells to either side; all of them must be there, or the answer
    // near a tile's edge would depend on which of its neighbours happen to be finished.
    requireOver(world, finest, position.x, position.z, position.x, position.z, 2 * specOf(finest).cell);
    TileReader reader{world.atlas, finest};
    SurfaceSample sample;
    readSurface(reader, position, sample);
    overlayLive(world, position, sample);
    return sample;
}

} // namespace derivean::sim
