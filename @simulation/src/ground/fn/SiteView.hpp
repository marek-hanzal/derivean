#pragma once

#include "ground/type/SiteView.hpp"
#include "landscape/fn/Build.hpp"

namespace derivean::sim {

// The view around one ground tile, from the site tiles its replay was given.
SiteView readSites(TileKey ground, const TileLookup& lookup);

// The rock ice left at a position, from the site tile under it; Air where no ice came.
IceSource iceSourceAt(const TileLookup& lookup, Vec2 position);

// Follows the channel up from a position: against the flow, by the donor that brings the most water.
Upstream upstreamOf(const SiteView& view, Vec2 position, float distance);

} // namespace derivean::sim
