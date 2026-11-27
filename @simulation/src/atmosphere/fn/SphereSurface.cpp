#include "atmosphere/fn/SphereSurface.hpp"

#include "atmosphere/constant/SphereAir.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace derivean::sim {

void setSurface(SphereFlow& flow, const SphereGrid& grid, const std::vector<double>& height) {
    const size_t cells = size_t(grid.cells);
    if (flow.vorticity.size() != cells) throw std::invalid_argument("the flow is not of this grid");
    if (!height.empty() && height.size() != cells) throw std::invalid_argument("the terrain is not one height per cell");
    for (double h : height)
        if (!(std::isfinite(h) && h >= 0)) throw std::invalid_argument("a terrain height is not finite and at least zero");
    flow.coriolis.resize(cells);
    for (size_t c = 0; c < cells; ++c) {
        const double planetary = 2 * flow.rotation * grid.centre[c].z;   // sin φ, as makeSphereFlow
        flow.coriolis[c] = height.empty() ? planetary : planetary * (1 + std::min(height[c], SphereTerrainLimit) / SphereTerrainDepth);
    }
}

} // namespace derivean::sim
