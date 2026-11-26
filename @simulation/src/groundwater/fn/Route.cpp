#include "groundwater/fn/Stages.hpp"

#include "groundwater/fn/Chemistry.hpp"
#include "groundwater/fn/Route.hpp"

namespace derivean::sim {
namespace {

constexpr float MagmaticGas = 4000.0f;   // times the air's carbon dioxide that cooling melt gives deep water above it
constexpr float GasDepth = 400.0f;       // m of circulation depth at which water meets that gas in full
constexpr float OldestWater = 1.0e6f;    // yr
constexpr float HottestRock = 350.0f;    // °C

// Flow-weighted sum of what arriving water carries.
struct Mixing {
    float flux{};
    Carried sum;

    void take(float flow, const Carried& from, float step) {
        if (flow <= 0) return;
        flux += flow;
        sum.lime += flow * from.lime, sum.far += flow * (from.far + step), sum.high += flow * from.high;
        sum.rate += flow * from.rate, sum.age += flow * from.age, sum.deep += flow * from.deep;
    }
};

// What the water takes on in the node it has reached.
Carried passedThrough(const WaterField& field, size_t cell, int storey, const Mixing& mix) {
    const AquiferColumn& column = field.column[cell];
    const float inverse = 1.0f / mix.flux;
    Carried out{mix.sum.lime * inverse, mix.sum.far * inverse, mix.sum.high * inverse, mix.sum.rate * inverse, mix.sum.age * inverse, mix.sum.deep * inverse};
    const float depth = flowDepth(field, cell, storey, out.high);
    out.deep = std::max(out.deep, depth);
    // Carbonate: up to what the rock's temperature and the gas in the water allow, as far as the
    // storey is soluble rock at all. Melt cooling below charges deep water with carbon dioxide.
    const float soluble = storey == Upper ? column.upperSoluble / std::max(1.0f, std::min(20.0f, column.ground - column.floor))
                                          : std::max(0.0f, column.solubleTop - column.solubleBottom) / 60.0f;
    const float gas = AirCarbonDioxide * (1.0f + MagmaticGas * column.heat.magmatic * std::min(1.0f, depth / GasDepth));
    const float warm = std::min(HottestRock, rockTemperature(column.heat, column.warmth, depth));
    out.lime += std::max(0.0f, calciteLoad(gas, warm) - out.lime) * std::min(1.0f, 4.0f * soluble);
    const float level = field.level[cell * 2 + size_t(storey)];
    const float wet = storey == Upper ? 0.3f : std::clamp((level - column.base) / std::max(1.0f, column.roof - column.base), 0.05f, 1.0f);
    out.age = std::min(OldestWater, out.age + (storey == Upper ? column.upperStore : column.lowerStore) * wet * CellArea * inverse);
    return out;
}

} // namespace

float flowDepth(const WaterField& field, size_t cell, int storey, float high) {
    const AquiferColumn& column = field.column[cell];
    if (storey == Upper) return std::max(0.0f, column.ground - column.floor);
    const float level = std::min(field.level[cell * 2 + Lower], column.ground), through = column.shallowT + column.deepT;
    float depth = column.ground - level;
    if (through > 0) depth += column.deepT / through * std::min(60.0f, 0.5f * std::max(0.0f, std::min(level, column.shallowBase) - column.base));
    // Down a fault water goes as deep as the head of its catchment drives it and the fault stays open.
    return depth + std::min(column.crush * column.crush * FaultDepth, DriveRatio * std::max(0.0f, high - column.ground));
}

NodeSplit splitNode(const WaterField& field, size_t cell, int storey, float flux) {
    const AquiferColumn& column = field.column[cell];
    const size_t node = cell * 2 + size_t(storey);
    NodeSplit split;
    float rest = flux;
    if (storey == Upper) rest -= split.down = std::min(rest, column.floorLeak);
    else rest -= split.lost = std::min(rest, column.baseLeak);
    if (field.karst[node]) {
        split.captured = rest;
    } else if (column.drain != Drain::None) {
        split.out = rest;
    } else {
        // Water under a tight bed cannot come up through it: it goes on while anywhere lower is left.
        const bool held = storey == Lower && column.perched && field.room[node] > 0;
        split.side = held ? rest : std::min(rest, field.room[node]);
        split.out = rest - split.side;
    }
    return split;
}

void routeGroundWater(WaterField& field) {
    const int size = field.size;
    const size_t cells = field.cells();
    std::vector<float> side(cells * 2, 0.0f), sideNext(cells * 2, 0.0f), down(cells, 0.0f), downNext(cells, 0.0f), fluxNext(cells * 2, 0.0f);
    std::vector<Carried> carriedNext(cells * 2);
    field.flux.assign(cells * 2, 0.0f);
    field.carried.assign(cells * 2, Carried{});
    field.fate.assign(cells * 2, NodeFate{});

    // Every sweep a node gathers what its neighbours passed on in the sweep before, so water moves
    // one cell a sweep and a node's flux only grows. The sweep after the last finds what was still
    // under way: that water is booked where it would have arrived.
    for (int sweep = 0; sweep <= RouteSweeps; ++sweep) {
        const bool beyond = sweep == RouteSweeps;
        forRows(field, [&](int begin, int end) {
            for (int z = begin; z < end; ++z)
                for (int x = 0; x < size; ++x) {
                    const size_t i = field.index(x, z);
                    const AquiferColumn& column = field.column[i];
                    for (int storey = column.perched ? Upper : Lower; storey <= Lower; ++storey) {
                        const size_t node = i * 2 + size_t(storey);
                        Mixing mix;
                        if (storey == Upper || !column.perched) mix.take(column.recharge + column.sunk, Carried{0, 0, column.ground, column.recharge / CellArea, 0, 0}, 0);
                        else mix.take(down[i], field.carried[i * 2 + Upper], 0);
                        for (int k = 0; k < 8; ++k) {
                            const int nx = x - NeighborX[k], nz = z - NeighborZ[k];
                            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                            const size_t j = field.index(nx, nz), upper = j * 2 + Upper, lower = j * 2 + Lower;
                            const float step = float(LocalCell) * NeighborReach[k];
                            const Way way = Way(field.way[j * 8 + size_t(k)]);
                            if (storey == Lower) {
                                mix.take(side[lower] * field.share[lower * 8 + size_t(k)], field.carried[lower], step);
                                if (way == Way::Under) mix.take(side[upper] * field.share[upper * 8 + size_t(k)], field.carried[upper], step);
                            } else if (way == Way::Along) {
                                mix.take(side[upper] * field.share[upper * 8 + size_t(k)], field.carried[upper], step);
                            }
                        }
                        if (beyond) {
                            field.fate[node].lag = mix.flux - field.flux[node];
                            continue;
                        }
                        const NodeSplit split = splitNode(field, i, storey, mix.flux);
                        fluxNext[node] = mix.flux;
                        sideNext[node] = split.side;
                        if (storey == Upper) downNext[i] = split.down;
                        carriedNext[node] = mix.flux > 0 ? passedThrough(field, i, storey, mix) : Carried{};
                    }
                }
        });
        if (beyond) break;
        field.flux.swap(fluxNext), side.swap(sideNext), down.swap(downNext), field.carried.swap(carriedNext);
    }

    field.sideways = side;
    forRows(field, [&](int begin, int end) {
        for (int z = begin; z < end; ++z)
            for (int x = 0; x < size; ++x) {
                const size_t i = field.index(x, z);
                for (int storey = field.column[i].perched ? Upper : Lower; storey <= Lower; ++storey) {
                    const size_t node = i * 2 + size_t(storey);
                    const NodeSplit split = splitNode(field, i, storey, field.flux[node]);
                    NodeFate& fate = field.fate[node];
                    fate.emerges = split.out, fate.lost = split.lost, fate.captured = split.captured;
                    if (storey == Upper)
                        for (int k = 0; k < 8; ++k)
                            if (Way(field.way[i * 8 + size_t(k)]) == Way::Outcrop) fate.outcrop += split.side * field.share[node * 8 + size_t(k)];
                }
            }
    });
}

} // namespace derivean::sim
