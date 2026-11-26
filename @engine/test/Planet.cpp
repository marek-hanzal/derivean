#include "game/type/Player.hpp"
#include "simulation/fn/Query.hpp"

#include "PlanetSession.hpp"

#include <random>

using namespace derivean;

namespace {

constexpr double BlockedMs = 250.0;   // far beyond a frame even on a loaded machine; a blocking build takes seconds
// Two billion years is about the youngest this seed offers a mainland coast to land on; older planets take longer to evolve.
constexpr double Age = 2e9;

// The checks need ground to stand on. Where the simulation's landing lies under water, the player
// is put on the nearest dry ground a widening search finds, as a spawn the launcher asked for would be.
void standOnLand(Game& game) {
    sim::World& world = game.nature.world;
    const auto dry = [&](sim::Vec2 place, sim::Level level) {
        const sim::SurfaceSample ground = level == sim::Level::Ground ? sim::surveyAt(world, place, level) : sim::surfaceAt(world, place, level);
        return ground.found && ground.ground > ground.water + (level == sim::Level::Ground ? 0.0f : 3.0f);
    };
    const sim::Vec2 landing{game.spawn[0], game.spawn[2]};
    if (dry(landing, sim::Level::Ground)) return;
    for (double reach = 250; reach < 200e3; reach *= 2)
        for (int turn = 0; turn < 8; ++turn) {
            const sim::Vec2 place{landing.x + reach * std::cos(turn * 0.785398), landing.z + reach * std::sin(turn * 0.785398)};
            if (!dry(place, sim::Level::Site) || !dry(place, sim::Level::Ground)) continue;
            game.spawn = findSpawn(world, std::array{place.x, place.z});
            game.physics.returnPlayer(game.spawn);
            std::cout << "the simulation's landing lies under water; the checks start on dry ground " << reach << " m from it\n";
            return;
        }
    expect(false, "No dry ground within 200 km of the landing");
}

void spawnsOnDryLand(PlanetSession& session) {
    Game& game = session.game;
    const sim::SurfaceSample ground = sim::surveyAt(game.nature.world, {game.spawn[0], game.spawn[2]}, sim::Level::Ground);
    expect(ground.ground > ground.water && std::abs(game.spawn[1] - ground.ground) < 0.5, "The spawn must stand on the surveyed dry ground");
    std::cout << "spawn " << game.spawn[0] << " " << game.spawn[1] << " " << game.spawn[2] << ", ground " << ground.ground - ground.water
              << " m above the water\n";
}

void streamsWithoutBlocking(PlanetSession& session) {
    Game& game = session.game;
    const Frame first = step(session, idle());
    expect(!first.streamingReady && !first.voxelBox && game.held, "A player without voxels around must wait");
    const Position eye = first.eye;
    const bool waited = step(session, idle()).streamingReady == false && run(session, idle(), 4).streamingReady == false;
    if (waited) expect(session.last.eye == eye, "A waiting player must not fall");
    expect(waitFor(session, idle(), [&](const Frame&) { return session.patches.size() >= 16; }, 10), "The far view must show a first surface at once");
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.streamingReady && frame.voxelBox; }), "Voxels never arrived at the spawn");
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.pendingChunks == 0; }), "The voxel region never completed");
    const Frame& frame = run(session, idle(), 120);
    expect(frame.ground && std::abs(frame.eye[1] - PlayerEyeHeight - *frame.ground) < 0.4, "The player must come to rest on the ground");
    expect(frame.voxelBox->min[0] < frame.eye[0] && frame.voxelBox->max[0] > frame.eye[0] && frame.voxelBox->min[2] < frame.eye[2] &&
           frame.voxelBox->max[2] > frame.eye[2] && frame.voxelBox->max[0] - frame.voxelBox->min[0] >= 80,
           "The voxel box must surround the player");
    expect(!session.patches.empty(), "Far patches must arrive while voxels load");
    expect(session.slowestMs < BlockedMs, "advance must not wait for generation");
}

