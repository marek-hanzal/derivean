#pragma once

#include "core/type/Field.hpp"
#include "landscape/type/Level.hpp"

#include <cstdint>

namespace derivean::sim {

struct World;

// Finished tiles of one level laid side by side: the cell fields the measurements read. Column and
// row 0 belong to the tile `first`; a tile edge lies before every multiple of TileCells.
struct Mosaic {
    Level level{Level::Region};
    TileKey first;
    int tiles{};         // per side
    double cell{};       // m
    double seaLevel{};   // m
    Field<float> ground, water, discharge;
    Field<float> cover, datum;   // m of loose sediment; m, original top of the layering
    Field<float> coarse;         // m of that cover that is blocks and gravel
    Field<float> rain, warmth;   // m/yr; °C annual mean at the ground
    Field<float> ice, iceAge;    // m of glacier ice now and at the last glacial maximum
    Field<uint8_t> flowTo;
    Field<uint8_t> rock;         // Material of the solid rock at the top of the column

    int side() const { return ground.width; }
    bool contains(int x, int z) const { return ground.contains(x, z); }
    bool flooded(int x, int z) const { return water.at(x, z) > ground.at(x, z); }
    // Land in the open air: neither under standing water nor below the sea level.
    bool dry(int x, int z) const { return !flooded(x, z) && ground.at(x, z) >= seaLevel; }
    // Standing water that is not the sea.
    bool lake(int x, int z) const { return flooded(x, z) && water.at(x, z) > seaLevel + 0.5; }
    // What an observer sees as the surface: the ground, or the water standing on it.
    float top(int x, int z) const { return std::max(ground.at(x, z), water.at(x, z)); }
};

// World position of the centre of a cell, and the cell under a world position (which may lie outside).
inline Vec2 cellCentre(const Mosaic& mosaic, int x, int z) {
    return {(double(mosaic.first.x) * TileCells + x + 0.5) * mosaic.cell, (double(mosaic.first.z) * TileCells + z + 0.5) * mosaic.cell};
}
inline int cellX(const Mosaic& mosaic, Vec2 at) { return int(std::floor(at.x / mosaic.cell)) - mosaic.first.x * TileCells; }
inline int cellZ(const Mosaic& mosaic, Vec2 at) { return int(std::floor(at.z / mosaic.cell)) - mosaic.first.z * TileCells; }
// A distance in whole cells of the mosaic, at least one.
inline int cellsIn(const Mosaic& mosaic, double metres) { return std::max(1, int(std::lround(metres / mosaic.cell))); }

// Builds `tiles` × `tiles` tiles around `center` and blocks until they are finished.
Mosaic gatherMosaic(World& world, Level level, Vec2 center, int tiles);

// The smallest square block of tiles that holds everything within `reach` of `center`.
Mosaic gatherAround(World& world, Level level, Vec2 center, double reach);

// The same block of tiles given by its north-west key.
Mosaic gatherMosaic(World& world, TileKey first, int tiles);

// Discharge in m³/yr above which the measurements call the water of a cell a stream of its level.
double riverThreshold(Level level);

} // namespace derivean::sim
