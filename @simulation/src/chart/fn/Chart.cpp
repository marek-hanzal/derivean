#include "chart/fn/Chart.hpp"

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Reader.hpp"

#include <array>
#include <limits>

namespace derivean::sim {
namespace {

constexpr double CellsPerPixel = 4.0;   // a pixel wider than this many cells of a level would only alias them
constexpr float Wet = 0.01f;       // m of water over the ground below which a pixel is dry
constexpr float SeaReach = 0.5f;   // m above sea level up to which standing water belongs to the sea
constexpr double StreamReach = 0.5;   // pixels to either side of a stream's path that carry its flow
constexpr float NoWater = std::numeric_limits<float>::lowest();

const Tile* tileOf(TileReader& reader, int64_t cellX, int64_t cellZ, size_t& at) {
    const int32_t tileX = int32_t(std::floor(double(cellX) / TileCells)), tileZ = int32_t(std::floor(double(cellZ) / TileCells));
    at = size_t(cellZ - int64_t(tileZ) * TileCells) * TileCells + size_t(cellX - int64_t(tileX) * TileCells);
    return reader.tile(tileX, tileZ);
}

// A stream is one cell wide, so a pixel wider than a cell would step over most of it: the pixel
// takes the largest flow among the cells it covers.
float flowAcross(TileReader& reader, Vec2 centre, double pitch) {
    const double cell = specOf(reader.level).cell, half = 0.5 * pitch;
    const int64_t lowX = int64_t(std::floor((centre.x - half) / cell)), highX = int64_t(std::ceil((centre.x + half) / cell)) - 1;
    const int64_t lowZ = int64_t(std::floor((centre.z - half) / cell)), highZ = int64_t(std::ceil((centre.z + half) / cell)) - 1;
    float flow = 0;
    for (int64_t z = lowZ; z <= highZ; ++z)
        for (int64_t x = lowX; x <= highX; ++x) {
            size_t at;
            if (const Tile* tile = tileOf(reader, x, z, at)) flow = std::max(flow, tile->discharge[at]);
        }
    return flow;
}

// The largest stream whose path passes a pixel, and the cell that path starts in.
struct PathFlow {
    float flow{};
    const Tile* tile{};
    size_t at{};
    Vec2 from;
};

// A cell wider than a pixel is not a stream that wide. The water of a cell takes the straight path
// from its centre to the centre of the cell it flows into, and stays that cell's stream all the
// way. A path that passes a pixel starts in the pixel's own cell or in one next to it.
PathFlow flowAlong(TileReader& reader, Vec2 centre, double pitch, int64_t cellX, int64_t cellZ) {
    const double cell = specOf(reader.level).cell;
    PathFlow largest;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            size_t at;
            const Tile* tile = tileOf(reader, cellX + dx, cellZ + dz, at);
            if (!tile || tile->flowTo[at] > 7 || tile->discharge[at] <= largest.flow) continue;
            const Vec2 from{(double(cellX + dx) + 0.5) * cell, (double(cellZ + dz) + 0.5) * cell};
            const Vec2 path{NeighborX[tile->flowTo[at]] * cell, NeighborZ[tile->flowTo[at]] * cell};
            const double along = std::clamp(dot(centre - from, path) / dot(path, path), 0.0, 1.0);
            if (length(centre - (from + path * along)) <= StreamReach * pitch) largest = {tile->discharge[at], tile, at, from};
        }
    return largest;
}

// m, the level of the water standing in a cell; NoWater where it is dry. A dry cell stores a level
// below its own surface.
float standingLevel(const Tile& tile, size_t at) {
    const float surface = tile.now.rock[at] + tile.now.sediment[Coarse][at] + tile.now.sediment[Sand][at] + tile.now.sediment[Fines][at];
    return tile.water[at] > surface ? tile.water[at] : NoWater;
}

// The highest water standing in the cells around one. Where cells are wider than a pixel a lake
// must not end at a cell's edge: it reaches into a dry cell as far as the ground there lets it,
// which is how the far terrain draws its shores.
float shoreLevel(TileReader& reader, int64_t cellX, int64_t cellZ) {
    float level = NoWater;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            size_t at;
            if (const Tile* tile = tileOf(reader, cellX + dx, cellZ + dz, at)) level = std::max(level, standingLevel(*tile, at));
        }
    return level;
}

