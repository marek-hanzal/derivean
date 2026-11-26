// Watches the readiness signals a capture waits for while the player stands still, and says what
// keeps each of them false: which tiles are owed at which level, which cube levels are short of
// their data, and whether the far view waits for tiles it has not looked at yet.
#include "Bench.hpp"

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
    std::printf("  flying %zu  pending chunks %zu cubes %zu", game.nature.tiles.flying.size(), frame.pendingChunks, frame.pendingPatches);
    if (game.cubes) {
        std::printf("  audit %zu seen %zu of %zu |", game.cubes->audit.size(), game.cubes->tilesSeen, game.nature.world.atlas.finished());
        for (const CubeLevelView& level : frame.cubeLevels)
            if (level.wanted != level.settled) std::printf(" L%d settled %zu held %zu of %zu", level.level, level.settled, level.shown, level.wanted);
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
                                      frame.pendingChunks == 0 && frame.pendingPatches == 0, frame.streamingReady};
        if (time - printed >= every || now != last) {
            line(time, game, frame);
            printed = time;
            last = now;
        }
        next += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / 60.0));
        std::this_thread::sleep_until(next);
    }
}

} // namespace derivean
