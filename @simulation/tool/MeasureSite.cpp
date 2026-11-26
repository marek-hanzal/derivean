#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "measure/fn/Orientation.hpp"
#include "measure/fn/Plot.hpp"
#include "measure/fn/Regularity.hpp"
#include "measure/fn/Scale.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

void printHeights(const char* name, std::vector<float> values) {
    // Classes widen with height so one line serves a beach and a mountain belt.
    constexpr float Edges[] = {10, 50, 200, 500, 1000, 2000, 4000};
    size_t counts[8]{};
    for (const float value : values) {
        size_t bin = 0;
        while (bin < 7 && value >= Edges[bin]) ++bin;
        ++counts[bin];
    }
    const double total = double(std::max<size_t>(1, values.size()));
    const Distribution d = distributionOf(std::move(values));
    std::printf("  %-11s m: p05 %.0f  p25 %.0f  p50 %.0f  p75 %.0f  p95 %.0f  p99 %.0f  max %.0f  mean %.0f\n", name, d.p05, d.p25, d.p50, d.p75, d.p95,
                d.p99, d.high, d.mean);
    std::printf("    share %%: <10 m %.0f | 10-50 %.0f | 50-200 %.0f | 200-500 %.0f | 500-1000 %.0f | 1-2 km %.0f | 2-4 km %.0f | >4 km %.0f\n",
                100 * counts[0] / total, 100 * counts[1] / total, 100 * counts[2] / total, 100 * counts[3] / total, 100 * counts[4] / total,
                100 * counts[5] / total, 100 * counts[6] / total, 100 * counts[7] / total);
}

} // namespace

void printScale(const Mosaic& mosaic, const std::string& out, const std::string& tag) {
    const LandHeights heights = landHeights(mosaic, riverThreshold(mosaic.level));
    std::printf("scale: dry %.1f %%  lake %.2f %%  sea %.1f %%\n", 100 * heights.dryShare, 100 * heights.lakeShare, 100 * heights.seaShare);
    if (!heights.aboveSea.empty()) {
        const Distribution sea = distributionOf(heights.aboveSea);
        writePng(out + "/heights-" + tag + ".png", 480, 160, barPlot(histogramOf(heights.aboveSea, 0, std::max(1.0, sea.high / 48), 48), 480, 160));
        printHeights("above sea", heights.aboveSea);
        printHeights("above water", heights.aboveWater);
        std::printf("    %.0f %% of dry cells drain to water inside the mosaic (sea, lake or a river of %.0e m3/yr)\n", 100 * heights.traced,
                    riverThreshold(mosaic.level));
        // Only plateaux stand this high on Earth: land that is high everywhere has no base level to drain to.
        if (sea.p50 > 1500) std::printf("    FLAG: half of the dry land stands above 1500 m\n");
    }
    const double lengths[3][3] = {{16e3, 64e3, 0}, {1e3, 4e3, 16e3}, {250, 1e3, 0}};
    for (const double length : lengths[int(mosaic.level) - 1]) {
        if (length <= 0) continue;
        const TransectStats lines = transectsOf(mosaic, length);
        if (lines.relief.count == 0) continue;
        const ReliefBand earth = earthRelief(length);
        std::printf("  lines of %5.2f km (%zu): relief p50 %.0f (%s)  p95 %.0f (%s)  max %.0f (%s) | climb p50 %.0f  p95 %.0f  max %.0f m"
                    " | Earth: plain <%.0f, hills <%.0f, mountains <%.0f\n",
                    length / 1000, lines.relief.count, lines.relief.p50, reliefClass(lines.relief.p50, length), lines.relief.p95,
                    reliefClass(lines.relief.p95, length), lines.relief.high, reliefClass(lines.relief.high, length), lines.climb.p50,
                    lines.climb.p95, lines.climb.high, earth.plain, earth.hills, earth.mountains);
        // Total climb over relief: 1 for one long slope, large where the line crosses gully after gully.
        std::printf("    climb over relief %.1f  mean slope along the line %.0f %%\n", lines.climb.p50 / std::max(1.0, lines.relief.p50), 200 * lines.climb.p50 / length);
        if (lines.relief.high > earth.mountains)
            std::printf("    FLAG: relief of %.0f m over %.2f km exceeds anything on Earth; the line starts at %.0f %.0f\n", lines.relief.high, length / 1000,
                        lines.steepest.x, lines.steepest.z);
    }
}

