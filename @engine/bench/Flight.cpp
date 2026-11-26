// Measures what streaming a planet costs the game thread: a walk along the ground, then a minute of
// fast flight into ground that was never loaded, both paced at 60 frames a second of real time so
// that tile builds, voxel columns and far cubes compete with the frames the way they do in play.
#include "Bench.hpp"
#include "game/type/Game.hpp"
#include "nature/type/TileRequests.hpp"
#include "simulation/fn/Query.hpp"
#include "threads/type/Threads.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

using namespace derivean;
using Clock = std::chrono::steady_clock;

namespace {

constexpr double FrameSeconds = 1.0 / 60.0;

struct Series {
    std::vector<double> step, streaming;
};

double seconds(Clock::time_point since) { return std::chrono::duration<double>(Clock::now() - since).count(); }

void report(const char* phase, const char* name, std::vector<double> values) {
    std::sort(values.begin(), values.end());
    double sum = 0;
    for (double value : values) sum += value;
    const auto at = [&](double share) { return values[std::min(values.size() - 1, size_t(share * double(values.size())))]; };
    std::printf("%-6s %-22s mean %6.3f  p95 %6.3f  p99 %6.3f  max %6.3f ms  (%zu frames)\n", phase, name, sum / double(values.size()),
                at(0.95), at(0.99), values.back(), values.size());
}

// Runs `script` for `duration` seconds of real time, or until it asks to stop.
template<typename Script>
Series play(Game& game, double duration, Script&& script) {
    Series series;
    const auto start = Clock::now();
    auto next = start;
    for (double time = 0; time < duration; time = seconds(start)) {
        Command command;
        command.elapsed = FrameSeconds;
        command.active = true;
        command.direction = {1, 0, 0};
        script(time, command);
        const Frame frame = advance(game, command);
        series.step.push_back(frame.nativeMs);
        series.streaming.push_back(frame.streamingMs);
        if (!script(time, frame)) break;
        next += std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(FrameSeconds));
        std::this_thread::sleep_until(next);
    }
    return series;
}

// Waits for the view to fill in and notes when each ring of it did: the first far surface, voxels
// to stand on, 64 m data over the local reach, 1 km data over the region reach.
struct Fill {
    Game& game;
    bool quick{};   // voxels to stand on are enough
    double surface{-1}, voxels{-1}, local{-1}, region{-1}, far{-1};
    size_t planned{}, samples{};

    void operator()(double, Command&) const {}
    bool operator()(double time, const Frame& frame) {
        if (surface < 0 && game.cubes.ids > 0) surface = time;
        if (far < 0 && surface >= 0 && frame.pendingCubes == 0) far = time;
        if (voxels < 0 && frame.streamingReady && frame.voxelBox && frame.pendingChunks == 0) voxels = time;
        if (local < 0 && frame.localReady) local = time;
        if (region < 0 && frame.regionReady) region = time;
        if (!frame.localReady || !frame.regionReady) {
            planned += game.nature.tiles.flying.size();
            ++samples;
        }
        return voxels < 0 || (!quick && (local < 0 || region < 0));
    }
};

void report(const char* phase, const Fill& fill, const Game& game) {
    std::printf("%-6s first far surface %.1f s, far view whole %.1f s, voxels to stand on %.1f s, %.0f km at 64 m data %.1f s, %.0f km at 1 km data %.1f s; "
                "%.1f active-plan requests while tiles were owed (%u simulation workers)\n",
                phase, fill.surface, fill.far, fill.voxels, game.nature.tiles.reach[size_t(sim::Level::Local)] / 1000, fill.local,
                game.nature.tiles.reach[size_t(sim::Level::Region)] / 1000, fill.region,
                double(fill.planned) / double(std::max<size_t>(1, fill.samples)), game.nature.world.workers.threads());
}

// On foot, with the calendar at an hour a second so that the atmosphere steps while the ground streams.
struct Walk {
    void operator()(double, Command& command) const {
        command.x = 1;
        command.timeRate = 3600;
    }
    bool operator()(double, const Frame&) const { return true; }
};

// Ten seconds skimming the ground at boost speed, then a climb to height and level flight until a
// hundred kilometres lie behind, then down to the ground there.
struct Flight {
    Position from;
    bool arrived{};
    double height{};

    void operator()(double time, Command& command) {
        command.fly = time == 0;
        command.x = 1;
        command.boost = !arrived || height > 300;
        if (time < 1) command.ascend = 1;
        else if (!arrived && time >= 10 && time < 14) command.direction = {0.7071, 0.7071, 0};
        else if (arrived) command.direction = {0.3, -0.954, 0};
    }
    bool operator()(double, const Frame& frame) {
        arrived |= frame.eye[0] - from[0] > 100e3;
        height = frame.ground ? frame.eye[1] - *frame.ground : 1e9;
        return !arrived || height > 30;
    }
};

// Waits on the ground until the voxels round the player are whole.
struct Arrive {
    double voxels{-1};
    void operator()(double, Command&) const {}
    bool operator()(double time, const Frame& frame) {
        if (frame.streamingReady && frame.voxelBox && frame.pendingChunks == 0) voxels = time;
        return voxels < 0;
    }
};

} // namespace