void voxelsMatchSimulation(PlanetSession& session) {
    Game& game = session.game;
    const Cell here = chunkOf(cellAt(game.physics.feet()));
    std::mt19937 random(7);
    int solid = 0, open = 0;
    for (int level = -1; level <= 1; ++level) {
        const Bounds area = chunkBounds(here + Cell{0, level, 0});
        const Voxels voxels = sampleWorld(game.world, area);
        for (int probe = 0; probe < 200; ++probe) {
            const Cell cell = area.min + Cell{int(random() % ChunkSide), int(random() % ChunkSide), int(random() % ChunkSide)};
            const Position center = cellCenter(cell);
            const Material expected = sim::materialAt(game.nature.world, center[0], center[1], center[2]);
            expect(voxels[cell] == expected, "A streamed voxel differs from the simulation's material at its centre");
            (expected == Material::Air ? open : solid) += 1;
        }
    }
    expect(solid > 50 && open > 50, "The compared chunks should hold the ground surface");
}

// Where the finest patches lie under loaded voxels, both must show the same material: the hand-off
// between them is then one colour.
void farColoursMatchVoxels(PlanetSession& session) {
    Game& game = session.game;
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.pendingPatches == 0 && frame.siteReady; }), "The far view never settled");
    int compared = 0, agreed = 0, shaded = 0;
    for (const auto& [id, patch] : session.finest) {
        const double size = sizeOf(0);
        for (int j = 1; j < PatchCells; j += 3)
            for (int i = 1; i < PatchCells; i += 3) {
                const double x = patch.key.x * size + i * cellOf(0), z = patch.key.z * size + j * cellOf(0);
                const size_t vertex = size_t(j) * PatchSide + i;
                Cell cell = cellAt({x + 0.5 * VoxelSize, patch.base + patch.heights[vertex] + 1.0, z + 0.5 * VoxelSize});
                if (!game.world.streamed->loaded.contains(chunkOf(cell))) continue;
                while (materialAt(game.world, cell) == Material::Air) --cell.y;
                if (!game.world.streamed->loaded.contains(chunkOf(cell))) continue;
                const uint32_t color = materialInfo(materialAt(game.world, cell)).color;
                const uint8_t* rgb = &patch.colors[vertex * 3];
                ++compared;
                agreed += color == (uint32_t(rgb[0]) << 16 | uint32_t(rgb[1]) << 8 | rgb[2]);
                shaded += patch.shade[(size_t(j) * ShadeDensity * ShadeSide + size_t(i) * ShadeDensity) * 4 + 2] == uint8_t(materialAt(game.world, cell));
            }
    }
    std::cout << "of " << compared << " far vertices over loaded voxels, " << agreed << " have the top voxel's colour and " << shaded
              << " shading texels its material\n";
    // The far view knows the cover's classes, not which stones or silts the ground level laid there; the rest must agree.
    expect(compared > 200 && agreed >= compared * 3 / 4 && shaded >= compared * 7 / 10, "Far patches and voxels disagree about what the ground is made of");
}

// A day at an hour a second: the frame's clock, sun and weather stay sound, clouds arrive, and no
// frame waits for the atmosphere, not even when the calendar leaves it far behind.
void keepsTime(PlanetSession& session) {
    Game& game = session.game;
    Command command = idle();
    command.timeRate = 3600;
    const double began = game.calendar.seconds, slowest = session.slowestMs;
    const auto wall = std::chrono::steady_clock::now();
    int snapshots = 0;
    for (int frame = 0; frame < 24 * 60; ++frame) {
        const Frame& out = step(session, command);
        expect(out.calendar.era == Age && out.hour >= 0 && out.hour < 24 && out.dayOfYear >= 0 && out.dayOfYear < 365, "The frame's clock is out of range");
        expect(std::abs(std::hypot(out.sun[0], out.sun[1], out.sun[2]) - 1) < 1e-6, "The sun direction must be a unit vector");
        const LocalWeather& weather = out.weather;
        expect(std::isfinite(weather.temperature) && weather.temperature > -90 && weather.temperature < 70 && weather.rain >= 0 && weather.snow >= 0 &&
               weather.cloudCover >= 0 && weather.cloudCover <= 1 && weather.humidity >= 0 && weather.humidity <= 1 &&
               weather.pressure > 2e4 && weather.pressure < 1.1e5 && weather.snowpack >= 0 && weather.lag >= 0, "The frame's weather is out of range");
        if (out.clouds) {
            ++snapshots;
            const size_t cells = size_t(out.clouds->size) * size_t(out.clouds->size);
            expect(out.clouds->band[0].cover.size() == cells && out.clouds->band[2].genus.size() == cells && out.clouds->rain.size() == cells,
                   "A cloud snapshot must cover the weather grid");
            expect(out.cloudsTime <= out.calendar.seconds && std::fmod(out.cloudsTime, sim::AirStep) == 0, "A cloud snapshot must belong to a whole step behind the clock");
        }
        if (frame % 10 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    expect(std::abs(game.calendar.seconds - began - 86400) < 1e-6, "A day at an hour a second takes twenty-four seconds of play");
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall).count();
    expect(snapshots >= 1 && snapshots <= int(seconds) + 2, "Cloud snapshots must arrive, and no more than about one a second");
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.weather.lag == 0; }, 60), "The atmosphere never caught up at an hour a second");

    command.timeRate = MaxTimeRate / 12;
    double lag = 0;
    for (int frame = 0; frame < 60; ++frame) lag = std::max(lag, step(session, command).weather.lag);
    expect(lag > 86400, "A calendar that outruns the atmosphere must report the lag");
    expect(session.slowestMs < BlockedMs, "advance must not wait for the atmosphere");
    command.timeRate = 0;
    expect(waitFor(session, command, [](const Frame& frame) { return frame.weather.lag == 0; }, 240), "The atmosphere never caught up after the calendar ran ahead");
    std::cout << "a month of weather caught up in the background at " << game.weather.stepMs / double(std::max<size_t>(1, game.weather.steps))
              << " ms a step; slowest advance meanwhile " << std::max(slowest, session.slowestMs) << " ms\n";
}

