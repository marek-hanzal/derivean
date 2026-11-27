#pragma once

#include "ground/type/Clast.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

// The surface at one point, read from the finest level that is finished there.
struct SurfaceSample {
    float ground{};        // m, top of loose or solid ground
    float rock{};          // m, top of solid rock
    float datum{};         // m, original top of the layering; `datum - y` is the depth a point lies in it
    float water{};         // m, surface of standing water; below `ground` where the land is dry
    float sediment[SedimentClasses]{};   // m of coarse, sand and fine cover
    float discharge{};     // m³/yr, mean annual flow through the cell
    float rain{};          // m/yr
    float warmth{};        // °C, annual mean at the ground
    float erosionRate{}, depositionRate{};   // m/yr, recent
    Material bedrock{Material::Bedrock};
    float deposit{};       // m of mineral a spring laid on the cover; counted in `ground`. Ground level only
    Material depositRock{Material::Air};   // Sinter or Travertine where there is any
    Material coarseAs{Material::Gravel};   // what the coarse cover reads as there: Gravel, Scree or Till. Ground level only
    Level level{Level::Region};
    bool found{};          // false where not even a region tile is finished
};

// Never blocks and never starts work: answers from what the atlas already holds, no finer than
// `finest`. Voxels are cut from the ground level: ask for `Level::Ground` where a caller needs the
// surface they show, and for the site level where four metres are fine enough.
SurfaceSample surfaceAt(const World& world, Vec2 position, Level finest = Level::Site);

// What the session has changed, laid over a sample read from the untouched tiles of the site or
// ground level at `position`: ground and cover as dug and built, live water, and the sea at the
// level the ocean has now. surfaceAt and surveyAt have done it; a caller that reads tiles itself
// (readSurface) does it with this. It only reads, and like every reader of the session's state it
// must not run while the thread that drives time and edits is inside one of its calls
// (intervention/fn/Intervention.hpp).
void overlayLive(const World& world, Vec2 position, SurfaceSample& sample);

// Queues every tile of `level` within `radius` of `center`; nearer tiles first.
void requestAround(World& world, Vec2 center, double radius, Level level);

// True once every tile of `level` within `radius` is finished.
bool readyAround(const World& world, Vec2 center, double radius, Level level);

// Blocks until the tiles of `finest` under the point exist, then answers from them. Asked for the
// ground level this is the authoritative surface: what collision, voxels and the inspector must
// agree with. Stones stand on it and are not part of it.
SurfaceSample surveyAt(World& world, Vec2 position, Level finest = Level::Site);

// Authoritative material at a point in space; blocks like surveyAt.
Material materialAt(World& world, double x, double y, double z);

// The stone a point lies in, with the rock it is a piece of, the process that left it there, the
// place that rock crops out and how far it came; blocks like materialAt. `found` is false in air,
// loose cover and bedrock. This is where a boulder, a piece of vein float in a stream bed or an
// erratic in a till says where it is from.
struct StoneSample {
    bool found{};
    Clast stone;       // positions inside are relative to its ground tile; `at` is where it lies
    Vec2 at;           // m, world position of its centre
    float height{};    // m, elevation of its centre now
};

StoneSample stoneAt(World& world, double x, double y, double z);

// Materials at the centres of a regular lattice of cubes: `count` cubes of edge `step` along each
// axis from the corner `origin`. Output is indexed (y * countZ + z) * countX + x. One call shares
// the column work of a whole voxel chunk; the answers equal materialAt at each centre. Ground the
// session dug or built reads as it is now.
struct Block {
    double originX{}, originY{}, originZ{};
    double step{};
    int countX{}, countY{}, countZ{};
};

void sampleBlock(World& world, const Block& block, Material* materials);

// Blocks of one footprint at several heights: each span is filled as sampleBlock fills the block of
// its originY and countY (the footprint's own are not read), and the work of the footprint's columns
// is done once.
struct BlockSpan {
    double originY{};
    int countY{};
    Material* materials{};
};

void sampleBlocks(World& world, const Block& footprint, const BlockSpan* spans, size_t count);

} // namespace derivean::sim
