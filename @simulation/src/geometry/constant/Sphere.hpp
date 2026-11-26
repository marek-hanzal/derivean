#pragma once

#include <cstdint>

namespace derivean::sim {

// The planet is a sphere of Earth's mean radius in real metres (Moritz 2000, GRS 80: R1 = 6 371 008.8 m,
// rounded). Shrinking it would break the atmosphere's scale height, the Rossby radius, the plates and
// the horizon. Earth's flattening (WGS 84, NIMA 2000) is a known deviation, not modelled yet: the
// equatorial and polar radii differ by 21 km, so latitudes are geocentric and gravity has no latitude
// term.
inline constexpr double PlanetRadius = 6371000.0;               // m
inline constexpr double EarthFlattening = 1.0 / 298.257223563;  // recorded only

// Six cube faces in S2's numbering and orientation, each a quadtree of cells. Level L has 2^L × 2^L
// cells per face; indices fit int32 up to MaxLevel.
inline constexpr int FaceCount = 6;
inline constexpr int MaxLevel = 30;

// The level ladder of tiles on a face. A tile at level L holds 2^TileLevels = 256 cells per side at
// level L + TileLevels (the landscape's TileCells today); a Ground cell is nominally 0.298 m (face
// centre arc / 2^25) and a spatial sampler turns it into 0.125 m voxels.
inline constexpr int RegionLevel = 5;
inline constexpr int LocalLevel = 9;
inline constexpr int SiteLevel = 13;
inline constexpr int GroundLevel = 17;
inline constexpr int TileLevels = 8;
inline constexpr int GroundCellLevel = GroundLevel + TileLevels;

// Version of the face mapping and the key format together; anything stored by key (caches, saves,
// edit logs) records it, and a change to either bumps it.
//   Version 1: S2 faces and orientation, equiangular (tangent) ST<->UV mapping exact at the face edges,
//   the S2 quadtree of cells. A key packs face (bits 61..63), the cell's position as a Z-order
//   interleave of i and j (Morton 1966; i's bit above j's at each level, the coarsest level highest,
//   from bit 60 down), then one sentinel bit below the last level; the bits under the sentinel are
//   zero. The ordering is not S2's Hilbert order, so keys are not S2CellIds.
inline constexpr std::uint32_t GeometryVersion = 1;

} // namespace derivean::sim
