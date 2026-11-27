#include "measure/fn/Plane.hpp"

#include "climate/type/Climate.hpp"
#include "core/fn/Noise.hpp"
#include "landscape/fn/Build.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

constexpr float Foot = 60.0f;        // m the lowest corner stands above the sea
constexpr float Warmth = 10.0f;      // °C at the ground everywhere, so the rain runs off evenly

} // namespace

// Every per-cell array of Domain must be listed here, as in the model's own allocation in
// landscape/fn/Build.cpp: a process that indexes an array this function forgot reads past its end.
void allocateBare(Domain& domain) {
    const size_t cells = domain.cells();
    domain.ground.resize(cells);
    domain.hold.assign(cells, Free);
    domain.outlet.assign(cells, 0);
    domain.fold.assign(cells, 0.0f);
    for (auto* field : {&domain.tectonic, &domain.residual, &domain.weakness, &domain.rebound, &domain.trend, &domain.uplift, &domain.rainBase,
                        &domain.rain, &domain.warmth, &domain.runoff, &domain.warmthNow, &domain.inflow, &domain.inflowLoad, &domain.settledFlow,
                        &domain.settledLoad, &domain.windX, &domain.windZ, &domain.rimSlope, &domain.rimFloor, &domain.surface, &domain.level,
                        &domain.discharge, &domain.ice, &domain.iceAge, &domain.iceSpeed, &domain.erosionRate, &domain.depositionRate, &domain.eroded, &domain.deposited, &domain.uplifted,
                        &domain.shed})
        field->assign(cells, 0.0f);
    domain.quality.assign(cells, 1.0f);
    domain.sound.assign(cells, 1.0f);
    domain.order.reserve(cells);
    domain.rank.assign(cells, 0);
    domain.spill.assign(cells, -1);
    domain.stride = 8;
    domain.nodes = (domain.size + domain.stride - 1) / domain.stride + 1;
    domain.strata.assign(size_t(domain.nodes) * size_t(domain.nodes), StrataColumn{});
}

Domain planeDomain(const PlaneSpec& spec) {
    const LevelSpec& level = specOf(spec.level);
    Domain domain;
    domain.key = {spec.level, 0, 0};
    domain.cell = level.cell;
    domain.step = level.step;
    domain.origin = {-Halo * level.cell, -Halo * level.cell};
    const int steps = spec.steps > 0 ? spec.steps : int(std::llround(level.span / level.step));
    domain.startTime = domain.time = 0;
    domain.endTime = steps * level.step;
    domain.seaLevel = 0;
    allocateBare(domain);

    const int size = domain.size, last = size - 1;
    const double middle = 0.5 * last, half = 0.5 * size;
    const Vec2 fall{std::cos(spec.azimuth), std::sin(spec.azimuth)};
    const double reach = half * (std::abs(fall.x) + std::abs(fall.z));   // cells from the middle to the lowest corner
    const bool led = spec.level != Level::Region;
    // One unit that never ends: the cached traits stay valid however deep the surface cuts.
    const UnitCache rock{-1e30f, 1e30f, traitsOf(spec.rock)};
    domain.unit.assign(domain.cells(), rock);
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const Vec2 offset{x - middle, z - middle};
            const double along = dot(offset, fall), across = cross(fall, offset);
            const double trough = spec.swale * (std::pow(std::min(1.0, std::abs(across) / half), spec.swalePower) - 1.0);
            const double plane = Foot + spec.slope * level.cell * (reach - along) + spec.swale + trough;
            const double rough = spec.roughness * fractalNoise(spec.seed, x / 5.3, z / 5.3, 3);
            domain.ground.rock[i] = domain.ground.datum[i] = float(plane + rough);
            domain.trend[i] = float(plane);
            domain.rain[i] = domain.rainBase[i] = float(spec.rain);
            // Reduced to sea level so that the lapse rate brings every cell back to the same warmth.
            domain.warmth[i] = Warmth + float(LapseRate * plane);
            domain.rimFloor[i] = float(domain.seaLevel);
            if (x > 0 && z > 0 && x < last && z < last) continue;
            domain.hold[i] = Rim;
            domain.rimSlope[i] = led ? 0.0f : float(spec.slope);
            const Vec2 outward{x == 0 ? -1.0 : x == last ? 1.0 : 0.0, z == 0 ? -1.0 : z == last ? 1.0 : 0.0};
            domain.outlet[i] = dot(outward, fall) > 0.05;
        }
    }
    domain.settledFrom = domain.settledTo = domain.ground;
    return domain;
}

Domain replayPlane(const PlaneSpec& spec) {
    Domain domain = planeDomain(spec);
    // stepDomain asks the planet for the sea level and its date and nothing else, so an empty one
    // with those two set stands in for a world.
    Planet planet;
    planet.seaLevel = domain.seaLevel;
    planet.time = domain.endTime;
    Backdrop backdrop;
    const Setting setting{spec.seed, domain.endTime, &planet, &backdrop};
    FlowScratch scratch;
    const int steps = int(std::llround((domain.endTime - domain.startTime) / domain.step));
    for (int step = 0; step < steps; ++step) stepDomain(domain, scratch, setting);
    return domain;
}

} // namespace derivean::sim
