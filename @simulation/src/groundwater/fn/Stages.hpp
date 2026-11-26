#pragma once

#include "core/service/Workers.hpp"
#include "groundwater/type/WaterField.hpp"
#include "groundwater/type/Groundwater.hpp"
#include "landscape/type/Setting.hpp"

namespace derivean::sim {

// Runs `body(begin, end)` over the rows of a field, on its pool where it has one. A body writes
// only what belongs to its own rows and reads nothing another row writes in the same call.
inline void forRows(const WaterField& field, const std::function<void(int, int)>& body) {
    if (field.workers) field.workers->parallelFor(field.size, body, 8);
    else body(0, field.size);
}

// Storeys.cpp — the two storeys of a column from its layers, top first. The caller has set the
// column's ground, recharge, base and shallowBase.
void fillStoreys(AquiferColumn& column, const WaterLayer* layers, int count);

// Columns.cpp — sizes the field for a local tile and reads its columns from the nine finished
// tiles around it, row-major with the tile itself in the middle.
void readColumns(const Setting& setting, TileKey key, const Tile* const around[9], WaterField& field);

// Levels.cpp — where each storey's water would stand with nothing arriving, which way it leaves
// each cell, and how much a cell can pass on.
void settleLevels(WaterField& field);

// Sinks.cpp — how much of each stream on soluble rock goes underground, cell by cell.
void sinkStreams(WaterField& field);

// Route.cpp — follows the recharge through both storeys to where it comes out.
void routeGroundWater(WaterField& field);

// Vents.cpp — the springs of the tile's own cells and the per-cell fields a tile keeps.
void findVents(const WaterField& field, GroundwaterTile& tile);

// Karst.cpp — the conduit systems of the tile's soluble rock, their outlets and their voids.
void growConduits(const WaterField& field, GroundwaterTile& tile);

// Solve.cpp — everything after the columns are set, for a tile of the world or a field built by hand.
std::shared_ptr<GroundwaterTile> solveField(WaterField& field);

// The tile as the world's setting and the atlas's finished tiles give it.
std::shared_ptr<GroundwaterTile> buildGroundwater(const Setting& setting, TileKey key, const Tile* const around[9]);

} // namespace derivean::sim
