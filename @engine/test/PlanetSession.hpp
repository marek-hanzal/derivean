#pragma once

#include "game/type/Game.hpp"

#include "Expect.hpp"

#include <chrono>
#include <functional>
#include <limits>
#include <thread>
#include <unordered_map>
#include <unordered_set>

// Far cube chunks as a renderer would hold them, by id: without their faces except at the finest level.
struct Held {
    std::unordered_map<uint32_t, derivean::CubeChunk> byId;
    std::unordered_map<derivean::Cell, int, derivean::CellHash> keys;   // {x, level, z}: chunks held over that square of ground
};

// Applies a frame's delta as a renderer would and checks what the frame promises: no chunk twice, a
// square of ground swapped whole, faces inside their chunk. The chunks are moved out of the delta.
inline void hold(Held& held, derivean::CubeDelta& delta) {
    using namespace derivean;
    for (uint32_t id : delta.removed) {
        const auto chunk = held.byId.find(id);
        expect(chunk != held.byId.end(), "A cube chunk was removed that was never added");
        const Cell square{chunk->second.at.x, chunk->second.level, chunk->second.at.z};
        if (--held.keys[square] == 0) held.keys.erase(square);
        held.byId.erase(chunk);
    }
    std::unordered_set<Cell, CellHash> arrived;
    for (CubeChunk& chunk : delta.added) {
        const Cell square{chunk.at.x, chunk.level, chunk.at.z};
        expect(chunk.faces.size() % FaceWords == 0 && !chunk.faces.empty(), "A cube chunk must hold whole quads and at least one");
        for (size_t at = 0; at < chunk.faces.size(); at += FaceWords) {
            const int16_t* face = &chunk.faces[at];
            expect(face[7] > 0 && size_t(face[7]) < Materials.size(), "A cube face names no material");
            expect(face[2] >= 0 && face[2] <= CubeSide && face[3] >= 0 && face[4] >= 0 && face[3] + face[5] <= CubeSide && face[4] + face[6] <= CubeSide,
                   "A cube face leaves its chunk");
        }
        if (arrived.insert(square).second) expect(!held.keys.contains(square), "Two builds of one square of ground are held at once");
        expect(chunk.dataLevel <= uint8_t(cubeSource(chunk.level)), "A cube chunk reads finer data than its cubes can show");
        ++held.keys[square];
        if (chunk.level != FinestCubes) chunk.faces.clear();
        chunk.water.clear();
        expect(held.byId.emplace(chunk.id, std::move(chunk)).second, "A cube chunk id was used twice");
    }
}

// Every level is whole inside its box and a band beyond, and boxes are nested.
inline void checkBoxes(const derivean::FarCubes& far, const Held& held, derivean::Position eye) {
    using namespace derivean;
    double outer = std::numeric_limits<double>::max();
    for (int level = CoarsestCubes; level >= FinestCubes; --level) {
        const CubeLevel& state = far.levels[size_t(level - FinestCubes)];
        if (!state.box) { outer = 0; continue; }
        // A box ends a band short of the nearest square not held, so that square itself may lie exactly at `reach`.
        const double side = state.box->max[0] - eye[0], size = CubeSide * edgeOf(level), reach = side + CubeBand * edgeOf(level) - 1e-6;
        expect(side <= outer + 1e-6, "The box of a finer level reaches beyond the coarser one");
        outer = side;
        for (int z = int(std::floor((eye[2] - reach) / size)); z <= int(std::floor((eye[2] + reach) / size)); ++z)
            for (int x = int(std::floor((eye[0] - reach) / size)); x <= int(std::floor((eye[0] + reach) / size)); ++x) {
                if (held.keys.contains({x, level, z})) continue;
                const double near = std::max({x * size - eye[0], eye[0] - (x + 1) * size, z * size - eye[2], eye[2] - (z + 1) * size, 0.0});
                std::cout << "level " << level << " box side " << side << " m, square " << x << "," << z << " not held at " << near << " m\n";
                expect(false, "A level's box reaches over ground the renderer holds no chunk for");
            }
    }
}

// One planet game of cubes shared by the planet checks, with every frame's far delta validated on the way.
struct PlanetSession {
    derivean::Game& game;
    Held cubes;   // what a renderer would hold
    derivean::Frame last;
    double slowestMs{};
    size_t frames{};
};

inline derivean::Command idle() {
    derivean::Command command;
    command.elapsed = 1.0 / 60.0;
    command.active = true;
    command.direction = {0, 0, -1};
    return command;
}

inline const derivean::Frame& step(PlanetSession& session, const derivean::Command& command) {
    using namespace derivean;
    session.last = advance(session.game, command);
    Frame& frame = session.last;
    session.slowestMs = std::max(session.slowestMs, frame.nativeMs);
    ++session.frames;
    expect(frame.far.added.empty() && frame.far.removed.empty(), "A world of cubes sent far patches");
    hold(session.cubes, frame.cubes);
    checkBoxes(*session.game.cubes, session.cubes, frame.eye);
    return frame;
}

inline const derivean::Frame& run(PlanetSession& session, const derivean::Command& command, int frames) {
    for (int frame = 0; frame < frames; ++frame) step(session, command);
    return session.last;
}

// Steps in real time until `done` holds; background work needs wall-clock time, not frames.
inline bool waitFor(PlanetSession& session, const derivean::Command& command, const std::function<bool(const derivean::Frame&)>& done,
                    double seconds = 900.0) {
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < seconds) {
        if (done(step(session, command))) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

// Top of the highest loaded solid voxel under a point, in metres.
inline double surfaceUnder(const derivean::Game& game, double x, double fromY, double z) {
    using namespace derivean;
    Cell cell = cellAt({x, fromY, z});
    while (materialAt(game.world, cell) == Material::Air) --cell.y;
    return (cell.y + 1) * double(VoxelSize);
}

void testPlanetEdits(PlanetSession& session);
void testCubes(PlanetSession& session);
void testDeterminism(derivean::sim::World& world, derivean::Position around);
