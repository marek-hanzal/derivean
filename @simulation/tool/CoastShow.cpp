#include "Coast.hpp"

#include "Measure.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Scene.hpp"
#include "diagnostics/fn/View.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

// Kinds of shore around the one body of water that holds the cell nearest `at`.
void tallyWater(const ShoreGrid& grid, const ShoreSurvey& survey, Vec2 origin, Vec2 at, double length[]) {
    const int side = grid.side;
    const int cx = std::clamp(int((at.x - origin.x) / grid.cell), 0, side - 1), cz = std::clamp(int((at.z - origin.z) / grid.cell), 0, side - 1);
    int start = -1;
    for (int r = 0; r < 40 && start < 0; ++r)
        for (int dz = -r; dz <= r && start < 0; ++dz)
            for (int dx = -r; dx <= r && start < 0; ++dx) {
                const int x = cx + dx, z = cz + dz;
                if (x >= 0 && z >= 0 && x < side && z < side && grid.wet(grid.index(x, z))) start = z * side + x;
            }
    if (start < 0) return;
    std::vector<uint8_t> body(grid.rock.size(), 0);
    std::vector<int> queue{start};
    body[size_t(start)] = 1;
    for (size_t next = 0; next < queue.size(); ++next) {
        const int x = queue[next] % side, z = queue[next] / side;
        for (int k = 0; k < 8; k += 2) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= side || nz >= side) continue;
            const size_t n = grid.index(nx, nz);
            if (body[n] || !grid.wet(n) || std::abs(grid.water[n] - grid.water[size_t(queue[next])]) > 1.0f) continue;
            body[n] = 1;
            queue.push_back(nz * side + nx);
        }
    }
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const uint8_t kind = survey.kind[grid.index(x, z)];
            if (!kind) continue;
            for (int k = 0; k < 8; k += 2) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx >= 0 && nz >= 0 && nx < side && nz < side && body[grid.index(nx, nz)]) length[kind] += grid.cell;
            }
        }
}

struct Surveyed {
    ShoreGrid grid;
    ShoreSurvey survey;
    Vec2 origin;   // m, north-west corner of the grid
};

// The 2 × 2 tiles of `level` whose middle lies nearest `at`, surveyed, with a map under `out`.
Surveyed confirm(World& world, Level level, Vec2 at, const std::string& out, const std::string& tag) {
    Lap lap;
    const size_t before = world.atlas.finished();
    const double span = tileSpan(level);
    const TileKey first{level, int32_t(std::floor(at.x / span - 0.5)), int32_t(std::floor(at.z / span - 0.5))};
    std::vector<std::shared_ptr<const Tile>> held;
    std::vector<const Tile*> tiles;
    for (int dz = 0; dz < 2; ++dz)
        for (int dx = 0; dx < 2; ++dx) world.atlas.request({level, first.x + dx, first.z + dz});
    for (int dz = 0; dz < 2; ++dz)
        for (int dx = 0; dx < 2; ++dx) {
            held.push_back(world.atlas.require({level, first.x + dx, first.z + dz}));
            tiles.push_back(held.back().get());
        }
    Surveyed found{shoreGridOf(tiles, 2), {}, {first.x * span, first.z * span}};
    found.survey = surveyShore(found.grid);
    double around[size_t(ShoreKind::Count)] = {};
    tallyWater(found.grid, found.survey, found.origin, at, around);
    std::printf("  %s  X %.0f Z %.0f  level %d tiles from X %.0f Z %.0f  (%zu tiles in %.0f s)\n", tag.c_str(), at.x, at.z, int(level), found.origin.x, found.origin.z,
                world.atlas.finished() - before, lap());
    std::printf("    the water nearest the place:");
    for (int k = 1; k < int(ShoreKind::Count); ++k) std::printf("  %s %.1f km", ShoreKindNames[k], around[k] / 1000);
    std::printf("\n");
    printShore(found.grid, found.survey);
    if (!out.empty()) {
        const std::string path = out + "/coast-find-" + tag + "-L" + std::to_string(int(level)) + ".png";
        writeShoreMap(found.grid, found.survey, path, level == Level::Site ? 1 : 2);
        std::printf("    %s\n", path.c_str());
    }
    std::fflush(stdout);
    return found;
}

// A shore cell of `kind` to show, and the way from it out over its water: of all such cells the one
// the highest waves strike (for a cliff, the highest cliff), with at least `run` cells of the same kind around it.
struct Showcase {
    bool found{};
    Vec2 at, outward;
    float score{};
};

