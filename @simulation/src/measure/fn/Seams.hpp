#pragma once

#include "measure/type/Mosaic.hpp"

namespace derivean::sim {

// How much the edges between the tiles of a mosaic show. Every ratio compares cells at a tile
// edge with cells inside tiles; 1 means the edge cannot be told from the interior.
struct SeamReport {
    double stepAcross{}, stepInside{};     // m, mean height step between neighbouring dry cells
    double stepRatio{};
    double bendAcross{}, bendInside{};     // m, mean change of that step from one pair of cells to the next
    double bendRatio{};
    double lakeNear{}, lakeFar{};          // share of cells under a lake within three cells of an edge, and elsewhere
    double lakeRatio{};                    // 1 when neither has lakes
    int crossings{};                       // rivers whose flow leaves a tile into its neighbour
    // Median discharge of the next cell over the river's own. Across an edge the next cell is the
    // wettest within two cells of where the flow points: the neighbour routed its own water and may
    // carry the river a cell or two aside without having lost it.
    double keptAcross{}, keptInside{};
    double lostAcross{}, lostInside{};     // share of river steps where the next cell carries less than 0.7 of it
    double keptRatio{};
    // The same crossings by which side was solved first. A river entering a tile of an earlier pass
    // meets cells that were finished before its own tile existed.
    int intoEarlier{}, intoLater{};
    double lostIntoEarlier{}, lostIntoLater{};
};

SeamReport seamsOf(const Mosaic& mosaic, double river);

} // namespace derivean::sim
