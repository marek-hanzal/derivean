#include "coast/fn/Scenario.hpp"

#include "climate/type/Climate.hpp"
#include "coast/fn/Coast.hpp"
#include "core/fn/Noise.hpp"
#include "measure/fn/Plane.hpp"

namespace derivean::sim {
namespace {

constexpr float Warmth = 10.0f;       // °C at the ground
constexpr float Rain = 0.9f;          // m/yr
constexpr float LakeFloor = 400.0f;   // m, elevation a lake scene stands at, far above its sea
constexpr double LoadShare = 3e-4;    // m³ of sediment a scene's river brings per m³ of water
constexpr double JointGap = 31.0;     // cells between joint zones

struct Shape {
    float ground{};   // m against the scene's water level
    bool loose{};     // loose cover lies here
};

// The ground of a scene in cells from the middle of the domain: `shore` cells is the scale of its features.
Shape shapeOf(const CoastSpec& spec, double x, double z, double cell, double rough) {
    const double landSlope = spec.level == Level::Site ? 0.35 : 0.22, seaSlope = spec.level == Level::Site ? 0.05 : 0.025;
    const auto profile = [&](double inland) {
        // The upland keeps a fall of half a percent towards the shore, so that its rain runs off.
        return inland >= 0 ? std::min(spec.upland + 0.005 * inland * cell, 0.5 + landSlope * inland * cell) + rough * std::min(1.0, inland / 6.0)
                           : std::max(-spec.shelf + 0.002 * inland * cell, seaSlope * inland * cell - 0.5);
    };
    if (spec.scene == CoastScene::Lake) {
        const double radius = std::hypot(x, z), shoreAt = 70.0;
        // A hollow with an outlet to the south, so the lake stands at the lip of that channel.
        const bool channel = std::abs(x) < 2.5 && z > 0;
        const double inland = radius - shoreAt;
        return {float(channel && inland > 0 ? 0.02 - 0.001 * inland : inland >= 0 ? 0.4 + 0.04 * inland * cell + rough : 0.03 * inland * cell - 0.3), std::abs(inland) < 24};
    }
    double shoreline = 0;
    if (spec.scene == CoastScene::Bay) {
        // A rounded bay 70 cells wide and 60 deep in an otherwise straight shore.
        const double across = std::abs(x) / 35.0;
        shoreline = across < 1 ? 60.0 * std::sqrt(1.0 - across * across) : 0.0;
    }
    const bool west = spec.scene != CoastScene::Bay || x < -38;
    return {float(profile(z - shoreline)), west && z - shoreline < 2 && z - shoreline > -25};
}

} // namespace

Domain coastDomain(const CoastSpec& spec) {
    const LevelSpec& level = specOf(spec.level);
    Domain domain;
    domain.key = {spec.level, 0, 0};
    domain.cell = level.cell;
    domain.step = level.step;
    domain.origin = {-Halo * level.cell, -Halo * level.cell};
    const int steps = spec.steps > 0 ? spec.steps : int(std::llround(level.span / level.step));
    domain.startTime = domain.time = 0;
    domain.endTime = steps * level.step;
    const bool lake = spec.scene == CoastScene::Lake;
    domain.seaLevel = 0;
    allocateBare(domain);

    const int size = domain.size, last = size - 1;
    const double middle = 0.5 * last, bearing = spec.windFrom * 0.017453292519943295;
    const Vec2 travel{-std::sin(bearing), std::cos(bearing)};
    const float base = lake ? LakeFloor : 0.0f;
    domain.unit.assign(domain.cells(), UnitCache{-1e30f, 1e30f, traitsOf(spec.rock)});
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const double rough = 0.004 * level.cell * fractalNoise(spec.seed, x / 9.0, z / 9.0, 3);
            const Shape shape = shapeOf(spec, x - middle, z - middle, level.cell, rough);
            if (spec.scene != CoastScene::Lake && x > middle) domain.unit[i].traits = traitsOf(spec.other);
            domain.ground.rock[i] = domain.ground.datum[i] = base + shape.ground;
            if (shape.loose) {
                domain.ground.sediment[Coarse][i] = float(spec.gravel), domain.ground.sediment[Sand][i] = float(spec.sand), domain.ground.sediment[Fines][i] = float(spec.mud);
                domain.ground.rock[i] -= float(spec.gravel + spec.sand + spec.mud);
            }
            // Joint zones two cells wide, a little off the grid's own directions.
            if (spec.joints > 0 && std::fmod(std::abs(x + 0.25 * z) + 0.5, JointGap) < 2.0) domain.weakness[i] = float(spec.joints);
            domain.trend[i] = domain.ground.rock[i] + domain.sedimentAt(i);
            domain.rain[i] = domain.rainBase[i] = Rain;
            domain.warmth[i] = Warmth + float(LapseRate) * std::max(0.0f, domain.trend[i]);
            domain.windX[i] = float(travel.x * spec.wind), domain.windZ[i] = float(travel.z * spec.wind);
            domain.rimFloor[i] = float(domain.seaLevel);
            if (x > 0 && z > 0 && x < last && z < last) continue;
            domain.hold[i] = Rim;
            // A lake scene drains across its southern edge; a sea scene needs no outlet but the sea.
            domain.outlet[i] = lake && z == last;
        }
    }
    if (spec.river > 0 && !lake) {
        const size_t source = domain.index(int(middle), last - 1);
        domain.inflow[source] = float(spec.river), domain.inflowLoad[source] = float(spec.river * LoadShare);
    }
    domain.settledFrom = domain.settledTo = domain.ground;

    // The open rim: each train as high as the given sea, by how nearly it runs with the wind.
    Shore& shore = shoreOf(domain);
    shore.beyond.assign(domain.cells() * WaveDirections, 0.0f);
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            if (x > 0 && z > 0 && x < last && z < last) continue;
            for (int k = 0; k < WaveDirections; ++k) {
                const double along = std::max(0.0, (travel.x * NeighborX[k] + travel.z * NeighborZ[k]) / NeighborReach[k]);
                shore.beyond[domain.index(x, z) * WaveDirections + size_t(k)] = float(spec.swell * spec.swell * along * along * along * along);
            }
        }
    return domain;
}

void workCoast(Domain& domain, int steps) {
    FlowScratch scratch;
    for (size_t i = 0; i < domain.cells(); ++i) domain.runoff[i] = 0.5f, domain.warmthNow[i] = Warmth;
    for (int step = 0; step < steps; ++step) {
        floodLevels(domain, scratch);
        routeWater(domain, scratch);
        workShores(domain, scratch);
        domain.time += domain.step;
        ++domain.taken;
    }
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
}

Domain replayCoast(const CoastSpec& spec, Workers* workers) {
    Domain domain = coastDomain(spec);
    domain.workers = workers;
    // stepDomain asks the planet for the sea level and its date and nothing else.
    Planet planet;
    planet.seaLevel = domain.seaLevel;
    planet.time = domain.endTime;
    Backdrop backdrop;
    const Setting setting{spec.seed, domain.endTime, &planet, &backdrop};
    FlowScratch scratch;
    const int steps = int(std::llround((domain.endTime - domain.startTime) / domain.step));
    for (int step = 0; step < steps; ++step) stepDomain(domain, scratch, setting);
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    return domain;
}

} // namespace derivean::sim
