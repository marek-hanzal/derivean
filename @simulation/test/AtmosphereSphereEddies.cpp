// The eddies of the sphere's circulation (SphereClosures, S4.3c): the band of ζ refreshed every
// eddyEvery of windows taken, growing at each band's growth until its envelope reaches EddyLimit,
// held through a window as its context and published only with the window taken.
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/EddyBand.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "atmosphere/fn/SphereTransport.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Degrees = 180.0 / std::numbers::pi;
constexpr double Hour = 3600.0;

// The envelope as the header states it, by latitude: the band at centre + o · width, mirrored back
// across a pole it passes (90° + d is 90° − d), weighted R + 1 − |o|.
std::vector<double> feedOracle(const SphereClosures& closures, const std::vector<double>& strength, int reach) {
    const LatitudeBands& bands = closures.bands;
    std::vector<double> feed(size_t(bands.count));
    for (int b = 0; b < bands.count; ++b) {
        double envelope = 0;
        for (int o = -reach; o <= reach; ++o) {
            double latitude = (bands.centre[size_t(b)] + o * bands.width) * Degrees;
            if (latitude > 90) latitude = 180 - latitude;
            if (latitude < -90) latitude = -180 - latitude;
            const int k = int(std::lround((latitude / Degrees - bands.centre[0]) / bands.width));
            envelope += (reach + 1 - std::abs(o)) * strength[size_t(k)];
        }
        envelope /= (reach + 1) * (reach + 1);
        feed[size_t(b)] = closures.growth[size_t(b)] * std::max(0.0, 1 - envelope / (SphereEddyLimit * SphereEddyLimit));
    }
    return feed;
}

// Eddies whose strength rises from nothing at the south pole past the cap in the north, at levels 5
// and 6 (R 2 and 4 bands, the flat world's ±11.25°), and a growth varying by band: every cell's
// tendency is the header's feed spread to it times its eddy, to 1e-15 of the largest; bands well
// below the cap grow nearly at their rate, bands above it not at all (exactly zero), some between.
void checkEnvelope() {
    for (int level : {5, 6}) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        SpherePoisson poisson = makeSpherePoisson(grid, stencil);
        SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
        for (int b = 0; b < grid.side; ++b) closures.growth[size_t(b)] = 1e-5 * (1.5 + std::sin(0.7 * b));
        std::vector<double> eddy = harmonicField(grid, 12, 5);
        for (int c = 0; c < grid.cells; ++c) eddy[size_t(c)] *= 2.2e-5 * (1 + grid.centre[size_t(c)].z);
        std::vector<double> feeding, square(eddy.size()), strength, cell;
        eddyFeeding(closures, grid, eddy, feeding);
        for (size_t c = 0; c < eddy.size(); ++c) square[c] = eddy[c] * eddy[c];
        bandMeans(closures.bands, grid, square, strength);
        const int reach = level == 5 ? 2 : 4;
        const std::vector<double> feed = feedOracle(closures, strength, reach);
        spreadBands(closures.bands, feed, cell);
        double worst = 0, top = 0;
        for (size_t c = 0; c < eddy.size(); ++c) {
            top = std::max(top, std::abs(cell[c] * eddy[c]));
            worst = std::max(worst, std::abs(feeding[c] - cell[c] * eddy[c]));
        }
        expect(top > 0 && worst <= 1e-15 * top, "the eddies' tendency is not the enveloped growth times eddy", worst / top);
        int free = 0, capped = 0, between = 0;
        for (int b = 0; b < grid.side; ++b) {
            const double share = closures.feed[size_t(b)] / closures.growth[size_t(b)];
            free += share > 0.95, capped += closures.feed[size_t(b)] == 0, between += share > 0.05 && share < 0.95;
        }
        expect(free > 2 && capped > 2 && between > 2, "the envelope's test reached too few bands", double(capped));
    }
}

