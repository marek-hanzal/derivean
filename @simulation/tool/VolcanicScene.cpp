#include "Volcanic.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "diagnostics/fn/Scene.hpp"
#include "diagnostics/fn/View.hpp"
#include "landscape/fn/Process.hpp"
#include "measure/constant/Angle.hpp"

#include <chrono>
#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr double Km3 = 1e9;

ProvingSpec sceneOf(const std::string& name) {
    ProvingSpec spec;
    spec.ground.level = Level::Region;
    spec.ground.slope = 0.002;
    spec.ground.roughness = 8;
    spec.ground.rain = 0.8;
    spec.ground.rock = Material::Granite;
    if (name == "shield") {
        spec.magma = Magma::Basalt, spec.volume = 60 * Km3, spec.episodes = 8;
    } else if (name == "cone") {
        spec.magma = Magma::Andesite, spec.volume = 60 * Km3, spec.episodes = 6;
    } else if (name == "valley") {
        // A valley 350 m deep runs down the plane through the vent.
        spec.magma = Magma::Basalt, spec.volume = 12 * Km3, spec.episodes = 1;
        spec.ground.slope = 0.012, spec.valley = 350, spec.valleyWidth = 5000, spec.site = {-90e3, 0};
    } else if (name == "caldera") {
        spec.magma = Magma::Andesite, spec.volume = 70 * Km3, spec.episodes = 6, spec.paroxysm = 45 * Km3, spec.quiet = 1;
    } else if (name == "worn") {
        spec.magma = Magma::Andesite, spec.volume = 60 * Km3, spec.episodes = 6, spec.quiet = 20;
    } else if (name == "cinder") {
        // The local level: only the small eruptions the region leaves to it.
        spec.ground.level = Level::Local;
        spec.ground.slope = 0.01, spec.ground.roughness = 1;
        spec.magma = Magma::Basalt, spec.volume = 20 * Km3, spec.episodes = 2;
    } else if (name == "summit") {
        spec.ground.level = Level::Local;
        spec.ground.slope = 0.01, spec.ground.roughness = 1;
        spec.magma = Magma::Andesite, spec.volume = 80 * Km3, spec.episodes = 2;
    }
    return spec;
}

void writeMap(Domain domain, const std::string& path) {
    // The water of the last step stood on ground that the step then cut: it is routed anew.
    FlowScratch scratch;
    floodLevels(domain, scratch);
    Field<float> ground(domain.size, domain.size), water(domain.size, domain.size);
    for (size_t i = 0; i < domain.cells(); ++i) {
        ground.cells[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        water.cells[i] = domain.level[i] > ground.cells[i] + 0.5f ? domain.level[i] : ground.cells[i] - 1;
    }
    std::vector<uint8_t> image = reliefImage(ground, domain.seaLevel, domain.cell, 2.0, &water);
    // Lava darkens the tint, ash and scoria warm it: what the pile is made of shows on the map.
    const VolcanicDomain& volcanic = domain.volcanic;
    for (size_t i = 0; i < domain.cells() && !volcanic.floor.empty(); ++i) {
        if (domain.ground.rock[i] <= domain.ground.datum[i] - volcanic.floor[i] || water.cells[i] > ground.cells[i]) continue;
        const float lava = volcanic.allLaid[i] > 0 ? volcanic.lavaLaid[i] / volcanic.allLaid[i] : 1.0f;
        image[i * 3] = uint8_t(image[i * 3] * (0.55f + 0.45f * (1 - lava)));
        image[i * 3 + 1] = uint8_t(image[i * 3 + 1] * (0.5f + 0.3f * (1 - lava)));
        image[i * 3 + 2] = uint8_t(image[i * 3 + 2] * 0.5f);
    }
    writePng(path, domain.size, domain.size, image);
}

void writeView(Domain domain, Vec2 site, double reach, const std::string& path) {
    FlowScratch scratch;
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    Scene scene;
    scene.seaLevel = domain.seaLevel;
    scene.layers.push_back(layerOf(domain));
    finishScene(scene);
    Camera camera;
    camera.width = 1280, camera.height = 720;
    const Vec2 eye = site + Vec2{-0.8 * reach, 0.6 * reach};
    camera.eye = {eye.x, sceneTop(scene, eye.x, eye.z) + 0.03 * reach, eye.z};
    camera.heading = std::atan2(site.x - eye.x, -(site.z - eye.z));
    camera.pitch = std::atan2(0.5 * (sceneTop(scene, site.x, site.z) + sceneTop(scene, eye.x, eye.z)) - camera.eye.y, length(site - eye));
    camera.fov = 50 / DegreesPerRadian;
    Light light;
    light.azimuth = camera.heading - 100 / DegreesPerRadian;
    light.elevation = 30 / DegreesPerRadian;
    light.visibility = 40 * reach;
    Workers workers(4);
    writePng(path, camera.width, camera.height, renderView(scene, camera, light, workers));
}

} // namespace

