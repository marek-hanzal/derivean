#pragma once

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Reader.hpp"
#include "simulation/type/World.hpp"

namespace derivean {

// A run of surface samples that takes the atlas lock only when it crosses into another tile.
// sim::surfaceAt locks for every sample, which stalls the game thread while background tasks
// read the same atlas. A reader remembers which tiles were missing, so it must not outlive the
// batch of samples it was made for.
struct SurfaceReader {
    const sim::World* world;
    std::array<sim::TileReader, 4> levels;   // region, local, site, ground
    sim::Level finest;
};

inline SurfaceReader surfaceReader(const sim::World& world, sim::Level finest = sim::Level::Site) {
    return {&world, {{{world.atlas, sim::Level::Region}, {world.atlas, sim::Level::Local}, {world.atlas, sim::Level::Site},
                      {world.atlas, sim::Level::Ground}}}, finest};
}

// Same answer as sim::surfaceAt(world, position, reader.finest).
inline sim::SurfaceSample surfaceAt(SurfaceReader& reader, sim::Vec2 position) {
    sim::SurfaceSample sample;
    for (int level = int(reader.finest); level >= int(sim::Level::Region); --level)
        if (sim::readSurface(reader.levels[size_t(level) - 1], position, sample)) break;
    sim::overlayLive(*reader.world, position, sample);
    return sample;
}

// What stands in for a level where not even a region tile is finished: the planet's own grid.
inline constexpr sim::Level PlanetLevel = sim::Level(0);

// The surface as above and, where no tile is finished yet, the planet's own 65 km surface: coarse,
// but a range on the skyline is there from the first frame. Such a sample reports PlanetLevel.
inline sim::SurfaceSample surfaceOrPlanet(SurfaceReader& reader, sim::Vec2 position) {
    sim::SurfaceSample sample = surfaceAt(reader, position);
    if (sample.found) return sample;
    const sim::Planet& planet = reader.world->planet;
    sample.ground = sample.rock = sample.datum = float(sim::samplePlanet(planet.elevation, sim::planetPoint(planet.seed, position)));
    sample.water = sample.ground < planet.seaLevel ? float(planet.seaLevel) : sample.ground - 1.0f;
    sample.level = PlanetLevel;
    sample.found = true;
    return sample;
}

// The level surfaceOrPlanet answers a point from, without reading the sample: the finest level whose
// tile holds the point's cell.
inline sim::Level surfaceLevel(SurfaceReader& reader, sim::Vec2 position) {
    for (int level = int(reader.finest); level >= int(sim::Level::Region); --level)
        if (sim::surfaceHome(reader.levels[size_t(level) - 1], position).tile) return sim::Level(level);
    return PlanetLevel;
}

// Metres of glacier ice on the ground at a point, from the tile `sample` was read from.
inline float iceAt(SurfaceReader& reader, const sim::SurfaceSample& sample, sim::Vec2 position) {
    if (sample.level == PlanetLevel) return 0;
    const auto [x, z, tileX, tileZ, tile] = sim::surfaceHome(reader.levels[size_t(sample.level) - 1], position);
    const size_t at = size_t(z - int64_t(tileZ) * sim::TileCells) * sim::TileCells + size_t(x - int64_t(tileX) * sim::TileCells);
    return tile && at < tile->ice.size() ? tile->ice[at] : 0;
}

// Cells of a level over which its data eases into the coarser level towards a tile that is not finished.
inline constexpr int EaseCells = 16;

// The surface as above, eased across the edge of what is finished. Fine data ends at the straight
// edge of a tile whose neighbour is not built, and the coarser level beyond it is a different
// surface: taken as they come, the two meet in a line across the country. Near such an edge the
// sample moves over to the coarser level, so the change of data draws no line.
inline sim::SurfaceSample surfaceEased(SurfaceReader& reader, sim::Vec2 position) {
    sim::SurfaceSample sample = surfaceOrPlanet(reader, position);
    if (sample.level == PlanetLevel) return sample;
    const sim::Level level = sample.level;
    const double span = sim::tileSpan(level), margin = EaseCells * sim::specOf(level).cell;
    const int32_t x = int32_t(std::floor(position.x / span)), z = int32_t(std::floor(position.z / span));
    const double west = position.x - x * span, east = (x + 1) * span - position.x;
    const double north = position.z - z * span, south = (z + 1) * span - position.z;
    if (std::min({west, east, north, south}) >= margin) return sample;
    sim::TileReader& tiles = reader.levels[size_t(level) - 1];
    double edge = margin;
    if (west < edge && !tiles.tile(x - 1, z)) edge = west;
    if (east < edge && !tiles.tile(x + 1, z)) edge = east;
    if (north < edge && !tiles.tile(x, z - 1)) edge = north;
    if (south < edge && !tiles.tile(x, z + 1)) edge = south;
    if (edge >= margin) return sample;

    const sim::Level finest = reader.finest;
    reader.finest = sim::Level(int(level) - 1);
    const sim::SurfaceSample coarse = surfaceOrPlanet(reader, position);
    reader.finest = finest;
    const float share = float(sim::smoothstep(0.0, margin, edge));
    const auto ease = [share](float& fine, float from) { fine = from + (fine - from) * share; };
    ease(sample.ground, coarse.ground);
    ease(sample.rock, coarse.rock);
    ease(sample.datum, coarse.datum);
    for (int kind = 0; kind < sim::SedimentClasses; ++kind) ease(sample.sediment[kind], coarse.sediment[kind]);
    // A water level and a rock are one or the other, never a mix.
    if (share < 0.5f) {
        sample.water = coarse.water;
        sample.bedrock = coarse.bedrock;
    }
    return sample;
}

} // namespace derivean
