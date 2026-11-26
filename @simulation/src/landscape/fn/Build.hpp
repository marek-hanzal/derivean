#pragma once

#include "landscape/fn/Process.hpp"
#include "landscape/type/Setting.hpp"

#include <functional>
#include <memory>

namespace derivean::sim {

// Returns a finished tile or null. A builder only ever asks for the tiles `dependencies` listed.
using TileLookup = std::function<const Tile*(TileKey)>;

// Tiles that must be finished first: the parents that cover the domain and the neighbours of an
// earlier pass. Fixed by the key alone, which is what makes the result independent of visiting order.
std::vector<TileKey> dependencies(TileKey key);

// Build.cpp — the domain at the start of its window.
Domain buildDomain(const Setting& setting, TileKey key, const TileLookup& lookup);

// Seed.cpp — initial ground of a domain from the planet (region) or from the parent level.
void seedFromPlanet(Domain& domain, const Setting& setting);
void seedFromParent(Domain& domain, const Setting& setting, const TileLookup& lookup);

// Structure.cpp — joints, corridors and bounding faults of the rock each cell starts in (Region, Local, Site).
void readStructure(Domain& domain, const Setting& setting);

// Uplift.cpp — temperature at the ground for the date of the step, the rain relief wrings out of
// the wind and the share of it that runs off; then the rise of the rock: tectonic forcing plus, for
// a region, the plate floating back up by most of what erosion took off it, and for a finer level
// the rate that holds its averaged surface on its parent's.
void readClimate(Domain& domain, const Setting& setting);
void raise(Domain& domain, FlowScratch& scratch);

// Extract.cpp — cells a finished neighbour owns move along the straight path from their start to
// its result; at the end the tile's own edge cells are faded into what those neighbours found for
// them (and written back into the domain), and the finished tile is read off the domain.
void followNeighbours(Domain& domain, double time);
void settleSeams(Domain& domain, Tile& tile);
// Weight of a tile's own value `distance` cells in from an edge it blends with a finished
// neighbour's (blendSeams). Labels blended along the seam fade with the same weight; where the
// ground's courses take the blend back (keepCourses), they still follow the full blend.
inline float seamWeight(int distance) { return float(smoothstep(0.0, 1.0, (distance + 0.5) / Apron)); }
void extractTile(const Domain& domain, const FlowScratch& scratch, Tile& tile);

// Replay.cpp — one step, and the whole window from build to the finished tile.
void stepDomain(Domain& domain, FlowScratch& scratch, const Setting& setting);
std::shared_ptr<Tile> replayTile(const Setting& setting, TileKey key, const TileLookup& lookup);

// A coarser flow field read by global cell index: discharge in m³/yr and the neighbour it leaves by.
struct CoarseFlow {
    double cell{};
    std::function<float(int64_t, int64_t)> discharge;
    std::function<float(int64_t, int64_t)> load;   // m³/yr of sediment
    std::function<uint8_t(int64_t, int64_t)> flowTo;
};

// Adds the water of coarse rivers that enter across the domain edge to the cells where they cross,
// and marks the edge cells through which the coarse rivers leave. Water may only leave there: an
// edge that let it out anywhere would give every domain a divide of its own along its border.
void injectInflow(Domain& domain, const CoarseFlow& coarse);

} // namespace derivean::sim