// False when the reader's level has no finished tile under the pixel centre.
bool readTiles(TileReader& reader, Vec2 centre, double pitch, float sea, Chart& chart, size_t pixel) {
    SurfaceSample sample;
    if (!readSurface(reader, centre, sample)) return false;
    const double cell = specOf(reader.level).cell;
    const int64_t cellX = int64_t(std::floor(centre.x / cell)), cellZ = int64_t(std::floor(centre.z / cell));
    // The tile readSurface just answered from; the atlas never gives a finished tile up.
    size_t at;
    const Tile* home = tileOf(reader, cellX, cellZ, at);
    const bool magnified = pitch < cell;

    chart.ground[pixel] = sample.ground;
    chart.cover[pixel] = sample.ground - sample.rock;
    chart.warmth[pixel] = sample.warmth;
    chart.ice[pixel] = home->ice[at];
    chart.bedrock[pixel] = uint8_t(sample.bedrock);
    chart.level[pixel] = uint8_t(reader.level);
    const PathFlow path = magnified ? flowAlong(reader, centre, pitch, cellX, cellZ) : PathFlow{};
    chart.discharge[pixel] = magnified ? path.flow : pitch > cell ? flowAcross(reader, centre, pitch) : sample.discharge;
    float standing = standingLevel(*home, at);
    if (magnified && standing <= sample.ground + Wet) standing = shoreLevel(reader, cellX, cellZ);
    if (standing > sample.ground + Wet) {
        chart.water[pixel] = standing;
        chart.waters[pixel] = uint8_t(standing <= sea + SeaReach ? ChartWater::Sea : ChartWater::Lake);
    } else if (!magnified) {
        // Not standing water: the depth readSurface gives a running stream.
        if (sample.water > sample.ground + Wet) {
            chart.water[pixel] = sample.water;
            chart.waters[pixel] = uint8_t(ChartWater::Stream);
        }
    } else if (path.tile && standingLevel(*path.tile, path.at) == NoWater) {
        // On a path: the running water of the cell the path starts in, as deep as readSurface finds it there.
        SurfaceSample source = sample;
        if (path.tile != home || path.at != at) readSurface(reader, path.from, source);
        if (source.water > source.ground + Wet) {
            chart.water[pixel] = sample.ground + (source.water - source.ground);
            chart.waters[pixel] = uint8_t(ChartWater::Stream);
        }
    }
    return true;
}

// The planet as the landscapes read it: through planetPoint, wrapping on its own grid. Its rivers
// are known by the cell they leave, not by where in 65 km they run, so a planet pixel carries none.
void readPlanet(const Planet& planet, Vec2 centre, Chart& chart, size_t pixel) {
    const Vec2 point = planetPoint(planet.seed, centre);
    const int x = wrapIndex(floorToInt(point.x / PlanetCell), PlanetCells), z = wrapIndex(floorToInt(point.z / PlanetCell), PlanetCells);
    const float ground = float(samplePlanet(planet.elevation, point));
    chart.ground[pixel] = ground;
    chart.warmth[pixel] = float(samplePlanet(planet.temperature, point, Kernel::Linear));
    chart.cover[pixel] = planet.columns.at(x, z).load;
    if (ground < planet.seaLevel) {
        chart.water[pixel] = float(planet.seaLevel);
        chart.waters[pixel] = uint8_t(ChartWater::Sea);
    }
}

} // namespace

bool chartFinest(double pitch, Level& finest) {
    for (int level = int(Level::Site); level >= int(Level::Region); --level) {
        if (pitch > CellsPerPixel * specOf(Level(level)).cell) continue;
        finest = Level(level);
        return true;
    }
    return false;
}

Chart chartOf(const World& world, const ChartRequest& request) {
    Chart chart;
    if (request.size <= 0 || !(request.span > 0)) return chart;
    chart.size = request.size;
    chart.pitch = request.span / request.size;
    chart.origin = {request.centre.x - 0.5 * request.span, request.centre.z - 0.5 * request.span};
    // Taken first: a tile that finishes while the chart is read makes the chart older than it, never newer.
    chart.tiles = world.atlas.finished();

    const size_t pixels = size_t(request.size) * size_t(request.size);
    for (auto* field : {&chart.ground, &chart.discharge, &chart.ice, &chart.warmth, &chart.cover}) field->assign(pixels, 0.0f);
    chart.water.assign(pixels, std::numeric_limits<float>::quiet_NaN());
    chart.bedrock.assign(pixels, uint8_t(Material::Bedrock));
    chart.waters.assign(pixels, uint8_t(ChartWater::Dry));
    chart.level.assign(pixels, 0);

    Level finest{};
    const int first = chartFinest(chart.pitch, finest) ? int(finest) : 0;
    chart.finest = uint8_t(first);
    const float sea = float(world.planet.seaLevel);
    // One reader per level for the whole chart: a run of pixels takes the atlas lock only when it
    // crosses into another tile, so reading a chart does not hold up whoever else reads the atlas.
    std::array<TileReader, 3> readers{{{world.atlas, Level::Region}, {world.atlas, Level::Local}, {world.atlas, Level::Site}}};
    for (int j = 0; j < request.size; ++j)
        for (int i = 0; i < request.size; ++i) {
            const Vec2 centre{chart.origin.x + (i + 0.5) * chart.pitch, chart.origin.z + (j + 0.5) * chart.pitch};
            const size_t pixel = size_t(j) * size_t(request.size) + size_t(i);
            bool read = false;
            for (int level = first; level >= int(Level::Region) && !read; --level)
                read = readTiles(readers[size_t(level) - 1], centre, chart.pitch, sea, chart, pixel);
            if (!read) readPlanet(world.planet, centre, chart, pixel);
        }
    return chart;
}

} // namespace derivean::sim
