#pragma once

#include "core/type/Vec.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace derivean::sim {

// How a cell of the grid takes what reaches it.
enum class Rest : uint8_t {
    Open,      // material comes to rest and raises the ground
    // Ground another owner has already finished. An eruption behaves on it as on open ground, so
    // that what crosses it into open cells is what that owner saw cross; but what it lays there is
    // not the grid's to keep: whoever reads `laid` drops it and books it as passed.
    Passing,
    Closed,    // the edge: what arrives has left the grid
};

// The ground an eruption works on, as plain heights: a window of a replayed domain, a patch built
// by hand, or live ground during play. The emplacement laws (volcanic/fn/Emplace.hpp) know nothing
// else, which is what lets one set of laws build a mountain over a million years and run a flow
// past a player in an afternoon.
struct Emplacement {
    int width{}, height{};
    double cell{};                // m
    Vec2 origin;                  // world position of the outer corner of cell (0, 0)
    std::vector<float> ground;    // m, surface the material comes to lie on; rises as it is laid
    std::vector<float> water;     // m, level of standing water; below the ground where the cell is dry
    std::vector<float> laid;      // m laid since `laid` was last cleared
    std::vector<int32_t> fresh;   // cells `laid` rose from nothing on since then, so a reader visits only those
    std::vector<float> mobile;    // m still moving
    std::vector<float> heat;      // 0..1 of the mobile layer: 1 as it leaves the vent, 0 once it has set
    std::vector<Rest> rest;
    std::vector<uint8_t> queued;  // cells waiting in the worklist of a relaxation
    double lost{};                // m³ that left over a Closed cell
    double passed{};              // m³ laid on Passing cells and dropped by the reader of `laid`

    size_t cells() const { return size_t(width) * size_t(height); }
    size_t index(int x, int z) const { return size_t(z) * size_t(width) + size_t(x); }
    bool contains(int x, int z) const { return x >= 0 && z >= 0 && x < width && z < height; }
    Vec2 center(int x, int z) const { return {origin.x + (x + 0.5) * cell, origin.z + (z + 0.5) * cell}; }
    bool submerged(size_t i) const { return water[i] > ground[i] + 1.0f; }
    void lay(size_t i, float part) {
        if (laid[i] <= 0) fresh.push_back(int32_t(i));
        laid[i] += part, ground[i] += part;
    }

    void resize(int w, int h) {
        width = w, height = h;
        ground.assign(cells(), 0.0f);
        water.assign(cells(), -1e9f);
        laid.assign(cells(), 0.0f);
        fresh.clear();
        mobile.assign(cells(), 0.0f);
        heat.assign(cells(), 0.0f);
        rest.assign(cells(), Rest::Open);
        queued.assign(cells(), 0);
    }
};

} // namespace derivean::sim