// From rest (no relaxation, no damping) the step's tendency is the eddies' alone: fillClosures'
// tendency is eddyFeeding of the published eddies bit for bit, and at zero growth zero.
void checkGrowth() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
    const SphereFlow rest = makeSphereFlow(grid, rotationRateAt(PresentAge));
    closures.eddy = harmonicField(grid, 12, 5);
    for (double& v : closures.eddy) v *= 1e-5;
    SphereForcing still;
    fillClosures(closures, grid, poisson, rest, still);
    for (double v : still.tendency) expect(v == 0, "eddies grow without growth", v);
    closures.growth.assign(size_t(grid.side), 1 / (1.5 * 86400));
    SphereForcing forcing;
    fillClosures(closures, grid, poisson, rest, forcing);
    std::vector<double> feeding;
    eddyFeeding(closures, grid, closures.eddy, feeding);
    double top = 0;
    for (double v : feeding) top = std::max(top, std::abs(v));
    expect(top > 0 && forcing.tendency == feeding, "the step's tendency is not the eddies' growth", top);
}

struct Run {
    SphereGrid grid = makeSphereGrid(4);
    SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
    Run() {
        std::vector<double> start = rossbyHaurwitz(grid, flow.rotation, 0), wave = harmonicField(grid, 12, 5);
        for (size_t c = 0; c < start.size(); ++c) start[c] += 1e-5 * wave[c];
        expect(setVorticity(flow, grid, stencil, poisson, start), "the eddies' start did not invert");
        closures.growth.assign(size_t(grid.side), 1 / (1.5 * 86400));
    }
};

// Windows of an hour: the eddies stay zero through the first two, the third refreshes them at its
// start, its eddies the band of the ζ it starts from bit for bit, the fourth keeps them, the fifth
// refreshes again; `since` counts the windows taken since the refresh.
void checkCadence() {
    Run run;
    SphereHour hour;
    EddyBand band = run.closures.band;
    std::vector<double> expected(run.closures.eddy.size(), 0.0);
    for (int window = 1; window <= 5; ++window) {
        const bool refresh = window == 3 || window == 5;
        if (refresh) filterEddyBand(band, run.flow.vorticity, expected);
        expect(advanceClosedHour(hour, run.flow, run.grid, run.stencil, run.poisson, run.closures), "a closed window was not taken", window);
        expect(run.closures.eddy == expected, "the eddies are not the band of the window's start", window);
        expect(run.closures.since == (window == 1 || window == 3 || window == 5 ? Hour : 2 * Hour), "the eddies' clock moved", window);
    }
    double top = 0;
    for (double v : expected) top = std::max(top, std::abs(v));
    expect(top > 1e-7, "the eddies' band is empty", top);
}

