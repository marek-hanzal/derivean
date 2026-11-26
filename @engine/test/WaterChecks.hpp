#pragma once

#include "water/type/Water.hpp"

#include "Expect.hpp"

// Checks and fixtures the water tests share.

namespace derivean {

inline void checkBalance(const WaterState& water) {
    uint64_t cells = 0, parcels = 0;
    for (const auto& [point, amount] : water.cells) {
        expect(amount > 0 && amount <= WaterUnits, "Overfilled or empty water cell");
        cells += amount;
    }
    for (const auto& parcel : water.parcels) parcels += parcel.amount;
    expect(cells == water.budget.bulk && parcels == water.budget.airborne, "Water ledger disagrees with stored state");
    expect(cells + parcels + water.budget.escaped == water.budget.initial, "Water volume changed");
}

inline void simulate(WaterState& water, const VoxelWorld& world, int steps, const JoltRuntime* runtime = nullptr) {
    for (int step = 0; step < steps; ++step) {
        advanceWater(water, world, 1.0 / 120.0, runtime, runtime ? 0.0 : 1e100);
        checkBalance(water);
    }
}

inline uint64_t below(const WaterState& water, int level) {
    uint64_t count = 0;
    for (const auto& [cell, amount] : water.cells)
        if (cell.y < level) count += amount;
    return count;
}

inline void trough(VoxelWorld& world, int floor, int width) {
    fillBox(world, {{-width, floor, -4}, {width + 1, floor + 1, 5}}, Material::Basalt);
    fillBox(world, {{-width, floor + 1, -4}, {-width + 1, floor + 8, 5}}, Material::Basalt);
    fillBox(world, {{width, floor + 1, -4}, {width + 1, floor + 8, 5}}, Material::Basalt);
    fillBox(world, {{-width + 1, floor + 1, -4}, {width, floor + 8, -3}}, Material::Basalt);
    fillBox(world, {{-width + 1, floor + 1, 4}, {width, floor + 8, 5}}, Material::Basalt);
}

} // namespace derivean
