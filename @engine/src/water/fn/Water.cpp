#include "water/type/WaterTransfer.hpp"

#include <algorithm>
#include <bitset>
#include <cmath>

namespace derivean {

namespace {

size_t cellIndex(Cell point, Cell tile) {
    const Cell offset = point - tile * VoxelsPerMeter;
    return size_t((offset.y * VoxelsPerMeter + offset.z) * VoxelsPerMeter + offset.x);
}

// Marks a cell that holds water awake.
void rouse(WaterState& state, Cell point) {
    const Cell tile = tileOf(point);
    WaterBrick* found = brickAt(state, tile);
    if (!found) return;
    WaterBrick& brick = *found;
    const size_t index = cellIndex(point, tile);
    if (!brick.amount[index]) return;
    brick.awake[index / 64] |= uint64_t(1) << (index % 64);
    if (brick.listed) return;
    brick.listed = true;
    state.rousing.push_back(tile);
}

} // namespace

WaterBrick* brickAt(const WaterState& state, Cell tile) {
    return cachedFind(state.found, const_cast<WaterState&>(state).bricks, tile);
}

void dropBrick(WaterState& state, Cell tile) {
    forget(state.found, tile);
    state.bricks.erase(tile);
}

int waterAt(const WaterState& state, Cell point) {
    const Cell tile = tileOf(point);
    const WaterBrick* brick = brickAt(state, tile);
    return brick ? brick->amount[cellIndex(point, tile)] : 0;
}

void wakeWater(WaterState& state, Cell point) {
    rouse(state, point);
    for (Cell direction : Neighbors) rouse(state, point + direction);
    for (int y = point.y - 2; waterAt(state, {point.x, y, point.z}); --y) rouse(state, {point.x, y, point.z});
}

void wakeWater(WaterState& state, std::span<const Cell> changed) {
    for (Cell point : changed) {
        wakeWater(state, point);
        const Cell key = chunkOf(point);
        state.meshDirty.insert(key);
        for (Cell direction : Neighbors)
            if (const Cell boundary = chunkOf(point + direction); boundary != key) state.meshDirty.insert(boundary);
    }
}

void addWater(WaterState& state, Cell point, int delta) {
    if (delta == 0) return;
    const Cell tile = tileOf(point);
    const Cell key = chunkOf(point);
    WaterBrick* found = brickAt(state, tile);
    if (!found) {
        forget(state.found, tile);
        found = &state.bricks.try_emplace(tile).first->second;
    }
    WaterBrick& brick = *found;
    uint16_t& cell = brick.amount[cellIndex(point, tile)];
    const int before = cell, after = before + delta;
    if (before == 0 && brick.wet++ == 0) ++state.chunks[key];
    cell = uint16_t(after);
    if (after == 0 && --brick.wet == 0) {
        const auto chunk = state.chunks.find(key);
        if (--chunk->second == 0) state.chunks.erase(chunk);
        if (std::all_of(brick.awake.begin(), brick.awake.end(), [](uint64_t word) { return word == 0; })) dropBrick(state, tile);
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
    // Cells within four steps of the point, by their offset in the 9³ box around it.
    constexpr int Reach = 4, Box = 2 * Reach + 1;
    std::bitset<Box * Box * Box> seen;
    const auto see = [&](Cell distance) {
        const size_t index = size_t(((distance.y + Reach) * Box + distance.z + Reach) * Box + distance.x + Reach);
        if (seen[index]) return false;
        seen[index] = true;
        return true;
    };
    see({0, 0, 0});
    std::vector<Cell> frontier{point};
    std::vector<std::pair<Cell, int>> transfers;
    for (size_t i = 0; i < frontier.size() && remaining; ++i) {
        for (Cell offset : Neighbors) {
            const Cell neighbor = frontier[i] + offset;
            const Cell distance = neighbor - point;
            if (std::abs(distance.x) + std::abs(distance.y) + std::abs(distance.z) > Reach ||
                materialAt(world, neighbor) != Material::Air || !see(distance)) continue;
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
