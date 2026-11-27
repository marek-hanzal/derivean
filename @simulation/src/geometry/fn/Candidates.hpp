#pragma once

#include "geometry/type/SphereGrid.hpp"
#include "geometry/type/SpherePolygon.hpp"

#include <vector>

namespace derivean::sim {

// Every cell of the grid whose overlap with a convex footprint can be positive, by the footprint's
// envelope: the cap of capOf(footprint) (centre m, chord ρ). A cell meets the footprint only where it
// meets that cap, and the cell lies in its own cap (centre c, chord grid.reach), so by the triangle
// inequality of chords it qualifies only if |m − c| ≤ ρ + reach. The cells passing that test are
// collected by a flood over shared edges from the cell holding m, across face edges and round cube
// vertices like any other edge. The flood misses none: between two cells that meet the footprint
// runs a geodesic inside it (it is convex), every cell that geodesic crosses passes the test, and
// where it crosses a vertex instead of an edge the vertex lies in the footprint, so every cell round
// that vertex passes too. A centre and its neighbours are not enough: a footprint rotated about a
// point near a face's edge reaches cells two rows away (Codex DREV-CLAUDE-003-02).
//
// `cells` receives the candidates in flood order (deterministic). It is cleared first; its capacity
// is reused. Meant for footprints of about one cell (4 to 13 candidates measured at levels 5 and 7): the
// visited set is a linear list.
void candidateCells(const SphereGrid& grid, const SpherePolygon& footprint, std::vector<int>& cells, std::vector<int>& work);
// The same from the footprint's cap (capOf), when the caller has it.
void candidateCells(const SphereGrid& grid, const PolygonCap& cap, std::vector<int>& cells, std::vector<int>& work);

} // namespace derivean::sim
