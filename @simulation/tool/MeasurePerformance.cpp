#include "Measure.hpp"

#include "measure/fn/Dependencies.hpp"
#include "measure/fn/Levels.hpp"
#include "measure/fn/Place.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>
#include <sys/resource.h>

using namespace derivean::sim;

namespace {

// Peak resident memory of the process in megabytes.
double peakMegabytes() {
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    return double(usage.ru_maxrss) / (1024.0 * 1024.0);
#else
    return double(usage.ru_maxrss) / 1024.0;
#endif
}

double megabytesOf(const Tile& tile) {
    size_t bytes = sizeof(Tile) + tile.flowTo.capacity() + tile.rock.capacity();
    for (const Ground* ground : {&tile.now, &tile.nowWide}) {
        bytes += (ground->rock.capacity() + ground->datum.capacity()) * sizeof(float);
        for (const auto& layer : ground->sediment) bytes += layer.capacity() * sizeof(float);
    }
    for (const auto* field : {&tile.water, &tile.discharge, &tile.load, &tile.uplift, &tile.rebound, &tile.ice, &tile.iceAge, &tile.erosionRate,
                              &tile.depositionRate, &tile.eroded, &tile.deposited, &tile.uplifted, &tile.rain, &tile.warmth})
        bytes += field->capacity() * sizeof(float);
    return double(bytes) / (1024.0 * 1024.0);
}

// One tile replayed on the calling thread, its start and its steps timed apart.
void timeTile(World& world, TileKey key) {
    const Dependencies needed = requireDependencies(world, key);
    Lap lap;
    Domain domain = buildDomain(world.atlas.setting(), key, needed.lookup());
    const double build = lap();
    FlowScratch scratch;
    const Window window = windowAt(key.level, world.options.age);
    for (int step = 0; step < window.steps; ++step) stepDomain(domain, scratch, world.atlas.setting());
    const double steps = lap();
    std::printf("  level %d tile on one thread: start %.2f s  %d steps %.2f s (%.1f ms each)  waits for %zu tiles\n", int(key.level), build, window.steps,
                steps, window.steps ? 1000 * steps / window.steps : 0.0, needed.keys.size());
}

} // namespace

// terra measure performance [--x --z | --at shore|coast] [--block 4]
int measurePerformance(const Arguments& arguments) {
    const uint64_t seed = arguments.seed();
    std::printf("performance  seed %llu\n", static_cast<unsigned long long>(seed));
    {
        Workers workers;
        for (const double age : {3e8, 1.2e9, 4.6e9}) {
            Lap lap;
            const Planet planet = evolvePlanet(seed, age, workers);
            std::printf("  planet to %.1f Gyr: %.2f s on %u threads\n", planet.time / 1e9, lap(), workers.threads() + 1);
        }
    }
    Lap lap;
    World world({seed, arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    const double planetSeconds = lap();
    Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    const std::string at = arguments.text("at", "");
    if (at == "shore" || at == "coast") focus = findShore(world.planet, focus);
    if (at == "coast") {
        // The search builds Region tiles, so the first-tile time is taken in a second, untouched world.
        World scratch({seed, world.options.age, 0});
        focus = coastNear(scratch, focus);
    }
    lap();
    surveyAt(world, focus);
    const double first = lap();
    std::printf("  first site tile at %.0f %.0f in a fresh world: %.1f s after the planet's %.1f s, %zu tiles built, %u threads\n", focus.x, focus.z, first,
                planetSeconds, world.atlas.finished(), world.workers.threads());

    for (int level = 1; level <= 3; ++level) timeTile(world, tileAt(Level(level), focus));

    const int block = int(arguments.number("block", 4));
    for (int level = 1; level <= 3; ++level) {
        // Parents first, so the rate is that of the level alone.
        const TileKey middle = tileAt(Level(level), focus);
        if (level > 1) {
            Mosaic outline;
            outline.level = Level(level), outline.first = {Level(level), middle.x - block / 2 - 1, middle.z - block / 2 - 1}, outline.tiles = block + 2;
            gatherParents(world, outline);
        }
        const size_t before = world.atlas.finished();
        const double memory = peakMegabytes();
        lap();
        gatherMosaic(world, {Level(level), middle.x - block / 2, middle.z - block / 2}, block);
        const double seconds = lap();
        const size_t built = world.atlas.finished() - before;
        std::printf("  level %d  %d x %d block: %zu tiles in %.1f s = %.2f tiles/s on %u threads  %.1f MB kept per tile, peak memory +%.0f MB\n", level, block,
                    block, built, seconds, built / std::max(seconds, 1e-9), world.workers.threads(), megabytesOf(*world.atlas.find(middle)),
                    peakMegabytes() - memory);
    }
    std::printf("  peak resident memory %.0f MB for %zu tiles\n", peakMegabytes(), world.atlas.finished());
    return 0;
}
