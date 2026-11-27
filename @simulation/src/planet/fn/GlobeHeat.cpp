#include "planet/fn/GlobeHeat.hpp"

#include "planet/fn/Coupling.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// The flat Heat.cpp's numbers.
constexpr double MeltCooling = 3 * Myr;
constexpr float MeltForHeat = 600.0f;

} // namespace

void refreshGlobeMeltHeat(Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const size_t cells = size_t(grid.cells);
    std::vector<double> raw(cells, 0.0);
    for (size_t cell = 0; cell < cells; ++cell)
        raw[cell] = viewOf(globe.coupling, globe.pieces, int(cell), [&](const CrustPiece& piece) {
            const CrustColumn& column = piece.column;
            if (column.meltTime < 0) return 0.0;
            // Melt that rose during the planet's last step is the present-day activity.
            const double age = std::max(0.0, globe.time - double(column.meltTime) - PlanetStep);
            return std::exp(-age / MeltCooling) * std::min(1.0, double(column.melt / MeltForHeat));
        });
    for (size_t cell = 0; cell < cells; ++cell) {
        // makeRings lists the four edge neighbours first, then the corners.
        double sum = 0.25 * grid.area[cell] * raw[cell], weight = 0.25 * grid.area[cell];
        const CellRing& ring = (*globe.rings)[cell];
        for (size_t k = 0; k < ring.size(); ++k) {
            if (ring[k] < 0) continue;
            const double w = (k < 4 ? 0.125 : 0.0625) * grid.area[size_t(ring[k])];
            sum += w * raw[size_t(ring[k])], weight += w;
        }
        globe.meltHeat.value[cell] = float(sum / weight);
    }
}

} // namespace derivean::sim