int volcanicScene(const Arguments& arguments) {
    const std::string name = arguments.text("scene", "cone"), out = arguments.text("out", ".");
    ProvingSpec spec = sceneOf(name);
    spec.volume = arguments.number("volume", spec.volume / Km3) * Km3;
    spec.episodes = int(arguments.number("episodes", spec.episodes));
    spec.quiet = int(arguments.number("quiet", spec.quiet));
    spec.paroxysm = arguments.number("paroxysm", spec.paroxysm / Km3) * Km3;
    spec.ground.rain = arguments.number("rain", spec.ground.rain);
    spec.ground.slope = arguments.number("slope", spec.ground.slope);
    spec.ground.roughness = arguments.number("roughness", spec.ground.roughness);
    const auto started = std::chrono::steady_clock::now();
    const Domain domain = replayProving(spec);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    const Vent& vent = domain.volcanic.vents.front();
    const double reach = spec.ground.level == Level::Region ? 60e3 : 6e3;
    const EdificeShape shape = measureEdifice(gridOf(domain), vent.centre, reach);
    const VolcanicLedger& ledger = domain.volcanic.ledger;
    std::printf("scene %s  %s  level %d  %d steps in %.1f s (%.2f s a step)\n", name.c_str(), vent.magma == Magma::Basalt ? "basalt" : "andesite", int(domain.key.level),
                domain.taken, seconds, seconds / std::max(1, domain.taken));
    std::printf("  erupted %.2f km3 = lava %.2f + cone %.2f + ash %.2f + left %.2f + drifted %.2f (unaccounted %.2e)  sealed %.2f  foundered %.2f\n", ledger.erupted / Km3,
                ledger.lava / Km3, ledger.cone / Km3, ledger.ash / Km3, ledger.left / Km3, ledger.drifted / Km3,
                (ledger.erupted - ledger.lava - ledger.cone - ledger.ash - ledger.left - ledger.drifted) / Km3, ledger.sealed / Km3, ledger.foundered / Km3);
    std::printf("  edifice: %.0f m above its foot (summit %.0f m), radius %.1f km, slope %.1f deg at the top, %.1f at half height, %.1f at the foot; pit %.0f m deep, %.1f km to its rim; pile %.1f km3\n",
                shape.height, shape.summit, shape.radius / 1000, shape.topSlope, shape.midSlope, shape.footSlope, shape.pit, shape.pitRadius / 1000, shape.pile / Km3);
    const TileLedger& land = domain.ledger;
    std::printf("  the tile's own cells, km3: eroded %.2f  deposited %.2f  weathered %.2f  failed %.2f  dissolved %.2f  carried out %.2f\n", land.eroded / Km3, land.deposited / Km3,
                land.weathered / Km3, land.failed / Km3, land.dissolved / Km3, land.sedimentOut / Km3);
    // A section west to east through the vent.
    const int vz = int((vent.centre.z - domain.origin.z) / domain.cell), vx = int((vent.centre.x - domain.origin.x) / domain.cell);
    std::printf("  section through the vent, m, every %d cells:", std::max(1, int(reach / domain.cell / 16)));
    for (int x = vx - int(reach / domain.cell); x <= vx + int(reach / domain.cell); x += std::max(1, int(reach / domain.cell / 16)))
        if (x >= 0 && x < domain.size) std::printf(" %.0f", domain.ground.rock[domain.index(x, vz)] + domain.sedimentAt(domain.index(x, vz)));
    std::printf("\n");
    writeMap(domain, out + "/volcanic-" + name + "-map.png");
    writeView(domain, vent.centre, std::max(3.5 * shape.radius, 12 * domain.cell), out + "/volcanic-" + name + "-view.png");
    std::printf("%s/volcanic-%s-map.png\n%s/volcanic-%s-view.png\n", out.c_str(), name.c_str(), out.c_str(), name.c_str());
    return 0;
}
