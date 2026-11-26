#pragma once

#include "groundwater/type/WaterField.hpp"
#include "rockmass/type/RockMass.hpp"

namespace derivean::sim {

// The ground under one cell as the landscape and the layering give it.
struct BedColumn {
    uint64_t seed{};
    StrataColumn strata;     // the cell's own layering: its node's, with thicknesses blended
    const StrataColumn* node{};   // the layering node the cell reads its rock mass through
    Vec2 material;           // crust-attached position of the cell
    float fold{};
    float rockTop{}, datum{};          // m
    float cover[SedimentClasses]{};    // m
    float pile{};            // m of volcanic rock under the rock surface, on top of the layering
    float lava{};            // 0..1 of it that is lava; the rest is ash and scoria
};

// What reading it finds.
struct BedWalk {
    WaterLayer layers[WalkBeds + 3];
    int count{};
    RockMass mass;           // of the first bed of the layering
    Material top{Material::Bedrock};   // rock at the top of the solid rock
    float base{};            // m, elevation where circulation ends
    float shallowBase{};     // m, elevation the opened joints reach down to
    bool soluble{};          // a bed of soluble rock was met
    RockMass solubleMass;    // of the first such bed, as every reader of its planes asks for it
    float solubleDepth{};    // m below the datum of its base
};

// Reads the cover, a volcanic pile, the beds down to WalkDepth below the rock surface, and one
// more layer of the last rock met from there to the base of circulation.
void walkBeds(const BedColumn& column, BedWalk& walk);

// Where the zones and corridors of a rock mass pass a point.
Fissures fissuresAt(uint64_t seed, const RockMass& mass, Vec2 material, float depth);

} // namespace derivean::sim
