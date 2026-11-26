#include "hydrology/fn/Ocean.hpp"
#include "hydrology/fn/Water.hpp"

namespace derivean::sim {

void shapeGrid(WaterGrid& grid, int width, int height, double cell) {
    grid.width = width;
    grid.height = height;
    grid.cell = cell;
    const size_t cells = grid.cells();
    grid.solid.resize(cells);
    for (auto* field : {&grid.ground, &grid.held, &grid.salinity, &grid.rain, &grid.evaporation, &grid.runoff, &grid.erodibility, &grid.balance,
                        &grid.discharge, &grid.flux[0], &grid.flux[1], &grid.flux[2], &grid.flux[3]})
        field->assign(cells, 0.0f);
    grid.depth.assign(cells, 0.0);
    grid.salt.assign(cells, 0.0);
    grid.kind.assign(cells, uint8_t(CellKind::Open));
    grid.sources.clear();
}

Tally stockOf(const WaterGrid& grid) {
    Tally stock;
    const double area = grid.cell * grid.cell;
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        stock.water += grid.depth[cell] * area;
        stock.salt += grid.salt[cell] * area;
    }
    return stock;
}

} // namespace derivean::sim
