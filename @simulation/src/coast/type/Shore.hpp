#pragma once

#include "coast/constant/Waves.hpp"
#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// Totals in m³ of solid over every cell of a domain for one replay. They close: loose cover and
// surf debris at the end = at the start + cut − dissolved + entered − left.
struct CoastLedger {
    double cut{};         // rock the waves broke off
    double dissolved{};   // the part of it that went into solution
    double entered{};     // sand and gravel the surf lifts from the bed of cells that do not evolve
    double left{};        // what such cells take off a train (dropped, stranded, slid onto them) and mud the surf takes out of reach
};

inline constexpr float NoWater = -1e30f;

// The coast's working state of one replay. Per-cell arrays have Domain::cells() entries; the wave
// arrays have WaveDirections values per cell.
struct Shore {
    std::vector<float> gale;      // m²/s², per train: square of the storm wind that drives it over a cell; fixed for a replay
    std::vector<float> beyond;    // m², per train: what arrives at a Rim cell from outside the domain; empty where none does
    std::vector<float> wave;      // m², per train: what leaves a cell
    std::vector<float> arrive;    // m², per train: what reaches a cell, before the cell's own depth acts on it
    std::vector<float> crest;     // m², per train: the highest it was on its way to the cell, before the depth broke it (the breaker height of the beach laws)
    std::vector<float> top;       // m, rock plus loose cover as the coast works on it
    std::vector<float> stood;     // m, water surface where water stood when the step was flooded; NoWater elsewhere
    std::vector<float> still;     // m, still water that waves ride on to reach a cell; NoWater where none can
    std::vector<float> notch;     // m the foot of each column has been cut back since it last came down
    std::vector<float> pending[SedimentClasses];   // m of broken rock in the surf of a cell, not yet carried off or laid
    std::vector<float> cut, laid; // m of rock the waves took and of cover they left, net, over the replay
    std::vector<float> before;    // m of loose cover when the step's work began
    std::vector<float> load[SedimentClasses];      // m of sand and gravel on its way through a cell in the current sweep
    std::vector<uint8_t> surf;    // 1 where some train can move the bed or runs over dry ground; 2 where a load waits in the current sweep
    int minX{}, maxX{-1}, minZ{}, maxZ{-1};        // box around every cell with still water, one cell wider
    CoastLedger ledger;
};

// What a finished tile keeps of its shore. Null for a tile no standing water reaches.
struct ShoreRecord {
    // Per cell and train, in steps of WaveQuantum: the height that leaves a cell of water and the
    // height that strikes a cell of land. A finer level reads its open boundary from this.
    std::vector<uint8_t> height;
    std::vector<float> notch;       // m the foot of the column is cut back: the notch under a face, the bench before it where wider
    std::vector<float> cut, laid;   // m, as in Shore
    CoastLedger ledger;
};

// m of water under which a flooded cell is wet ground, not a body of water: the flood of a step
// leaves such films in every flat a river crosses.
inline constexpr float Film = 0.1f;

// What a step's flood left standing over ground at `top`: the sea, and lakes their rivers still reach.
// Standing is judged against `Domain::surface`, the ground the flood was computed on: rivers and ice
// cut their beds after the flood within a step, and a bed cut below a level that has not been
// flooded again is a channel, not a lake.
inline bool standsOver(const Domain& domain, size_t i, float top) {
    const float level = domain.level[i];
    return level > top + Film && level > domain.surface[i] + Film && (level <= float(domain.seaLevel) + SeaBand || domain.discharge[i] > 0);
}

// Half the range the waterline of water standing at `water` moves through: the sea's or a lake's.
inline float swingAt(float water, double seaLevel) { return water <= float(seaLevel) + SeaBand ? SeaSwing : LakeSwing; }
inline float swingOf(const Domain& domain, float water) { return swingAt(water, domain.seaLevel); }

// Calls `visit(x, z)` for every cell of the shore's box so that a cell comes after the three it
// receives a train travelling towards neighbour `k` from, and before the three it hands it to.
template<typename Visit>
void sweepDownwind(const Shore& shore, int k, Visit&& visit) {
    const int dx = NeighborX[k], dz = NeighborZ[k];
    if (dz == 0) {
        for (int x = dx > 0 ? shore.minX : shore.maxX; x >= shore.minX && x <= shore.maxX; x += dx)
            for (int z = shore.minZ; z <= shore.maxZ; ++z) visit(x, z);
        return;
    }
    for (int z = dz > 0 ? shore.minZ : shore.maxZ; z >= shore.minZ && z <= shore.maxZ; z += dz)
        for (int x = dx >= 0 ? shore.minX : shore.maxX; x >= shore.minX && x <= shore.maxX; x += dx >= 0 ? 1 : -1) visit(x, z);
}

} // namespace derivean::sim
