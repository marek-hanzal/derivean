#pragma once

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Regions.hpp"

#include <algorithm>
#include <cmath>

// Helpers of the hand-made globes of GlobeCohesion.cpp, GlobeCover.cpp and GlobeReal.cpp.
namespace derivean::sim::globe_test {

inline int cellAtFace0(const SphereGrid& grid, int i, int j) {
    Cell cell = cellAt(grid, 0);
    cell.i = i, cell.j = j;
    return indexOf(grid, cell);
}

inline int ownerId(const Globe& globe, int cell) {
    const int piece = dominantPiece(globe.coupling, cell);
    return piece >= 0 ? globe.motion[globe.pieces[size_t(piece)].plate].id : -1;
}

// The stock with the interior closes, each part to 1e-15 of itself: the interior's ledger adds a seam's
// births and consumptions one by one in plain doubles (planet/fn/Lay, Transfers), so a closing may leave
// an ulp or two of it (the rounding fixture: 4 m³ of basalt, a few ulps of its sum), where the fixtures with no
// seam close exactly.
inline bool stockCloses(const Globe& globe, CrustStock before) {
    const CrustStock after = totalStock(globe.pieces);
    return std::abs(after.felsic + globe.ledger.interior.felsic - before.felsic) <= 1e-15 * before.felsic &&
           std::abs(after.basalt + globe.ledger.interior.basalt - before.basalt) <= 1e-15 * before.basalt;
}

// m² cell `c` holds of the given pieces' footprints, by clipping and union.
inline double cellCover(const Globe& globe, int c, const std::vector<CrustPiece>& pieces) {
    const SphereGrid& grid = *globe.grid;
    const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(c)]);
    const PlanePolygon cell = projected(plane, polygonOf(grid, c));
    std::vector<PlanePolygon> parts;
    for (const CrustPiece& piece : pieces) {
        const PlanePolygon part = clipped(projected(plane, footprintOf(grid, piece)), cell);
        if (part.count >= 3) parts.push_back(part);
    }
    return parts.empty() ? 0 : unionArea(parts).area;
}

// sr a region's cells hold of the given pieces' footprints.
inline double coverOf(const Globe& globe, const PlateRegions& regions, int32_t region, const std::vector<CrustPiece>& pieces) {
    double cover = 0;
    for (int c = 0; c < globe.grid->cells; ++c)
        if (regions.region[size_t(c)] == region) cover += cellCover(globe, c, pieces);
    return cover / (PlanetRadius * PlanetRadius);
}

// Whether one of the pieces covers OwnedShare of cell `c` by its real footprint, as a cell is owned.
inline bool standsOn(const Globe& globe, int c, const std::vector<CrustPiece>& pieces) {
    return std::any_of(pieces.begin(), pieces.end(), [&](const CrustPiece& piece) { return cellCover(globe, c, {piece}) >= OwnedShare * globe.grid->area[size_t(c)]; });
}

// `piece` turned so that its centre, born at its birth cell's centre, stands at `target`.
inline CrustPiece turnedTo(const Globe& globe, CrustPiece piece, Vec3 target) {
    const Vec3 born = globe.grid->centre[size_t(piece.birth)], axis = cross(born, target);
    piece.rotation = rotationAbout(axis, std::atan2(length(axis), dot(born, target)));
    return piece;
}

// Whether every piece names a plate there is.
inline bool platesValid(const Globe& globe) {
    return std::all_of(globe.pieces.begin(), globe.pieces.end(), [&](const CrustPiece& piece) { return piece.plate < globe.epoch.plates.size(); }) &&
           globe.motion.size() == globe.epoch.plates.size() && globe.applied.size() == globe.epoch.plates.size();
}

} // namespace derivean::sim::globe_test
