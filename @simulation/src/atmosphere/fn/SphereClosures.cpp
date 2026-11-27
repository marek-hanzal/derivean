#include "atmosphere/fn/SphereClosures.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereDamping.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace derivean::sim {

SphereClosures makeSphereClosures(const SphereGrid& grid, const SpherePoisson& poisson, const std::vector<double>& height, int bands) {
    assert(height.empty() || height.size() == size_t(grid.cells));
    SphereClosures closures;
    closures.stiffness = &poisson.levels[0].stiffness;
    closures.bands = makeLatitudeBands(grid, bands);
    closures.damping = makeSphereDamping(grid, *closures.stiffness, SphereGridDamping);
    closures.friction.resize(size_t(grid.cells));
    for (size_t c = 0; c < closures.friction.size(); ++c) {
        assert(height.empty() || height[c] >= 0);
        const double rough = height.empty() ? 0.0 : height[c] / SphereRoughHeight;
        closures.friction[c] = (1 + rough) / SphereSurfaceDrag;
    }
    closures.jet.assign(size_t(bands), 0.0);
    closures.relaxation = SphereMeanRelaxation;
    return closures;
}

void fillClosures(SphereClosures& closures, const SphereGrid& grid, const StencilMatrix& stiffness, const SphereFlow& flow,
                  SphereForcing& forcing) {
#ifndef NDEBUG
    double net = 0, size = 0;
    for (size_t b = 0; b < closures.jet.size(); ++b) {
        const double content = closures.bands.area[b] * closures.jet[b];
        net += content;
        size += std::abs(content);
    }
    assert(std::abs(net) <= 1e-12 * size);
#endif
    const std::vector<double>& level = baseLevel(flow);
    bandMeans(closures.bands, grid, level, closures.mean);
    for (size_t b = 0; b < closures.mean.size(); ++b) closures.mean[b] = (closures.jet[b] - closures.mean[b]) / closures.relaxation;
    spreadBands(closures.bands, closures.mean, forcing.tendency);
    addDamping(closures.damping, grid, stiffness, level, forcing.tendency);
    forcing.friction.assign(closures.friction.begin(), closures.friction.end());
}

bool advanceClosedHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                       SphereClosures& closures) {
    const StencilMatrix& stiffness = poisson.levels[0].stiffness;
    if (closures.stiffness != &stiffness) throw std::invalid_argument("the closures were made from another Poisson");
    hour.stiffest = dampingStiffness(closures.damping);
    return advanceSphereHour(hour, flow, grid, stencil, poisson, [&](const SphereFlow& state, SphereForcing& forcing) {
        fillClosures(closures, grid, stiffness, state, forcing);
    });
}

} // namespace derivean::sim
