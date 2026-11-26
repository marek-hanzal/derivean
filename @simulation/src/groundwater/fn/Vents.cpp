#include "groundwater/fn/Vents.hpp"

#include "core/fn/Hash.hpp"
#include "groundwater/fn/Chemistry.hpp"
#include "groundwater/fn/Stages.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t SpringStream = 0x5350524e;
constexpr float CoolingFlow = 2.0e-6f;     // m³/s per metre of rise at which rising water keeps 1/e of its heat
constexpr float HottestRock = 350.0f;      // °C
constexpr float DeepWater = 150.0f;        // m of circulation from which rising water is a fault's and not the slope's
constexpr float RockDensity = 2600.0f;     // kg/m³

// Whether surface water already runs into the cell from above: then what emerges in it feeds a
// stream that exists, and the head of that stream is the spring.
bool gaining(const WaterField& field, int x, int z) {
    for (int k = 0; k < 8; ++k) {
        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
        if (nx < 0 || nz < 0 || nx >= field.size || nz >= field.size) continue;
        const size_t j = field.index(nx, nz);
        const AquiferColumn& donor = field.column[j];
        if (donor.flowTo != ((k + 4) & 7)) continue;
        if (donor.drain != Drain::None) return true;
        const NodeFate &upper = field.fate[j * 2 + Upper], &lower = field.fate[j * 2 + Lower];
        if (upper.emerges + upper.outcrop + lower.emerges >= LeastSeep) return true;
    }
    return false;
}

} // namespace

void lipOf(const WaterField& field, int x, int z, Vec2& position, float& elevation) {
    const size_t i = field.index(x, z);
    const AquiferColumn& column = field.column[i];
    const Vec2 centre = field.centre(x, z);
    double weight = 0, offsetX = 0, offsetZ = 0, height = 0;
    for (int k = 0; k < 8; ++k) {
        if (Way(field.way[i * 8 + size_t(k)]) != Way::Outcrop) continue;
        const float part = field.share[(i * 2 + Upper) * 8 + size_t(k)];
        if (part <= 0) continue;
        const AquiferColumn& other = field.column[field.index(x + NeighborX[k], z + NeighborZ[k])];
        const float sheet = other.perched && other.sheet == column.sheet ? other.floor : sheetToward(column, k);
        const float above = std::max(0.0f, column.ground - column.floor), below = std::max(0.01f, sheet - other.ground);
        const float along = std::clamp(above / (above + below), 0.05f, 0.49f);
        weight += part;
        offsetX += part * along * NeighborX[k] * WaterCell, offsetZ += part * along * NeighborZ[k] * WaterCell;
        height += part * (column.ground + (other.ground - column.ground) * along);
    }
    position = weight > 0 ? centre + Vec2{offsetX / weight, offsetZ / weight} : centre;
    elevation = weight > 0 ? float(height / weight) : column.ground;
}

void addSpring(const WaterField& field, const AquiferColumn& column, const VentWater& water, GroundwaterTile& tile) {
    Spring spring;
    SpringRecord record;
    spring.position = water.position;
    spring.elevation = water.elevation;
    spring.discharge = water.flow / PerYear;
    const int64_t cellX = int64_t(std::floor(water.position.x / WaterCell)), cellZ = int64_t(std::floor(water.position.z / WaterCell));
    spring.id = hash(stream(field.seed, SpringStream), cellX, cellZ, int64_t(water.kind) * 2 + water.storey);

    const Borne& carried = water.carried;
    const float reservoir = std::min(HottestRock, rockTemperature(column.heat, column.warmth, carried.deep));
    const float kept = std::exp(-CoolingFlow * carried.deep / std::max(1e-9f, spring.discharge));
    const float arriving = column.warmth + (reservoir - column.warmth) * kept;
    const float boiling = boilingPoint(water.elevation - field.seaLevel);
    spring.steamShare = arriving > boiling ? flashShare(arriving, boiling) : 0.0f;
    spring.temperature = std::min(arriving, boiling);
    // Silica comes out of water that held more at depth than it can keep once cool; steam leaving
    // concentrates what stays. Carbonate comes out as the gas that held it escapes to the air.
    record.dissolvedSilica = quartzSilica(reservoir);
    record.dissolvedCarbonate = carried.lime;
    const float open = std::max(15.0f, column.warmth);
    spring.silica = kept * std::max(0.0f, record.dissolvedSilica / (1.0f - spring.steamShare) - glassSilica(open));
    spring.carbonate = std::max(0.0f, carried.lime - calciteLoad(AirCarbonDioxide, std::max(open, spring.temperature)));

    record.kind = water.kind;
    record.recharge = carried.rate;
    record.catchment = water.flow / std::max(1e-4f, carried.rate);
    record.path = carried.far;
    record.drop = carried.high - water.elevation;
    record.depth = carried.deep;
    record.years = carried.age;
    record.aquifer = water.aquifer, record.floor = water.floor;
    record.heat = column.heat;
    record.reservoir = reservoir;
    record.crush = column.crush;
    record.cell = water.cell, record.storey = water.storey;
    tile.budget.springs += water.flow;
    tile.budget.dissolved += double(water.flow) * (carried.lime + record.dissolvedSilica) / RockDensity;
    tile.springs.push_back(spring);
    tile.records.push_back(record);
}

void findVents(const WaterField& field, GroundwaterTile& tile) {
    readFields(field, tile);
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const int fx = x + field.margin, fz = z + field.margin;
            const size_t i = field.index(fx, fz), own = size_t(z) * TileCells + size_t(x);
            const AquiferColumn& column = field.column[i];
            const NodeFate &upper = field.fate[i * 2 + Upper], &lower = field.fate[i * 2 + Lower];
            const float emerging = upper.emerges + lower.emerges;
            // A spring gathers the water of ground around it. What a cell gives back of its own
            // recharge is a wet slope, not a vent.
            const float least = std::max(LeastSpring, GatherShare * (column.recharge + column.sunk));
            tile.emergence[own] = emerging + upper.outcrop;
            if (column.drain != Drain::None || (emerging > 0 && gaining(field, fx, fz))) {
                tile.budget.baseflow += emerging;
            } else if (emerging >= least) {
                const int storey = lower.emerges >= upper.emerges ? Lower : Upper;
                VentWater water;
                water.carried = field.carried[i * 2 + size_t(storey)];
                water.kind = storey == Lower && column.crush >= FaultShare && water.carried.deep >= DeepWater ? SpringKind::Fault : SpringKind::Table;
                water.position = field.centre(fx, fz), water.elevation = column.ground;
                water.flow = emerging;
                water.cell = int32_t(own), water.storey = uint8_t(storey);
                water.aquifer = storey == Upper ? column.upperRock : column.lowerRock;
                water.floor = storey == Upper ? column.floorRock : Material::Air;
                addSpring(field, column, water, tile);
            } else {
                tile.budget.seepage += emerging;
            }
            if (upper.outcrop >= least) {
                VentWater water;
                water.kind = SpringKind::Contact;
                lipOf(field, fx, fz, water.position, water.elevation);
                water.flow = upper.outcrop;
                water.carried = field.carried[i * 2 + Upper];
                water.cell = int32_t(own), water.storey = Upper;
                water.aquifer = column.upperRock, water.floor = column.floorRock;
                addSpring(field, column, water, tile);
            } else {
                tile.budget.seepage += upper.outcrop;
            }
        }
}

} // namespace derivean::sim
