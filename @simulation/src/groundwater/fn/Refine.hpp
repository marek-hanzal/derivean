#pragma once

#include "groundwater/type/Groundwater.hpp"

namespace derivean::sim {

struct Tile;

// A spring as the local level found it, put where the site level's ground says its water comes
// out: inside the local cell it left the ground in, at the lowest point of that cell for water
// that rises where the table meets the ground, and at the height of the tight bed's lip for water
// turned out by a bed. A conduit's mouth keeps its place and takes the height of the finer ground.
// Builds the site tile under that cell. The answer depends on the spring and that tile alone.
Spring refineVent(World& world, const GroundwaterTile& tile, size_t index);

TileKey ventSite(const GroundwaterTile& tile, size_t index);
Spring refineVent(const Tile& site, const GroundwaterTile& tile, size_t index);

// Global index of the local cell a spring of the tile left the ground in.
void cellOf(const GroundwaterTile& tile, size_t index, int64_t& x, int64_t& z);

// Whether the local cell a spring of the tile left the ground in reaches into a rectangle grown by
// `margin`: only then can its vent lie inside it.
bool ventNear(const GroundwaterTile& tile, size_t index, Vec2 low, Vec2 high, double margin = 0);

} // namespace derivean::sim
