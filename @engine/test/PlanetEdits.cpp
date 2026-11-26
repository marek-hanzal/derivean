#include "game/type/Player.hpp"
#include "game/type/WorldProjection.hpp"

#include "PlanetSession.hpp"

using namespace derivean;

namespace {

bool settled(const Frame& frame) { return frame.streamingReady && frame.voxelBox && frame.pendingChunks == 0; }

// Lifts the player far above the ground so every voxel column unloads, then brings them back.
void leaveAndReturn(PlanetSession& session, Position home, Cell chunk) {
    Game& game = session.game;
    Command command = idle();
    command.fly = !game.flying;
    command.teleport = Position{home[0], home[1] + 400, home[2]};
    step(session, command);
    expect(waitFor(session, idle(), [&](const Frame&) { return !game.world.streamed->loaded.contains(chunk); }, 30),
           "Columns far below the player must unload");
    expect(waitFor(session, idle(), [&](const Frame& frame) { return frame.visibleChunks == 0; }, 30),
           "Every chunk the renderer holds must be unloaded once its column is");
    expect(game.world.chunks.empty() && game.terrain.empty() && game.view.queue.empty(),
           "No voxels or colliders may linger after the player left");
    command = idle();
    command.teleport = home;
    step(session, command);
    expect(waitFor(session, idle(), settled), "Voxels never came back");
    command = idle();
    command.fly = true;
    step(session, command);
}

void editsSurviveReload(PlanetSession& session) {
    Game& game = session.game;
    const uint16_t stock = session.last.buildStock;
    Command command = idle();
    command.direction = {0.6, -0.8, 0};
    command.place = true;
    expect(step(session, command).buildStock == stock - 1, "Placing must reach streamed ground");
    command = idle();
    command.direction = {0, -1, 0};
    command.dig = true;
    command.radius = 5;
    const Frame& dug = step(session, command);
    expect(dug.excavatedKg > 0 && dug.bodies == 0, "Digging must reach streamed ground without shaking the ground around loose");
    run(session, idle(), 240);
    expect(game.sand.grains.size() < 50, "Loose ground around a cut must come to rest");

    double removed = 0, placed = 0;
    for (const Edit& edit : takeEdits(game)) {
        if (edit.after == Material::Air) removed += edit.volume;
        if (edit.before == Material::Air && edit.after == Material::Basalt) placed += edit.volume;
        expect(edit.before != edit.after && edit.volume > 0, "An edit record must describe a change");
    }
    expect(removed > 0.2 && std::abs(placed - double(VoxelVolume)) < 1e-9, "The edit record must hold what was dug and built");
    expect(takeEdits(game).empty(), "Taking the edits hands them over");

    run(session, idle(), 30);
    const Position home = game.physics.feet();
    const Cell chunk = chunkOf(cellAt(home));
    const Bounds area = chunkBounds(chunk).padded(ChunkSide);
    const Voxels before = sampleWorld(game.world, area);
    leaveAndReturn(session, home, chunk);
    expect(sampleWorld(game.world, area).cells == before.cells, "Edited and untouched voxels must come back identical after unloading");
    expect(takeEdits(game).empty(), "Reloading a chunk is not an edit");
}

void resolvesOverlapOnly(PlanetSession& session) {
    Game& game = session.game;
    run(session, idle(), 30);
    const Position home = game.physics.feet();
    const double x = home[0] + 6, z = home[2] + 6;
    const double top = surfaceUnder(game, x, home[1] + 6, z);

    // Asked directly, so that what the ground then does to a body standing on it stays out of the answer.
    game.physics.returnPlayer({x, top - 1.0, z});
    resolveOverlap(game);
    Position feet = game.physics.feet();
    expect(feet[0] == x && feet[2] == z, "A buried player must be moved straight up");
    expect(feet[1] > top - 0.01 && feet[1] < top + 0.3, "A buried player must end up standing on the surface above");
    Command command = idle();

    // A pocket under a rock roof, cut into loaded voxels: 17 voxels cubed with its ceiling 0.8 m below the surface.
    const Cell middle = cellAt({x, top - 0.8 - 9 * VoxelSize, z});
    const auto cut = carve(game.world, middle, 9, BrushShape::Cube);
    markTerrainChanged(game, cut);
    const double floor = (middle.y - 8) * double(VoxelSize), ceiling = (middle.y + 9) * double(VoxelSize);
    const Position inside = cellCenter(middle);
    command.teleport = Position{inside[0], floor + 0.02, inside[2]};
    step(session, command);
    feet = run(session, idle(), 60).eye;
    feet[1] -= PlayerEyeHeight;
    expect(feet[1] > floor - 0.01 && feet[1] + PlayerHeight < ceiling + 0.01, "A player in a roofed pocket must be left there");

    game.physics.returnPlayer({inside[0], floor - 0.3, inside[2]});
    resolveOverlap(game);
    feet = game.physics.feet();
    expect(feet[0] == inside[0] && feet[2] == inside[2] && feet[1] > floor - 0.01 && feet[1] + PlayerHeight < ceiling + 0.01,
           "A player sunk into a pocket's floor must be lifted inside the pocket, not above the roof");

    command.teleport = home;
    step(session, command);
    run(session, idle(), 30);
}

// Voxel water keeps its exact ledger on a planet and reaches the renderer as meshes.
void voxelWaterIsProjectedAndKept(PlanetSession& session) {
    Game& game = session.game;
    const Position home = game.physics.feet();
    const Cell base = cellAt({home[0] - 4, surfaceUnder(game, home[0] - 4, home[1] + 4, home[2]) + 0.01, home[2]});
    seedWater(game.water, game.world, {base, base + Cell{4, 3, 4}});
    const uint64_t seeded = game.water.budget.initial;
    expect(seeded >= 8 * uint64_t(WaterUnits), "Water must be seeded into the open air above the ground");
    bool meshed = false;
    for (int frame = 0; frame < 180; ++frame) {
        const Frame& out = step(session, idle());
        expect(out.water.bulk + out.water.airborne + out.water.escaped == seeded, "The water ledger must balance every frame");
        for (const ChunkMesh& mesh : out.waterMeshes) meshed |= !mesh.faces.empty();
    }
    expect(meshed, "Voxel water must reach the renderer as a mesh");
    leaveAndReturn(session, home, chunkOf(cellAt(home)));
    run(session, idle(), 60);
    expect(game.water.budget.escaped == 0 && game.water.budget.bulk + game.water.budget.airborne == seeded,
           "Water whose ground unloaded must still be there, to the unit");
}

void looseMatterOutlivesUnloading(PlanetSession& session) {
    Game& game = session.game;
    const Position home = game.physics.feet();
    const Cell above = cellAt({home[0] - 3, home[1] + 3, home[2] - 3});
    const Cell cells[]{above, above + Cell{2, 0, 0}, above + Cell{0, 0, 2}, above + Cell{2, 0, 2}};
    const uint64_t depositedBefore = game.sand.deposited;
    scatterSand(game.sand, cells);

    Voxels block({{0, 0, 0}, {3, 3, 3}});
    std::fill(block.cells.begin(), block.cells.end(), Material::Basalt);
    block.bounds = {above + Cell{20, 8, 0}, above + Cell{23, 11, 0 + 3}};
    const float mass = 27 * materialInfo(Material::Basalt).density * VoxelVolume;
    const JPH::BodyID body = game.physics.addDebris({std::move(block), mass, 0.8f});
    game.bodies.push_back({body, mass});

    leaveAndReturn(session, home, chunkOf(cellAt(home)));
    expect(game.sand.escaped == 0 && game.debrisEscapedKg == 0 && game.bodies.size() == 1,
           "Matter whose surroundings unloaded has not escaped");
    run(session, idle(), 360);
    expect(game.sand.grains.empty() && game.sand.deposited == depositedBefore + 4, "Waiting grains must settle once the ground is back");
    expect(!game.bodies.front().suspended && game.physics.pose(body).position[1] > home[1] - 2 && game.debrisEscapedKg == 0,
           "A waiting body must come to rest on the returned ground");
}

} // namespace

void testPlanetEdits(PlanetSession& session) {
    editsSurviveReload(session);
    resolvesOverlapOnly(session);
    voxelWaterIsProjectedAndKept(session);
    looseMatterOutlivesUnloading(session);
}
