#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Orientation.hpp"
#include "measure/fn/Plane.hpp"
#include "measure/fn/Reference.hpp"
#include "measure/fn/Regularity.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

// Ground against the starting surface over the tile's own cells: white stands, black has cut deepest.
void savePlane(const std::string& path, const Domain& domain) {
    std::vector<float> residual(size_t(TileCells) * TileCells);
    float low = 0, high = 0;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = domain.index(x + Halo, z + Halo);
            const float value = domain.ground.rock[i] + domain.sedimentAt(i) - domain.trend[i];
            residual[size_t(z) * TileCells + size_t(x)] = value;
            low = std::min(low, value), high = std::max(high, value);
        }
    std::vector<uint8_t> image(residual.size() * 3);
    for (size_t i = 0; i < residual.size(); ++i) {
        const uint8_t shade = uint8_t(255.0f * (residual[i] - low) / std::max(1e-6f, high - low));
        image[i * 3] = image[i * 3 + 1] = image[i * 3 + 2] = shade;
    }
    writePng(path, TileCells, TileCells, image);
}

void printRegularity(const char* name, const Field<float>& surface) {
    const CombReport comb = combOf(surface, 1.0);
    const SawtoothReport saw = sawtoothOf(surface, 1.0);
    std::printf("  %-34s gullies: strength p50 %.2f p95 %.2f  repeating %5.1f %%  grid scale %5.1f %%  spacing %4.1f  roughness p50 %.3f p95 %.3f  notched %5.1f %%"
                " | crests: scatter p05 %.2f p50 %.2f  saw-tooth %5.1f %%\n",
                name, comb.strength.p50, comb.strength.p95, 100 * comb.regularShare, 100 * comb.gridShare, comb.spacing, comb.roughness.p50,
                comb.roughness.p95, 100 * comb.notchedShare, saw.spacingCv.p05, saw.spacingCv.p50, 100 * saw.regularShare);
}

} // namespace

// terra measure plane [--levels 23] [--slope s] [--rain m/yr] [--roughness m] [--steps n]: the
// solver on a featureless slope at three headings, and on the same slope with a swale.
int measurePlanes(const Arguments& arguments) {
    const std::string out = arguments.text("out", ".");
    std::printf("plane: gullies are crest-to-trough over 16 cells along contours; a sheet of water reads foot share 0.10, gathered 0, axis share 0.07;"
                " heading is of water at twice its contour mean\n");
    for (const char digit : arguments.text("levels", "23")) {
        if (digit < '1' || digit > '3') continue;
        PlaneSpec spec;
        spec.level = Level(digit - '0');
        spec.slope = arguments.number("slope", 0.03);
        spec.rain = arguments.number("rain", 0.9);
        spec.roughness = arguments.number("roughness", 0.03);
        spec.steps = int(arguments.number("steps", 0));
        spec.seed = arguments.seed();
        const double cell = specOf(spec.level).cell;
        // Deep enough that the ground falls towards the middle line half as steeply as down the plane.
        const double swale = arguments.number("swale", 0.5 * spec.slope * cell * DomainCells / 2);
        for (const double degrees : {0.0, 22.5, 45.0}) {
            for (const int shape : {0, 2, 1}) {
                const double trough = shape ? swale : 0.0;
                spec.azimuth = degrees / DegreesPerRadian;
                spec.swale = trough;
                spec.swalePower = shape ? shape : 2;
                Lap lap;
                const Domain domain = replayPlane(spec);
                const PlaneReport report = measurePlane(spec, domain);
                const char* form = shape == 0 ? "plane" : shape == 2 ? "swale" : "crease";
                std::printf("level %c  %4.1f deg  %-6s %5.1f m  fall %4.0f m  lowered %6.2f m  %.1f s\n", digit, degrees, form, trough, report.fall, report.lowered, lap());
                std::printf("  gullies p50 %.2f p95 %.2f max %.2f m  a notch every %.1f cells  roughness p50 %.2f | contour lines: strength p50 %.2f p95 %.2f  repeating %.0f %%  grid scale %.0f %%\n",
                            report.gully.p50, report.gully.p95, report.gully.high, report.notchSpacing, report.roughness.p50, report.strength.p50,
                            report.strength.p95, 100 * report.regularShare, 100 * report.gridShare);
                std::printf("  mean cross profile: range %.2f m  strength %.2f at %d cells  %d troughs %.1f cells apart (scatter %.2f) %.2f m deep\n",
                            report.crossRange, report.crossPattern.strength, report.crossPattern.lag, report.crossPattern.marks, report.crossPattern.spacing,
                            report.crossPattern.spacingCv, report.crossPattern.height);
                std::printf("  water: foot share %.2f  %.2f m lower per doubling (r %.2f)  heading error %.1f deg over %d paths | gathered %.2f  axis share %.2f  axis %.2f m below flanks\n",
                            report.footShare, report.incisionPerDoubling, report.correlation, report.headingError, report.paths, report.gathered,
                            report.axisShare, report.axisDepth);
                char name[96];
                std::snprintf(name, sizeof(name), "/plane-L%c-%02d-%s.png", digit, int(degrees), form);
                savePlane(out + name, domain);
            }
        }
    }
    return 0;
}

