#include "Arguments.hpp"

#include "simulation/fn/Query.hpp"
#include "simulation/fn/Reader.hpp"

#include <cstdio>

using namespace derivean::sim;

// terra probe --x metres --z metres [--span metres]: the same place as each level describes it.
int probeCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const Vec2 at{arguments.number("x", 0), arguments.number("z", 0)};
    const double span = arguments.number("span", 0);
    for (int level = 1; level <= 3; ++level) {
        world.atlas.require(tileAt(Level(level), at));
        TileReader reader{world.atlas, Level(level)};
        SurfaceSample sample;
        readSurface(reader, at, sample);
        std::printf("level %d  ground %.1f  rock %.1f  cover %.2f/%.2f/%.2f  water %.1f  flow %.3g m3/yr  rain %.2f  warmth %.1f  rock %s\n",
                    level, sample.ground, sample.rock, sample.sediment[0], sample.sediment[1], sample.sediment[2], sample.water,
                    sample.discharge, sample.rain, sample.warmth, std::string(definitionOf(sample.bedrock).name).c_str());
        if (arguments.number("profile", 0) > 0) {
            // West-east profile through the point: where the ground, the rock and the layering datum lie.
            const double reach = arguments.number("profile", 0), stride = reach / 12;
            for (double x = at.x - reach / 2; x <= at.x + reach / 2 + 1e-6; x += stride) {
                SurfaceSample here;
                if (readSurface(reader, {x, at.z}, here)) std::printf("         x %+7.0f  ground %8.1f  rock %8.1f  datum %8.1f  cut %7.1f\n", x - at.x, here.ground, here.rock, here.datum, here.datum - here.rock);
            }
        }
        if (span <= 0) continue;
        double sum = 0, low = 1e9, high = -1e9;
        int count = 0;
        const double stride = std::max(specOf(Level(level)).cell, span / 64);
        for (double z = at.z - span / 2; z < at.z + span / 2; z += stride)
            for (double x = at.x - span / 2; x < at.x + span / 2; x += stride) {
                SurfaceSample here;
                if (!readSurface(reader, {x, z}, here)) continue;
                sum += here.ground, low = std::min(low, double(here.ground)), high = std::max(high, double(here.ground));
                ++count;
            }
        if (count) std::printf("         over %.0f m: mean %.1f  range %.1f .. %.1f  (%d samples)\n", span, sum / count, low, high, count);
    }
    return 0;
}
