#pragma once

#include "aeolian/type/Aeolian.hpp"
#include "groundwater/type/Sinking.hpp"
#include "landscape/type/Level.hpp"
#include "ore/type/Ore.hpp"
#include "strata/type/Column.hpp"
#include "volcanic/type/Pile.hpp"

#include <memory>
#include <vector>

namespace derivean::sim {

struct GroundDetail;
struct Shore;
struct ShoreRecord;
struct SheetRecord;
struct SheetWork;
class Workers;

// Loose sediment is tracked in three transport classes; solutes leave with the water.
inline constexpr int SedimentClasses = 3;
enum SedimentClass { Coarse, Sand, Fines };

// The evolving state of a column. `datum` rides up with the rock and never erodes, so
// `datum - rock` is how deep the surface has cut into the layering.
struct Ground {
    std::vector<float> rock;      // m, top of solid rock
    std::vector<float> datum;     // m, original top of the cover
    std::vector<float> sediment[SedimentClasses];   // m of loose material on the rock

    void resize(size_t cells) {
        rock.assign(cells, 0.0f);
        datum.assign(cells, 0.0f);
        for (auto& layer : sediment) layer.assign(cells, 0.0f);
    }
};

// Totals in m³ of solid over the tile's own cells for the replayed window.
struct TileLedger {
    double uplifted{}, eroded{}, deposited{}, weathered{}, dissolved{}, failed{};
    double sedimentIn{}, sedimentOut{};   // carried across the domain edge by rivers
};

// Rock the ice of the last glacial maximum passed over on its way to a place: where the boulders
// it left there come from. One entry per parent cell under a site tile.
struct IceSource {
    Vec2 source;                        // m, where that rock crops out
    Material rock{Material::Air};       // Air where no ice came
};

// What a finished tile keeps. All arrays have TileCells² entries, row-major.
struct Tile {
    TileKey key;
    double time{};                  // yr, end of the replay
    double seaLevel{};
    Ground now;                     // state at `time`
    // The same state as this tile's own replay left it, including Apron cells beyond its edge. A
    // later neighbour blends towards it so the shared edge carries no step.
    // At Local and Site it is the ground before the last ice sheet passed (landscape/fn/Replay.cpp).
    Ground nowWide;                 // (TileCells + 2·Apron)² entries
    std::vector<float> water;       // m, water surface where water stands or flows; below ground when dry
    std::vector<float> discharge;   // m³/yr, mean annual
    std::vector<float> sunk;        // m³/yr that went down into soluble rock in each cell: soaked in and taken from streams (groundwater/type/Sinking.hpp); empty where none did
    std::vector<float> seeped;      // m³/yr of passing flood water each cell's loose bed took in (aeolian/fn/Soak.cpp), the groundwater's recharge; empty where none did
    std::vector<float> load;        // m³/yr of sediment the water carries in
    std::vector<uint8_t> flowTo;    // neighbour index 0..7 the main flow leaves by, 255 for none
    std::vector<uint8_t> rock;      // Material of the solid rock at the top of the column
    std::vector<float> uplift;      // m/yr, rock uplift at the end
    std::vector<float> rebound;     // m/yr the plate floats back up as erosion unloads it; carried down to children
    std::vector<float> ice;         // m of glacier ice under the present climate
    std::vector<float> iceAge;      // m of ice at the last glacial maximum
    std::vector<float> erosionRate, depositionRate;   // m/yr, recent mean
    std::vector<float> eroded, deposited, uplifted;   // m over the window
    std::vector<float> rain, warmth;                  // m/yr and °C used at the end
    std::vector<float> shed;        // m/yr of rock breaking off faces too steep to stand, recent mean
    std::vector<IceSource> upIce;   // site tiles only: (TileCells / Refinement)² entries, row-major
    std::shared_ptr<const GroundDetail> detail;   // ground tiles only: stones, mineral crusts and what the cover is made of
    std::shared_ptr<const ShoreRecord> shore;     // waves at every cell and what they cut and laid (coast/type/Shore.hpp); null where no water stands
    std::shared_ptr<const VolcanicTile> volcanic; // rock laid on top of the layering by eruptions, and the vents (volcanic/type/Pile.hpp); null where there is none
    std::shared_ptr<const SheetRecord> sheet;     // what continental ice wore off and left (icesheet/type/Sheet.hpp); null where no sheet ever came
    WindRecord aeolian;             // what wind and evaporation left in the cover (aeolian/type/Aeolian.hpp); its arrays are empty on ground tiles
    TileLedger ledger;
};

enum Hold : uint8_t { Free, Settled, Rim };

inline constexpr int NeighborX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
inline constexpr int NeighborZ[8] = {0, 1, 1, 1, 0, -1, -1, -1};
inline constexpr float NeighborReach[8] = {1, 1.41421356f, 1, 1.41421356f, 1, 1.41421356f, 1, 1.41421356f};

inline constexpr int Apron = 4;
inline constexpr int WideCells = TileCells + 2 * Apron;

// Per-cell rock properties cached while the surface stays inside one unit.
struct UnitCache {
    float top{1}, bottom{0};   // depth range below the datum the traits are valid for
    RockTraits traits;
};

// One tile plus its halo while it is being replayed. Arrays have size*size entries.
struct Domain {
    TileKey key;
    Workers* workers{};             // pool the replay may spread independent rows over; null for the calling thread alone
    int size{DomainCells};
    double cell{};
    Vec2 origin;                    // world position of the outer corner of cell (0, 0)
    double time{};
    double step{};
    double seaLevel{};