// engine-bench [age] start: what a game's start waits for, step by step
// engine-bench age at|jump x z height [seconds, 300]: the readiness signals in flight `height` m above the ground there, the
//   game started there (at) or at its own spawn and teleported (jump)
// engine-bench [age] [engine workers, 0 for the machine's] [plain|favoured: the frame thread] [quick: no waiting for the far rings | ready [seconds, 300]: watch the readiness signals standing still]
int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IOLBF, 0);
    const double age = argc > 1 ? std::atof(argv[1]) : 2e9;
    if (argc > 2 && !std::strcmp(argv[2], "start")) {
        timeStart(age);
        return 0;
    }
    if (argc > 2 && (!std::strcmp(argv[2], "at") || !std::strcmp(argv[2], "jump"))) {
        if (argc < 6) {
            std::fprintf(stderr, "engine-bench age %s x z height [seconds]\n", argv[2]);
            return 1;
        }
        favourCallingThread();
        watchPlace(age, std::atof(argv[3]), std::atof(argv[4]), std::atof(argv[5]), argc > 6 ? std::atof(argv[6]) : 300, !std::strcmp(argv[2], "jump"));
        return 0;
    }
    const bool plain = argc > 3 && !std::strcmp(argv[3], "plain");
    const bool quick = argc > 4 && !std::strcmp(argv[4], "quick"), ready = argc > 4 && !std::strcmp(argv[4], "ready");
    if (!plain) favourCallingThread();
    auto start = Clock::now();
    Game game(PlanetOptions{42, age, std::nullopt, 0});
    if (const int crew = argc > 2 ? std::atoi(argv[2]) : 0) game.nature.crew.emplace(unsigned(crew));
    std::printf("seed 42 age %g, frame thread %s: planet and spawn after %.1f s, %u simulation workers, %u engine workers\n", age,
                plain ? "plain" : "favoured", seconds(start), game.nature.world.workers.threads(), game.nature.crew->threads());

    if (ready) {
        watchReadiness(game, argc > 5 ? std::atof(argv[5]) : 300, 5);
        return 0;
    }
    Fill spawn{game, quick};
    const Series waiting = play(game, 900, spawn);
    report("spawn", spawn, game);
    report("spawn", "game-thread step", waiting.step);
    report("spawn", "of which streaming", waiting.streaming);

    const Position before = game.physics.feet();
    const Series walk = play(game, 20, Walk{});
    std::printf("walked %.0f m\n", std::hypot(game.physics.feet()[0] - before[0], game.physics.feet()[2] - before[2]));
    report("walk", "game-thread step", walk.step);
    report("walk", "of which streaming", walk.streaming);

    // A jump of a kilometre and a half, where site data is already there: what the voxels round the player take.
    Command jump;
    jump.elapsed = FrameSeconds;
    jump.active = true;
    jump.direction = {1, 0, 0};
    jump.teleport = Position{before[0] + 1500, sim::surfaceAt(game.nature.world, {before[0] + 1500, before[2]}).ground + 2.0, before[2]};
    advance(game, jump);
    Arrive arrive;
    const Series landing = play(game, 900, arrive);
    std::printf("jump   voxels round the player whole %.1f s after a teleport of 1.5 km\n", arrive.voxels);
    report("jump", "game-thread step", landing.step);

    Flight flight{before};
    const Series flown = play(game, 180, flight);
    const Position after = game.physics.feet();
    std::printf("flew %.1f km in %.0f s\n", std::hypot(after[0] - before[0], after[2] - before[2]) / 1000, double(flown.step.size()) * FrameSeconds);
    report("flight", "game-thread step", flown.step);
    report("flight", "of which streaming", flown.streaming);

    if (quick) return 0;
    Command stop;
    stop.elapsed = FrameSeconds;
    stop.active = true;
    stop.direction = {1, 0, 0};
    stop.fly = true;
    advance(game, stop);
    Fill arrival{game};
    arrival.surface = 0;
    const Series settling = play(game, 900, arrival);
    report("away", arrival, game);
    report("away", "game-thread step", settling.step);

    const TerrainStream& stream = game.stream;
    std::printf("background: %zu voxel columns, %.3f ms each; %zu chunks, %.3f ms each; %zu tiles finished\n",
                stream.builtColumns, stream.builtMs / double(std::max<size_t>(1, stream.builtColumns)), stream.builtChunks,
                stream.builtMs / double(std::max<size_t>(1, stream.builtChunks)), game.nature.world.atlas.finished());
    std::printf("background: %zu hourly weather steps, %.2f ms each\n", game.weather.steps,
                game.weather.stepMs / double(std::max<size_t>(1, game.weather.steps)));
    size_t held = 0;
    for (int level = FinestCubes; level <= CoarsestCubes; ++level) {
        const CubeLevel& state = game.cubes.levels[size_t(level - FinestCubes)];
        const double builds = double(std::max<size_t>(1, state.builds));
        std::printf("cubes of %7.1f m: %4zu chunks held, %5zu built, %7.0f faces and %6.2f ms a chunk\n", edgeOf(level), state.shown, state.builds,
                    double(state.faces) / builds, state.buildMs / builds);
        held += size_t(double(state.shown) * double(state.faces) / builds);
    }
    std::printf("cubes: about %zu faces held by the renderer\n", held);
    return 0;
}
