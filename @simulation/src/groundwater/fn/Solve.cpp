#include "groundwater/fn/Stages.hpp"

namespace derivean::sim {

std::shared_ptr<GroundwaterTile> solveField(WaterField& field) {
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