Showcase pick(const Surveyed& s, ShoreKind kind) {
    const ShoreGrid& g = s.grid;
    Showcase best;
    for (int z = 3; z < g.side - 3; ++z)
        for (int x = 3; x < g.side - 3; ++x) {
            const size_t i = g.index(x, z);
            if (ShoreKind(s.survey.kind[i]) != kind) continue;
            int alike = 0;
            float wave = 0;
            Vec2 out{};
            for (int dz = -3; dz <= 3; ++dz)
                for (int dx = -3; dx <= 3; ++dx) {
                    const size_t n = g.index(x + dx, z + dz);
                    alike += ShoreKind(s.survey.kind[n]) == kind;
                    if (g.wet(n)) out = out + Vec2{double(dx), double(dz)}, wave = std::max(wave, g.wave[n]);
                }
            if (alike < 4 || length(out) < 1e-6) continue;
            float rise = 0;
            for (int dz = -2; dz <= 2; ++dz)
                for (int dx = -2; dx <= 2; ++dx) rise = std::max(rise, g.top(g.index(x + dx, z + dz)) - g.top(i));
            const float score = kind == ShoreKind::Cliff ? rise + 10 * wave : wave + 0.01f * float(alike);
            if (best.found && score <= best.score) continue;
            best = {true, s.origin + Vec2{(x + 0.5) * g.cell, (z + 0.5) * g.cell}, normalized(out), score};
        }
    return best;
}

// Two pictures of a shore point: from over its water, and from the shore itself looking along it.
void views(World& world, const Showcase& show, const std::string& out, const std::string& tag) {
    if (out.empty()) return;
    Lap lap;
    SceneReach reach;
    reach.site = 1600, reach.local = 20e3, reach.region = 120e3;
    const Light base;
    for (int shot = 0; shot < 2; ++shot) {
        Camera camera;
        const Vec2 along = perpendicular(show.outward);
        const Vec2 eye = shot == 0 ? show.at + show.outward * 450.0 + along * 150.0 : show.at - show.outward * 6.0 - along * 30.0;
        const Vec2 target = shot == 0 ? show.at : show.at + along * 400.0 + show.outward * 25.0;
        camera.eye = {eye.x, 0, eye.z};
        camera.heading = std::atan2(target.x - eye.x, -(target.z - eye.z));
        SceneCount count;
        const Scene scene = gatherScene(world, camera, reach, &count);
        const double ground = std::max(sceneTop(scene, eye.x, eye.z), float(scene.seaLevel));
        camera.eye.y = ground + (shot == 0 ? 45.0 : 1.8);
        camera.pitch = std::atan2(sceneTop(scene, target.x, target.z) - camera.eye.y, length(target - eye));
        Light light = base;
        light.azimuth = camera.heading - 1.75;
        light.elevation = 0.56;
        const std::string path = out + "/view-" + tag + (shot == 0 ? "-water" : "-shore") + ".png";
        writePng(path, camera.width, camera.height, renderView(scene, camera, light, world.workers));
        std::printf("    view from X %.0f Z %.0f (%.0f m up) to X %.0f Z %.0f: %s  (%d site tiles, %.0f s)\n", eye.x, eye.z, camera.eye.y - ground, target.x, target.z, path.c_str(), count.site,
                    lap());
        std::fflush(stdout);
    }
}

} // namespace

// Local survey of a candidate, then Site tiles and pictures at the shore of each `kinds` it shows best.
void examineShore(World& world, Vec2 at, const std::vector<ShoreKind>& kinds, bool site, const std::string& out, const std::string& tag) {
    const Surveyed local = confirm(world, Level::Local, at, out, tag);
    if (!site) return;
    for (const ShoreKind kind : kinds) {
        const Showcase show = pick(local, kind);
        const std::string name = tag + "-" + std::string(ShoreKindNames[size_t(kind)]).substr(0, std::string(ShoreKindNames[size_t(kind)]).find(' '));
        if (!show.found) {
            std::printf("    no %s shore to show at %s\n", ShoreKindNames[size_t(kind)], tag.c_str());
            continue;
        }
        const Surveyed fine = confirm(world, Level::Site, show.at, out, name);
        // The finer level may have moved the shore: the picture is aimed at its own shore of that kind, if it has one.
        const Showcase there = pick(fine, kind);
        views(world, there.found ? there : show, out, name);
    }
}

