#pragma once

#include "chronology/constant/Timeline.hpp"
#include "core/type/Vec.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace derivean::sim {

// A region replays real time: the last tens of Myr of the planet's history, from the state the
// planet recorded at the start of that window. The finer levels refine their parent sixteen-fold
// each. Relief of their size has no memory of where it started, so instead of a date they are given
// the parent's present surface and rock uplift and are integrated until valleys, benches and talus
// of their own scale stand in balance with them: `span` is that settling time, not a calendar window.
// Ground is the finest: its cells are two voxels wide, so what it leaves is what a person stands on.
enum class Level : uint8_t { Region = 1, Local = 2, Site = 3, Ground = 4 };

struct LevelSpec {
    double cell;     // m
    double step;     // yr
    double anchor;   // yr; a region window may only start at a multiple of this
    double span;     // yr, shortest replay (region) or settling time (finer levels)
};

inline constexpr int LevelCount = 5;   // entries of a table indexed by Level
inline constexpr LevelSpec LevelSpecs[LevelCount] = {
    {},
    {1024.0, RegionStep, 8 * Myr, 24 * Myr},
    {64.0, LocalStep, 0, 6 * Myr},
    {4.0, SiteStep, 0, 1 * Myr},
    {0.25, 1000.0, 0, 24e3},
};

inline constexpr int TileCells = 256;                      // cells a tile owns per side
inline constexpr int Halo = 32;                            // extra cells simulated around it
inline constexpr int DomainCells = TileCells + 2 * Halo;
inline constexpr int Refinement = 16;
// Site cells around a ground tile that reach it: the run-out of a block that falls from a cliff.
inline constexpr int FallReachCells = 32;

// Parent cells a domain reads beyond its own edge: what cubic sampling and inflow tracing reach
// into, and at ground level the slopes above it that shed rock onto it.
inline constexpr int parentMargin(Level level) { return level == Level::Ground ? FallReachCells : Halo / Refinement + 3; }

inline constexpr const LevelSpec& specOf(Level level) { return LevelSpecs[size_t(level)]; }
inline constexpr double tileSpan(Level level) { return TileCells * specOf(level).cell; }

// The integration of one level for a world of the given age. A region's ends sit on absolute
// grids, so a slightly later age continues the same replay by whole steps instead of starting anew.
struct Window {
    double start{}, end{};
    int steps{};
};

inline Window windowAt(Level level, double time) {
    const LevelSpec& spec = specOf(level);
    Window window;
    if (level != Level::Region) {
        window.end = time;
        window.start = time - spec.span;
        window.steps = int(std::llround(spec.span / spec.step));
        return window;
    }
    window.end = std::floor(time / spec.step) * spec.step;
    window.start = std::max(0.0, std::floor((time - spec.span) / spec.anchor) * spec.anchor);
    window.steps = int(std::llround((window.end - window.start) / spec.step));
    return window;
}

struct TileKey {
    Level level{Level::Region};
    int32_t x{}, z{};
    bool operator==(const TileKey&) const = default;
};

struct TileKeyHash {
    size_t operator()(TileKey key) const {
        uint64_t mixed = uint64_t(uint32_t(key.x)) * 0x9e3779b185ebca87ULL + uint64_t(uint32_t(key.z)) * 0xc2b2ae3d27d4eb4fULL + uint64_t(key.level);
        return size_t(mixed ^ (mixed >> 31));
    }
};

inline TileKey tileAt(Level level, Vec2 position) {
    const double span = tileSpan(level);
    return {level, int32_t(std::floor(position.x / span)), int32_t(std::floor(position.z / span))};
}

inline TileKey parentOf(TileKey key) {
    return {Level(int(key.level) - 1), int32_t(std::floor(key.x / double(Refinement))), int32_t(std::floor(key.z / double(Refinement)))};
}

// Tiles are solved in four passes by parity. A tile only waits for neighbours of an earlier pass
// and then treats their finished cells as fixed, so the result never depends on visiting order
// while rivers and slopes still meet exactly on every shared edge.
inline int passOf(TileKey key) { return (key.x & 1) + 2 * (key.z & 1); }

} // namespace derivean::sim
