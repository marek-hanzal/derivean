#pragma once

#include "landscape/type/Level.hpp"

#include <memory>
#include <vector>

namespace derivean::sim {

struct World;

// Caves are voids dissolved by groundwater: an outcome of the groundwater model, never placed.
// A cave is a set of conduits, each a tube around a straight segment in world coordinates, so a
// void is a property of a point in space and does not depend on how a reader cuts space into
// blocks or columns.
struct Conduit {
    double ax{}, ay{}, az{};   // m, one end of the axis; y is elevation
    double bx{}, by{}, bz{};   // m, the other end
    float radius{};            // m
    float flux{};              // m³/s of groundwater that passes through it
    float age{};               // yr it has been growing
    uint32_t id{};             // stable for a world
};

// One span of void on a vertical line.
struct CaveSpan {
    float bottom{}, top{};     // m, elevation
};

// Every conduit of one local tile, with an index for point queries. No tube comes within a cell of
// the tile's edge (fn/Conduits.cpp), so the field of the tile a point lies in is all a reader
// needs. A field is a pure function of the world's setting and the tile key.
struct CaveField {
    TileKey key;                         // Level::Local
    std::vector<Conduit> conduits;
    int cellsPerSide{};                  // index cells along one side of the tile
    std::vector<uint32_t> cellStart;     // cellsPerSide² + 1 offsets into cellItems, row-major
    std::vector<uint32_t> cellItems;     // conduits whose tube overlaps the index cell
};

// The field of the local tile under a position. Blocks while it is built; once built it is kept
// and the call only looks it up. Hold the pointer for a whole chunk of queries.
std::shared_ptr<const CaveField> caveFieldAt(World& world, Vec2 position);

// Pure and cheap: no lock, no allocation. Void spans of the vertical line through a world
// position, merged and sorted from the lowest up; writes at most `capacity` and returns how many.
// `position` must lie in the field's tile.
int caveSpans(const CaveField& field, Vec2 position, CaveSpan* spans, int capacity);

// The same test for one point; `inCave(f, x, y, z)` is true exactly where a span of
// `caveSpans(f, {x, z}, …)` contains y.
bool inCave(const CaveField& field, double x, double y, double z);

} // namespace derivean::sim
