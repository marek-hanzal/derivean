#include "groundwater/fn/Stages.hpp"

#include <cassert>

namespace derivean::sim {

std::shared_ptr<GroundwaterTile> solveField(WaterField& field) {
    assert(field.size - 2 * field.margin == TileCells);
    auto tile = std::make_shared<GroundwaterTile>();
    sinkStreams(field);
    settleLevels(field);
    routeGroundWater(field);
    findVents(field, *tile);
    growConduits(field, *tile);
    return tile;
}

std::shared_ptr<GroundwaterTile> buildGroundwater(const Setting& setting, TileKey key, const Tile* const around[9]) {
    WaterField field;
    readColumns(setting, key, around, field);
    return solveField(field);
}

} // namespace derivean::sim
