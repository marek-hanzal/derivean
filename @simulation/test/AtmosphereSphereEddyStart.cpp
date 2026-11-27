// The eddies of a flow's start (startEddies, Codex DREV-CLAUDE-010-03 point 3).
#include "AtmosphereSphereFixture.hpp"

#include "astronomy/constant/DeepTime.hpp"
#include "atmosphere/fn/EddyBand.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereHour.hpp"
#include "atmosphere/fn/SpherePoisson.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <stdexcept>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

// At level 4, from a wave with a part in the band: startEddies publishes the band of the start's ζ
// bit for bit with `since` zero, and the first window already grows them: its forcing is
// fillClosures' with those eddies (the hand-stepped window bit for bit), not the zero eddies of fresh
// closures. An eddy band or bands of another grid are refused with the closures as they were.
void derivean::sim::atmosphere_sphere_test::checkEddyStart() {
    const SphereGrid grid = makeSphereGrid(4);
    const SphereStencil stencil = makeSphereStencil(grid);
    SpherePoisson poisson = makeSpherePoisson(grid, stencil);
    SphereFlow flow = makeSphereFlow(grid, rotationRateAt(PresentAge));
    std::vector<double> start = rossbyHaurwitz(grid, flow.rotation, 0), wave = harmonicField(grid, 12, 5);
    for (size_t c = 0; c < start.size(); ++c) start[c] += 1e-5 * wave[c];
    expect(setVorticity(flow, grid, stencil, poisson, start), "the start did not invert");
    SphereClosures closures = makeSphereClosures(grid, poisson, {}, grid.side);
    closures.growth.assign(size_t(grid.side), 1 / (1.5 * 86400));
    closures.since = 3600;
    std::vector<double> band;
    EddyBand filter = closures.band;
    filterEddyBand(filter, flow.vorticity, band);
    startEddies(closures, flow);
    expect(closures.eddy == band && closures.since == 0, "the start's eddies are not the band of its ζ");

    SphereClosures by = closures, fresh = makeSphereClosures(grid, poisson, {}, grid.side);
    fresh.growth = closures.growth;
    SphereFlow byHand = flow, unfed = flow;
    SphereHour hour, other;
    expect(advanceClosedHour(hour, flow, grid, stencil, poisson, closures) && advanceClosedHour(other, unfed, grid, stencil, poisson, fresh),
           "a first window was not taken");
    setStep(byHand, flow.step);
    for (int k = 0; k < int(3600 / flow.step + 0.5); ++k) {
        SphereForcing forcing;
        fillClosures(by, grid, poisson, byHand, forcing);
        expect(stepSphereFlow(byHand, grid, stencil, poisson, forcing), "a hand step was not accepted", k);
    }
    expect(sameState(flow, byHand) && !sameState(flow, unfed), "the first window did not grow the start's eddies");

    const SphereGrid coarse = makeSphereGrid(3);
    SphereClosures wrong = closures, alsoWrong = closures;
    wrong.band = makeEddyBand(coarse, SphereEddyWavelength, SphereEddyReach);
    alsoWrong.bands = makeLatitudeBands(coarse, coarse.side);
    for (SphereClosures* bad : {&wrong, &alsoWrong}) {
        bad->since = 3600;
        bool threw = false;
        try {
            startEddies(*bad, flow);
        } catch (const std::invalid_argument&) { threw = true; }
        expect(threw && bad->eddy == closures.eddy && bad->since == 3600, "a band or bands of another grid started the eddies");
    }
}
