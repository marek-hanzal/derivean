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

bool balancedJet(const SphereClosures& closures) {
    if (closures.jet.size() != closures.bands.area.size()) return false;
    double net = 0, size = 0;
    for (size_t b = 0; b < closures.jet.size(); ++b) {
        const double content = closures.bands.area[b] * closures.jet[b];
        net += content;
        size += std::abs(content);
    }
    return std::isfinite(size) && std::abs(net) <= JetBalance * size;   // a NaN fails the comparison, an infinity the first test
}

bool closuresFit(const SphereClosures& closures, const SpherePoisson& poisson) {
    return closures.damping.stiffness == &poisson.levels[0].stiffness && balancedJet(closures);
}

SphereClosures makeSphereClosures(const SphereGrid& grid, const SpherePoisson& poisson, const std::vector<double>& height, int bands) {
    assert(height.empty() || height.size() == size_t(grid.cells));
    SphereClosures closures;
    closures.bands = makeLatitudeBands(grid, bands);
    closures.damping = makeSphereDamping(grid, poisson.levels[0].stiffness, SphereGridDamping);
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

void fillClosures(SphereClosures& closures, const SphereGrid& grid, const SpherePoisson& poisson, const SphereFlow& flow,
                  SphereForcing& forcing) {
    if (!closuresFit(closures, poisson)) throw std::invalid_argument("the closures do not fit this Poisson or their jet");
    const std::vector<double>& level = baseLevel(flow);
    bandMeans(closures.bands, grid, level, closures.mean);
    for (size_t b = 0; b < closures.mean.size(); ++b) closures.mean[b] = (closures.jet[b] - closures.mean[b]) / closures.relaxation;
    spreadBands(closures.bands, closures.mean, forcing.tendency);
    addDamping(closures.damping, grid, poisson.levels[0].stiffness, level, forcing.tendency);
    forcing.friction.assign(closures.friction.begin(), closures.friction.end());
}

bool advanceClosedHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                       SphereClosures& closures) {
    if (!closuresFit(closures, poisson)) throw std::invalid_argument("the closures do not fit this Poisson or their jet");
    hour.stiffest = dampingStiffness(closures.damping);
    return advanceSphereHour(hour, flow, grid, stencil, poisson,
                             [&](const SphereFlow& state, SphereForcing& forcing) { fillClosures(closures, grid, poisson, state, forcing); });
}

} // namespace derivean::sim