void flies(PlanetSession& session) {
    Game& game = session.game;
    const Position start = game.physics.feet();
    Command command = idle();
    command.fly = true;
    expect(step(session, command).flying, "The fly command must switch flight on");
    command.fly = false;
    command.ascend = 1;
    const double risen = run(session, command, 60).eye[1] - PlayerEyeHeight - start[1];
    expect(risen > 10 && risen < 14, "Flight should climb at about 12 m/s");
    command.ascend = 0;
    const double before = session.last.eye[1];
    expect(std::abs(run(session, command, 60).eye[1] - before) < 1e-3, "Flight must hold its height without gravity");

    command.boost = true;
    command.x = 1;
    command.direction = {1, 0, 0};
    const double from = session.last.eye[0];
    const double flown = run(session, command, 60).eye[0] - from;
    expect(flown > 200 && flown < 400, "Boosted flight should cover hundreds of metres a second");
    command.direction = {0.6, 0.8, 0};
    const double climbed = run(session, command, 30).eye[1] - before;
    expect(climbed > 60, "Flight must follow the view upwards");

    // Fast and far into ground that has never been loaded: every frame must still return at once.
    command.x = 0;
    command.ascend = 1;
    run(session, command, 120);
    command.ascend = 0;
    command.x = 1;
    command.direction = {1, 0, 0};
    for (int frame = 0; frame < 600; ++frame) {
        step(session, command);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    expect(session.last.eye[0] - from > 20e3, "Boosted flight at height should cross tens of kilometres in seconds");
    expect(session.slowestMs < BlockedMs, "advance must not wait for tiles while flying into unloaded ground");
    std::cout << "slowest advance " << session.slowestMs << " ms over " << session.frames << " frames\n";

    command = idle();
    command.teleport = start;
    step(session, command);
    command.teleport.reset();
    command.fly = true;
    expect(!step(session, command).flying, "The fly command must switch flight off again");
    command = idle();
    command.teleport = Position{start[0], sim::WorldFloor - 50, start[2]};
    step(session, command);
    const Frame& back = step(session, idle());
    expect(std::abs(back.eye[0] - game.spawn[0]) < 1e-6 && std::abs(back.eye[1] - PlayerEyeHeight - game.spawn[1]) < 1.0,
           "A player below the world floor must start over at the spawn");
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.streamingReady && frame.voxelBox && frame.pendingChunks == 0; }),
           "Voxels never came back after the flight");
}

} // namespace

void testPlanet() {
    const auto start = std::chrono::steady_clock::now();
    Game game(PlanetOptions{42, Age, std::nullopt, 0});
    std::cout << "planet ready after " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << " s\n";
    standOnLand(game);
    PlanetSession session{game, {}, {}, {}, 0.0, 0};
    spawnsOnDryLand(session);
    streamsWithoutBlocking(session);
    voxelsMatchSimulation(session);
    farColoursMatchVoxels(session);
    testCubes(session);
    keepsTime(session);
    testPlanetEdits(session);
    flies(session);
    expect(game.sand.escaped == 0 && game.debrisEscapedKg == 0 && game.water.budget.escaped == 0,
           "Nothing may count as escaped into the void on a planet");
    testDeterminism(game.nature.world, game.spawn);
}
