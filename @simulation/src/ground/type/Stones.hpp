#pragma once

#include "ground/type/Detail.hpp"
#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// The stones being laid on one ground domain. Each is paid for out of the coarse layer of the
// tile's own cells around it, so the layer and the stones together stay what the layer was.
struct StoneBed {
    Domain& domain;
    uint64_t seed{};
    std::vector<Clast> stones;
    std::vector<uint8_t> taken;   // per domain cell: a stone already lies here
    std::vector<float> stepGaps;  // m along its channel from each step of a stream bed to the next one below
    ClastLedger ledger;

    Vec2 tileCorner() const { return {domain.origin.x + Halo * domain.cell, domain.origin.z + Halo * domain.cell}; }
    bool owns(Vec2 at) const;
    size_t cellAt(Vec2 at) const;
    float groundAt(size_t cell) const { return domain.ground.rock[cell] + domain.sedimentAt(cell); }
};

// One stone a process wants to lay.
struct StoneOffer {
    Vec2 at;                  // m, world; must lie in the tile's own cells
    float size{};             // m, intermediate axis
    float flatness{1};        // shortest over intermediate axis
    float roundness{};
    float sink{0.2f};         // share of its height below the ground
    float heading{};          // rad, direction it came from or the water runs in
    bool imbricate{};         // lie across `heading` and lean against it, as a stream leaves its stones
    Material rock{Material::Bedrock};
    ClastOrigin origin{ClastOrigin::Lag};
    Vec2 source;
    float travel{};
    uint64_t draw{};          // seeded bits that vary its proportions and attitude
};

} // namespace derivean::sim
