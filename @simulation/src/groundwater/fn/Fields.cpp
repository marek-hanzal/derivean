#include "groundwater/fn/Vents.hpp"

#include "groundwater/fn/Route.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

// Share of a storey's full transmissivity that lies under a water level.
float wetShare(const AquiferColumn& column, int storey, float level) {
    if (storey == Upper) return float(saturate((level - column.floor) / std::max(0.1f, column.ground - column.floor)));
    const float through = lowerT(column);
    if (through <= 0) return 0;
    const float seam = std::min(column.roof, column.shallowBase), middle = 0.5f * (seam + column.base);
    const float low = float(saturate((level - column.base) / std::max(0.1f, middle - column.base)));
    const float high = float(saturate((level - middle) / std::max(0.1f, seam - middle)));
    const float shallow = float(saturate((level - column.shallowBase) / std::max(0.1f, column.roof - column.shallowBase)));
    return (column.deepT[1] * low + column.deepT[0] * high + column.shallowT * shallow) / through;
}

// m, where a storey's water stands: the lowest stand between its resting level and its top at
// which what is under water can carry the flow that passes sideways. The top is the ground, or for
// water under a tight bed the underside of that bed: above it the level would be a pressure.
float tableOf(const WaterField& field, size_t cell, int storey) {
    const AquiferColumn& column = field.column[cell];
    const size_t node = cell * 2 + size_t(storey);
    const float top = storey == Lower && column.perched ? column.roof : column.ground;
    const float rest = std::min(field.level[node], top);
    if (field.fate[node].emerges > LeastSeep) return top;
    const float passing = field.sideways[node];
    if (passing <= 0) return rest;
    const auto carries = [&](float level) { return wetShare(column, storey, level) * (field.room[node] - field.pull[node] * (column.ground - level)); };
    if (carries(rest) >= passing) return rest;
    float low = rest, high = top;
    for (int step = 0; step < 18; ++step) {
        const float middle = 0.5f * (low + high);
        (carries(middle) >= passing ? high : low) = middle;
    }
    return high;
}

// The neighbour most of a node's water arrives from; 8 for the storey above, 9 for the regional flow, 255 for its own recharge.
uint8_t mainSource(const WaterField& field, int x, int z, int storey) {
    const size_t i = field.index(x, z);
    const AquiferColumn& column = field.column[i];
    const bool fed = storey == Upper || !column.perched;
    float most = fed ? column.recharge + column.sunk : 0.0f;
    uint8_t source = 255;
    if (storey == Lower && column.regional > most) most = column.regional, source = 9;
    if (!fed) {
        const float down = splitNode(field, i, Upper, field.flux[i * 2 + Upper]).down;
        if (down > most) most = down, source = 8;
    }
    for (int k = 0; k < 8; ++k) {
        const int nx = x - NeighborX[k], nz = z - NeighborZ[k];
        if (nx < 0 || nz < 0 || nx >= field.size || nz >= field.size) continue;
        const size_t j = field.index(nx, nz), upper = j * 2 + Upper, lower = j * 2 + Lower;
        const Way way = Way(field.way[j * 8 + size_t(k)]);
        float flow = 0;
        if (storey == Lower) flow = field.sideways[lower] * field.share[lower * 8 + size_t(k)] + (way == Way::Under ? field.sideways[upper] * field.share[upper * 8 + size_t(k)] : 0.0f);
        else if (way == Way::Along) flow = field.sideways[upper] * field.share[upper * 8 + size_t(k)];
        if (flow > most) most = flow, source = uint8_t((k + 4) & 7);
    }
    return source;
}

// m³/yr a node passes to neighbours that are the tile's own cells, or to those that are not.
double crossing(const WaterField& field, int x, int z, int storey, bool toOwned) {
    const size_t i = field.index(x, z), node = i * 2 + size_t(storey);
    double total = 0;
    for (int k = 0; k < 8; ++k) {
        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
        if (nx < 0 || nz < 0 || nx >= field.size || nz >= field.size || field.owned(nx, nz) != toOwned) continue;
        if (storey == Upper && Way(field.way[i * 8 + size_t(k)]) == Way::Outcrop) continue;
        total += double(field.sideways[node]) * field.share[node * 8 + size_t(k)];
    }
    return total;
}

} // namespace

void readFields(const WaterField& field, GroundwaterTile& tile) {
    const size_t cells = size_t(TileCells) * TileCells;
    tile.key = field.key;
    tile.table.resize(cells), tile.deepTable.resize(cells), tile.recharge.resize(cells);
    tile.emergence.assign(cells, 0.0f);
    tile.state.assign(cells, 0);
    for (int storey = Upper; storey <= Lower; ++storey) tile.rock[storey].assign(cells, uint8_t(Material::Air)), tile.from[storey].assign(cells, 255);
    WaterBudget& budget = tile.budget;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const int fx = x + field.margin, fz = z + field.margin;
            const size_t i = field.index(fx, fz), own = size_t(z) * TileCells + size_t(x);
            const AquiferColumn& column = field.column[i];
            tile.recharge[own] = column.recharge / WaterCellArea;
            tile.deepTable[own] = tableOf(field, i, Lower);
            const bool wet = column.perched && field.flux[i * 2 + Upper] > LeastSeep;
            tile.table[own] = wet ? tableOf(field, i, Upper) : tile.deepTable[own];
            tile.state[own] = uint8_t((column.perched ? HasPerched : 0) | (field.karst[i * 2 + Upper] ? UpperKarst : 0) | (field.karst[i * 2 + Lower] ? LowerKarst : 0) |
                                      (column.drain != Drain::None ? Drained : 0) | (column.sunk >= LeastSpring ? Swallows : 0));
            tile.rock[Lower][own] = uint8_t(column.lowerRock);
            if (column.perched) tile.rock[Upper][own] = uint8_t(column.upperRock);
            budget.rain += double(column.rain) * WaterCellArea;
            budget.runoff += double(runoffOf(column.rain, column.warmth)) * WaterCellArea;
            budget.recharge += column.recharge, budget.sunk += column.sunk, budget.regional += column.regional;
            for (int storey = column.perched ? Upper : Lower; storey <= Lower; ++storey) {
                const NodeFate& fate = field.fate[i * 2 + size_t(storey)];
                tile.from[storey][own] = mainSource(field, fx, fz, storey);
                budget.deep += fate.lost, budget.underflow += fate.lag;
                budget.lateralOut += crossing(field, fx, fz, storey, false);
            }
        }
    // What the ring around the tile passes in.
    for (int z = field.margin - 1; z <= field.size - field.margin; ++z)
        for (int x = field.margin - 1; x <= field.size - field.margin; ++x) {
            if (field.owned(x, z) || x < 0 || z < 0 || x >= field.size || z >= field.size) continue;
            const size_t i = field.index(x, z);
            for (int storey = field.column[i].perched ? Upper : Lower; storey <= Lower; ++storey) budget.lateralIn += crossing(field, x, z, storey, true);
        }
}

} // namespace derivean::sim