// A refreshing window refused at its first try (the limit three quarters of one step's Courant
// number) and taken in two is the clean two-step run with the refreshed eddies bit for bit, flow and
// published eddies, and each of its steps is fillClosures' forcing with them; a window refused at
// every try publishes nothing (the eddies, `since` and the flow as they came in), and the same call
// after it is that clean run too.
void checkRetry() {
    Run base;
    base.closures.since = base.closures.eddyEvery;
    const SphereFlow begin = base.flow;
    std::vector<double> refreshed;
    filterEddyBand(base.closures.band, begin.vorticity, refreshed);
    const double one = outflowCourant(base.grid, base.stencil, begin.flux, Hour);

    SphereClosures clean = base.closures;
    SphereHour preset;
    preset.steps = 2;
    SphereFlow cleanFlow = begin;
    expect(advanceClosedHour(preset, cleanFlow, base.grid, base.stencil, base.poisson, clean) && preset.refusals.empty(),
           "the clean two-step window was not taken at once");
    expect(clean.eddy == refreshed && clean.since == Hour, "the clean window did not publish its refresh");

    SphereClosures by = base.closures;
    by.eddy = refreshed;
    SphereFlow byHand = begin;
    setStep(byHand, Hour / 2);
    for (int k = 0; k < 2; ++k) {
        SphereForcing forcing;
        fillClosures(by, base.grid, base.poisson, byHand, forcing);
        expect(stepSphereFlow(byHand, base.grid, base.stencil, base.poisson, forcing), "a hand step was not accepted", k);
    }
    expect(sameState(byHand, cleanFlow), "the window's steps are not fillClosures' with the refreshed eddies");

    SphereClosures retried = base.closures;
    SphereHour hour;
    hour.limit = 0.75 * one;
    SphereFlow flow = begin;
    expect(advanceClosedHour(hour, flow, base.grid, base.stencil, base.poisson, retried), "the retried window was not taken");
    expect(hour.refusals.size() == 1 && hour.refusals[0].reason == HourRefusal::Courant, "the first try was not refused", double(hour.refusals.size()));
    expect(sameState(flow, cleanFlow) && retried.eddy == refreshed && retried.since == Hour, "the retried window is not the clean run");

    SphereClosures refused = base.closures;
    SphereHour never;
    never.limit = 1e-3 * one;
    never.attempts = 1;
    SphereFlow kept = begin;
    expect(!advanceClosedHour(never, kept, base.grid, base.stencil, base.poisson, refused), "the window was not refused");
    expect(sameState(kept, begin) && refused.eddy == base.closures.eddy && refused.since == base.closures.since,
           "a refused window published its eddies");
    SphereHour again;
    again.steps = 2;
    expect(advanceClosedHour(again, kept, base.grid, base.stencil, base.poisson, refused) && sameState(kept, cleanFlow) &&
               refused.eddy == refreshed,
           "the call after a refusal is not the clean run");
}

// Growth not one finite rate ≥ 0 per band, eddies not one finite value per cell, a band of another
// grid, a cadence not finite or a clock below zero do not fit: fillClosures and advanceClosedHour
// throw before anything changes.
void checkEddyRefusals() {
    Run run;
    const SphereFlow begin = run.flow;
    const auto refused = [&](SphereClosures closures, const char* what) {
        expect(!closuresFit(closures, run.poisson), what);
        const SphereClosures before = closures;
        SphereForcing forcing;
        bool threw = false;
        try {
            fillClosures(closures, run.grid, run.poisson, run.flow, forcing);
        } catch (const std::invalid_argument&) { threw = true; }
        SphereHour hour;
        SphereFlow flow = begin;
        try {
            (void)advanceClosedHour(hour, flow, run.grid, run.stencil, run.poisson, closures);
            threw = false;
        } catch (const std::invalid_argument&) {}
        expect(threw && sameState(flow, begin) && closures.eddy == before.eddy && closures.since == before.since && hour.stiffest == 0, what);
    };
    const std::pair<const char*, std::function<void(SphereClosures&)>> cases[] = {
        {"a negative growth fits", [](SphereClosures& c) { c.growth[3] = -1e-6; }},
        {"a NaN growth fits", [](SphereClosures& c) { c.growth[3] = std::numeric_limits<double>::quiet_NaN(); }},
        {"a growth short of a band fits", [](SphereClosures& c) { c.growth.pop_back(); }},
        {"an infinite eddy fits", [](SphereClosures& c) { c.eddy[7] = std::numeric_limits<double>::infinity(); }},
        {"eddies short of a cell fit", [](SphereClosures& c) { c.eddy.pop_back(); }},
        {"a band of another grid fits", [](SphereClosures& c) { c.band = makeEddyBand(makeSphereGrid(3), SphereEddyWavelength, SphereEddyReach); }},
        {"a NaN cadence fits", [](SphereClosures& c) { c.eddyEvery = std::numeric_limits<double>::quiet_NaN(); }},
        {"a negative clock fits", [](SphereClosures& c) { c.since = -1; }},
    };
    for (const auto& [what, spoil] : cases) {
        SphereClosures c = run.closures;
        spoil(c);
        refused(c, what);
    }
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkEddies() {
    checkEnvelope();
    checkGrowth();
    checkCadence();
    checkRetry();
    checkEddyRefusals();
}
