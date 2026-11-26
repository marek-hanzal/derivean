#include "groundwater/fn/Stages.hpp"

namespace derivean::sim {
namespace {

constexpr float Face[8] = {2.0f / 3, 1.0f / 6, 2.0f / 3, 1.0f / 6, 2.0f / 3, 1.0f / 6, 2.0f / 3, 1.0f / 6};   // nine-point weights of a flux
constexpr float Lip = 0.05f;   // m of water over the lip of an outcrop

// How readily a column passes water towards neighbour k, against the mean of all directions.
float ease(const AquiferColumn& column, int k) {
    const float x = float(NeighborX[k]) / NeighborReach[k], z = float(NeighborZ[k]) / NeighborReach[k];
    return std::max(0.05f, column.spread[0] * x * x + 2.0f * column.spread[1] * x * z + column.spread[2] * z * z);
}

// Least gradient a storey's water table takes: what carries the recharge of a hillside through it.
float leastGradient(const AquiferColumn& column, float transmissivity) {
    if (transmissivity <= 0) return MostGradient;
    return std::clamp(shallowRecharge(column) / WaterCellArea * HillLength / transmissivity, LeastGradient, MostGradient);
}

float harmonic(float a, float b) { return a + b > 0 ? 2.0f * a * b / (a + b) : 0.0f; }

bool sameSheet(const AquiferColumn& a, const AquiferColumn& b) {
    return a.perched && b.perched && a.sheet == b.sheet && std::abs(a.sheetOffset - b.sheetOffset) < SameSheet;
}

// The tight bed is the top of the solid rock: whatever rock lay on it is gone and the bed crops out.
bool stripped(const AquiferColumn& column) { return column.floor >= column.bedrock - 0.5f; }

// Which way the upper storey's water leaves each cell.
void findWays(WaterField& field) {
    const int size = field.size;
    field.way.assign(field.cells() * 8, uint8_t(Way::Closed));
    forRows(field, [&](int begin, int end) {
        for (int z = begin; z < end; ++z)
            for (int x = 0; x < size; ++x) {
                const size_t i = field.index(x, z);
                const AquiferColumn& column = field.column[i];
                if (!column.perched) continue;
                for (int k = 0; k < 8; ++k) {
                    const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                    if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                    const AquiferColumn& other = field.column[field.index(nx, nz)];
                    // Water that leaves rock for a neighbour where the same bed lies bare has come out
                    // at the bed's lip; water already moving in the cover on the bed moves on in it.
                    const bool along = sameSheet(column, other) && (stripped(column) || !stripped(other));
                    const bool daylight = sameSheet(column, other) || sheetToward(column, k) >= other.ground - 0.5f;
                    field.way[i * 8 + size_t(k)] = uint8_t(along ? Way::Along : daylight ? Way::Outcrop : Way::Under);
                }
            }
    });
}

// The stand of water each storey takes when little arrives: the level of its outlets, rising away
// from them by the least gradient, never under its floor and never over the ground. One sweep
// carries an outlet's level one cell farther.
//
// An open lower storey stands on the drain it rises least from, not on the lowest one in reach.
// The water a Local field follows is the shallow share (the regional flow takes the deep one,
// fn/Regional.cpp), and shallow water comes out at the stream nearest it: a stream that does not
// lose (fn/Columns.cpp) is in contact with the ground water beside it, which therefore cannot stand
// under its stage, however far down the valley a lower drain lies. Under a tight bed, out of
// contact with the streams above, and where no drain is in reach, the lowest stand around is taken.
void spreadLevels(WaterField& field) {
    const int size = field.size;
    const size_t cells = field.cells();
    constexpr float Unreached = 1e30f;
    std::vector<float> next(cells * 2), spill(cells, 1e9f), rise(cells, Unreached), riseNext(cells);
    field.level.resize(cells * 2);
    for (size_t i = 0; i < cells; ++i) {
        const AquiferColumn& column = field.column[i];
        if (column.perched) {
            if (column.drain != Drain::None) spill[i] = std::max(column.floor, column.stage);
            for (int k = 0; k < 8; ++k) {
                const Way way = Way(field.way[i * 8 + size_t(k)]);
                if (way != Way::Outcrop && way != Way::Under) continue;
                const AquiferColumn& other = field.column[field.index(int(i % size_t(size)) + NeighborX[k], int(i / size_t(size)) + NeighborZ[k])];
                spill[i] = std::min(spill[i], std::max(column.floor, std::min(sheetToward(column, k), other.ground)));
            }
        }
        field.level[i * 2 + Upper] = column.perched ? std::clamp(spill[i], column.floor, std::max(column.floor, column.ground)) : column.ground;
        field.level[i * 2 + Lower] = drainsLower(column) ? column.stage : column.ground;
        if (drainsLower(column)) rise[i] = 0;
    }
    for (int sweep = 0; sweep < LevelSweeps; ++sweep) {
        forRows(field, [&](int begin, int end) {
            for (int z = begin; z < end; ++z)
                for (int x = 0; x < size; ++x) {
                    const size_t i = field.index(x, z);
                    const AquiferColumn& column = field.column[i];
                    const float upperOwn = leastGradient(column, column.upperT), lowerOwn = leastGradient(column, lowerT(column));
                    // `nearest` is the stand by the drain it rises least from, `lowest` the lowest stand around.
                    float upper = spill[i], lowest = column.ground, nearest = column.ground, least = Unreached;
                    for (int k = 0; k < 8; ++k) {
                        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                        if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                        const size_t j = field.index(nx, nz);
                        const AquiferColumn& other = field.column[j];
                        const float run = float(WaterCell) * NeighborReach[k] * 0.5f;
                        const float up = run * (lowerOwn / ease(column, k) + leastGradient(other, lowerT(other)) / ease(other, k));
                        const float stand = field.level[j * 2 + Lower] + up;
                        lowest = std::min(lowest, stand);
                        if (rise[j] < Unreached && (rise[j] + up < least || (rise[j] + up == least && stand < nearest))) least = rise[j] + up, nearest = stand;
                        if (Way(field.way[i * 8 + size_t(k)]) == Way::Along)
                            upper = std::min(upper, field.level[j * 2 + Upper] + run * (upperOwn / ease(column, k) + leastGradient(other, other.upperT) / ease(other, k)));
                    }
                    next[i * 2 + Upper] = column.perched ? std::clamp(upper, column.floor, std::max(column.floor, column.ground)) : column.ground;
                    const bool drains = drainsLower(column), reached = !column.perched && least < Unreached;
                    next[i * 2 + Lower] = drains ? column.stage : std::max(column.base, reached ? std::min(column.ground, nearest) : lowest);
                    riseNext[i] = drains ? 0.0f : reached ? least : Unreached;
                }
        });
        field.level.swap(next);
        rise.swap(riseNext);
    }
}

// The shares in which a node's sideways flow leaves towards its neighbours, by conductance times
// fall of the level, and what it could pass if its water stood at the ground.
void findShares(WaterField& field) {
    const int size = field.size;
    const size_t cells = field.cells();
    field.share.assign(cells * 16, 0.0f);
    field.room.assign(cells * 2, 0.0f);
    field.pull.assign(cells * 2, 0.0f);
    field.karst.assign(cells * 2, 0);
    forRows(field, [&](int begin, int end) {
        for (int z = begin; z < end; ++z)
            for (int x = 0; x < size; ++x) {
                const size_t i = field.index(x, z);
                const AquiferColumn& column = field.column[i];
                for (int storey = column.perched ? Upper : Lower; storey <= Lower; ++storey) {
                    const size_t node = i * 2 + size_t(storey);
                    const float here = field.level[node];
                    float* share = &field.share[node * 8];
                    float total = 0, room = 0, pull = 0;
                    for (int k = 0; k < 8; ++k) {
                        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                        if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                        const size_t j = field.index(nx, nz);
                        const AquiferColumn& other = field.column[j];
                        float target, through;
                        if (storey == Lower) {
                            target = field.level[j * 2 + Lower], through = harmonic(lowerT(column), lowerT(other));
                        } else if (Way(field.way[i * 8 + size_t(k)]) == Way::Along) {
                            target = field.level[j * 2 + Upper], through = harmonic(column.upperT, other.upperT);
                        } else {
                            // Out over the lip of the bed, or on along it under the neighbour's ground.
                            target = std::max(column.floor, std::min(sheetToward(column, k), other.ground)) - Lip, through = column.upperT;
                        }
                        if (target >= here) continue;
                        const float conductance = Face[k] * through * 0.5f * (ease(column, k) + ease(other, k));
                        share[k] = conductance * (here - target);
                        total += share[k];
                        room += conductance * std::max(0.0f, column.ground - target);
                        pull += conductance;
                    }
                    if (total > 0)
                        for (int k = 0; k < 8; ++k) share[k] /= total;
                    field.room[node] = room;
                    field.pull[node] = pull;
                }
                // Soluble rock drains by conduits where the water moves in it: where the water table lies in
                // it or just over it, and where water runs through it on a tight bed.
                const float rest = field.level[i * 2 + Lower];
                field.karst[i * 2 + Upper] = column.perched && column.upperSoluble >= KarstThickness;
                field.karst[i * 2 + Lower] = column.solubleTop - column.solubleBottom >= KarstThickness && rest > column.solubleBottom + 1.0f && rest < column.solubleTop + KarstReach;
            }
    });
}

} // namespace

void settleLevels(WaterField& field) {
    findWays(field);
    spreadLevels(field);
    findShares(field);
}

} // namespace derivean::sim