void printRegularity(const Mosaic& mosaic, const std::string& out, const std::string& tag) {
    const double river = riverThreshold(mosaic.level);
    // Long chords resolve the heading best but need long unbroken reaches; where lakes and tile
    // edges leave too few of them the chord is halved, at the cost of a reading near 1.1 for an isotropic network.
    int chord = 32;
    std::vector<float> azimuths = valleyAzimuths(mosaic, river, chord);
    if (azimuths.size() < 1000) azimuths = valleyAzimuths(mosaic, river, chord = 16);
    const Rose rose = roseOf(azimuths);
    // Three standard deviations above an isotropic network of as many independent segments, on either pair of directions or on both.
    const double sigma = std::max({excessSigma(rose.gridExcess, rose.segments, chord, 1.0 / 3), excessSigma(rose.axisExcess, rose.segments, chord, 1.0 / 6),
                                   excessSigma(rose.diagonalExcess, rose.segments, chord, 1.0 / 6)});
    const bool aligned = sigma > 3 && std::max({rose.gridExcess, rose.axisExcess, rose.diagonalExcess}) > GridExcessLimit;
    std::printf("regularity: valley segments %d (>= %.0e m3/yr, %d cells long)  grid excess %.2f (axes %.2f, diagonals %.2f; 1 isotropic, flag > %.1f at 3 sigma)  %.1f sigma%s\n",
                rose.segments, river, chord, rose.gridExcess, rose.axisExcess, rose.diagonalExcess, GridExcessLimit, sigma, aligned ? "  FLAG" : "");
    if (rose.segments > 0) writePng(out + "/rose-" + tag + ".png", 256, 256, rosePlot(rose, 256));

    Field<uint8_t> land(mosaic.side(), mosaic.side());
    for (int z = 0; z < mosaic.side(); ++z)
        for (int x = 0; x < mosaic.side(); ++x) land.at(x, z) = mosaic.dry(x, z);
    const double straight = straightShoreShare(land, false);
    std::printf("  shore cells in runs of 5 or more along a grid axis: %.1f %% (flag above %.0f %%)%s\n", 100 * straight, 100 * StraightShoreLimit,
                straight > StraightShoreLimit ? "  FLAG" : "");
    const CombReport comb = combOf(mosaic.ground, mosaic.cell, &mosaic.water);
    std::printf("  gullies along contours: %d profiles  strength p50 %.2f p95 %.2f  repeating %.1f %%  at grid scale %.1f %%  spacing %.1f cells  depth %.2f m%s\n",
                comb.profiles, comb.strength.p50, comb.strength.p95, 100 * comb.regularShare, 100 * comb.gridShare, comb.spacing, comb.depth,
                comb.gridShare > 0.02 ? "  FLAG" : "");
    std::printf("    cell roughness p50 %.3f p95 %.3f  profiles notched every <= %.0f cells %.1f %%  (one notch per %.1f cells overall)%s\n", comb.roughness.p50,
                comb.roughness.p95, GridSpacing, 100 * comb.notchedShare, comb.notchSpacing, comb.notchedShare > 0.05 ? "  FLAG" : "");
    const SawtoothReport saw = sawtoothOf(mosaic.ground, mosaic.cell, &mosaic.water);
    std::printf("  crests along lines: %d lines  spacing scatter p05 %.2f p50 %.2f  saw-tooth %.1f %%  spacing %.1f cells  height %.2f m%s\n", saw.lines,
                saw.spacingCv.p05, saw.spacingCv.p50, 100 * saw.regularShare, saw.spacing, saw.height, saw.regularShare > 0.05 ? "  FLAG" : "");
}
