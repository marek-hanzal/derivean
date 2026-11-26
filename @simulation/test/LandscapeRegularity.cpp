#include "LandscapeFixture.hpp"

#include "measure/fn/Orientation.hpp"
#include "measure/fn/Reference.hpp"
#include "measure/fn/Regularity.hpp"

using namespace derivean::sim;

// The regularity indicators on surfaces with a known answer. Their thresholds are only worth
// quoting while relief without a pattern stays below them and a built pattern stays above.
void testLandscapeRegularity() {
    for (const double gain : {0.5, 0.65}) {
        const Field<float> relief = fractalRelief(11, 512, 96, 400, gain);
        const CombReport comb = combOf(relief, 1.0);
        expectLandscape(comb.regularShare < 0.15, "fractal relief reads as repeating gullies", comb.regularShare);
        expectLandscape(comb.gridShare == 0 && comb.notchedShare < 0.02, "fractal relief reads as a grid-scale comb", comb.gridShare + comb.notchedShare);
        expectLandscape(comb.roughness.p95 < 0.2, "fractal relief reads as rough from cell to cell", comb.roughness.p95);
        expectLandscape(sawtoothOf(relief, 1.0).regularShare < 0.1, "fractal relief reads as a saw-tooth skyline");
    }
    const CombReport wide = combOf(combRelief(11, 512, 12, 4, 0.4), 1.0);
    expectLandscape(wide.regularShare > 0.9 && wide.gridShare == 0, "gullies every 12 cells are not found as a pattern above the grid scale", wide.regularShare);
    expectLandscape(std::abs(wide.spacing - 12) < 1, "their spacing is misread", wide.spacing);
    const CombReport fine = combOf(combRelief(11, 512, 3, 4, 0.4), 1.0);
    expectLandscape(fine.gridShare > 0.5 && fine.notchedShare > 0.9, "gullies every 3 cells are not found as a grid-scale comb", fine.gridShare);
    expectLandscape(fine.roughness.p50 > 0.5, "a grid-scale comb does not read as rough from cell to cell", fine.roughness.p50);
    expectLandscape(sawtoothOf(sawRelief(11, 512, 20, 60, 3), 1.0).regularShare > 0.5, "equal ridges every 20 cells are not found as a saw-tooth skyline");

    // A coast with no preferred direction against the same coast drawn along the grid.
    const Field<float> relief = fractalRelief(11, 512, 96, 400, 0.5);
    Field<uint8_t> organic(512, 512), blocky(512, 512);
    for (int z = 0; z < 512; ++z)
        for (int x = 0; x < 512; ++x) organic.at(x, z) = relief.at(x, z) > 0, blocky.at(x, z) = relief.at(x & ~7, z & ~7) > 0;
    expectLandscape(straightShoreShare(organic, false) < StraightShoreLimit, "a fractal coast reads as drawn along the grid", straightShoreShare(organic, false));
    expectLandscape(straightShoreShare(blocky, false) > 0.8, "a coast of blocks does not read as drawn along the grid", straightShoreShare(blocky, false));

    // On a cone every heading is as common as any other. Paths that keep the fall line fill all
    // sixteenths of the half circle alike; a drawn choice that leans to one side empties every other one.
    const std::vector<float> headings = valleyAzimuths(coneMosaic(11, 512, false), 0.5);
    double sectors[8]{};
    for (const float heading : headings) sectors[size_t(std::clamp(int((heading - 180.0 * std::floor(heading / 180.0)) / 22.5), 0, 7))] += 1.0 / double(headings.size());
    expectLandscape(*std::min_element(sectors, sectors + 8) > 0.6 / 8 && *std::max_element(sectors, sectors + 8) < 1.4 / 8, "the reference cone favours some headings",
                    *std::max_element(sectors, sectors + 8) * 8);
    const Rose free = roseOf(headings);
    const Rose locked = roseOf(valleyAzimuths(coneMosaic(11, 512, true), 0.5));
    expectLandscape(std::abs(free.gridExcess - 1) < 0.12, "paths that keep the fall line read as following the grid", free.gridExcess);
    expectLandscape(locked.gridExcess > 2, "paths locked to eight directions do not read as following the grid", locked.gridExcess);
    expectLandscape(free.gridExcess < GridExcessLimit && locked.gridExcess > GridExcessLimit, "the grid-alignment limit does not separate the two");
}
