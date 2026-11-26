#include "Measure.hpp"

#include "measure/fn/Determinism.hpp"
#include "measure/fn/History.hpp"
#include "measure/fn/Place.hpp"
#include "measure/fn/Regularity.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

void printSummary(const PlanetSummary& s, double seconds) {
    std::printf("%5.2f Gyr  land %4.1f %%  mean %4.0f m  1-in-100 %4.0f m  highest %4.0f m  above 2 km %4.1f %% of land  deepest %5.0f m  crust thickest %4.1f km mean %5.2f km  sea %5.0f m  %.1f s\n",
                s.time / 1e9, 100 * s.landShare, s.meanLand, s.high99, s.highest, 100 * s.above2km, s.deepest, s.thickest / 1000, s.meanCrust / 1000,
                s.seaLevel, seconds);
    std::printf("           shore cells in runs of 5 or more along a grid axis: %.1f %% (flag above %.0f %%)%s\n", 100 * s.straightShore, 100 * StraightShoreLimit,
                s.straightShore > StraightShoreLimit ? "  FLAG" : "");
    std::printf("           km3: crust %.4g  cover %.3g | eroded %.3g  deposited %.3g  erupted %.3g  subducted %.3g  accreted %.3g  precipitated %.3g\n",
                s.crustVolume / 1e9, s.coverVolume / 1e9, s.ledger.eroded / 1e9, s.ledger.deposited / 1e9, s.ledger.erupted / 1e9,
                s.ledger.subducted / 1e9, s.ledger.accreted / 1e9, s.ledger.precipitated / 1e9);
}

} // namespace

// terra measure determinism [--x --z] [--at shore|peak|landing] [--finest 1|2|3] [--order-level 1|2|3] [--pass 1|3]
int measureDeterminism(const Arguments& arguments) {
    Lap lap;
    World first({arguments.seed(), arguments.number("age", 4.6e9), 0});
    Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    const std::string at = arguments.text("at", "");
    if (at == "shore") focus = findShore(first.planet, focus);
    if (at == "peak") focus = highestNear(first.planet, focus, arguments.number("reach", 500e3));
    if (at == "landing") focus = findLanding(first, focus);
    const DeterminismReport report = checkDeterminism(first, focus, Level(int(arguments.number("finest", 3))), Level(int(arguments.number("order-level", 1))),
                                                      int(arguments.number("pass", 3)), true);
    std::printf("determinism  seed %llu  age %.3g yr  focus %.0f %.0f  %.1f s\n", static_cast<unsigned long long>(first.options.seed), first.options.age, focus.x,
                focus.z, lap());
    std::printf("  first world: all threads, finest tile asked first; second world: one worker thread, every dependency asked by hand, last first\n");
    std::printf("  ordered tile: level %d pass %d next to the focus\n", int(arguments.number("order-level", 1)), int(arguments.number("pass", 3)));
    std::printf("  planet %016llx %s  (ledger eroded differs by %.2e relative)\n", static_cast<unsigned long long>(report.planetDigest),
                report.planetSame ? "identical" : "DIFFERENT", report.ledgerDrift);
    for (const DeterminismReport::Entry& entry : report.tiles) {
        std::printf("  L%d %5d %5d pass %d  ground %016llx water %016llx flow %016llx  ", int(entry.key.level), entry.key.x, entry.key.z, passOf(entry.key),
                    static_cast<unsigned long long>(entry.digest.ground), static_cast<unsigned long long>(entry.digest.water),
                    static_cast<unsigned long long>(entry.digest.discharge));
        if (entry.difference.cells == 0) std::printf("identical\n");
        else std::printf("DIFFERENT in %zu cells: ground %.3g m  water %.3g m  flow %.3g relative\n", entry.difference.cells, entry.difference.ground,
                         entry.difference.water, entry.difference.discharge);
    }
    std::printf("  finest tile replayed again in the same world: %zu cells differ\n", report.replayed.cells);
    std::printf("  sampleBlock of %zu voxels across a ground-tile edge: %zu differ in the second world", report.blockCells, report.blockFresh);
    if (report.blockRevisitChecked) std::printf(", %zu changed after the ground tiles around were built", report.blockRevisited);
    std::printf("\n");
    if (report.surveyEdgeShift < 0) std::printf("  site-level surveyAt at a site tile's edge: not measured, the tile across it existed already\n");
    else std::printf("  site-level surveyAt 0.5 m inside a site tile's edge moved by %.3f m once the tile across it existed%s\n", report.surveyEdgeShift,
                     report.surveyEdgeShift > 0 ? "  (the surveyed surface depends on what was built first)" : "");
    std::printf("  %s\n", report.identical() ? "bit-identical" : "NOT deterministic");
    return report.identical() ? 0 : 1;
}

// terra measure future [--ages 4.6e9,8e9,12e9]: the stock of rock is finite, so nothing may keep growing.
int measureFuture(const Arguments& arguments) {
    Workers workers(unsigned(arguments.number("threads", 0)));
    std::string ages = arguments.text("ages", "1.2e9,4.6e9,8e9,12e9");
    std::printf("future  seed %llu\n", static_cast<unsigned long long>(arguments.seed()));
    for (size_t start = 0; start < ages.size();) {
        const size_t end = std::min(ages.find(',', start), ages.size());
        const double age = std::strtod(ages.substr(start, end - start).c_str(), nullptr);
        Lap lap;
        const Planet planet = evolvePlanet(arguments.seed(), age, workers);
        printSummary(summarize(planet), lap());
        start = end + 1;
    }
    return 0;
}
