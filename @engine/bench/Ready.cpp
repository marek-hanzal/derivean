// Watches the readiness signals a capture waits for while the player stands still, and says what
// keeps each of them false: which tiles are owed at which level, which cube levels are short of
// their data, and whether the far view waits for tiles it has not looked at yet.
#include "Bench.hpp"
#include "simulation/fn/Query.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <thread>

namespace derivean {
namespace {

using Clock = std::chrono::steady_clock;

double since(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }

void line(double time, const Game& game, const Frame& frame) {
    std::printf("%6.1f s  ready site %d local %d region %d far %d streaming %d  tiles", time, frame.siteReady, frame.localReady, frame.regionReady,
                frame.farReady, frame.streamingReady);
    for (size_t level = 1; level < sim::LevelCount; ++level) std::printf(" %u/%u", frame.tilesFinished[level], frame.tilesWanted[level]);   // region, local, site, ground
    std::array<int, sim::LevelCount> flying{};
    int water = 0;
    for (const TileRequest& request : game.nature.tiles.flying) request.water ? ++water : ++flying[size_t(request.key.level)];
    std::printf("  flying %zu (%d %d %d %d water %d)  pending chunks %zu cubes %zu", game.nature.tiles.flying.size(), flying[1], flying[2], flying[3],
                flying[4], water, frame.pendingChunks, frame.pendingCubes);
    std::printf("  audit %zu seen %zu of %zu |", game.cubes.audit.size(), game.cubes.tilesSeen, game.nature.world.atlas.finished());
    for (const CubeLevelView& level : frame.cubeLevels) {
        if (level.wanted == level.settled) continue;
        // Over every column the level keeps, not only the wanted ones: what holds the unsettled back.
        size_t missing = 0, coarse = 0, stale = 0, building = 0;
        for (const auto& [chunk, slot] : game.cubes.levels[size_t(level.level - FinestCubes)].slots) {
            missing += slot.shown.empty();
            coarse += !slot.shown.empty() && !cubeFinal(level.level, slot.detail);
            stale += slot.stale;
            building += slot.ticket || slot.built;
        }
        std::printf(" L%d settled %zu held %zu of %zu (kept: missing %zu coarse %zu stale %zu building %zu)", level.level, level.settled, level.shown,
                    level.wanted, missing, coarse, stale, building);
    }
    std::printf("\n");
}

} // namespace

void watchReadiness(Game& game, double duration, double every) {
    const auto start = Clock::now();
    auto next = start;
    double printed = -every;
    std::array<bool, 4> last{};
    for (double time = 0; time < duration; time = since(start)) {
        Command command;
        command.elapsed = 1.0 / 60.0;
        command.active = true;
        command.direction = {1, 0, 0};
        const Frame frame = advance(game, command);
        const std::array<bool, 4> now{frame.siteReady && frame.localReady && frame.regionReady, frame.farReady,
                                      frame.pendingChunks == 0 && frame.pendingCubes == 0, frame.streamingReady};
        if (time - printed >= every || now != last) {
            line(time, game, frame);
            printed = time;
            last = now;
        }
        next += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / 60.0));
        std::this_thread::sleep_until(next);
    }
}

void watchPlace(double age, double x, double z, double height, double duration, bool jump) {
    const auto start = Clock::now();
    Game game(PlanetOptions{42, age, jump ? std::nullopt : std::optional{std::array{x, z}}, 0});
    Command lift;
    lift.elapsed = 1.0 / 60.0;
    lift.active = true;
    lift.fly = true;
    lift.direction = {1, 0, 0};
    // A jump must not build the place before it arrives: its height is over what is known there now.
    const double ground = jump ? sim::surfaceAt(game.nature.world, {x, z}).ground : sim::surveyAt(game.nature.world, {x, z}).ground;
    lift.teleport = Position{x, ground + height, z};
    advance(game, lift);
    std::printf("seed 42 age %g %s x %.0f z %.0f, %.0f m above the ground, flying %d, after %.1f s\n", age, jump ? "jumped to" : "at", x, z, height,
                int(game.flying), since(start));
    watchReadiness(game, duration, 5);
}

} // namespace derivean
