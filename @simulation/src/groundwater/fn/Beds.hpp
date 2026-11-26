#pragma once

#include "groundwater/type/WaterField.hpp"
#include "rockmass/type/RockMass.hpp"

namespace derivean::sim {

// The ground under one cell as the layering gives it.
struct BedColumn {
    uint64_t seed{};
    StrataColumn strata;     // the cell's own layering: its node's, with thicknesses blended
    const StrataColumn* node{};   // the layering node the cell reads its rock mass through
    Vec2 material;           // crust-attached position of the cell
    float fold{};
    float rockTop{}, datum{};          // m
    float cover[SedimentClasses]{};    // m
};

// What reading it finds.
struct BedWalk {
    WaterLayer layers[WalkBeds + 2];
    int count{};
    RockMass mass;           // of the bed at the top of the rock
    Material top{Material::Bedrock};
    float base{};            // m, elevation where circulation ends
    float shallowBase{};     // m, elevation the opened joints reach down to
    bool soluble{};          // a bed of soluble rock was met
    RockMass solubleMass;    // of the first such bed, as every reader of its planes asks for it
    float solubleDepth{};    // m below the datum of its base
};

// Reads the cover and the beds down to WalkDepth, and one more layer of the last rock met from
// there to the base of circulation.
void walkBeds(const BedColumn& column, BedWalk& walk);

// Where the zones and corridors of a rock mass pass a point.
Fissures fissuresAt(uint64_t seed, const RockMass& mass, Vec2 material, float depth);

} // namespace derivean::sim
