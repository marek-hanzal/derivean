#pragma once

#include "planet/type/Planet.hpp"

namespace derivean::sim {

// World position of the dry planet cell nearest to `near` that touches the sea and belongs to a
// sizeable landmass; `near` itself on a planet with no such shore. Unwrapped, next to `near`.
Vec2 findShore(const Planet& planet, Vec2 near);

// World position of a hospitable low planet cell with a high range standing over it: the country
// a first view should open on. Trades the height of the range against the distance from `near`;
// the answer of findShore on a planet that has no such place.
Vec2 findVale(const Planet& planet, Vec2 near);

struct World;

// A place to stand at the water's edge under high ground: starts from findVale, picks in the region
// the shore of a sea, lake or river with the most country rising over it, then walks down through
// the local and site levels to a dry site cell with water beside it. Each level searches only the
// one tile under the answer so far, within a reach of it. Blocks while those tiles are built. Falls
// back to the best position found when a level has no shore within its reach.
Vec2 findLanding(World& world, Vec2 near);

} // namespace derivean::sim
