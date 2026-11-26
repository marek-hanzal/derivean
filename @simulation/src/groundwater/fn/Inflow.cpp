#include "groundwater/fn/Inflow.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr int PerRegion = Refinement;   // local cells along a region cell
static_assert(WaterReach % PerRegion == 0, "a field holds whole region cells");

int64_t floorDiv(int64_t value, int64_t by) { return value >= 0 ? value / by : -((-value + by - 1) / by); }

} // namespace

void addRegionalInflow(WaterField& field, const std::vector<const RegionalTile*>& regions) {
    field.upwelled.clear();
    for (AquiferColumn& column : field.column) column.regional = 0;
    const int blocks = field.size / PerRegion;
    const int64_t firstX = floorDiv(field.firstX, PerRegion), firstZ = floorDiv(field.firstZ, PerRegion);
    for (int bz = 0; bz < blocks; ++bz)
        for (int bx = 0; bx < blocks; ++bx) {
            const int64_t rx = firstX + bx, rz = firstZ + bz;
            const TileKey key{Level::Region, int32_t(floorDiv(rx, TileCells)), int32_t(floorDiv(rz, TileCells))};
            const auto found = std::find_if(regions.begin(), regions.end(), [&](const RegionalTile* tile) { return tile && tile->key == key; });
            if (found == regions.end()) continue;
            const RegionalTile& region = **found;
            const size_t from = size_t(rz - int64_t(key.z) * TileCells) * TileCells + size_t(rx - int64_t(key.x) * TileCells);
            const float up = region.upwelling[from];
            if (up <= 0) continue;
            const Borne borne{0, region.far[from], region.high[from], region.rate[from], 0, region.deep[from]};
            double weight = 0;
            size_t lowest = SIZE_MAX;
            for (int z = bz * PerRegion; z < (bz + 1) * PerRegion; ++z)
                for (int x = bx * PerRegion; x < (bx + 1) * PerRegion; ++x) {
                    const size_t i = field.index(x, z);
                    const AquiferColumn& column = field.column[i];
                    if (lowest == SIZE_MAX || column.ground < field.column[lowest].ground) lowest = i;
                    if (drainsLower(column)) weight += std::max(column.stream, DrainingFlow);
                }
            if (field.upwelled.empty()) field.upwelled.assign(field.cells(), Borne{});
            if (weight <= 0) {
                field.column[lowest].regional = up, field.upwelled[lowest] = borne;
                continue;
            }
            for (int z = bz * PerRegion; z < (bz + 1) * PerRegion; ++z)
                for (int x = bx * PerRegion; x < (bx + 1) * PerRegion; ++x) {
                    const size_t i = field.index(x, z);
                    AquiferColumn& column = field.column[i];
                    if (!drainsLower(column)) continue;
                    column.regional = float(double(up) * std::max(column.stream, DrainingFlow) / weight), field.upwelled[i] = borne;
                }
        }
}

} // namespace derivean::sim
