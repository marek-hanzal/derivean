#include "atmosphere/fn/SphereClosures.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/EddyBand.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereDamping.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
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

namespace {

// The eddies' state fits the bands' grid: growth per band, eddies and the band's cells per cell (a band
// of another grid would read past the flow's ζ), and a clock that can come due.
bool admissibleEddies(const SphereClosures& closures) {
    const size_t cells = closures.bands.first.size();
    if (closures.growth.size() != closures.bands.area.size() || closures.eddy.size() != cells || closures.band.memberCell.size() != cells)
        return false;
    if (!(std::isfinite(closures.eddyEvery) && closures.eddyEvery > 0 && std::isfinite(closures.since) && closures.since >= 0)) return false;
    for (double rate : closures.growth)
        if (!(std::isfinite(rate) && rate >= 0)) return false;
    for (double value : closures.eddy)
        if (!std::isfinite(value)) return false;
    return true;
}

} // namespace

bool closuresFit(const SphereClosures& closures, const SpherePoisson& poisson) {
    return closures.damping.stiffness == &poisson.levels[0].stiffness && balancedJet(closures) && admissibleEddies(closures);
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
    closures.growth.assign(size_t(bands), 0.0);
    closures.relaxation = SphereMeanRelaxation;
    closures.band = makeEddyBand(grid, SphereEddyWavelength, SphereEddyReach);
    closures.eddy.assign(size_t(grid.cells), 0.0);
    closures.eddyEvery = SphereEddyEvery;
    return closures;
}

void startEddies(SphereClosures& closures, const SphereFlow& flow) {
    if (closures.band.memberCell.size() != flow.vorticity.size() || closures.bands.first.size() != flow.vorticity.size())
        throw std::invalid_argument("the eddy band or the bands are not of the flow's grid");
    filterEddyBand(closures.band, flow.vorticity, closures.staged);
    closures.eddy.swap(closures.staged);
    closures.since = 0;
}

void eddyFeeding(SphereClosures& closures, const SphereGrid& grid, const std::vector<double>& eddy, std::vector<double>& feeding) {
    const size_t cells = eddy.size(), count = closures.bands.area.size();
    closures.square.resize(cells);
    for (size_t c = 0; c < cells; ++c) closures.square[c] = eddy[c] * eddy[c];
    bandMeans(closures.bands, grid, closures.square, closures.mean);
    const double width = closures.bands.width * 180 / std::numbers::pi;
    const long reach = std::max(1L, std::lround(SphereEnvelopeReach / width));
    assert(reach < long(count));
    const double norm = double((reach + 1) * (reach + 1)), cap = SphereEddyLimit * SphereEddyLimit;
    closures.feed.resize(count);
    for (long b = 0; b < long(count); ++b) {
        double envelope = 0;
        for (long offset = -reach; offset <= reach; ++offset) {
            long k = b + offset;
            if (k < 0) k = -k - 1;                                // past the south pole
            if (k >= long(count)) k = 2 * long(count) - 1 - k;    // past the north pole
            const double term = double(reach + 1 - std::abs(offset)) * closures.mean[size_t(k)];
            envelope += term;
        }
        envelope /= norm;
        closures.feed[size_t(b)] = closures.growth[size_t(b)] * std::max(0.0, 1.0 - envelope / cap);
    }
    spreadBands(closures.bands, closures.feed, feeding);
    for (size_t c = 0; c < cells; ++c) feeding[c] *= eddy[c];
}

namespace {

// The relaxation, the eddies' tendency of the window (closures.feeding, eddyFeeding) and the damping
// of closures that fit `poisson` (closuresFit, checked by the caller). addDamping still compares its
// K with the damping's own at every step.
void fillTendency(SphereClosures& closures, const SphereGrid& grid, const SpherePoisson& poisson, const SphereFlow& flow,
                  std::vector<double>& tendency) {
    const std::vector<double>& level = baseLevel(flow);
    bandMeans(closures.bands, grid, level, closures.mean);
    for (size_t b = 0; b < closures.mean.size(); ++b) closures.mean[b] = (closures.jet[b] - closures.mean[b]) / closures.relaxation;
    spreadBands(closures.bands, closures.mean, tendency);
    assert(closures.feeding.size() == tendency.size());
    for (size_t c = 0; c < tendency.size(); ++c) tendency[c] += closures.feeding[c];
    addDamping(closures.damping, grid, poisson.levels[0].stiffness, level, tendency);
}

} // namespace

void fillClosures(SphereClosures& closures, const SphereGrid& grid, const SpherePoisson& poisson, const SphereFlow& flow,
                  SphereForcing& forcing) {
    if (!closuresFit(closures, poisson)) throw std::invalid_argument("the closures do not fit this Poisson or their jet");
    eddyFeeding(closures, grid, closures.eddy, closures.feeding);
    fillTendency(closures, grid, poisson, flow, forcing.tendency);
    forcing.friction.assign(closures.friction.begin(), closures.friction.end());
}

// closuresFit once a window (Neon DREV-NEON-065): within it the closures and the Poisson are the same
// objects and nothing changes them, the source only reads them, so the fit holds at every step; the
// friction is the hour's fixed one, read in place. The eddies' tendency is the window's context
// (SphereForcingSource), made once before the window from the eddies it holds; a refresh waits in
// `staged` and is swapped in, which cannot throw, only once the window is taken.
bool advanceClosedHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                       SphereClosures& closures) {
    if (!closuresFit(closures, poisson)) throw std::invalid_argument("the closures do not fit this Poisson or their jet");
    const bool refresh = closures.since >= closures.eddyEvery;
    if (refresh) filterEddyBand(closures.band, flow.vorticity, closures.staged);
    eddyFeeding(closures, grid, refresh ? closures.staged : closures.eddy, closures.feeding);
    hour.stiffest = dampingStiffness(closures.damping);
    const bool taken = advanceSphereHour(
        hour, flow, grid, stencil, poisson,
        [&](const SphereFlow& state, SphereForcing& forcing) { fillTendency(closures, grid, poisson, state, forcing.tendency); },
        closures.friction);
    if (!taken) return false;
    if (refresh) {
        closures.eddy.swap(closures.staged);
        closures.since = 0;
    }
    closures.since += hour.window;
    return true;
}

} // namespace derivean::sim
