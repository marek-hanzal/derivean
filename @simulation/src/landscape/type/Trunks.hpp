#pragma once

#include "planet/constant/Scale.hpp"

#include <cstdint>
#include <vector>

namespace derivean::sim {

// The world's trunk rivers on a grid between the planet's and a region's: where its large rivers
// run and how much they carry, decided once for the world so that the two regions on either side of
// an edge are told of the same river at the same place with the same water. The coarser level is the
// authority on where water goes and how much. Wrapped like the planet.
struct Trunks {
    static constexpr int Ratio = 16;                        // cells per planet cell along each axis
    static constexpr int Cells = PlanetCells * Ratio;       // per side
    static constexpr double Cell = PlanetCell / Ratio;      // m
    std::vector<float> ground;      // m, the ground the trunks were routed over, after their own erosion
    std::vector<float> discharge;   // m³/yr leaving each cell
    std::vector<float> load;        // m³/yr of sediment the planet's river carries with it, not what the trunks' own erosion adds
    std::vector<uint8_t> flowTo;    // neighbour index the water leaves by, 255 into the sea

    size_t index(int64_t gx, int64_t gz) const {
        const int64_t x = ((gx % Cells) + Cells) % Cells, z = ((gz % Cells) + Cells) % Cells;
        return size_t(z) * size_t(Cells) + size_t(x);
    }
};

} // namespace derivean::sim
