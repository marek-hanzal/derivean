#include "water/type/WaterTransfer.hpp"

#include <cmath>

namespace derivean {

int waterAt(const WaterState& state, Cell point) {
    const auto found = state.cells.find(point);
    return found == state.cells.end() ? 0 : found->second;
}

void wakeWater(WaterState& state, Cell point) {
    if (waterAt(state, point) != 0) state.awake.insert(point);
    for (Cell direction : Neighbors) {
        const Cell other = point + direction;
        if (waterAt(state, other) != 0) state.awake.insert(other);
    }
    for (int y = point.y - 2; waterAt(state, {point.x, y, point.z}); --y)
        state.awake.insert({point.x, y, point.z});
}

void wakeWater(WaterState& state, std::span<const Cell> changed) {
    for (Cell point : changed) {
        wakeWater(state, point);
        state.meshDirty.insert(chunkOf(point));
        for (Cell direction : Neighbors) state.meshDirty.insert(chunkOf(point + direction));
    }
}

void addWater(WaterState& state, Cell point, int delta) {
    if (delta == 0) return;
    const int before = waterAt(state, point);
    const int after = before + delta;
    const Cell key = chunkOf(point);
    if (before == 0) ++state.chunks[key];
    if (after != 0) {
        state.cells.insert_or_assign(point, uint16_t(after));
    } else {
        state.cells.erase(point);
        auto chunk = state.chunks.find(key);
        if (--chunk->second == 0) state.chunks.erase(chunk);
    }
    if (delta > 0) state.budget.bulk += uint64_t(delta);
    else state.budget.bulk -= uint64_t(-delta);
    wakeWater(state, point);
    state.meshDirty.insert(key);
    for (Cell direction : Neighbors) {
        const Cell boundary = chunkOf(point + direction);
        if (key != boundary) state.meshDirty.insert(boundary);
    }
}

void seedWater(WaterState& state, const VoxelWorld& world, Bounds area, int fill) {
    for (int y = area.min.y; y < area.max.y; ++y)
        for (int z = area.min.z; z < area.max.z; ++z)
            for (int x = area.min.x; x < area.max.x; ++x) {
                const Cell point{x, y, z};
                if (materialAt(world, point) != Material::Air) continue;
                const int units = std::min(fill, WaterUnits - waterAt(state, point));
                addWater(state, point, units);
                state.budget.initial += uint64_t(units);
            }
}

bool displaceWater(WaterState& state, const VoxelWorld& world, Cell point) {
    int remaining = waterAt(state, point);
    if (!remaining) return true;
    std::vector<Cell> frontier{point};
    std::unordered_set<Cell, CellHash> seen{point};
    std::vector<std::pair<Cell, int>> transfers;
    for (size_t i = 0; i < frontier.size() && remaining; ++i) {
        for (Cell offset : Neighbors) {
            const Cell neighbor = frontier[i] + offset;
            const Cell distance = neighbor - point;
            if (std::abs(distance.x) + std::abs(distance.y) + std::abs(distance.z) > 4 ||
                materialAt(world, neighbor) != Material::Air || !seen.insert(neighbor).second) continue;
            const int amount = std::min(remaining, WaterUnits - waterAt(state, neighbor));
            remaining -= amount;
            if (amount) transfers.emplace_back(neighbor, amount);
            if (!remaining) break;
            if (waterAt(state, neighbor) + amount == WaterUnits) frontier.push_back(neighbor);
        }
    }
    if (remaining) return false;
    addWater(state, point, -waterAt(state, point));
    for (const auto& [cell, amount] : transfers) addWater(state, cell, amount);
    return true;
}

void advanceWater(WaterState& state, const VoxelWorld& world, double step,
                  const JoltRuntime* physics, double lowestMoving) {
    state.accumulator += step;
    while (state.accumulator + 1e-9 >= WaterTick) {
        state.accumulator -= WaterTick;
        tickWater(state, world);
    }
    moveWaterParcels(state, world, step, physics, lowestMoving);
}

std::optional<float> waterDepth(const WaterState& state, Position point) {
    const Cell cell{int(std::floor(point[0] / VoxelSize)), int(std::floor(point[1] / VoxelSize)),
                    int(std::floor(point[2] / VoxelSize))};
    int amount = waterAt(state, cell);
    if (amount == 0) return std::nullopt;
    int level = cell.y;
    while (amount == WaterUnits && waterAt(state, {cell.x, level + 1, cell.z}))
        amount = waterAt(state, {cell.x, ++level, cell.z});
    const float depth = (level + amount / float(WaterUnits)) * VoxelSize - float(point[1]);
    return depth > 0 ? std::optional<float>(depth) : std::nullopt;
}

} // namespace derivean
