// Times what a game's start waits for, step by step in the order Game's constructor takes them:
// the planet's evolution, the landing (which builds region, local and site tiles), the search for
// dry ground where that lies under water (findSpawn), and the first state of the atmosphere. The
// ground tile under the landing is timed apart: the game builds it after the start, with the voxels.
// Each step runs alone here, so the sum is the start without the overlap Game has between the
// atmosphere and the landing.
#include "Bench.hpp"
#include "atmosphere/type/Atmosphere.hpp"
#include "game/type/Player.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>

namespace derivean {
namespace {

using Clock = std::chrono::steady_clock;

double since(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }

} // namespace

void timeStart(double age) {
    auto start = Clock::now();
    sim::World world(sim::WorldOptions{42, age, 0});
    std::printf("start  planet, backdrop and ocean %.1f s on %u workers\n", since(start), world.workers.threads());
    start = Clock::now();
    const sim::Vec2 landing = sim::findLanding(world, {0, 0});
    std::printf("start  landing %.1f s, %zu tiles finished\n", since(start), world.atlas.finished());
    start = Clock::now();
    const Position spawn = findSpawn(world, std::nullopt);
    std::printf("start  spawn %.1f s (landing again, then the dry search), %.0f m from the landing\n", since(start),
                std::hypot(spawn[0] - landing.x, spawn[2] - landing.z));
    start = Clock::now();
    const sim::SurfaceSample ground = sim::surveyAt(world, {spawn[0], spawn[2]}, sim::Level::Ground);
    std::printf("after  ground tile under the spawn %.1f s, %s\n", since(start), ground.ground > ground.water ? "dry" : "under water");
    start = Clock::now();
    sim::Workers air(2);
    sim::startAtmosphere(world.planet, sim::Calendar{age, 0}, air);
    std::printf("start  first atmosphere %.1f s on 2 workers\n", since(start));
}

} // namespace derivean
