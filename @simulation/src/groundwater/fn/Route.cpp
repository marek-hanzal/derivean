#include "groundwater/fn/Stages.hpp"

#include "groundwater/fn/Chemistry.hpp"
#include "groundwater/fn/Route.hpp"

#include <algorithm>
#include <bit>

namespace derivean::sim {
namespace {

constexpr float MagmaticGas = 4000.0f;   // times the air's carbon dioxide that cooling melt gives deep water above it
constexpr float GasDepth = 400.0f;       // m of circulation depth at which water meets that gas in full
constexpr float OldestWater = 1.0e6f;    // yr

// Flow-weighted sum of what arriving water carries.
struct Mixing {
    float flux{};
    Borne sum;
};

// With `carried` false only the flux is summed, in the same order.
void take(Mixing& mix, float flow, const Borne& from, float step, bool carried = true) {
    if (flow <= 0) return;
    mix.flux += flow;
    if (!carried) return;
    mix.sum.lime += flow * from.lime, mix.sum.far += flow * (from.far + step), mix.sum.high += flow * from.high;
    mix.sum.rate += flow * from.rate, mix.sum.age += flow * from.age, mix.sum.deep += flow * from.deep;
}

// What the water takes on in the node it has reached.
Borne passedThrough(const WaterField& field, size_t cell, int storey, const Mixing& mix) {
    const AquiferColumn& column = field.column[cell];
    const float inverse = 1.0f / mix.flux;
    Borne out{mix.sum.lime * inverse, mix.sum.far * inverse, mix.sum.high * inverse, mix.sum.rate * inverse, mix.sum.age * inverse, mix.sum.deep * inverse};
    const float depth = flowDepth(field, cell, storey, out.high);
    out.deep = std::max(out.deep, depth);
    // Carbonate: up to what the rock's temperature and the gas in the water allow, as far as the
    // storey is soluble rock at all. Melt cooling below charges deep water with carbon dioxide.
    const float soluble = storey == Upper ? column.upperSoluble / std::max(1.0f, std::min(20.0f, column.ground - column.floor))
                                          : std::max(0.0f, column.solubleTop - column.solubleBottom) / 60.0f;
    // Where the storey holds none it would add max(0, load - lime) × 0 = +0, and lime is never -0
    // (it is a flow-weighted mean of loads that start at +0), so the load is not worked out there.
    if (soluble != 0.0f) {
        const float gas = AirCarbonDioxide * (1.0f + MagmaticGas * column.heat.magmatic * std::min(1.0f, depth / GasDepth));
        const float warm = std::min(HottestRock, rockTemperature(column.heat, column.warmth, depth));
        out.lime += std::max(0.0f, calciteLoad(gas, warm) - out.lime) * std::min(1.0f, 4.0f * soluble);
    }
    const float level = field.level[cell * 2 + size_t(storey)];
    const float wet = storey == Upper ? 0.3f : std::clamp((level - column.base) / std::max(1.0f, column.roof - column.base), 0.05f, 1.0f);
    out.age = std::min(OldestWater, out.age + (storey == Upper ? column.upperStore : column.lowerStore) * wet * WaterCellArea * inverse);
    return out;
}

} // namespace

float flowDepth(const WaterField& field, size_t cell, int storey, float high) {
    const AquiferColumn& column = field.column[cell];
    if (storey == Upper) return std::max(0.0f, column.ground - column.floor);
    const float level = std::min(field.level[cell * 2 + Lower], column.ground), through = lowerT(column);
    float depth = column.ground - level;
    if (through > 0) depth += (column.deepT[0] + column.deepT[1]) / through * std::min(60.0f, 0.5f * std::max(0.0f, std::min(level, column.shallowBase) - column.base));
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
    } else if (storey == Upper ? column.drain != Drain::None : drainsLower(column)) {
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
    std::vector<Borne> borneNext(cells * 2);
    // Per row: whether a node of it passed on other water or other loads in the last sweep.
    std::vector<uint8_t> rowChanged(size_t(size), 1), rowNext(size_t(size), 0);
    field.flux.assign(cells * 2, 0.0f);
    field.borne.assign(cells * 2, Borne{});
    field.fate.assign(cells * 2, NodeFate{});
    const auto same = [](float a, float b) { return std::bit_cast<uint32_t>(a) == std::bit_cast<uint32_t>(b); };
    const auto sameBorne = [&](const Borne& a, const Borne& b) {
        return same(a.lime, b.lime) && same(a.far, b.far) && same(a.high, b.high) && same(a.rate, b.rate) && same(a.age, b.age) && same(a.deep, b.deep);
    };

    // Every sweep a node gathers what its neighbours passed on in the sweep before, so water moves
    // one cell a sweep and a node's flux only grows. The sweep after the last finds what was still
    // under way: that water is booked where it would have arrived. A node reads only its own row
    // and the rows beside it, so a row none of whose rows around changed in the last sweep gathers
    // what it gathered then: it is copied, and in the sweep beyond it has nothing under way (its
    // lag would be its flux less itself, 0). Nodes no sweep writes hold their start in both buffers.
    // Both short cuts, and skipping the neighbours in the first sweep, take recharges, levels and
    // shares to be finite, as the budget of every tile does (testGroundwaterTile).
    for (int sweep = 0; sweep <= RouteSweeps; ++sweep) {
        const bool beyond = sweep == RouteSweeps;
        forRows(field, [&](int begin, int end) {
            for (int z = begin; z < end; ++z) {
                const size_t row = field.index(0, z);
                if (sweep > 0 && !rowChanged[size_t(z)] && !(z > 0 && rowChanged[size_t(z) - 1]) && !(z + 1 < size && rowChanged[size_t(z) + 1])) {
                    if (!beyond) {
                        const auto from = std::ptrdiff_t(row * 2);
                        std::copy_n(field.flux.begin() + from, size * 2, fluxNext.begin() + from);
                        std::copy_n(side.begin() + from, size * 2, sideNext.begin() + from);
                        std::copy_n(field.borne.begin() + from, size * 2, borneNext.begin() + from);
                        std::copy_n(down.begin() + std::ptrdiff_t(row), size, downNext.begin() + std::ptrdiff_t(row));
                    }
                    rowNext[size_t(z)] = 0;
                    continue;
                }
                uint8_t anyChanged = 0;
                for (int x = 0; x < size; ++x) {
                    const size_t i = field.index(x, z);
                    const AquiferColumn& column = field.column[i];
                    for (int storey = column.perched ? Upper : Lower; storey <= Lower; ++storey) {
                        const size_t node = i * 2 + size_t(storey);
                        Mixing mix;
                        // The sweep beyond reads only the flux.
                        const bool carried = !beyond;
                        if (storey == Upper || !column.perched) take(mix, shallowRecharge(column) + column.sunk, Borne{0, 0, column.ground, column.recharge / WaterCellArea, 0, 0}, 0, carried);
                        else take(mix, down[i], field.borne[i * 2 + Upper], 0, carried);
                        if (storey == Lower && column.regional > 0) take(mix, column.regional, field.upwelled[i], 0, carried);
                        // Nothing is under way before the first sweep: every side flow is 0 and every share finite.
                        for (int k = 0; k < 8 && sweep > 0; ++k) {
                            const int nx = x - NeighborX[k], nz = z - NeighborZ[k];
                            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                            const size_t j = field.index(nx, nz), upper = j * 2 + Upper, lower = j * 2 + Lower;
                            const float step = float(WaterCell) * NeighborReach[k];
                            const Way way = Way(field.way[j * 8 + size_t(k)]);
                            if (storey == Lower) {
                                take(mix, side[lower] * field.share[lower * 8 + size_t(k)], field.borne[lower], step, carried);
                                if (way == Way::Under) take(mix, side[upper] * field.share[upper * 8 + size_t(k)], field.borne[upper], step, carried);
                            } else if (way == Way::Along) {
                                take(mix, side[upper] * field.share[upper * 8 + size_t(k)], field.borne[upper], step, carried);
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
                        borneNext[node] = mix.flux > 0 ? passedThrough(field, i, storey, mix) : Borne{};
                        anyChanged |= uint8_t(!same(sideNext[node], side[node]) || !sameBorne(borneNext[node], field.borne[node]) || (storey == Upper && !same(downNext[i], down[i])));
                    }
                }
                rowNext[size_t(z)] = anyChanged;
            }
        });
        if (beyond) break;
        field.flux.swap(fluxNext), side.swap(sideNext), down.swap(downNext), field.borne.swap(borneNext);
        rowChanged.swap(rowNext);
        // Once no row changes, the sweeps left and the one beyond would change nothing.
        if (std::none_of(rowChanged.begin(), rowChanged.end(), [](uint8_t changed) { return changed != 0; })) break;
    }

    field.sideways = std::move(side);
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
