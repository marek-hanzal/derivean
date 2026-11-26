#pragma once

#include "measure/type/Distribution.hpp"
#include "measure/type/Mosaic.hpp"

namespace derivean::sim {

// How high dry land stands, cell by cell.
struct LandHeights {
    double dryShare{}, lakeShare{}, seaShare{};
    std::vector<float> aboveSea;     // m, every dry cell against the sea level
    // m, against the first water the cell's flow path meets: the sea, a lake or a river. A path that
    // leaves the mosaic first is measured against the last ground it crossed, a lower bound.
    std::vector<float> aboveWater;
    double traced{};                 // share of dry cells whose path met water inside the mosaic
};

LandHeights landHeights(const Mosaic& mosaic, double river);

// Straight lines of one length laid over the mosaic along both axes and both diagonals. Lines with
// more than a quarter of their samples under the sea are left out.
struct TransectStats {
    double length{};       // m
    double spacing{};      // m between samples on the axes; total climb depends on it
    Distribution relief;   // m, highest minus lowest point of a line
    Distribution climb;    // m, sum of every rise along a line
    Vec2 steepest;         // world position where the line of greatest relief starts
};

TransectStats transectsOf(const Mosaic& mosaic, double length);

// Relief a line of the given length has on Earth: the upper end of plains, of hill country and of
// the highest mountain fronts. Rounded from general knowledge of Earth's relief, not from a data
// set; they are yardsticks for flagging outliers, not targets.
struct ReliefBand {
    double plain{}, hills{}, mountains{};   // m
};

ReliefBand earthRelief(double length);
const char* reliefClass(double relief, double length);

} // namespace derivean::sim
