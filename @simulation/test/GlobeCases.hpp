#pragma once

#include "geometry/fn/Cell.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>

// Helpers of the hand-made globes of GlobeCohesion.cpp and GlobeCover.cpp.
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

// Whether every piece names a plate there is.
inline bool platesValid(const Globe& globe) {
    return std::all_of(globe.pieces.begin(), globe.pieces.end(), [&](const CrustPiece& piece) { return piece.plate < globe.epoch.plates.size(); }) &&
           globe.motion.size() == globe.epoch.plates.size() && globe.applied.size() == globe.epoch.plates.size();
}

} // namespace derivean::sim::globe_test
