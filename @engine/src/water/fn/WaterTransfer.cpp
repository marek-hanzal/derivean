#include "water/type/WaterTransfer.hpp"

#include <utility>

namespace derivean {
namespace {

constexpr Cell Down{0, -1, 0};
constexpr std::array<Cell, 4> Lateral{{{1, 0, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 0, -1}}};
constexpr int MaxLateralUnits = WaterUnits / 4;

int64_t waterHead(const WaterState& state, Cell at) {
    int amount = waterAt(state, at);
    while (amount == WaterUnits && waterAt(state, at - Down)) {
        at = at - Down;
        amount = waterAt(state, at);
    }
    return int64_t(at.y) * WaterUnits + amount;
}

void fall(WaterState& state, const VoxelWorld& world, Cell from, int units) {
    const Cell below = from + Down;
    if (materialAt(world, below) != Material::Air) return;
    if (!waterAt(state, below) && !waterAt(state, below + Down) &&
        materialAt(world, below + Down) == Material::Air) {
        addWater(state, from, -units);
        state.parcels.push_back({{(from.x + 0.5) * VoxelSize, from.y * VoxelSize, (from.z + 0.5) * VoxelSize},
                                 {0, 0, 0}, units});
        state.budget.airborne += uint64_t(units);
        return;
    }

    const int transfer = std::min(units, WaterUnits - waterAt(state, below));
    addWater(state, from, -transfer);
    addWater(state, below, transfer);
}

} // namespace

void tickWater(WaterState& state, const VoxelWorld& world) {
    const size_t rotation = size_t(state.ticks++ % Lateral.size());
    if (state.awake.empty()) return;
    std::vector<Cell> active(state.awake.begin(), state.awake.end());
    state.awake.clear();
    std::sort(active.begin(), active.end(), [](Cell a, Cell b) {
        if (a.y != b.y) return a.y < b.y;
        return std::pair(a.x, a.z) < std::pair(b.x, b.z);
    });

    for (Cell from : active) {
        const int amount = waterAt(state, from);
        if (amount) fall(state, world, from, amount);
    }
    for (Cell from : active) {
        for (size_t i = 0; i < Lateral.size() && waterAt(state, from); ++i) {
            const Cell to = from + Lateral[(i + rotation) % Lateral.size()];
            if (materialAt(world, to) != Material::Air) continue;
            const int available = WaterUnits - waterAt(state, to);
            if (available == 0) continue;
            const int slope = int(std::clamp((waterHead(state, from) - waterHead(state, to)) / 2, int64_t(0), int64_t(MaxLateralUnits)));
            if (slope <= 0) continue;
            const int transfer = std::min({slope, available, waterAt(state, from), MaxLateralUnits});
            addWater(state, from, -transfer);
            addWater(state, to, transfer);
        }
    }
    applyBackwater(state, world, active);
}

} // namespace derivean
