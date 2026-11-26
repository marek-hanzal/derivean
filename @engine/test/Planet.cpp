// 279 lines, one owner: the checks of one evolved planet's session, in the order the session lives
// them (spawn, streaming, voxels, cubes, readiness, time, flight). The planet takes minutes to evolve,
// so they share it rather than split into files that each evolve their own.
#include "simulation/fn/Query.hpp"

#include "PlanetSession.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace derivean;

namespace {

constexpr double BlockedMs = 250.0;   // far beyond a frame even on a loaded machine; a blocking build takes seconds
// Two billion years is about the youngest this seed offers a mainland coast to land on; older planets take longer to evolve.
constexpr double Age = 2e9;

// The spawn is found on the site level; the ground level must not have been built for it.
void spawnsOnDryLand(PlanetSession& session) {
    Game& game = session.game;
    sim::World& world = game.nature.world;
    expect(!world.atlas.find(sim::tileAt(sim::Level::Ground, {game.spawn[0], game.spawn[2]})),
           "The ground tile under the spawn must come with the voxels, not before the game opens");
    const sim::SurfaceSample ground = sim::surveyAt(world, {game.spawn[0], game.spawn[2]}, sim::Level::Site);
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
    expect(waitFor(session, idle(), [&](const Frame&) { return session.cubes.byId.size() >= 16; }, 10), "The far view must show a first surface at once");
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.streamingReady && frame.voxelBox; }), "Voxels never arrived at the spawn");
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.pendingChunks == 0; }), "The voxel region never completed");
    const Frame& frame = run(session, idle(), 120);
    if (!frame.ground || std::abs(frame.eye[1] - PlayerEyeHeight - *frame.ground) >= 0.4) {
        const double feet = frame.eye[1] - PlayerEyeHeight;
        const sim::StoneSample stone = sim::stoneAt(game.nature.world, frame.eye[0], feet - 0.1, frame.eye[2]);
        std::cout << "player at " << frame.eye[0] << " " << feet << " " << frame.eye[2] << " (feet), ground " << frame.ground.value_or(NAN)
                  << ", water " << frame.waterLevel.value_or(NAN) << ", stone under the feet " << (stone.found ? stone.height : NAN) << "\n";
    }
    expect(frame.ground && std::abs(frame.eye[1] - PlayerEyeHeight - *frame.ground) < 0.4, "The player must come to rest on the ground");
    const sim::SurfaceSample finest = sim::surfaceAt(game.nature.world, {frame.eye[0], frame.eye[2]}, sim::Level::Ground);
    expect(finest.found && finest.level == sim::Level::Ground, "The player must stand on the ground level once the voxels are in");
    expect(frame.voxelBox->min[0] < frame.eye[0] && frame.voxelBox->max[0] > frame.eye[0] && frame.voxelBox->min[2] < frame.eye[2] &&
           frame.voxelBox->max[2] > frame.eye[2] && frame.voxelBox->max[0] - frame.voxelBox->min[0] >= 80,
           "The voxel box must surround the player");
    expect(!session.cubes.byId.empty(), "Far cubes must arrive while voxels load");
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