    Ground ground;
    // Cells owned by a finished neighbour move from the state this domain would start them in to the
    // state that neighbour ended in, so they neither dam nor undercut the cells that evolve next to them.
    Ground settledFrom, settledTo;
    double startTime{}, endTime{};
    const Tile* finished[9]{};      // neighbours of an earlier pass, row-major around this tile; null otherwise
    std::vector<uint8_t> hold;      // Free cells evolve; Settled cells belong to a finished neighbour; Rim is the open edge
    int stride{1};                  // cells between strata nodes
    int nodes{};                    // strata nodes per side
    std::vector<StrataColumn> strata;   // on the node lattice; the layering varies far more slowly than relief
    std::vector<float> fold;        // m, per cell
    std::vector<UnitCache> unit;

    // Forcing, fixed for the replay unless noted.
    std::vector<float> tectonic;    // m/yr of surface rise from stacking or stretching, distributed by structure
    std::vector<float> residual;    // m/yr of surface motion the planet has that is not crustal
    std::vector<float> weakness;    // 0..1 crushed-rock share
    std::vector<float> quality;     // seeded factor on rock erodibility, around one
    std::vector<float> sound;       // cohesion of the jointed rock mass against what its kind usually has; one where nothing is known (Region, Local, Site)
    std::vector<float> rebound;     // m/yr of isostatic response to erosion and deposition (updated each region step)
    std::vector<float> trend;       // m, surface a finer level is held to on average: its parent's, smoothed
    std::vector<float> uplift;      // m/yr, rock uplift applied this step
    std::vector<float> rainBase;    // m/yr the planet supplies before relief redistributes it
    std::vector<float> rain, warmth, runoff;   // m/yr; °C annual mean reduced to sea level; m/yr
    std::vector<float> warmthNow;   // °C at the ground for the date of the current step
    std::vector<float> inflow;      // m³/yr entering from outside the domain
    std::vector<float> inflowLoad;  // m³/yr of sediment that water brings
    std::vector<float> settledFlow; // m³/yr a finished neighbour already carries through its cells
    std::vector<float> settledLoad; // m³/yr of sediment it carries with that water
    std::vector<float> standing;    // m, water surface the parent holds over the cell; far below the ground where it is dry; empty in a region
    // Neighbour 0..7 by which a finished neighbour's water leaves a cell into a cell this domain
    // evolves, 255 elsewhere. That water and its load enter there as `inflow`, wherever this
    // domain's own ground would send them: a finished cell does not erode, and a river that met it
    // as a dam would be lost to both tiles.
    std::vector<uint8_t> settledWay;
    std::vector<float> windX, windZ;   // m/s, prevailing
    std::vector<uint8_t> outlet;    // Rim cells through which the coarser level says water leaves the domain
    std::vector<float> rimSlope;    // slope a free Rim cell keeps cutting at; 0 for a rim that follows a given path
    std::vector<float> rimFloor;    // m, level a free Rim cell may not cut below

