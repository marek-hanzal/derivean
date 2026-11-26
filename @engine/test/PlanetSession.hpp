#pragma once

#include "game/type/Game.hpp"

#include "Expect.hpp"

#include <chrono>
#include <functional>
#include <thread>
#include <unordered_map>

// One planet game shared by the planet checks, with every frame's far-terrain delta validated on the way.
struct PlanetSession {
    derivean::Game& game;
    std::unordered_map<uint32_t, derivean::PatchKey> patches;   // what a renderer would hold
    std::unordered_map<uint32_t, derivean::FarPatch> finest;    // its patches of the finest level, kept whole
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

inline bool overlap(derivean::PatchKey a, derivean::PatchKey b) {
    if (a.level > b.level) std::swap(a, b);
    const int shift = b.level - a.level;
    return (a.x >> shift) == b.x && (a.z >> shift) == b.z;
}

// A patch may only show what the simulation and the voxel palette hold: a material's own colour,
// heights measured from its base, and water only as a level sheet with its shore.
inline void checkPatch(const derivean::FarPatch& patch) {
    using namespace derivean;
    const auto palette = [](const uint8_t* rgb) {
        const uint32_t color = uint32_t(rgb[0]) << 16 | uint32_t(rgb[1]) << 8 | rgb[2];
        return std::any_of(Materials.begin(), Materials.end(), [color](const MaterialInfo& info) { return info.color == color; });
    };
    expect(patch.shade.size() == size_t(ShadeSide) * ShadeSide * 4, "The shading raster must cover the patch at its density");
    for (size_t texel = 0; texel < patch.shade.size(); texel += 4)
        expect(patch.shade[texel + 2] < Materials.size() && Material(patch.shade[texel + 2]) != Material::Air, "A shading texel names no material");
    expect(*std::min_element(patch.heights.begin(), patch.heights.end()) < 1, "Far patch heights must be measured from the patch's base");
    for (int j = 0; j < PatchSide; ++j)
        for (int i = 0; i < PatchSide; ++i) {
            const size_t vertex = size_t(j) * PatchSide + i;
            expect(palette(&patch.colors[vertex * 3]), "A far patch colour is not a material's colour");
            expect(patch.heights[vertex] >= 0 && patch.heights[vertex] < 30000, "Far patch heights must be measured from the patch's base");
            const float water = patch.water[vertex];
            if (std::isnan(water) || water - patch.heights[vertex] <= 0.03f || i == 0 || j == 0 || i == PatchCells || j == PatchCells) continue;
            bool level = false;
            for (int dj = -1; dj <= 1; ++dj)
                for (int di = -1; di <= 1; ++di)
                    level |= (di || dj) && std::abs(patch.water[size_t(j + dj) * PatchSide + i + di] - water) < 1e-3f;
            if (!level) {
                std::cout << "water alone at a vertex of patch level " << patch.key.level << " x " << patch.key.x << " z " << patch.key.z << " vertex " << i << "," << j << " base " << patch.base << " data " << int(patch.dataLevel) << "\n";
                for (int dj = -2; dj <= 2; ++dj) {
                    for (int di = -2; di <= 2; ++di) {
                        const int a = std::clamp(i + di, 0, PatchCells), b = std::clamp(j + dj, 0, PatchCells);
                        std::cout << "  w " << patch.water[size_t(b) * PatchSide + a] << " h " << patch.heights[size_t(b) * PatchSide + a];
                    }
                    std::cout << "\n";
                }
            }
            expect(level, "A far patch shows water that is not a level sheet");
        }
}

inline const derivean::Frame& step(PlanetSession& session, const derivean::Command& command) {
    using namespace derivean;
    session.last = advance(session.game, command);
    const Frame& frame = session.last;
    session.slowestMs = std::max(session.slowestMs, frame.nativeMs);
    ++session.frames;
    for (uint32_t id : frame.far.removed) {
        expect(session.patches.erase(id) == 1, "A far patch was removed that was never added");
        session.finest.erase(id);
    }
    for (const FarPatch& patch : frame.far.added) {
        expect(patch.heights.size() == PatchVertices && patch.morph.size() == PatchVertices && patch.water.size() == PatchVertices &&
               patch.normals.size() == size_t(PatchVertices) * 3 && patch.colors.size() == size_t(PatchVertices) * 3,
               "Far patch arrays must match its cell count");
        checkPatch(patch);
        if (patch.key.level == 0) session.finest.emplace(patch.id, patch);
        for (const auto& [id, key] : session.patches) expect(!overlap(key, patch.key), "Two far patches cover the same ground");
        expect(session.patches.emplace(patch.id, patch.key).second, "A far patch id was used twice");
    }
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
