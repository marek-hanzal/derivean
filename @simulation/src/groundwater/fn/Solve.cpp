#include "groundwater/fn/Stages.hpp"

#include "groundwater/fn/Inflow.hpp"

#include <cassert>

namespace derivean::sim {

std::shared_ptr<GroundwaterTile> solveField(WaterField& field) {
    assert(field.size - 2 * field.margin == TileCells);
    auto tile = std::make_shared<GroundwaterTile>();
    settleLevels(field);
    routeGroundWater(field);
    findVents(field, *tile);
    growConduits(field, *tile);
    return tile;
}

std::shared_ptr<GroundwaterTile> buildGroundwater(const Setting& setting, TileKey key, const Tile* const around[9], const std::vector<const RegionalTile*>& regions) {
    WaterField field;
    readColumns(setting, key, around, field);
    if (!regions.empty()) addRegionalInflow(field, regions);
    return solveField(field);
}

} // namespace derivean::sim