// terra measure references: what the regularity indicators read on surfaces with a known answer.
int measureReferences(const Arguments& arguments) {
    const uint64_t seed = arguments.seed();
    std::printf("references: thresholds are strength >= %.2f with spacing scatter <= %.2f (repeating), spacing <= %.0f cells (grid scale), crest scatter <= %.2f (saw-tooth)\n",
                RepeatStrength, RegularSpacing, GridSpacing, SawSpacing);
    printRegularity("fractal relief, gain 0.5", fractalRelief(seed, 512, 96, 400, 0.5));
    printRegularity("fractal relief, gain 0.65", fractalRelief(seed + 1, 512, 96, 400, 0.65));
    printRegularity("fractal relief, short waves", fractalRelief(seed + 2, 512, 24, 100, 0.5));
    printRegularity("gullies every 12 cells, 10 % noise", combRelief(seed, 512, 12, 4, 0.4));
    printRegularity("gullies every 3 cells, 10 % noise", combRelief(seed, 512, 3, 4, 0.4));
    printRegularity("gullies every 12 cells, 50 % noise", combRelief(seed, 512, 12, 4, 2.0));
    printRegularity("ridges every 20 cells, 5 % noise", sawRelief(seed, 512, 20, 60, 3));
    // Coasts: land where seeded relief stands above zero, and the same land drawn in blocks of eight cells.
    const Field<float> relief = fractalRelief(seed, 512, 96, 400, 0.5);
    Field<uint8_t> organic(512, 512), blocky(512, 512);
    for (int z = 0; z < 512; ++z)
        for (int x = 0; x < 512; ++x) organic.at(x, z) = relief.at(x, z) > 0, blocky.at(x, z) = relief.at(x & ~7, z & ~7) > 0;
    std::printf("  shore cells in runs of 5 or more along a grid axis: fractal coast %.1f %%, the same coast in 8-cell blocks %.1f %% (flag above %.0f %%)\n",
                100 * straightShoreShare(organic, false), 100 * straightShoreShare(blocky, false), 100 * StraightShoreLimit);
    for (const bool locked : {false, true}) {
        const Rose rose = roseOf(valleyAzimuths(coneMosaic(seed, 512, locked), 0.5));
        std::printf("  cone, %-28s valley segments %d  grid excess %.2f (axes %.2f, diagonals %.2f)\n",
                    locked ? "steepest neighbour only:" : "fall line kept on average:", rose.segments, rose.gridExcess, rose.axisExcess, rose.diagonalExcess);
    }
    return 0;
}
