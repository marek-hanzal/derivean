#include "Measure.hpp"

#include "measure/fn/Place.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

int usage() {
    std::fprintf(stderr,
                 "usage: terra measure <what> [--seed n] [--age years] [--x m --z m] [--at shore|coast|peak|landing] [--levels 123] [--tiles n] [--out directory]\n"
                 "  at a place : scale regularity seams levels budget site (all five)\n"
                 "  no place   : plane references conservation determinism history future performance earth fingerprint sphere\n");
    return 2;
}

} // namespace

// terra measure <what>: the numbers behind the acceptance criteria of the landscape.
int measureCommand(const std::string& what, const Arguments& arguments) {
    if (what == "plane") return measurePlanes(arguments);
    if (what == "references") return measureReferences(arguments);
    if (what == "conservation") return measureConservation(arguments);
    if (what == "determinism") return measureDeterminism(arguments);
    if (what == "history") return measureHistory(arguments);
    if (what == "future") return measureFuture(arguments);
    if (what == "performance") return measurePerformance(arguments);
    if (what == "earth") return measureEarth(arguments);
    if (what == "fingerprint") return measureFingerprint(arguments);
    if (what == "sphere") return measureSphere(arguments);
    const bool all = what == "site";
    if (!all && what != "scale" && what != "regularity" && what != "seams" && what != "levels" && what != "budget") return usage();

    Lap lap;
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    const std::string at = arguments.text("at", "");
    if (at == "shore" || at == "coast") focus = findShore(world.planet, focus);
    if (at == "coast") focus = coastNear(world, focus);
    if (at == "peak") focus = highestNear(world.planet, focus, arguments.number("reach", 500e3));
    if (at == "landing") focus = findLanding(world, focus);
    const std::string out = arguments.text("out", ".");
    const std::string levels = arguments.text("levels", "123");
    const int tiles = int(arguments.number("tiles", 3));
    const char* rain = std::getenv("DERIVEAN_RAIN");
    std::printf("measure %s  seed %llu  age %.3g yr  focus %.0f %.0f  sea level %.0f m  rain %s  planet %.1f s\n", what.c_str(),
                static_cast<unsigned long long>(arguments.seed()), world.options.age, focus.x, focus.z, world.planet.seaLevel,
                rain ? rain : "from the climate", lap());

    for (const char digit : levels) {
        if (digit < '1' || digit > '3') continue;
        const Level level = Level(digit - '0');
        const size_t before = world.atlas.finished();
        const Mosaic mosaic = gatherMosaic(world, level, focus, tiles);
        const std::string tag = "L" + std::string(1, digit);
        std::printf("\nlevel %d  %d x %d tiles  %.1f km across  %.0f m cells  built %zu tiles in %.1f s\n", int(level), tiles, tiles,
                    mosaic.side() * mosaic.cell / 1000, mosaic.cell, world.atlas.finished() - before, lap());
        if (all || what == "scale") printScale(mosaic, out, tag);
        if (all || what == "regularity") printRegularity(mosaic, out, tag);
        if (all || what == "seams") printSeams(mosaic);
        if ((all || what == "levels") && level != Level::Region) printLevels(world, mosaic);
        if (all || what == "budget") printBudget(world, mosaic);
        lap();
    }
    return 0;
}
