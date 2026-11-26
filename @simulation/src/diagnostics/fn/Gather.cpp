#include "diagnostics/fn/Scene.hpp"

#include "climate/fn/Seasons.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {
namespace {

constexpr double PlanetRaster = 8192.0;   // m between the samples the planet is drawn from

// True where a square of half-diagonal `radius` around `centre` can show in the picture.
bool inView(const Camera& camera, Vec2 centre, double radius, double reach) {
    const Vec2 offset{centre.x - camera.eye.x, centre.z - camera.eye.z};
    const double distance = length(offset);
    if (distance - radius > reach) return false;
    if (distance <= 1.5 * radius) return true;
    // A pitched camera sees a wider range of headings along its lower and upper edge.
    const double tanH = std::tan(0.5 * camera.fov), tanV = tanH * camera.height / camera.width;
    const double narrowest = std::max(0.15, std::cos(camera.pitch) - tanV * std::abs(std::sin(camera.pitch)));
    const double half = std::atan(tanH / narrowest) + std::asin(std::min(1.0, radius / distance)) + 0.05;
    const Vec2 forward{std::sin(camera.heading), -std::cos(camera.heading)};
    return dot(offset, forward) >= distance * std::cos(std::min(half, 3.14159));
}

float slopeAt(const std::vector<float>& top, int side, int x, int z, double cell) {
    const auto at = [&](int ix, int iz) { return top[size_t(std::clamp(iz, 0, side - 1)) * size_t(side) + size_t(std::clamp(ix, 0, side - 1))]; };
    return float(std::hypot(at(x + 1, z) - at(x - 1, z), at(x, z + 1) - at(x, z - 1)) / (2 * cell));
}

// The tiles of one level the camera can see, as a layer that spans their bounding square.
SceneLayer gatherLevel(World& world, const Camera& camera, Level level, double reach, bool snow, int& built) {
    const double span = tileSpan(level), cell = specOf(level).cell;
    const TileKey low = tileAt(level, {camera.eye.x - reach, camera.eye.z - reach}), high = tileAt(level, {camera.eye.x + reach, camera.eye.z + reach});
    std::vector<TileKey> wanted;
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x)
            if (inView(camera, {(x + 0.5) * span, (z + 0.5) * span}, 0.7072 * span, reach)) wanted.push_back({level, x, z});
    if (wanted.empty()) return {};
    TileKey first = wanted.front(), last = wanted.front();
    for (const TileKey& key : wanted) {
        first.x = std::min(first.x, key.x), first.z = std::min(first.z, key.z);
        last.x = std::max(last.x, key.x), last.z = std::max(last.z, key.z);
        world.atlas.request(key);
    }
    SceneLayer layer = emptyLayer(cell, {(double(first.x) * TileCells + 0.5) * cell, (double(first.z) * TileCells + 0.5) * cell},
                                  (last.x - first.x + 1) * TileCells, (last.z - first.z + 1) * TileCells);
    const Setting& setting = world.atlas.setting();
    for (const TileKey& key : wanted) {
        const auto tile = world.atlas.require(key);
        paintTile(layer, *tile, zonalClimate(setting.seed, (key.z + 0.5) * span, setting.age).seasonal, snow);
    }
    built = int(wanted.size());
    return layer;
}

SceneLayer planetLayer(const World& world, const Camera& camera, double reach, bool snow) {
    const Planet& planet = world.planet;
    const int side = 2 * int(reach / PlanetRaster) + 1;
    const Vec2 first{(std::floor(camera.eye.x / PlanetRaster) - side / 2) * PlanetRaster, (std::floor(camera.eye.z / PlanetRaster) - side / 2) * PlanetRaster};
    SceneLayer layer = emptyLayer(PlanetRaster, first, side, side);
    for (int z = 0; z < side; ++z) {
        const double halfRange = zonalClimate(planet.seed, first.z + z * PlanetRaster, planet.time).seasonal;
        for (int x = 0; x < side; ++x) {
            const Vec2 point = planetPoint(planet.seed, {first.x + x * PlanetRaster, first.z + z * PlanetRaster});
            const float ground = float(samplePlanet(planet.elevation, point));
            Exposed exposed;
            exposed.bedrock = Material::Regolith;
            exposed.standing = float(planet.seaLevel) - ground;
            exposed.snow = snow && annualSnowBalance(samplePlanet(planet.temperature, point, Kernel::Linear), halfRange, samplePlanet(planet.precipitation, point, Kernel::Linear)).net > 0;
            layer.cells[layer.index(x, z)] = paintCell(exposed, ground, 0, PlanetRaster);
            layer.weight[layer.index(x, z)] = 1;
        }
    }
    return layer;
}

} // namespace

