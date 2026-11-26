#pragma once

#include "planet/constant/Scale.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// Where the planet's water goes, on a grid fine enough to say where a river crosses the edge of a
// region domain. A region learns of the catchments beyond its edge only through this, and the two
// regions on either side of an edge must be told of the same river at the same place, so it is one
// field for the world. Wrapped like the planet.
struct Drainage {
    static constexpr int Ratio = 8;                         // cells per planet cell along each axis
    static constexpr int Cells = PlanetCells * Ratio;       // per side
    static constexpr double Cell = PlanetCell / Ratio;      // m
    std::vector<float> discharge;   // m³/yr leaving each cell
    std::vector<float> load;        // m³/yr of sediment with it
    std::vector<uint8_t> flowTo;    // neighbour index the water leaves by, 255 into the sea

    size_t index(int64_t gx, int64_t gz) const {
        const int64_t x = ((gx % Cells) + Cells) % Cells, z = ((gz % Cells) + Cells) % Cells;
        return size_t(z) * size_t(Cells) + size_t(x);
    }
};

} // namespace derivean::sim