// Where the finest far cubes lie over loaded voxels, the top of a column of cubes must be made of
// what the top voxel there is made of, a stone lying on the ground included: the hand-off between
// them is then one colour. What disagrees is told by cause and by how far the tops lie apart.
void cubesMatchVoxels(PlanetSession& session) {
    Game& game = session.game;
    expect(waitFor(session, idle(), [](const Frame& frame) { return frame.cubeLevels.front().settled == frame.cubeLevels.front().wanted && frame.siteReady; }),
           "The finest far cubes never settled");
    const double edge = edgeOf(FinestCubes);
    int compared = 0, agreed = 0, stones = 0;
    const auto stone = [&](Cell cell) {
        return sim::stoneAt(game.nature.world, (cell.x + 0.5) * VoxelSize, (cell.y + 0.5) * VoxelSize, (cell.z + 0.5) * VoxelSize).found;
    };
    const auto rock = [](int material) { return sim::definitionOf(sim::Material(material)).phase == sim::Phase::Solid; };
    std::map<std::pair<int, int>, int> differ;   // (voxel, cube) materials where they disagree
    std::map<std::string, int> causes;           // of the disagreements
    std::array<int, 4> apart{};                  // of the disagreements: tops within 1/8, 1/4, 1/2 m and farther
    for (const auto& [id, chunk] : session.cubes.byId) {
        if (chunk.level != FinestCubes) continue;
        for (size_t at = 0; at < chunk.faces.size(); at += FaceWords) {
            const int16_t* face = &chunk.faces[at];
            if (face[0] != 1 || face[1] < 0) continue;   // tops only
            // A top face spans z along u and x along v; one sample in the middle of the quad.
            const double x = (chunk.at.x * CubeSide + face[4] + 0.5 * face[6]) * edge, z = (chunk.at.z * CubeSide + face[3] + 0.5 * face[5]) * edge;
            const double top = (chunk.at.y * CubeSide + face[2]) * edge;
            Cell cell = cellAt({x, top + 1.0, z});
            if (!game.world.streamed->loaded.contains(chunkOf(cell))) continue;
            const int floor = cellAt({x, top - 4.0, z}).y;
            while (materialAt(game.world, cell) == Material::Air && cell.y > floor) --cell.y;
            if (!game.world.streamed->loaded.contains(chunkOf(cell)) || materialAt(game.world, cell) == Material::Air) continue;
            ++compared;
            const int voxel = int(materialAt(game.world, cell));
            const bool onStone = stone(cell);
            stones += onStone;
            agreed += voxel == face[7];
            if (voxel == face[7]) continue;
            ++differ[{voxel, face[7]}];
            const double gap = std::abs((cell.y + 1) * VoxelSize - top);
            ++apart[gap <= 0.125 ? 0 : gap <= 0.25 ? 1 : gap <= 0.5 ? 2 : 3];
            // A stone the far cubes missed or made up, else the cover's kinds the far view cannot know.
            const bool cubeStone = sim::stoneAt(game.nature.world, x, top - 0.5 * edge, z).found;
            ++causes[onStone && !cubeStone ? "stone in the voxels only" : !onStone && cubeStone ? "stone in the cubes only" : onStone ? "on a stone"
                     : rock(voxel) != rock(face[7]) ? "rock against cover" : "cover of another kind"];
        }
    }
    std::cout << "of " << compared << " finest cube tops over loaded voxels, " << agreed << " are of the top voxel's material; " << stones
              << " tops are stones\n  disagreeing tops lie apart by at most 1/8 m: " << apart[0] << ", 1/4 m: " << apart[1] << ", 1/2 m: " << apart[2]
              << ", more: " << apart[3] << "\n";
    for (const auto& [cause, count] : causes) std::cout << "  " << cause << ": " << count << "\n";
    std::vector<std::pair<int, std::pair<int, int>>> worst;
    for (const auto& [pair, count] : differ) worst.push_back({count, pair});
    std::sort(worst.rbegin(), worst.rend());
    for (size_t at = 0; at < std::min<size_t>(worst.size(), 6); ++at)
        std::cout << "  voxel " << sim::definitionOf(sim::Material(worst[at].second.first)).name << " under a cube of "
                  << sim::definitionOf(sim::Material(worst[at].second.second)).name << ": " << worst[at].first << "\n";
    // The far view knows the cover's classes, not which silts or gravels the ground level laid there; the rest must agree.
    expect(compared > 200 && agreed >= compared * 3 / 4, "Far cubes and voxels disagree about what the ground is made of");
}

// Standing still, every signal a capture waits for comes true: the tiles the view reads are all
// built and every level of cubes is cut from them.
void becomesReady(PlanetSession& session) {
    const auto start = std::chrono::steady_clock::now();
    expect(waitFor(session, idle(), [](const Frame& frame) {
               return frame.farReady && frame.siteReady && frame.localReady && frame.regionReady && frame.streamingReady && frame.pendingChunks == 0;
           }), "The view never became final at rest");
    const Frame& frame = session.last;
    for (const CubeLevelView& level : frame.cubeLevels) expect(level.settled == level.wanted, "A ready view holds a level that is not final");
    // What a settled column was cut from is what a build now would read: every shown column not
    // being rebuilt, while no tile has finished since the view was last compared with the tiles.
    const FarCubes& far = session.game.cubes;
    if (far.tilesSeen != session.game.nature.world.atlas.finished()) return;
    size_t final = 0;
    for (int level = FinestCubes; level <= CoarsestCubes; ++level)
        for (const auto& [square, slot] : far.levels[size_t(level - FinestCubes)].slots) {
            if (slot.shown.empty() || slot.stale || slot.ticket || slot.built || !cubeFinal(level, slot.detail)) continue;
            ++final;
            expect(cubeDetail(session.game.nature.world, {level, square.x, square.z}) == slot.detail, "A ready view shows a column cut from data that has changed");
        }
    expect(final > 0, "A ready view must hold final columns");
    std::cout << "view final " << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << " s after the finest cubes settled; tiles";
    for (size_t level = 1; level < sim::LevelCount; ++level) std::cout << " " << frame.tilesFinished[level];
    std::cout << "\n";
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

    // Ten degrees above the horizon, so that ground rising ahead of a spawn on a shore does not stop it.
    command.boost = true;
    command.x = 1;
    command.direction = {0.98481, 0.17365, 0};
    const double from = session.last.eye[0];
    const Frame& boosted = run(session, command, 60);
    const double flown = boosted.eye[0] - from;
    if (flown <= 200 || flown >= 400)
        std::cout << "boosted " << flown << " m along x, eye " << (boosted.ground ? boosted.eye[1] - *boosted.ground : -1) << " m above the ground\n";
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
    PlanetSession session{game, {}, {}, 0.0, 0};
    spawnsOnDryLand(session);
    streamsWithoutBlocking(session);
    voxelsMatchSimulation(session);
    cubesMatchVoxels(session);
    becomesReady(session);
    testCubes(session);
    keepsTime(session);
    testPlanetEdits(session);
    flies(session);
    expect(game.sand.escaped == 0 && game.debrisEscapedKg == 0 && game.water.budget.escaped == 0,
           "Nothing may count as escaped into the void on a planet");
    testDeterminism(game.nature.world, game.spawn);
}