    // Flow of the current step.
    std::vector<float> surface;     // m, rock plus sediment
    std::vector<float> level;       // m, surface after standing water has filled every depression
    std::vector<int32_t> order;     // cells from the lowest outlet upstream
    std::vector<int32_t> rank;      // position of each cell in `order`
    std::vector<int32_t> spill;     // neighbour that drains a cell with no lower neighbour, else -1
    std::vector<float> discharge;   // m³/yr
    std::vector<float> seeped;      // m³/yr of the water reaching each cell that its loose bed took in (routeWater); empty until a bed took any

    // Records.
    std::vector<float> ice, iceAge;
    int taken{};                    // steps done
    std::vector<float> erosionRate, depositionRate;
    std::vector<float> eroded, deposited, uplifted;
    std::vector<float> shed;        // m/yr of rock failing, recent mean
    std::vector<IceSource> upIce;   // site level only: per parent cell under the tile
    TileLedger ledger;
    std::shared_ptr<Shore> shore;   // the coast's working state of this replay (coast/type/Shore.hpp); null until water stands in the domain
    VolcanicDomain volcanic;        // vents and the pile they lay (volcanic/type/Pile.hpp); its arrays stay empty until something volcanic touches the domain
    std::shared_ptr<SheetWork> sheet;   // continental ice over the domain during this replay (icesheet/type/Sheet.hpp); null until first asked for
    WindField aeolian;              // the wind's regime, what it has laid and left, and its sand kept aside (aeolian/type/Aeolian.hpp)
    SinkWork sinking;               // surface water going down into soluble rock (groundwater/type/Sinking.hpp); empty until a step meets such rock

    // Ground level only.
    std::vector<Bed> bed;           // the bed each cell has cut into; `unit` then holds the parting inside it
    std::vector<double> laidDown;   // yr that bed's unit began to accumulate; 0 for basement
    std::vector<OreField> ore;      // per strata node: bodies that cut the beds there
    std::vector<float> stream;      // m³/yr working a cell's bed: its own flow or, within a river's width, the river's
    float frost{};                  // share of the year rock spends in the frost-cracking window, at the middle of the tile
    float halfRange{};              // °C between the annual mean and the warmest month

    size_t index(int x, int z) const { return size_t(z) * size_t(size) + size_t(x); }
    // A finished neighbour's cell `from` hands its river to the evolving cell `to`, which therefore
    // may not drain back into it.
    bool handsOn(size_t from, size_t to) const {
        if (settledWay.empty() || settledWay[from] > 7) return false;
        return index(int(from % size_t(size)) + NeighborX[settledWay[from]], int(from / size_t(size)) + NeighborZ[settledWay[from]]) == to;
    }
    bool handsOnward(size_t from) const { return !settledWay.empty() && settledWay[from] < 8; }
    size_t cells() const { return size_t(size) * size_t(size); }
    Vec2 center(int x, int z) const { return {origin.x + (x + 0.5) * cell, origin.z + (z + 0.5) * cell}; }
    bool owned(int x, int z) const { return x >= Halo && z >= Halo && x < Halo + TileCells && z < Halo + TileCells; }
    const StrataColumn& strataOf(size_t i) const {
        return strata[size_t(int(i / size_t(size)) / stride) * size_t(nodes) + size_t(int(i % size_t(size)) / stride)];
    }
    // The layering under one cell: thicknesses blended from the four nodes around it, so a bed
    // boundary never steps where the nearest node changes.
    StrataColumn strataAtCell(size_t i) const;

    // Crust-attached position of a cell: its strata node's, moved by the cell's offset from the node.
    Vec2 materialOf(size_t i) const {
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        const Vec2 node{origin.x + ((x / stride) * stride + 0.5) * cell, origin.z + ((z / stride) * stride + 0.5) * cell};
        return strataOf(i).material + (center(x, z) - node);
    }
    float sedimentAt(size_t i) const { return ground.sediment[0][i] + ground.sediment[1][i] + ground.sediment[2][i]; }
};

} // namespace derivean::sim
