#include "atmosphere/fn/SphereSurface.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/SphereFlow.hpp"

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
    bool changed = false;
    for (size_t c = 0; c < cells; ++c) {
        const double planetary = 2 * flow.rotation * grid.centre[c].z;   // sin φ, as makeSphereFlow
        const double background = height.empty() ? planetary : planetary * (1 + std::min(height[c], SphereTerrainLimit) / SphereTerrainDepth);
        changed = changed || background != flow.coriolis[c];
        flow.coriolis[c] = background;
    }
    // The leapfrog's older level and its filter were made with the old background: kept, the next
    // step would apply the new one over 2Δt, the first Δt before the change (1.95 times the response
    // at level 5, Codex DREV-CLAUDE-010-04). The same background keeps the run bit for bit.
    if (changed) restartHistory(flow);
}

} // namespace derivean::sim
