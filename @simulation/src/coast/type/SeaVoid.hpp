#pragma once

#include "groundwater/type/Cave.hpp"
#include "landscape/type/Setting.hpp"
#include "landscape/type/Terrain.hpp"

#include <functional>
#include <vector>

namespace derivean::sim {

// What the sea has opened in rock at the waterline and a height field cannot hold: the notch under
// a face the waves strike, the cave they drive in along a joint that meets that face, and the arch
// that is left where such a cave comes out on the far side of a headland or a stack. Each is a
// closed-form consequence of what the site level simulated (where waves reach rock, how high they
// arrive there, how far they have cut the foot back) and of the planes of the rock mass; no void
// is placed. Stacks need none of this: they stand in the site surface itself.
//
// A void is a tube of elliptic section around a level segment in world coordinates, so it is a
// property of a point in space and does not depend on how a reader cuts space into blocks.
enum class SeaVoidKind : uint8_t { Notch, Cave };

struct SeaVoid {
    double ax{}, az{};   // m, one end of the axis
    double bx{}, bz{};   // m, the other end
    float y{};           // m, elevation of the axis
    float wide{};        // m, half-width across the axis
    float high{};        // m, half-height
    float water{};       // m, still water that stands in it
    SeaVoidKind kind{SeaVoidKind::Notch};
};

// Every void that reaches into the rectangle a reader asked for, in an order fixed by the world.
struct SeaVoids {
    std::vector<SeaVoid> voids;
};

// m around a rectangle within which a site cell can send a void into it: the longest cave, its
// width and its mouth. A reader needs finished site tiles one cell farther, 40 m in all.
inline constexpr double SeaVoidReach = 36.0;

// A finished site tile by its index, or null.
using SiteTiles = std::function<const Tile*(int32_t x, int32_t z)>;

// Every site tile within SeaVoidReach and one cell of the rectangle should be finished: a void
// comes from one site cell, its four neighbours and world-anchored planes, so every point then
// reads the same voids whatever rectangle it is asked through. Never blocks or builds. `setting`
// is the atlas's.
SeaVoids seaVoidsOver(const SiteTiles& tiles, const Setting& setting, double minX, double minZ, double maxX, double maxZ);

// Pure and cheap: no lock, no allocation. Void spans of the vertical line through a world
// position, merged and sorted from the lowest up; writes at most `capacity` and returns how many.
int seaVoidSpans(const SeaVoids& voids, Vec2 position, CaveSpan* spans, int capacity);

// The same test for one point: true exactly where a span of `seaVoidSpans` contains y.
bool inSeaVoid(const SeaVoids& voids, double x, double y, double z);

// Opens the voids in one column of voxels already filled by the form of the ground: `count`
// materials `step` apart upwards from `bottom`, at `out[k * stride]`. Solid rock inside a void
// becomes air (the reader's water surface says how much of it is flooded); loose cover and air stay.
void cutSeaVoids(const SeaVoids& voids, double x, double z, double bottom, double step, int count, Material* out, size_t stride);

} // namespace derivean::sim
