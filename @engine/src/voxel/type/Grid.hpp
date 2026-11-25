#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>

namespace derivean {

inline constexpr int VoxelsPerMeter = 8;
inline constexpr float VoxelSize = 1.0f / VoxelsPerMeter;
inline constexpr float VoxelVolume = VoxelSize * VoxelSize * VoxelSize;
inline constexpr int ChunkSide = 4 * VoxelsPerMeter;

using Position = std::array<double, 3>;

struct Cell {
    int x{}, y{}, z{};
    auto operator<=>(const Cell&) const = default;

    int& operator[](int axis) { return axis == 0 ? x : axis == 1 ? y : z; }
    int operator[](int axis) const { return axis == 0 ? x : axis == 1 ? y : z; }
};

inline Cell operator+(Cell a, Cell b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Cell operator-(Cell a, Cell b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Cell operator*(Cell p, int scale) { return {p.x * scale, p.y * scale, p.z * scale}; }

struct CellHash {
    size_t operator()(Cell p) const {
        uint64_t hash = uint64_t(uint32_t(p.x)) * 0x9e3779b185ebca87ULL
                      + uint64_t(uint32_t(p.y)) * 0xc2b2ae3d27d4eb4fULL
                      + uint64_t(uint32_t(p.z)) * 0x165667b19e3779f9ULL;
        return size_t(hash ^ (hash >> 32));
    }
};

inline constexpr std::array<Cell, 6> Neighbors{{
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
}};

inline int floorDiv(int x, int divisor) {
    return x / divisor - int(x % divisor < 0);
}

inline Cell tileOf(Cell p) {
    return {floorDiv(p.x, VoxelsPerMeter), floorDiv(p.y, VoxelsPerMeter), floorDiv(p.z, VoxelsPerMeter)};
}

inline Cell chunkOf(Cell p) {
    return {floorDiv(p.x, ChunkSide), floorDiv(p.y, ChunkSide), floorDiv(p.z, ChunkSide)};
}

struct Bounds {
    Cell min, max;
    Cell size() const { return max - min; }
    bool contains(Cell p) const {
        return p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y && p.z >= min.z && p.z < max.z;
    }
    Bounds padded(int amount) const {
        const Cell padding{amount, amount, amount};
        return {min - padding, max + padding};
    }
};

inline Bounds intersection(Bounds a, Bounds b) {
    return {{std::max(a.min.x, b.min.x), std::max(a.min.y, b.min.y), std::max(a.min.z, b.min.z)},
            {std::min(a.max.x, b.max.x), std::min(a.max.y, b.max.y), std::min(a.max.z, b.max.z)}};
}

inline Bounds boundsOf(std::span<const Cell> cells) {
    Bounds bounds{cells.front(), cells.front() + Cell{1, 1, 1}};
    for (Cell point : cells) {
        for (int axis = 0; axis < 3; ++axis) {
            bounds.min[axis] = std::min(bounds.min[axis], point[axis]);
            bounds.max[axis] = std::max(bounds.max[axis], point[axis] + 1);
        }
    }
    return bounds;
}

inline Bounds chunkBounds(Cell chunk) {
    const Cell origin = chunk * ChunkSide;
    return {origin, origin + Cell{ChunkSide, ChunkSide, ChunkSide}};
}

} // namespace derivean