void paintTile(SceneLayer& layer, const Tile& tile, double halfRange, bool snow) {
    const double cell = specOf(tile.key.level).cell;
    const int firstX = int(std::llround((double(tile.key.x) * TileCells + 0.5) - layer.origin.x / cell));
    const int firstZ = int(std::llround((double(tile.key.z) * TileCells + 0.5) - layer.origin.z / cell));
    std::vector<float> ground(size_t(TileCells) * TileCells);
    for (size_t i = 0; i < ground.size(); ++i) ground[i] = tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i];
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = size_t(z) * TileCells + size_t(x);
            Exposed exposed;
            exposed.bedrock = Material(tile.rock[i]);
            exposed.coarse = tile.now.sediment[Coarse][i], exposed.sand = tile.now.sediment[Sand][i], exposed.fines = tile.now.sediment[Fines][i];
            exposed.standing = tile.water[i] - ground[i];
            exposed.ice = tile.ice[i];
            exposed.slope = slopeAt(ground, TileCells, x, z, cell);
            exposed.snow = snow && annualSnowBalance(tile.warmth[i], halfRange, tile.rain[i]).net > 0;
            const size_t to = layer.index(firstX + x, firstZ + z);
            layer.cells[to] = paintCell(exposed, ground[i], tile.discharge[i], cell);
            layer.weight[to] = 1;
        }
    }
}

SceneLayer layerOf(const Domain& domain) {
    SceneLayer layer = emptyLayer(domain.cell, {domain.origin.x + 0.5 * domain.cell, domain.origin.z + 0.5 * domain.cell}, domain.size, domain.size);
    std::vector<float> ground(domain.cells());
    for (size_t i = 0; i < ground.size(); ++i) ground[i] = domain.ground.rock[i] + domain.sedimentAt(i);
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            Exposed exposed;
            exposed.bedrock = domain.unit[i].traits.dominant;
            exposed.coarse = domain.ground.sediment[Coarse][i], exposed.sand = domain.ground.sediment[Sand][i], exposed.fines = domain.ground.sediment[Fines][i];
            exposed.standing = domain.level[i] > ground[i] + 1e-3f ? domain.level[i] - ground[i] : 0.0f;
            exposed.ice = domain.ice[i];
            exposed.slope = slopeAt(ground, domain.size, x, z, domain.cell);
            layer.cells[i] = paintCell(exposed, ground[i], domain.discharge[i], domain.cell);
            layer.weight[i] = 1;
        }
    }
    return layer;
}

Scene gatherScene(World& world, const Camera& camera, const SceneReach& reach, SceneCount* count) {
    Scene scene;
    scene.seaLevel = world.planet.seaLevel;
    SceneCount built;
    // Coarse levels first: the finer tiles wait for them anyway.
    SceneLayer region = gatherLevel(world, camera, Level::Region, reach.region, reach.snow, built.region);
    SceneLayer local = reach.local > 0 ? gatherLevel(world, camera, Level::Local, reach.local, reach.snow, built.local) : SceneLayer{};
    SceneLayer site = reach.site > 0 ? gatherLevel(world, camera, Level::Site, reach.site, reach.snow, built.site) : SceneLayer{};
    for (SceneLayer* layer : {&site, &local, &region})
        if (layer->width > 0) scene.layers.push_back(std::move(*layer));
    scene.layers.push_back(planetLayer(world, camera, reach.planet, reach.snow));
    finishScene(scene);
    if (count) *count = built;
    return scene;
}

} // namespace derivean::sim
