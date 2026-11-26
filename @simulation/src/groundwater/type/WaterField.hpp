#pragma once

#include "groundwater/constant/Hydraulics.hpp"
#include "landscape/type/Terrain.hpp"
#include "planet/fn/Heat.hpp"

#include <vector>

namespace derivean::sim {

class Workers;

// One run of ground that treats water alike down a column: the loose cover, or one bed.
struct WaterLayer {
    float top{}, bottom{};      // m, elevation
    float along{}, through{};   // m/yr, conductivity along the layering and across it
    float porosity{};
    float soluble{};            // m/Myr per m/yr of runoff; rock below SolubleEnough opens no conduits
    Material rock{Material::Bedrock};
    uint32_t unit{};            // the layering unit the bed belongs to; 0 for cover, lava piles and basement
    float offset{};             // m from the top of that unit to the top of the bed: the same in every column
};

enum class Drain : uint8_t { None, Stream, Still };

// Where the planes of the two joint sets pass a column: signed metres from its centre along each
// set's normal (-sin strike, cos strike) to the nearest zone and the nearest corridor of the set,
// beyond reach where none passes within one spacing.
struct Fissures {
    float strike[2]{};
    float zone[2]{1e9f, 1e9f};
    float corridor[2]{1e9f, 1e9f};
};

// What one cell's ground is to water. A column has two storeys. The upper one is the ground above
// the first bed too tight to pass the recharge: its water is perched, runs along that bed and comes
// out where the bed crops out. The lower one is everything below, down to where circulation ends.
struct AquiferColumn {
    float ground{};          // m, top of the ground
    float bedrock{};         // m, top of the solid rock under the loose cover
    float stage{};           // m, level of the surface water that drains the cell; read with `drain`
    float recharge{};        // m³/yr entering the ground
    float rain{};            // m/yr
    float warmth{};          // °C, annual mean at the ground
    float stream{};          // m³/yr of water on the surface of the cell: the landscape's, less what soaked in on its way
    float incision{};        // m/yr the landscape is lowering the cell
    Drain drain{Drain::None};
    uint8_t flowTo{255};     // neighbour the surface water leaves by
    float swallow{};         // m/yr a wetted square metre of the cell's bed takes underground: its rock's alone
    float sunk{};            // m³/yr of the stream that goes underground in the cell

    bool perched{};
    float floor{};           // m, top of the tight bed
    float floorLeak{};       // m³/yr it lets through
    float upperT{};          // m²/yr of the ground above it, saturated to the surface
    float upperStore{};      // m of water that ground holds when full
    float upperSoluble{};    // m of soluble rock that lies directly on the tight bed
    uint32_t sheet{};        // WaterLayer::unit and offset of the tight bed: equal in two columns that stand on the same bed
    float sheetOffset{};
    Material upperRock{Material::Regolith}, upperSolubleRock{Material::Air}, floorRock{Material::Shale};

    float roof{};            // m, top of the lower storey: the ground, or the underside of the tight bed
    float base{};            // m, where circulation ends
    float shallowBase{};     // m, base of the cover and of the joints weather has opened
    float shallowT{};        // m²/yr above it
    float deepT[2]{};        // m²/yr below it: of the upper and of the lower half of that depth
    float baseLeak{};        // m³/yr lost downwards through the base
    float lowerStore{};      // m of water the storey holds when full
    float solubleTop{}, solubleBottom{1};   // m, the thickest run of soluble rock in the lower storey; none while top < bottom
    Material lowerRock{Material::Bedrock}, solubleRock{Material::Air};

    float spread[3]{1, 0, 1};    // xx, xz, zz: how much easier water moves along the joint sets; trace 2
    float dipX{}, dipZ{};        // m the layering falls per metre east and per metre south
    float crush{};               // 0..1, closeness to the fault that bounds the structural domain
    Geotherm heat;
    bool fissured{};             // WaterField::fissures holds the joint planes of the column's soluble rock
};

inline float lowerT(const AquiferColumn& column) { return column.shallowT + column.deepT[0] + column.deepT[1]; }

inline constexpr int Upper = 0, Lower = 1;

// What the water arriving at a point brings with it, as flow-weighted means over where it came from.
struct Borne {
    float lime{};     // kg/m³ of dissolved carbonate
    float far{};      // m travelled underground
    float high{};     // m, elevation of the ground it entered
    float rate{};     // m/yr of recharge where it entered
    float age{};      // yr underground
    float deep{};     // m, greatest depth of rock it has passed through
};

// Where the water of a node ends.
struct NodeFate {
    float emerges{};   // m³/yr coming out at the surface of the cell
    float outcrop{};   // m³/yr leaving the upper storey where its floor crops out beside the cell
    float lost{};      // m³/yr through the base of circulation
    float captured{};  // m³/yr taken by the conduits of soluble rock
    float lag{};       // m³/yr still on its way after RouteSweeps: longer paths than the model follows
};

// A tile with WaterReach cells of its neighbours while its ground water is worked out. Arrays hold
// size² cells, row-major; per-node arrays hold two entries per cell, upper storey first. The cells
// inside the margin are a whole tile: size - 2 * margin is TileCells.
struct WaterField {
    TileKey key;
    uint64_t seed{};
    int size{FieldCells};
    int margin{WaterReach};         // cells around the tile's own
    int64_t firstX{}, firstZ{};     // global index of cell (0, 0)
    float seaLevel{};
    Workers* workers{};

    std::vector<AquiferColumn> column;
    std::vector<Fissures> fissures;

    std::vector<float> level;       // m, the stand of water in a storey that only its outlets and its floor decide
    std::vector<float> share;       // nodes × 8: part of a node's sideways flow that leaves towards each neighbour
    std::vector<float> room;        // m³/yr a node can pass sideways when its water stands at the ground
    std::vector<float> pull;        // m²/yr: by how much that falls per metre its water stands lower
    std::vector<uint8_t> way;       // cells × 8, upper storey: Way of each direction
    std::vector<uint8_t> karst;     // per node: its water is taken by conduits

    std::vector<float> flux;        // m³/yr arriving at a node
    std::vector<float> sideways;    // m³/yr leaving it towards its neighbours
    std::vector<Borne> borne;       // by the water leaving a node
    std::vector<NodeFate> fate;

    size_t cells() const { return size_t(size) * size_t(size); }
    size_t index(int x, int z) const { return size_t(z) * size_t(size) + size_t(x); }
    bool owned(int x, int z) const { return x >= margin && z >= margin && x < size - margin && z < size - margin; }
    Vec2 centre(int x, int z) const { return {(double(firstX + x) + 0.5) * WaterCell, (double(firstZ + z) + 0.5) * WaterCell}; }
};

// Where the upper storey's water goes when it leaves a cell in one direction.
enum class Way : uint8_t {
    Closed,    // nowhere: beyond the field
    Along,     // on along the same tight bed
    Outcrop,   // out at the surface: the bed comes to daylight between the two cells
    Under,     // on under the neighbour's ground, into its lower storey
};

// m, elevation at which a column's tight bed lies under the neighbour in direction k, by its dip.
inline float sheetToward(const AquiferColumn& column, int k) {
    return column.floor - float(WaterCell) * (column.dipX * float(NeighborX[k]) + column.dipZ * float(NeighborZ[k]));
}

} // namespace derivean::sim
