#pragma once

#include "ground/type/Clast.hpp"
#include "ground/type/Parting.hpp"
#include "landscape/fn/Build.hpp"

namespace derivean::sim {

// What the surface of the coarse layer is made of at one cell.
struct Fabric {
    float median{}, spread{0.5f};    // m of intermediate axis, and its log standard deviation
    float cover{};                   // share of the surface its stones take up
    float largest{1e9f};             // m: no stone of the fabric is larger
    float blockShare{1};             // share of the coarse layer that is blocks and not gravel
    float roundness{}, flatness{1};
    float sinkLow{0.2f}, sinkHigh{0.5f};   // share of a stone's height below the ground, drawn between the two
    float buried{-1};                // m of fine cover over the layer the stones sit in, for stones that show only through it; negative otherwise
    ClastOrigin origin{ClastOrigin::Lag};
    Material rock{Material::Bedrock};
    Vec2 source;
    float travel{};
    float heading{};
    bool imbricate{};
};

// Per-cell fields the classification reads, worked out once for the domain.
struct FabricContext {
    const Domain& domain;
    const FlowScratch& scratch;
    const TileLookup& lookup;
    std::vector<float> slope;        // rise over run of the ground
    std::vector<BlockHabit> habit;   // what the rock each cell has cut into breaks into
    std::vector<int32_t> face;       // cell of the failing rock upslope whose waste reaches this one, or -1
    std::vector<float> fallen;       // m that waste has come
};

} // namespace derivean::sim
