#include "game/type/Game.hpp"
#include "game/type/Player.hpp"
#include "game/type/TerrainEdits.hpp"
#include "game/type/WorldProjection.hpp"

#include <chrono>
#include <cmath>

namespace derivean {
namespace {

constexpr double FixedStep = 1.0 / 120.0;

using Clock = std::chrono::steady_clock;

double millisecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void retireDebris(Game& game, Frame& frame) {
    const double playerY = game.physics.feet()[1] + 0.035;
    std::erase_if(game.bodies, [&](const DebrisBody& body) {
        const double top = game.physics.verticalExtent(body.id).second;
        if (!inVoid(game.world, top) || top >= playerY) return false;
        frame.retired.push_back(body.id.GetIndexAndSequenceNumber());
        game.physics.removeBody(body.id);
        game.debrisEscapedKg += body.mass;
        return true;
    });
}

// A body that reaches into space whose voxels are not loaded has nothing to rest on there. It
// waits as it is and resumes, with the motion it had, when the ground around it is back; it has
// not left the world.
void suspendStrays(Game& game) {
    for (DebrisBody& body : game.bodies) {
        const auto [low, high] = game.physics.extent(body.id);
        const Cell first = chunkOf(cellAt(low)), last = chunkOf(cellAt(high));
        bool stray = false;
        for (int y = first.y; y <= last.y && !stray; ++y)
            for (int z = first.z; z <= last.z && !stray; ++z)
                for (int x = first.x; x <= last.x && !stray; ++x) stray = !known(game.world, {x, y, z});
        if (stray == body.suspended) continue;
        body.suspended = stray;
        game.physics.suspend(body.id, stray);
    }
}

void move(Game& game, const Command& command, double scale, const Footing& footing) {
    streamColliders(game);
    game.physics.movePlayer(float(FixedStep), playerMove(game, command, scale, footing));
    game.jumpQueued = false;

    double lowestMoving = game.physics.feet()[1] + 0.035;
    for (const auto& body : game.bodies) lowestMoving = std::min(lowestMoving, game.physics.verticalExtent(body.id).first);
    advanceWater(game.water, game.world, FixedStep, &game.physics, lowestMoving);
    stepSand(game.sand, game.world, game.water, FixedStep, game.landed, &game.physics, lowestMoving);
    if (!game.landed.empty()) {
        markTerrainChanged(game, game.landed);
        game.landed.clear();
    }
    game.accumulator -= FixedStep;
    game.clock += FixedStep;
}

Bounds particleView(Position eye) {
    const Cell center = chunkOf(cellAt(eye));
    const Cell spread{Visibility, Visibility, Visibility};
    return {chunkBounds(center - spread).min, chunkBounds(center + spread).max};
}

void streamTerrain(Game& game, Frame& frame, bool moved) {
    const auto start = Clock::now();
    Nature& nature = game.nature;
    const Position feet = game.physics.feet(), velocity = game.physics.velocity();
    const Footing footing = footingAt(game, feet);
    requestTiles(nature, feet, velocity, footing.ground ? feet[1] - *footing.ground : 0.0);
    advanceStream(game.stream, nature, game.world, game.edits, feet, std::hypot(velocity[0], velocity[1], velocity[2]));
    const bool changed = !game.stream.loaded.empty() || !game.stream.dropped.empty();
    if (changed) game.colliders.stale = true;
    suspendStrays(game);
    // Only voxels arriving around a body can newly overlap it; on foot the colliders keep it out.
    if (changed || moved || game.flying) resolveOverlap(game);
    frame.generationMs = game.stream.generationMs;
    frame.streamingMs += millisecondsSince(start);
}

void projectNature(Game& game, Frame& frame) {
    const auto start = Clock::now();
    projectScene(game, frame);
    const Footing footing = footingAt(game, frame.eye);
    const double clearance = footing.ground ? frame.eye[1] - *footing.ground : 0.0;
    if (game.cubes) {
        const Position velocity = game.physics.velocity();
        advanceCubes(*game.cubes, game.nature, frame.eye, clearance, std::hypot(velocity[0], velocity[1], velocity[2]), frame.cubes);
        for (int level = FinestCubes; level <= CoarsestCubes; ++level) {
            const CubeLevel& held = game.cubes->levels[size_t(level - FinestCubes)];
            frame.cubeLevels.push_back({level, held.wanted, held.held, held.settled, held.box});
        }
    } else {
        advanceHorizon(game.horizon, game.nature, frame.eye, clearance, frame.far);
    }
    frame.streamingMs += millisecondsSince(start);

    frame.ground = footing.ground;
    frame.waterLevel = footing.water;
    const auto& ready = game.nature.tiles.ready;
    frame.siteReady = ready[size_t(sim::Level::Site)];
    frame.localReady = ready[size_t(sim::Level::Local)];
    frame.regionReady = ready[size_t(sim::Level::Region)];
    frame.tilesWanted = game.nature.tiles.wanted;
    frame.tilesFinished = game.nature.tiles.finished;
    frame.pendingChunks = game.stream.missing + game.stream.arrived.size() + game.view.queue.size();
    frame.pendingPatches = game.cubes ? game.cubes->missing : game.horizon.missing;
    frame.farReady = game.cubes ? game.cubes->whole : game.horizon.missing == 0 && game.horizon.settled;
    frame.farData = game.cubes ? game.cubes->shownByData : game.horizon.shownByData;
    frame.streamingReady = !game.held;
    projectSky(game, frame, footing.ground);
}

// The one place the natural world is moved to a date. The weather follows the calendar here;
// whatever else lives on calendar time joins it on the next line.
void advanceNature(Game& game) {
    advanceWeather(game.weather, game.nature, game.calendar);
}

// Play runs in real time whatever the calendar does; the rate only says how much calendar passes with it.
void turnCalendar(Game& game, const Command& command) {
    game.calendar.seconds += std::max(0.0, command.timeJump);
    if (command.active) game.calendar.seconds += std::clamp(command.timeRate, 0.0, MaxTimeRate) * std::clamp(command.elapsed, 0.0, 0.1);
}

// The world floor is the only way out of the world; a player under it starts over.
void recoverPlayer(Game& game) {
    const double eyeY = game.physics.eye()[1];
    bool lost = inVoid(game.world, eyeY + 2);
    for (const auto& body : game.bodies) {
        const auto [bottom, top] = game.physics.verticalExtent(body.id);
        if (!inVoid(game.world, top) && bottom < eyeY) lost = false;
    }
    if (lost) game.physics.returnPlayer(game.spawn);
}

} // namespace

Game::Game(const PlanetOptions& options) : nature(NatureOptions{options.seed, options.age, options.threads}), calendar{options.age, 0} {
    startWeather(weather, nature, calendar);
    world.streamed.emplace();
    stream.finished = std::make_shared<Mailbox<ColumnResult>>();
    horizon.finished = std::make_shared<Mailbox<FarPatch>>();
    if (options.cubes) {
        cubes.emplace();
        cubes->reach = std::clamp(options.cubeReach, 64, 256);
        cubes->finished = std::make_shared<Mailbox<CubeColumn>>();
    }
    colliders.finished = std::make_shared<Mailbox<ShapeResult>>();
    spawn = findSpawn(nature.world, options.spawn);
    physics.returnPlayer(spawn);
    awaitWeather(weather);
}

Frame advance(Game& game, const Command& command) {
    const auto start = Clock::now();
    Frame frame{};
    if (command.teleport) game.physics.returnPlayer(*command.teleport);
    turnCalendar(game, command);
    advanceNature(game);
    if (command.active && command.fly) {
        game.flying = !game.flying;
        // Flight speed must not carry over into a fall.
        if (!game.flying) game.physics.returnPlayer(game.physics.feet());
    }
    streamTerrain(game, frame, command.teleport.has_value());
    streamColliders(game);
    game.held = awaitsGround(game);
    if (command.active) {
        if ((command.dig || command.place)) {
            if (const auto hit = game.physics.aim(command.direction)) editTerrain(game, frame, *hit, command);
            streamColliders(game);
        }
        game.jumpQueued |= command.jump;
        game.accumulator += std::clamp(command.elapsed, 0.0, 0.1);
        const double scale = std::max(1.0, std::hypot(double(command.x), double(command.z)));
        const Footing footing = footingAt(game, game.physics.feet());
        while (game.accumulator + 1e-9 >= FixedStep) move(game, command, scale, footing);
        recoverPlayer(game);
        retireDebris(game, frame);
        streamColliders(game);
        if (const auto hit = game.physics.aim(command.direction); hit && hit->terrain)
            frame.target = Target{hit->voxel, materialAt(game.world, hit->voxel)};
    } else {
        game.jumpQueued = false;
        game.accumulator = 0;
    }

    frame.eye = game.physics.eye();
    projectNature(game, frame);
    const Bounds view = particleView(frame.eye);
    const Cell anchor = chunkBounds(chunkOf(cellAt(frame.eye))).min;
    frame.particleOrigin = {anchor.x * double(VoxelSize), anchor.y * double(VoxelSize), anchor.z * double(VoxelSize)};
    projectWaterParticles(frame.particles, game.water, view, frame.particleOrigin);
    projectSand(frame.particles, game.sand, view, frame.particleOrigin);
    frame.water = game.water.budget;
    frame.waterDepth = waterDepth(game.water, {frame.eye[0], frame.eye[1] - (PlayerEyeHeight - 0.035), frame.eye[2]});
    for (const auto& body : game.bodies)
        frame.debris.push_back({body.id.GetIndexAndSequenceNumber(), game.physics.pose(body.id)});
    frame.flying = game.flying;
    frame.editedTiles = game.editedTiles.size();
    frame.visibleChunks = game.visible.size();
    frame.bodies = game.bodies.size();
    frame.sandAirborne = game.sand.grains.size();
    frame.sandDeposited = game.sand.deposited;
    frame.sandEscaped = game.sand.escaped;
    frame.debrisEscapedKg = game.debrisEscapedKg;
    frame.excavatedKg = game.excavatedKg;
    frame.buildStock = game.buildStock;
    frame.nativeMs = millisecondsSince(start);
    return frame;
}

std::vector<Edit> takeEdits(Game& game) { return takeEdits(game.edits); }

} // namespace derivean
